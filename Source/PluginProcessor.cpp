#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
float getParam(juce::AudioProcessorValueTreeState& s, const char* id)
{
    if (auto* p = s.getRawParameterValue(id)) return p->load();
    return 0.0f;
}
float dbToGain(float db) { return juce::Decibels::decibelsToGain(db); }
}

SauceAudioProcessor::SauceAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "SAUCE_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SauceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p](std::unique_ptr<juce::RangedAudioParameter> x) { p.push_back(std::move(x)); };
    add(std::make_unique<juce::AudioParameterFloat>("sauce", "SAUCE", juce::NormalisableRange<float>(0, 100, 0.01f), 35));
    add(std::make_unique<juce::AudioParameterFloat>("input", "Input", -18.0f, 18.0f, 0.0f));
    add(std::make_unique<juce::AudioParameterFloat>("output", "Output", -18.0f, 18.0f, 0.0f));
    add(std::make_unique<juce::AudioParameterFloat>("mix", "Dry Wet", 0.0f, 100.0f, 100.0f));
    add(std::make_unique<juce::AudioParameterBool>("autogain", "Auto Gain", true));
    add(std::make_unique<juce::AudioParameterFloat>("sub", "Sub Preservation", 0.0f, 100.0f, 55.0f));
    add(std::make_unique<juce::AudioParameterFloat>("transient", "Transient", 0.0f, 100.0f, 25.0f));
    add(std::make_unique<juce::AudioParameterFloat>("boost", "Sauce Boost", 0.0f, 12.0f, 0.0f));
    add(std::make_unique<juce::AudioParameterChoice>("oversampling", "Oversampling",
        juce::StringArray { "1x", "2x", "4x", "8x" }, 2));
    add(std::make_unique<juce::AudioParameterBool>("enabled", "Enabled", true));
    return { p.begin(), p.end() };
}

void SauceAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock, false, true, false);
    oversampling.reset();
    oversampling.initProcessing((size_t) samplesPerBlock);
    inputGain.reset(sampleRate, 0.02);
    outputGain.reset(sampleRate, 0.03);
    sauceSmooth.reset(sampleRate, 0.025);
    wetSmooth.reset(sampleRate, 0.025);
    inputGain.setCurrentAndTargetValue(1.0f);
    outputGain.setCurrentAndTargetValue(1.0f);
    sauceSmooth.setCurrentAndTargetValue(getParam(parameters, "sauce") / 100.0f);
    wetSmooth.setCurrentAndTargetValue(getParam(parameters, "mix") / 100.0f);
    envelope = 0.0f;
}

void SauceAudioProcessor::releaseResources()
{
    oversampling.reset();
    dryBuffer.setSize(0, 0);
}

bool SauceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == out;
}

void SauceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = getTotalNumInputChannels();
    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumOutputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, n);

    float inPeak = 0.0f;
    for (int ch = 0; ch < channels; ++ch)
        inPeak = juce::jmax(inPeak, buffer.getMagnitude(ch, 0, n));
    inputPeak.store(inPeak);

    const bool enabled = getParam(parameters, "enabled") > 0.5f;
    const float inDb = getParam(parameters, "input");
    const float outDb = getParam(parameters, "output");
    const float sauce = getParam(parameters, "sauce") / 100.0f;
    const float wet = getParam(parameters, "mix") / 100.0f;
    const float sub = getParam(parameters, "sub") / 100.0f;
    const float transient = getParam(parameters, "transient") / 100.0f;
    const float boost = getParam(parameters, "boost");
    const bool autoGain = getParam(parameters, "autogain") > 0.5f;

    inputGain.setTargetValue(dbToGain(inDb));
    outputGain.setTargetValue(dbToGain(outDb + (autoGain ? -2.0f * sauce : 0.0f)));
    sauceSmooth.setTargetValue(sauce);
    wetSmooth.setTargetValue(wet);

    if (!enabled || sauce <= 0.00001f)
    {
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < n; ++i)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * outputGain.getNextValue());
        outputPeak.store(inPeak);
        return;
    }

    // Dry path is preallocated in prepareToPlay: no per-block heap allocation.
    for (int ch = 0; ch < channels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, n);

    // Apply input trim at base sample rate before oversampling.
    for (int i = 0; i < n; ++i)
    {
        const float g = inputGain.getNextValue();
        for (int ch = 0; ch < channels; ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * g);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    auto highBlock = oversampling.processSamplesUp(block);
    const float highRate = (float)(sr * 4.0);
    const float release = std::exp(-1.0f / (0.045f * highRate));
    const float attack = std::exp(-1.0f / (0.0015f * highRate));

    for (size_t i = 0; i < highBlock.getNumSamples(); ++i)
    {
        const float drive = juce::jlimit(0.0f, 1.0f, sauceSmooth.getNextValue());
        float peak = 0.0f;
        for (size_t ch = 0; ch < highBlock.getNumChannels(); ++ch)
            peak = juce::jmax(peak, std::abs(highBlock.getChannelPointer(ch)[i]));
        const float coeff = peak > envelope ? attack : release;
        envelope = coeff * envelope + (1.0f - coeff) * peak;
        const float transientGain = 1.0f - transient * 0.18f * juce::jlimit(0.0f, 1.0f, envelope * 2.0f);
        const float driveGain = 1.0f + drive * (2.2f + boost * 0.20f);
        const float ceiling = 1.0f - drive * 0.68f;

        for (size_t ch = 0; ch < highBlock.getNumChannels(); ++ch)
        {
            float* x = highBlock.getChannelPointer(ch);
            const float driven = x[i] * driveGain * transientGain;
            const float pos = std::tanh(driven * (1.0f + drive * 2.0f));
            const float neg = std::tanh(driven * (1.0f + drive * 2.15f) - drive * 0.06f) + drive * 0.06f;
            const float asym = driven >= 0.0f ? pos : neg;
            const float clipped = juce::jlimit(-ceiling, ceiling, asym);
            const float clean = std::tanh(driven);
            // Blend a cleaner branch to retain weight in the fundamental region.
            x[i] = clean * (sub * 0.42f) + clipped * (1.0f - sub * 0.28f);
        }
    }

    oversampling.processSamplesDown(block);

    float outPeakValue = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float mixNow = wetSmooth.getNextValue();
        const float outG = outputGain.getNextValue();
        for (int ch = 0; ch < channels; ++ch)
        {
            const float dry = dryBuffer.getSample(ch, i);
            const float wetSample = buffer.getSample(ch, i);
            const float y = (dry * (1.0f - mixNow) + wetSample * mixNow) * outG;
            buffer.setSample(ch, i, y);
            outPeakValue = juce::jmax(outPeakValue, std::abs(y));
        }
    }
    outputPeak.store(outPeakValue);
}

