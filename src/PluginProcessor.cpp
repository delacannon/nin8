#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

NineightAudioProcessor::NineightAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void NineightAudioProcessor::prepareToPlay (double sampleRate, int)
{
    synth.prepare (sampleRate,
                   reinterpret_cast<const uint8_t*> (BinaryData::ym2608_rhythm_rom_bin),
                   (size_t) BinaryData::ym2608_rhythm_rom_binSize);
}

void NineightAudioProcessor::releaseResources()
{
}

bool NineightAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NineightAudioProcessor::renderSegment (juce::AudioBuffer<float>& buffer, int start, int numSamples)
{
    if (numSamples <= 0)
        return;
    synth.render (buffer.getWritePointer (0) + start,
                  buffer.getWritePointer (1) + start,
                  numSamples);
}

void NineightAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Register writes / notes from the UI bridge (mirrors the worklet message drain)
    commandQueue.drain ([this] (const EngineCommand& cmd)
    {
        switch (cmd.type)
        {
            case EngineCommand::writeReg:    synth.writeReg (cmd.a, (uint8_t) cmd.b, (uint8_t) cmd.c); break;
            case EngineCommand::noteOn:      synth.noteOn (cmd.a); break;
            case EngineCommand::noteOff:     synth.noteOff (cmd.a); break;
            case EngineCommand::allNotesOff: synth.allNotesOff(); break;
            case EngineCommand::resetChip:   synth.resetChip(); break;
        }
    });

    // Render in segments split at MIDI event positions (sample-accurate note timing)
    int pos = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        const int eventPos = juce::jlimit (0, buffer.getNumSamples(), metadata.samplePosition);
        renderSegment (buffer, pos, eventPos - pos);
        pos = eventPos;

        if (msg.isNoteOn())
            synth.noteOn (msg.getNoteNumber());
        else if (msg.isNoteOff())
            synth.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            synth.allNotesOff();
    }
    renderSegment (buffer, pos, buffer.getNumSamples() - pos);
}

juce::AudioProcessorEditor* NineightAudioProcessor::createEditor()
{
    return new NineightAudioProcessorEditor (*this);
}

void NineightAudioProcessor::getStateInformation (juce::MemoryBlock&)
{
    // Phase 2: APVTS state
}

void NineightAudioProcessor::setStateInformation (const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NineightAudioProcessor();
}
