#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr std::array<const char*, 4> macros { "TIGHT", "GRIND", "BODY", "SPACE" };
constexpr std::array<const char*, 4> captions {
    "PICK ATTACK / LOW-END", "HARMONICS / SATURATION", "MIDRANGE / WEIGHT", "STEREO DEPTH / AMBIENCE"
};
constexpr std::array<const char*, 7> snapshotParams { "TIGHT", "GRIND", "BODY", "SPACE", "OUTPUT", "HIGHCUT", "WIDTH" };
constexpr std::array<std::array<float, 4>, 5> factoryPresets {{
    {{76, 74, 65, 12}}, {{81, 88, 69, 20}}, {{55, 65, 74, 72}},
    {{43, 49, 57, 92}}, {{42, 8, 47, 49}}
}};

juce::Colour steel { 0xff3e443d };
void panelLine(juce::Graphics& g, float x, float y, float w)
{
    g.setColour(juce::Colour(0xff6f796d).withAlpha(.30f));
    g.drawLine(x, y, x + w, y);
}
void roundedButtonStyle(juce::TextButton& b)
{
    b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff30362f));
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffb58b55));
    b.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd1d7cc));
    b.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff1e1b13));
}
}

void DazampLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float startAngle, float endAngle,
                                         juce::Slider&)
{
    const float size = float(juce::jmin(width, height));
    const float radius = size * .42f;
    const float cx = float(x + width / 2), cy = float(y + height / 2);
    const float angle = startAngle + (endAngle - startAngle) * sliderPos;
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.fillEllipse(cx-radius-3, cy-radius-1, 2*radius+6, 2*radius+9);
    for (int i = 0; i <= 42; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * float(i) / 42.f;
        const auto s = std::sin(a), c = std::cos(a);
        g.setColour(i%7==0 ? juce::Colour(0xff949b8f) : juce::Colour(0xff596158));
        g.drawLine(cx + s*radius*.97f, cy - c*radius*.97f,
                   cx + s*radius*1.065f, cy - c*radius*1.065f, i%7==0 ? 1.5f : .7f);
    }
    g.setColour(juce::Colour(0xff151a16));
    g.fillEllipse(cx-radius*.85f, cy-radius*.85f, radius*1.7f, radius*1.7f);
    juce::ColourGradient metal(juce::Colour(0xff899185), cx-radius, cy-radius,
                               juce::Colour(0xff242c26), cx+radius, cy+radius, false);
    metal.addColour(.33, juce::Colour(0xff3d473e));
    metal.addColour(.63, juce::Colour(0xff5d665b));
    g.setGradientFill(metal);
    g.fillEllipse(cx-radius*.76f, cy-radius*.76f, radius*1.52f, radius*1.52f);
    g.setColour(juce::Colour(0xff8f988b).withAlpha(.60f));
    g.drawEllipse(cx-radius*.76f, cy-radius*.76f, radius*1.52f, radius*1.52f, 1.f);
    juce::ColourGradient centre(juce::Colour(0xff5c685c), cx-radius*.33f, cy-radius*.44f,
                                juce::Colour(0xff1c241e), cx+radius*.5f, cy+radius*.53f, true);
    g.setGradientFill(centre);
    g.fillEllipse(cx-radius*.63f, cy-radius*.63f, radius*1.26f, radius*1.26f);
    g.setColour(juce::Colour(0xffbdc2b4).withAlpha(.25f));
    g.drawEllipse(cx-radius*.63f, cy-radius*.63f, radius*1.26f, radius*1.26f, 1.1f);
    const float s = std::sin(angle), c = std::cos(angle);
    g.setColour(juce::Colour(0xffecb16b));
    g.drawLine(cx + s*radius*.27f, cy - c*radius*.27f,
               cx + s*radius*.68f, cy - c*radius*.68f, 3.0f);
    g.setColour(juce::Colour(0xffcbd0bf).withAlpha(.25f));
    g.fillEllipse(cx - 3.f, cy - 3.f, 6.f, 6.f);
}

