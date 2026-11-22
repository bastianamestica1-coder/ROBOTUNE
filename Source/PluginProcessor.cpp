#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout RoboTuneProcessor::createLayout()
{
    juce::StringArray keys, scales;
    for (int i = 0; i < 12; ++i) keys.add (AutoTuneEngine::keyName (i));
    for (int i = 0; i < AutoTuneEngine::kNumScales; ++i) scales.add (AutoTuneEngine::scaleName (i));

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "key", 1 },   "Key",   keys, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "scale", 1 }, "Scale", scales, 2)); // Minor
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "retune", 1 }, "Retune Speed",
        juce::NormalisableRange<float> (0.0f, 400.0f, 1.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    return layout;
}

RoboTuneProcessor::RoboTuneProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    keyParam    = apvts.getRawParameterValue ("key");
    scaleParam  = apvts.getRawParameterValue ("scale");
    retuneParam = apvts.getRawParameterValue ("retune");
}

void RoboTuneProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, 2);
    setLatencySamples (engine.getLatencySamples());
}

bool RoboTuneProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void RoboTuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    engine.setParams ((int) keyParam->load(), (int) scaleParam->load(), retuneParam->load());
    engine.process (buffer.getArrayOfWritePointers(),
                    juce::jmin (buffer.getNumChannels(), 2), numSamples);
}

juce::AudioProcessorEditor* RoboTuneProcessor::createEditor() { return new RoboTuneEditor (*this); }

void RoboTuneProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void RoboTuneProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RoboTuneProcessor(); }
