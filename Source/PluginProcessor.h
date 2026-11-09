#pragma once
#include <JuceHeader.h>

class SauceAudioProcessor : public juce::AudioProcessor
{
public:
    SauceAudioProcessor();
    ~SauceAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SAUCE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 8; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    float getInputPeak() const noexcept { return inputPeak.load(); }
    float getOutputPeak() const noexcept { return outputPeak.load(); }
    void setPreset(int index);
    void captureAB();
    void toggleAB();
    bool isB() const noexcept { return activeB; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    int currentProgram = 0;
    juce::dsp::Oversampling<float> oversampling { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> inputGain, outputGain, sauceSmooth, wetSmooth;
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };
    float envelope = 0.0f;
    double sr = 44100.0;
    juce::NamedValueSet abSnapshot;
    bool activeB = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SauceAudioProcessor)
};