DazampAudioProcessorEditor::DazampAudioProcessorEditor(DazampAudioProcessor& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    setLookAndFeel(&customLook);
    setOpaque(true);
    const juce::StringArray titles { "POST HUMAN / RHYTHM", "THE WALL / HEAVY",
                                     "AFTERLIGHT / LEAD", "DARK MATTER / AMBIENT", "COLD GLASS / CLEAN" };
    presetBox.addItemList(titles, 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this] { choosePreset(presetBox.getSelectedId() - 1); };
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff252a25));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe0d7c7));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff5a6158));
    addAndMakeVisible(presetBox);

    for (size_t i = 0; i < macros.size(); ++i)
    {
        auto& knob = macroKnobs[i];
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f,
                                 juce::MathConstants<float>::pi * 2.78f, true);
        knob.setRange(0,100,.1);
        knob.setName(macros[i]);
        addAndMakeVisible(knob);
        macroConnections[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, macros[i], knob);
        for (auto* label : { &macroNames[i], &macroValues[i], &descriptions[i] })
        {
            label->setJustificationType(juce::Justification::centred);
            addAndMakeVisible(*label);
        }
        macroNames[i].setText(macros[i], juce::dontSendNotification);
        macroNames[i].setFont(juce::FontOptions(15.f, juce::Font::bold));
        macroNames[i].setColour(juce::Label::textColourId, juce::Colour(0xffe0e4d9));
        macroValues[i].setFont(juce::FontOptions(18.f));
        macroValues[i].setColour(juce::Label::textColourId, amber);
        descriptions[i].setText(captions[i], juce::dontSendNotification);
        descriptions[i].setFont(juce::FontOptions(9.f));
        descriptions[i].setColour(juce::Label::textColourId, juce::Colour(0xff9ba699));
    }
    for (size_t i = 0; i < detailSliders.size(); ++i)
    {
        auto& slider = detailSliders[i];
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 75, 25);
        slider.setColour(juce::Slider::thumbColourId, amber);
        slider.setColour(juce::Slider::trackColourId, steel);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe2dbd0));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff191d19));
        addAndMakeVisible(slider);
        const char* key = i == 0 ? "OUTPUT" : (i == 1 ? "HIGHCUT" : "WIDTH");
        detailConnections[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, key, slider);
        detailNames[i].setText(i == 0 ? "OUTPUT" : (i == 1 ? "HIGH CUT" : "WIDTH"), juce::dontSendNotification);
        detailNames[i].setFont(juce::FontOptions(10.f));
        detailNames[i].setColour(juce::Label::textColourId, juce::Colour(0xffb7bfb2));
        addAndMakeVisible(detailNames[i]);
    }
    for (auto* button : { &aButton, &bButton, &copyButton, &loadIRButton, &loadNamButton, &bypassButton, &monoButton })
    {
        roundedButtonStyle(*button);
        addAndMakeVisible(*button);
    }
    monoButton.setClickingTogglesState(true);
    bypassButton.setClickingTogglesState(true);
    bypassConnection = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, "BYPASS", bypassButton);
    monoConnection = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, "MONO", monoButton);
    aButton.onClick = [this] { selectSnapshot(0); };
    bButton.onClick = [this] { selectSnapshot(1); };
    copyButton.onClick = [this] { copySnapshot(); };
    loadIRButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser>("Select a licensed WAV cabinet impulse response", juce::File{}, "*.wav");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                if (fc.getResult().existsAsFile()) processor.loadCabinetIR(fc.getResult());
            });
        fileChooser = std::move(chooser);
    };
    loadNamButton.setEnabled(processor.supportsNam());
    loadNamButton.setTooltip(processor.supportsNam() ? "Load a licensed NAM capture" : "Build with --nam to enable neural models");
    loadNamButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser>("Select a .nam amplifier capture", juce::File{}, "*.nam");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                if (!fc.getResult().existsAsFile()) return;
                juce::String result;
                processor.loadNamModel(fc.getResult(), result);
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                                        "DAZAMP / NAM", result);
            });
        fileChooser = std::move(chooser);
    };
    for (size_t i = 0; i < snapshotParams.size(); ++i)
    {
        const auto val = processor.parameters.getRawParameterValue(snapshotParams[i])->load();
        snapshots[0][i] = snapshots[1][i] = val;
    }
    updateSnapshotButtons();
    setSize(1020, 625);
    startTimerHz(30);
}

