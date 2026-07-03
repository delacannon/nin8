#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/Synth.h"
#include "engine/RegisterWriteQueue.h"
#include "engine/WaveformTap.h"

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
    CommandQueue& noteEventsForUi() { return noteOutbox; } // audio → GUI
    const WaveformTap& waveform() const { return waveformTap; }
    double currentSampleRate() const { return lastSampleRate; }
    double chipNativeRate() const { return nativeRate; }

    // Last editor size, persisted with the plugin state (UI scales to fit)
    std::atomic<int> editorWidth { 2048 }, editorHeight { 1280 };

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void cacheParameterPointers();
    void syncPatchFromParams();
    void renderSegment (juce::AudioBuffer<float>& buffer, int start, int numSamples);

    juce::AudioProcessorValueTreeState apvts;

    Synth synth;
    CommandQueue commandQueue;
    CommandQueue noteOutbox;
    WaveformTap waveformTap;
    std::atomic<bool> patchDirty { false };
    double lastSampleRate = 44100.0;
    double nativeRate = 55466.0;

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
