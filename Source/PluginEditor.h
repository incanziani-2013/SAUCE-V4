#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SauceAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SauceAudioProcessorEditor(SauceAudioProcessor&);
    ~SauceAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void drawCup(juce::Graphics&, juce::Rectangle<float>);
    SauceAudioProcessor& processor;
    juce::Slider sauce, input, output, mix, sub, transient, boost;
    juce::ComboBox preset, oversampling;
    juce::ToggleButton advanced {"ADVANCED"}, enabled {"ON"}, autoGain {"AUTO GAIN"};
    juce::TextButton prev {"<"}, next {">"}, abButton {"A/B"};
    juce::Label title, percent, inputLabel, outputLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment
        sauceAtt, inputAtt, outputAtt, mixAtt, subAtt, transientAtt, boostAtt;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment osAtt;
    juce::AudioProcessorValueTreeState::ButtonAttachment enabledAtt, autoGainAtt;
    bool showAdvanced = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SauceAudioProcessorEditor)
};
