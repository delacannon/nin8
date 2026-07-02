#pragma once

#include <cmath>
#include "OpnaChip.h"
#include "Resampler.h"
#include "FMPatch.h"
#include "VoiceAllocator.h"

// Real-time synth model: chip + resampler + voice allocation + current FM patch.
// Port of the non-test paths of AudioEngine.ts (noteOn/noteOff, init sequence,
// patch application) so the plugin sounds with the editor closed.
// All methods are audio-thread only unless noted.
class Synth
{
public:
    struct Patch
    {
        int algorithm = fmpatch::kDefaultAlgorithm;
        int feedback = fmpatch::kDefaultFeedback;
        fmpatch::OperatorEnvelope ops[4] = { fmpatch::kDefaultEnvelopes[0], fmpatch::kDefaultEnvelopes[1],
                                             fmpatch::kDefaultEnvelopes[2], fmpatch::kDefaultEnvelopes[3] };
        fmpatch::LFOSettings lfo;
    };

    void prepare (double outputSampleRate, const uint8_t* rhythmRom, size_t rhythmRomSize)
    {
        chip.reset();
        resampler.prepare ((double) chip.sampleRate(), outputSampleRate);
        voices.releaseAll();

        auto write = writeFn();
        fmpatch::enable6ChannelMode (write);
        applyPatch();

        // Rhythm setup = AudioEngine.loadRhythmRom + configureAdpcmASamples + initRhythm
        chip.loadRhythmRom (rhythmRom, rhythmRomSize);
        configureAdpcmASamples();
        initRhythm();
    }

    double nativeSampleRate() { return (double) chip.sampleRate(); }

    void noteOn (int midiNote)
    {
        if (! polyphony && lastNote >= 0)
            noteOff (lastNote);
        lastNote = midiNote;

        auto write = writeFn();
        const int channel = voices.allocate (midiNote);
        fmpatch::keyOff (write, channel);
        fmpatch::setFrequency (write, channel, midiNote);
        fmpatch::keyOn (write, channel);
    }

    void noteOff (int midiNote)
    {
        const int channel = voices.release (midiNote);
        if (channel != -1)
            fmpatch::keyOff (writeFn(), channel);
    }

    void allNotesOff()
    {
        auto write = writeFn();
        for (int ch = 0; ch < VoiceAllocator::kNumChannels; ++ch)
            fmpatch::keyOff (write, ch);
        voices.releaseAll();
        lastNote = -1;
    }

    void writeReg (int port, uint8_t addr, uint8_t data) { chip.write ((uint32_t) port, addr, data); }

    void resetChip()
    {
        chip.reset();
        auto write = writeFn();
        fmpatch::enable6ChannelMode (write);
        applyPatch();
        configureAdpcmASamples();
        initRhythm();
    }

    // Re-apply full FM patch to all 6 channels (AudioEngine.setAlgorithm/
    // setOperatorEnvelope/setLFO non-test loops).
    void applyPatch()
    {
        auto write = writeFn();
        const bool lfoEnabled = patch.lfo.ams > 0 || patch.lfo.pms > 0;
        fmpatch::setLFOGlobal (write, patch.lfo.freq, lfoEnabled);
        for (int ch = 0; ch < VoiceAllocator::kNumChannels; ++ch)
        {
            fmpatch::setAlgorithm (write, ch, patch.algorithm, patch.feedback);
            fmpatch::setPanAmsPms (write, ch, 3, patch.lfo.ams, patch.lfo.pms); // YM2608 pan always center
            for (int op = 0; op < 4; ++op)
                fmpatch::updateOperatorEnvelope (write, ch, op, patch.ops[op]);
        }
    }

    void render (float* left, float* right, int numSamples)
    {
        // Web Audio graph tail: masterGain → StereoPannerNode (spec equal-power,
        // stereo input; pan 0 = identity).
        constexpr float halfPi = 1.57079632679f;
        const float p = pan;
        const float x = (p <= 0.0f ? p + 1.0f : p) * halfPi;
        const float gl = std::cos (x), gr = std::sin (x);

        for (int i = 0; i < numSamples; ++i)
        {
            float l, r;
            resampler.renderNextSample (chip, l, r);
            l *= masterGain;
            r *= masterGain;
            if (p <= 0.0f)
            {
                left[i] = l + r * gl;
                right[i] = r * gr;
            }
            else
            {
                left[i] = l * gl;
                right[i] = r + l * gr;
            }
        }
    }

    Patch patch;
    float masterGain = 0.5f; // AudioEngine default
    float pan = 0.0f;
    bool polyphony = true;

private:
    fmpatch::WriteRegFn writeFn()
    {
        return [this] (int port, uint8_t addr, uint8_t data) { chip.write ((uint32_t) port, addr, data); };
    }

    // AudioEngine.configureAdpcmASamples: rhythm sample addresses (256-byte units)
    void configureAdpcmASamples()
    {
        struct { int ch, start, end; } drums[] = {
            { 0, 0x00, 0x01 }, // BD
            { 1, 0x01, 0x04 }, // SD
            { 2, 0x04, 0x1B }, // TOP/CYM
            { 3, 0x1B, 0x1C }, // HH
            { 4, 0x1D, 0x1F }, // TOM
            { 5, 0x1F, 0x1F }, // RIM/RS
        };
        auto write = writeFn();
        for (const auto& d : drums)
        {
            write (1, (uint8_t) (0x10 + d.ch), (uint8_t) (d.start & 0xff));
            write (1, (uint8_t) (0x18 + d.ch), (uint8_t) ((d.start >> 8) & 0xff));
            write (1, (uint8_t) (0x20 + d.ch), (uint8_t) (d.end & 0xff));
            write (1, (uint8_t) (0x28 + d.ch), (uint8_t) ((d.end >> 8) & 0xff));
        }
        write (1, 0x01, 0x00); // ADPCM-A master volume: max
        for (int ch = 0; ch < 6; ++ch)
            write (1, (uint8_t) (0x08 + ch), 0xc0); // center pan, max level
    }

    // RhythmPatch.initRhythm
    void initRhythm()
    {
        auto write = writeFn();
        write (0, 0x11, 20); // rhythm total level
        for (uint8_t addr = 0x18; addr <= 0x1d; ++addr)
            write (0, addr, 0xc0 | 10); // center pan, level 10
    }

    OpnaChip chip;
    Resampler resampler;
    VoiceAllocator voices;
    int lastNote = -1;
};
