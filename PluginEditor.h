#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class DazampLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;
};

class DazampAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit DazampAudioProcessorEditor(DazampAudioProcessor&);
    ~DazampAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void choosePreset(int);
    void selectSnapshot(int);
    void copySnapshot();
    void updateSnapshotButtons();
    static void setHostParameter(DazampAudioProcessor&, const juce::String&, float value);
    DazampAudioProcessor& processor;
    DazampLookAndFeel customLook;
    std::array<juce::Slider, 4> macroKnobs;
    std::array<juce::Label, 4> macroNames, macroValues, descriptions;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> macroConnections;
    std::array<juce::Slider, 3> detailSliders;
    std::array<juce::Label, 3> detailNames;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 3> detailConnections;
    juce::ComboBox presetBox;
    juce::TextButton aButton { "A" }, bButton { "B" }, copyButton { "COPY A/B" },
                     loadIRButton { "LOAD CAB IR" }, bypassButton { "BYPASS" }, monoButton { "MONO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassConnection;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoConnection;
    std::array<std::array<float, 7>, 2> snapshots {};
    int currentSnapshot = 0;
    std::shared_ptr<juce::FileChooser> fileChooser;
    float meter = -90.f;
    juce::Colour amber { 0xffd6a768 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DazampAudioProcessorEditor)
};
