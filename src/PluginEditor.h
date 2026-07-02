#pragma once

#include "PluginProcessor.h"

class NineightAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NineightAudioProcessorEditor (NineightAudioProcessor&);
    ~NineightAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;

private:
    NineightAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NineightAudioProcessorEditor)
};
