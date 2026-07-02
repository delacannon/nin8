#include "PluginProcessor.h"
#include "PluginEditor.h"

NineightAudioProcessor::NineightAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void NineightAudioProcessor::prepareToPlay (double, int)
{
}

void NineightAudioProcessor::releaseResources()
{
}

bool NineightAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NineightAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
}

juce::AudioProcessorEditor* NineightAudioProcessor::createEditor()
{
    return new NineightAudioProcessorEditor (*this);
}

void NineightAudioProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void NineightAudioProcessor::setStateInformation (const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NineightAudioProcessor();
}