void SauceAudioProcessor::setPreset(int index)
{
    static const float values[8][8] = {
        {28, 0, 0, 100, 1, 62, 20, 0}, {48, 1, -1, 100, 1, 50, 30, 1},
        {62, 2, -2, 100, 1, 45, 38, 2}, {82, 2, -3, 100, 1, 38, 45, 3},
        {92, 3, -4, 100, 1, 35, 55, 5}, {16, 0, 0, 75, 1, 75, 10, 0},
        {55, 1, -1, 100, 1, 58, 35, 2}, {100, 4, -6, 100, 1, 30, 65, 8}
    };
    index = juce::jlimit(0, 7, index);
    const char* ids[] = {"sauce","input","output","mix","autogain","sub","transient","boost"};
    for (int i = 0; i < 8; ++i)
        if (auto* param = parameters.getParameter(ids[i]))
            param->setValueNotifyingHost(param->convertTo0to1(values[index][i]));
    currentProgram = index;
}
void SauceAudioProcessor::setCurrentProgram(int index) { setPreset(index); }
const juce::String SauceAudioProcessor::getProgramName(int index)
{
    static const char* names[] = {"SPINZ ORIGINAL","ATLANTA SAUCE","UNDERGROUND","EVIL 808","RAGE MODE","CLEAN DRIP","PURPLE ICE","MAXIMUM SAUCE"};
    return names[juce::jlimit(0, 7, index)];
}
void SauceAudioProcessor::captureAB()
{
    abSnapshot.clear();
    for (const auto* p : parameters.processor.getParameters()) juce::ignoreUnused(p);
    for (const auto& id : { "sauce", "input", "output", "mix", "autogain", "sub", "transient", "boost", "oversampling" })
        if (auto* raw = parameters.getRawParameterValue(id))
            abSnapshot.set(juce::Identifier(id), raw->load());
}
void SauceAudioProcessor::toggleAB()
{
    if (abSnapshot.isEmpty()) { captureAB(); activeB = false; return; }
    juce::NamedValueSet current;
    for (const auto& id : { "sauce", "input", "output", "mix", "autogain", "sub", "transient", "boost", "oversampling" })
        if (auto* raw = parameters.getRawParameterValue(id))
            current.set(juce::Identifier(id), raw->load());
    for (const auto& id : { "sauce", "input", "output", "mix", "autogain", "sub", "transient", "boost", "oversampling" })
    {
        auto key = juce::Identifier(id);
        auto* param = parameters.getParameter(id);
        auto* raw = parameters.getRawParameterValue(id);
        if (param && raw && abSnapshot.contains(key))
        {
            const float oldValue = (float)abSnapshot.getWithDefault(key, raw->load());
            const float currentValue = (float)current.getWithDefault(key, raw->load());
            param->setValueNotifyingHost(param->convertTo0to1(activeB ? oldValue : currentValue));
            abSnapshot.set(key, activeB ? currentValue : oldValue);
        }
    }
    activeB = !activeB;
}
void SauceAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}
void SauceAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SauceAudioProcessor(); }
