#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include "engine/Params.h"

static const char* kOpFields[11] = { "ar", "dr", "sr", "rr", "sl", "tl", "ks", "mul", "dt", "am", "ssgeg" };

NineightAudioProcessor::NineightAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", params::createLayout())
{
    cacheParameterPointers();
    for (auto* p : getParameters())
        if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.addParameterListener (withID->paramID, this);
}

NineightAudioProcessor::~NineightAudioProcessor()
{
    for (auto* p : getParameters())
        if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.removeParameterListener (withID->paramID, this);
}

void NineightAudioProcessor::cacheParameterPointers()
{
    pAlg = apvts.getRawParameterValue ("alg");
    pFb = apvts.getRawParameterValue ("fb");
    pLfoFreq = apvts.getRawParameterValue ("lfo_freq");
    pLfoAms = apvts.getRawParameterValue ("lfo_ams");
    pLfoPms = apvts.getRawParameterValue ("lfo_pms");
    pGain = apvts.getRawParameterValue ("master_gain");
    pPan = apvts.getRawParameterValue ("pan");
    pPoly = apvts.getRawParameterValue ("poly");
    for (int op = 0; op < 4; ++op)
        for (int f = 0; f < 11; ++f)
            pOp[op][f] = apvts.getRawParameterValue (params::opId (op, kOpFields[f]));
}

void NineightAudioProcessor::parameterChanged (const juce::String&, float)
{
    patchDirty.store (true, std::memory_order_release);
}

// Audio thread: copy param atomics → synth patch, re-write registers
void NineightAudioProcessor::syncPatchFromParams()
{
    auto& patch = synth.patch;
    patch.algorithm = (int) pAlg->load();
    patch.feedback = (int) pFb->load();
    patch.lfo.freq = (int) pLfoFreq->load();
    patch.lfo.ams = (int) pLfoAms->load();
    patch.lfo.pms = (int) pLfoPms->load();
    for (int op = 0; op < 4; ++op)
    {
        auto& e = patch.ops[op];
        e.ar = (int) pOp[op][0]->load();
        e.dr = (int) pOp[op][1]->load();
        e.sr = (int) pOp[op][2]->load();
        e.rr = (int) pOp[op][3]->load();
        e.sl = (int) pOp[op][4]->load();
        e.tl = (int) pOp[op][5]->load();
        e.ks = (int) pOp[op][6]->load();
        e.mul = (int) pOp[op][7]->load();
        e.dt = (int) pOp[op][8]->load();
        e.am = pOp[op][9]->load() > 0.5f ? 1 : 0;
        e.ssgEg = (int) pOp[op][10]->load();
    }
    synth.applyPatch();
}

void NineightAudioProcessor::prepareToPlay (double sampleRate, int)
{
    lastSampleRate = sampleRate;
    syncPatchFromParams();
    synth.prepare (sampleRate,
                   reinterpret_cast<const uint8_t*> (BinaryData::ym2608_rhythm_rom_bin),
                   (size_t) BinaryData::ym2608_rhythm_rom_binSize);
    nativeRate = synth.nativeSampleRate();
    patchDirty.store (false, std::memory_order_release);
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

    synth.masterGain = pGain->load();
    synth.pan = pPan->load();
    synth.polyphony = pPoly->load() > 0.5f;

    if (patchDirty.exchange (false, std::memory_order_acq_rel))
        syncPatchFromParams();

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
        {
            synth.noteOn (msg.getNoteNumber());
            noteOutbox.push ({ EngineCommand::noteOn, msg.getNoteNumber(), 0, 0 });
        }
        else if (msg.isNoteOff())
        {
            synth.noteOff (msg.getNoteNumber());
            noteOutbox.push ({ EngineCommand::noteOff, msg.getNoteNumber(), 0, 0 });
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            synth.allNotesOff();
    }
    renderSegment (buffer, pos, buffer.getNumSamples() - pos);

    waveformTap.push (buffer.getReadPointer (0), buffer.getReadPointer (1), buffer.getNumSamples());
}

juce::AudioProcessorEditor* NineightAudioProcessor::createEditor()
{
    return new NineightAudioProcessorEditor (*this);
}

void NineightAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NineightAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            patchDirty.store (true, std::memory_order_release);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NineightAudioProcessor();
}
