#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace
{
float p(juce::AudioProcessorValueTreeState& params, const char* id)
{
    return params.getRawParameterValue(id)->load();
}
}

juce::AudioProcessorValueTreeState::ParameterLayout DazampAudioProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&layout](const char* id, const char* title, float lo, float hi, float initial)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), title,
            juce::NormalisableRange<float>(lo, hi, 0.1f), initial));
    };
    add("TIGHT", "Tight", 0.f, 100.f, 76.f);
    add("GRIND", "Grind", 0.f, 100.f, 74.f);
    add("BODY", "Body", 0.f, 100.f, 65.f);
    add("SPACE", "Space", 0.f, 100.f, 12.f);
    add("INPUT", "Input dB", -18.f, 18.f, 0.f);
    add("OUTPUT", "Output dB", -24.f, 6.f, -5.f);
    add("HIGHCUT", "Cab High Cut Hz", 3500.f, 10000.f, 6800.f);
    add("GATE", "Noise Gate dB", -70.f, -20.f, -48.f);
    add("WIDTH", "Stereo Width", 0.f, 100.f, 70.f);
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("BYPASS", 1), "Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("MONO", 1), "Mono Check", false));
    return layout;
}

DazampAudioProcessor::DazampAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "DAZAMP_PARAMETERS", makeParameters())
{
}

bool DazampAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo()
        && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void DazampAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    rate = sampleRate;
    previousTight = previousBody = previousCut = -1.0f;
    for (auto* filters : { &highPass, &lowShelf, &bodyFilter, &topCut })
        for (auto& f : *filters) f.reset();
    envelopes.fill(0.f);
    delayWrite = 0;
    const auto delayLength = juce::jmax(2, int(rate * 1.5));
    for (auto& line : delayLines) line.assign(size_t(delayLength), 0.f);
    for (auto& line : widthLines) line.assign(size_t(delayLength), 0.f);
    wetBuffer.setSize(2, juce::jmax(64, samplesPerBlock), false, true, false);
    wetBuffer.clear();
    oversample.reset();
    oversample.initProcessing(size_t(juce::jmax(64, samplesPerBlock)));
    juce::dsp::ProcessSpec spec { rate, static_cast<juce::uint32>(juce::jmax(64, samplesPerBlock)), 2 };
    cabinet.prepare(spec);
    installSyntheticCab();
    reverb.setSampleRate(rate);
    juce::Reverb::Parameters settings;
    settings.roomSize = 0.75f; settings.damping = .57f;
    settings.wetLevel = .35f; settings.dryLevel = .0f;
    settings.width = 1.f; settings.freezeMode = 0.f;
    reverb.setParameters(settings);
    reverb.reset();
}

void DazampAudioProcessor::releaseResources()
{
    cabinet.reset();
    reverb.reset();
}

void DazampAudioProcessor::installSyntheticCab()
{
    // Procedurally generated short impulse. Replace with an actual guitar cab IR for production sound.
    const int count = juce::jmax(1024, int(rate * .09));
    juce::AudioBuffer<float> ir(1, count);
    ir.clear();
    float* d = ir.getWritePointer(0);
    d[0] = .78f;
    uint32_t seed = 1761;
    for (int i = 1; i < count; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        const float noise = float((seed >> 8) & 0xffff) / 32768.f - 1.f;
        const float time = float(i / rate);
        d[i] = .2f * noise * std::exp(-time * 240.f)
             + .07f * std::sin(2.f * juce::MathConstants<float>::pi * 145.f * time) * std::exp(-time * 155.f);
    }
    d[juce::jlimit(0, count - 1, int(.0067 * rate))] += .14f;
    cabinet.loadImpulseResponse(std::move(ir), rate, juce::dsp::Convolution::Stereo::no,
                                juce::dsp::Convolution::Trim::no,
                                juce::dsp::Convolution::Normalise::no);
}