DazampAudioProcessorEditor::~DazampAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void DazampAudioProcessorEditor::setHostParameter(DazampAudioProcessor& p, const juce::String& id, float value)
{
    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(p.parameters.getParameter(id)))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1(value));
        param->endChangeGesture();
    }
}
void DazampAudioProcessorEditor::choosePreset(int index)
{
    if (index < 0 || index >= int(factoryPresets.size())) return;
    for (size_t i = 0; i < macros.size(); ++i)
        setHostParameter(processor, macros[i], factoryPresets[size_t(index)][i]);
    for (size_t i = 0; i < snapshotParams.size(); ++i)
        snapshots[size_t(currentSnapshot)][i] = processor.parameters.getRawParameterValue(snapshotParams[i])->load();
}
void DazampAudioProcessorEditor::selectSnapshot(int next)
{
    if (next == currentSnapshot) return;
    for (size_t i = 0; i < snapshotParams.size(); ++i)
        snapshots[size_t(currentSnapshot)][i] = processor.parameters.getRawParameterValue(snapshotParams[i])->load();
    currentSnapshot = next;
    for (size_t i = 0; i < snapshotParams.size(); ++i)
        setHostParameter(processor, snapshotParams[i], snapshots[size_t(currentSnapshot)][i]);
    updateSnapshotButtons();
}
void DazampAudioProcessorEditor::copySnapshot()
{
    const int to = 1 - currentSnapshot;
    for (size_t i = 0; i < snapshotParams.size(); ++i)
        snapshots[size_t(to)][i] = processor.parameters.getRawParameterValue(snapshotParams[i])->load();
}
void DazampAudioProcessorEditor::updateSnapshotButtons()
{
    aButton.setToggleState(currentSnapshot == 0, juce::dontSendNotification);
    bButton.setToggleState(currentSnapshot == 1, juce::dontSendNotification);
}
void DazampAudioProcessorEditor::timerCallback()
{
    meter = .65f * meter + .35f * processor.peakDb.load();
    for (size_t i = 0; i < macros.size(); ++i)
    {
        const auto val = processor.parameters.getRawParameterValue(macros[i])->load();
        macroValues[i].setText(juce::String(juce::roundToInt(val)), juce::dontSendNotification);
    }
    repaint(740, 108, 227, 106);
}
void DazampAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111512));
    juce::ColourGradient outer(juce::Colour(0xff363b34), 0.f, 0.f,
                               juce::Colour(0xff101411), 1020.f, 625.f, false);
    g.setGradientFill(outer);
    g.fillRoundedRectangle(15.f, 15.f, 990.f, 595.f, 8.f);
    g.setColour(juce::Colour(0xff747d6f));
    g.drawRoundedRectangle(15.f,15.f,990.f,595.f,8.f,1.2f);
    for (auto pos : { juce::Point<float>(28,29), juce::Point<float>(991,29),
                      juce::Point<float>(28,595), juce::Point<float>(991,595) })
    {
        g.setColour(juce::Colour(0xff161e18));g.fillEllipse(pos.x-7,pos.y-7,14,14);
        g.setColour(juce::Colour(0xff899185));g.drawEllipse(pos.x-5,pos.y-5,10,10,1.f);
        g.drawLine(pos.x-3,pos.y+3,pos.x+3,pos.y-3,1.f);
    }
    juce::ColourGradient face(juce::Colour(0xff404740), 50.f, 39.f,
                              juce::Colour(0xff212820), 970.f, 235.f, false);
    g.setGradientFill(face);
    g.fillRect(43,38,935,196);
    g.setColour(juce::Colour(0xff6d756c).withAlpha(.65f));
    g.drawRect(43,38,935,196,1);
    g.setColour(juce::Colour(0xffa3aba2));
    g.setFont(juce::FontOptions(100.f, juce::Font::bold));
    g.drawText("DAZAMP", 83, 57, 650, 113, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(10.f));
    g.setColour(juce::Colour(0xff9ca397));
    g.drawText("DISCRETE  ·  ANALOGUE  ·  ZERO-COMPROMISE  ·  AMPLIFICATION  ·  MODELLING  ·  PLATFORM",
               85,174,675,26,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff141b16));
    g.fillRect(43,236,935,73);
    // Speaker cloth and precise hardware weave.
    g.setColour(juce::Colour(0xff475047).withAlpha(.35f));
    for (int x = 44; x < 978; x += 4) g.drawVerticalLine(x,237.f,309.f);
    g.setColour(juce::Colour(0xff344034).withAlpha(.22f));
    for (int y = 238; y < 309; y += 4) g.drawHorizontalLine(y,43.f,977.f);
    g.setColour(juce::Colour(0xffbfc1b8));
    g.setFont(juce::FontOptions(40.f, juce::Font::bold));
    g.drawText("D – 800", 240, 250, 540, 52, juce::Justification::centred);
    g.setColour(juce::Colour(0xff262d26));
    g.fillRect(43,309,935,276);
    panelLine(g,65.f,340.f,890.f);
    panelLine(g,65.f,504.f,890.f);
    g.setColour(juce::Colour(0xffb2bdb0));
    g.setFont(juce::FontOptions(10.f));
    g.drawText("TONE SHAPING     /     FOUR CONTROLS. ONE SOUND.",72,315,555,28,juce::Justification::centredLeft);
    g.drawText("PRECISION MACROS   0—100",694,315,260,28,juce::Justification::centredRight);
    g.setColour(juce::Colour(0xff171b17));
    g.fillRoundedRectangle(743,105,219,112,3.f);
    g.setColour(juce::Colour(0xff626b60));
    g.drawRoundedRectangle(743,105,219,112,3.f,2.f);
    g.setColour(amber);
    g.setFont(juce::FontOptions(10.f));
    g.drawText("OUTPUT / dB",756,110,195,22,juce::Justification::centred);
    g.setColour(juce::Colour(0xff9f8462));
    juce::Path arc;
    arc.addCentredArc(851.f,207.f,78.f,78.f,0.f,
                      -juce::MathConstants<float>::pi*.44f,
                       juce::MathConstants<float>::pi*.44f,true);
    g.strokePath(arc,juce::PathStrokeType(1.5f));
    const float ratio = juce::jlimit(0.f,1.f,(meter+40.f)/40.f);
    const float angle = -juce::MathConstants<float>::pi*.44f + ratio * juce::MathConstants<float>::pi*.88f;
    g.setColour(juce::Colour(0xffe6ac69));
    g.drawLine(851.f,207.f,851.f + std::sin(angle)*70.f,207.f-std::cos(angle)*70.f,2.f);
    g.fillEllipse(847.f,203.f,8.f,8.f);
    g.setColour(juce::Colour(0xffa9a08a));
    g.drawText(juce::String(meter,1)+" dB",763,192,175,20,juce::Justification::centred);
    g.setColour(juce::Colour(0xffa9b3a7));
    g.setFont(juce::FontOptions(10.f));
    g.drawText("SIGNAL     HI-Z   —   BOOST   —   AMP   —   CAB   —   SPACE",75,508,595,35,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff788577));
    g.drawText("DAZAMP   /   D-800   /   v0.3 DSP",66,580,420,22,juce::Justification::centredLeft);
    g.drawText("ORIGINAL MODEL  ·  48 kHz RECOMMENDED",549,580,405,22,juce::Justification::centredRight);
}
void DazampAudioProcessorEditor::resized()
{
    presetBox.setBounds(70,49,320,32);
    // The preset lives within the hardware label area, not over DAZAMP engraving.
    presetBox.setBounds(670,55,210,30);
    for (size_t i = 0; i < macroKnobs.size(); ++i)
    {
        const int x = 94 + int(i)*219;
        macroKnobs[i].setBounds(x,348,168,104);
        macroNames[i].setBounds(x+8,452,152,26);
        macroValues[i].setBounds(x+8,478,152,25);
        descriptions[i].setBounds(x-11,530,190,23);
    }
    aButton.setBounds(700,513,34,30);
    bButton.setBounds(740,513,34,30);
    copyButton.setBounds(779,513,88,30);
    bypassButton.setBounds(873,513,82,30);
    detailNames[0].setBounds(65,551,68,23);
    detailSliders[0].setBounds(130,551,125,23);
    detailNames[1].setBounds(270,551,75,23);
    detailSliders[1].setBounds(345,551,125,23);
    detailNames[2].setBounds(485,551,65,23);
    detailSliders[2].setBounds(545,551,120,23);
    loadIRButton.setBounds(679,552,90,27);
    loadNamButton.setBounds(775,552,91,27);
    monoButton.setBounds(870,552,85,27);
}
