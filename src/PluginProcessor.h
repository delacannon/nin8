#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/Synth.h"
#include "engine/RegisterWriteQueue.h"

class NineightAudioProcessor : public juce::AudioProcessor,
                               private juce::AudioProcessorValueTreeState::Listener
{
public:
    NineightAudioProcessor();
    ~NineightAudioProcessor() override;

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
    juce::AudioProcessorValueTreeState& parameters() { return apvts; }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void cacheParameterPointers();
    void syncPatchFromParams();
    void renderSegment (juce::AudioBuffer<float>& buffer, int start, int numSamples);

    juce::AudioProcessorValueTreeState apvts;

    Synth synth;
    CommandQueue commandQueue;
    std::atomic<bool> patchDirty { false };

    // Raw APVTS atomics, RT-safe to read in processBlock
    std::atomic<float>* pAlg = nullptr;
    std::atomic<float>* pFb = nullptr;
    std::atomic<float>* pLfoFreq = nullptr;
    std::atomic<float>* pLfoAms = nullptr;
    std::atomic<float>* pLfoPms = nullptr;
    std::atomic<float>* pGain = nullptr;
    std::atomic<float>* pPan = nullptr;
    std::atomic<float>* pPoly = nullptr;
    std::atomic<float>* pOp[4][11] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NineightAudioProcessor)
};