void DazampAudioProcessor::loadCabinetIR(const juce::File& file)
{
    cabinet.loadImpulseResponse(file,
        juce::dsp::Convolution::Stereo::no,
        juce::dsp::Convolution::Trim::yes, 0,
        juce::dsp::Convolution::Normalise::yes);
}

void DazampAudioProcessor::updateFilters(float tight, float body, float cut)
{
    if (tight == previousTight && body == previousBody && cut == previousCut) return;
    previousTight = tight; previousBody = body; previousCut = cut;
    for (int channel = 0; channel < 2; ++channel)
    {
        highPass[size_t(channel)].setCoefficients(juce::IIRCoefficients::makeHighPass(rate, 58.0 + tight * .9));
        lowShelf[size_t(channel)].setCoefficients(juce::IIRCoefficients::makeLowShelf(rate, 175.0, .7, juce::Decibels::decibelsToGain(-1.f - tight * .065f)));
        bodyFilter[size_t(channel)].setCoefficients(juce::IIRCoefficients::makePeakFilter(rate, 750.0 + body * 3.8, .85, juce::Decibels::decibelsToGain(-1.8f + body * .071f)));
        topCut[size_t(channel)].setCoefficients(juce::IIRCoefficients::makeLowPass(rate, cut));
    }
}

void DazampAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inChannels = getTotalNumInputChannels();
    if (n == 0) return;
    for (int channel = 2; channel < buffer.getNumChannels(); ++channel) buffer.clear(channel, 0, n);
    if (inChannels == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, n);
    if (p(parameters, "BYPASS") >= .5f) { peakDb.store(-90.f); return; }

    const float tight = p(parameters, "TIGHT"), grind = p(parameters, "GRIND"),
          body = p(parameters, "BODY"), space = p(parameters, "SPACE") / 100.f;
    const float inputGain = juce::Decibels::decibelsToGain(p(parameters, "INPUT"));
    const float outputGain = juce::Decibels::decibelsToGain(p(parameters, "OUTPUT"));
    const float gateThreshold = juce::Decibels::decibelsToGain(p(parameters, "GATE"));
    updateFilters(tight, body, p(parameters, "HIGHCUT"));
    float peak = 0.f;
    const float gateAttack = std::exp(-1.f / float(rate * .004));
    const float gateRelease = std::exp(-1.f / float(rate * .065));
    const float attackGain = 1.15f + tight / 100.f * .75f;
    const float preGain = .60f + grind / 100.f * 1.55f;

    for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
    {
        float* samples = buffer.getWritePointer(ch);
        float env = envelopes[size_t(ch)];
        for (int i = 0; i < n; ++i)
        {
            const float input = samples[i] * inputGain;
            const float v = std::abs(input);
            const float coefficient = v > env ? gateAttack : gateRelease;
            env = coefficient * env + (1.f - coefficient) * v;
            const float gate = juce::jlimit(0.f, 1.f, env / juce::jmax(0.00001f, gateThreshold));
            float s = input * gate;
            s = highPass[size_t(ch)].processSingleSampleRaw(s);
            s = lowShelf[size_t(ch)].processSingleSampleRaw(s);
            s = std::tanh(s * attackGain * 1.25f);
            samples[i] = s * preGain;
        }
        envelopes[size_t(ch)] = env;
    }

    // Four-times oversampled non-linear preamp to reduce aliasing.
    juce::dsp::AudioBlock<float> mainBlock(buffer);
    auto highRate = oversample.processSamplesUp(mainBlock);
    const float drive = 1.7f + (grind / 100.f) * 4.8f;
    const float asymmetry = .025f + .035f * (grind / 100.f);
    for (size_t ch = 0; ch < highRate.getNumChannels(); ++ch)
    {
        float* data = highRate.getChannelPointer(ch);
        for (size_t i = 0; i < highRate.getNumSamples(); ++i)
        {
            const float x = data[i];
            const float y = std::tanh((x + asymmetry) * drive) - std::tanh(asymmetry * drive);
            data[i] = y * (.68f - grind * .0018f);
        }
    }
    oversample.processSamplesDown(mainBlock);

    for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
    {
        float* samples = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i)
        {
            auto s = bodyFilter[size_t(ch)].processSingleSampleRaw(samples[i]);
            samples[i] = topCut[size_t(ch)].processSingleSampleRaw(s);
        }
    }
    cabinet.process(juce::dsp::ProcessContextReplacing<float>(mainBlock));
    // Parallel stereo ambience maintains the dry, immediate pick attack.
    // Critically, DO NOT allocate memory in processBlock. Some DAWs change block
    // size without calling prepareToPlay; gracefully bypass reverb in that case.
    const bool hasWetStorage = n <= wetBuffer.getNumSamples();
    if (hasWetStorage)
    {
        wetBuffer.copyFrom(0, 0, buffer, 0, 0, n);
        wetBuffer.copyFrom(1, 0, buffer, 1, 0, n);
        reverb.processStereo(wetBuffer.getWritePointer(0), wetBuffer.getWritePointer(1), n);
    }
    const int length = int(delayLines[0].size());
    if (length == 0) return;
    const int delayL = int(rate * .215), delayR = int(rate * .389);
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    const float* revL = hasWetStorage ? wetBuffer.getReadPointer(0) : nullptr;
    const float* revR = hasWetStorage ? wetBuffer.getReadPointer(1) : nullptr;
    const float width = p(parameters, "WIDTH") * .01f * space;
    const bool monoCheck = p(parameters, "MONO") >= .5f;
    const int widthDelayL = juce::jmax(1, int(rate * .017));
    const int widthDelayR = juce::jmax(1, int(rate * .029));
    for (int i = 0; i < n; ++i)
    {
        const float dryL = left[i], dryR = right[i];
        const int readL = (delayWrite + length - delayL) % length;
        const int readR = (delayWrite + length - delayR) % length;
        const float dL = delayLines[0][size_t(readL)], dR = delayLines[1][size_t(readR)];
        delayLines[0][size_t(delayWrite)] = dryL * space + dL * .23f;
        delayLines[1][size_t(delayWrite)] = dryR * space + dR * .18f;
        // Short decorrelated, low-level stereo reflections give breadth without
        // putting the guitar's fundamental attack through the stereo processor.
        const int widthReadL = (delayWrite + length - widthDelayL) % length;
        const int widthReadR = (delayWrite + length - widthDelayR) % length;
        const float wideL = widthLines[0][size_t(widthReadL)];
        const float wideR = widthLines[1][size_t(widthReadR)];
        widthLines[0][size_t(delayWrite)] = dryL;
        widthLines[1][size_t(delayWrite)] = dryR;
        const float wetL = hasWetStorage ? .16f * revL[i] : 0.f;
        const float wetR = hasWetStorage ? .16f * revR[i] : 0.f;
        float outL = (dryL + space * (.33f * dL + wetL) + width * .17f * wideR) * outputGain;
        float outR = (dryR + space * (.33f * dR + wetR) + width * .17f * wideL) * outputGain;
        if (monoCheck) { const float sum = .5f * (outL + outR); outL = outR = sum; }
        left[i] = outL;
        right[i] = outR;
        delayWrite = (delayWrite + 1) % length;
        peak = std::max({peak, std::abs(outL), std::abs(outR)});
    }
    peakDb.store(juce::Decibels::gainToDecibels(peak, -90.f));
}

void DazampAudioProcessor::getStateInformation(juce::MemoryBlock& block)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, block);
}
void DazampAudioProcessor::setStateInformation(const void* bytes, int size)
{
    if (auto xml = getXmlFromBinary(bytes, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* DazampAudioProcessor::createEditor()
{
    return new DazampAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DazampAudioProcessor();
}
