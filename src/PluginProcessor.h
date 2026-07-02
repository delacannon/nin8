#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/Synth.h"
#include "engine/RegisterWriteQueue.h"

class NineightAudioProcessor : public juce::AudioProcessor
{
public:
    NineightAudioProcessor();
    ~NineightAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Message-thread producers (UI bridge / params)
    CommandQueue& commands() { return commandQueue; }

private:
    void renderSegment (juce::AudioBuffer<float>& buffer, int start, int numSamples);

    Synth synth;
    CommandQueue commandQueue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NineightAudioProcessor)
};
