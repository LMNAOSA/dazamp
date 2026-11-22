#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class DazampAudioProcessor : public juce::AudioProcessor
{
public:
    DazampAudioProcessor();
    ~DazampAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "DAZAMP"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Modern British"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void loadCabinetIR(const juce::File&);

    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> peakDb { -90.0f };

private:
    juce::dsp::Convolution cabinet;
    juce::dsp::Oversampling<float> oversample { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    std::array<juce::IIRFilter, 2> highPass, lowShelf, bodyFilter, topCut;
    std::array<float, 2> envelopes { 0.0f, 0.0f };
    std::array<std::vector<float>, 2> delayLines;
    std::array<std::vector<float>, 2> widthLines;
    // Allocated only in prepareToPlay, never on the real-time audio callback.
    juce::AudioBuffer<float> wetBuffer;
    int delayWrite = 0;
    juce::Reverb reverb;
    double rate = 48000.0;
    float previousTight = -1.f, previousBody = -1.f, previousCut = -1.f;
    void updateFilters(float tight, float body, float cut);
    void installSyntheticCab();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DazampAudioProcessor)
};
