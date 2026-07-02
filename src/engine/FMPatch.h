#pragma once

#include <cstdint>
#include <functional>

// Line-for-line port of packages/core/src/FMPatch.ts (register bit-packing only;
// UI-side helpers like interpolateFnum stay in JS for now).
namespace fmpatch
{

struct OperatorEnvelope
{
    int ar = 31, dr = 0, sr = 0, rr = 7, sl = 0, tl = 0;
    int ks = 0, mul = 1, dt = 0, am = 0, ssgEg = 0;
};

struct LFOSettings
{
    int freq = 0, ams = 0, pms = 0;
};

using WriteRegFn = std::function<void (int port, uint8_t addr, uint8_t data)>;

namespace reg
{
    constexpr uint8_t KEY_ON = 0x28;
    constexpr uint8_t MODE = 0x29;
    constexpr uint8_t LFO = 0x22;
    // YM2608 uses non-sequential operator offsets: 0, 8, 4, 12
    constexpr uint8_t DT_MUL[4] = { 0x30, 0x38, 0x34, 0x3c };
    constexpr uint8_t TL[4]     = { 0x40, 0x48, 0x44, 0x4c };
    constexpr uint8_t KS_AR[4]  = { 0x50, 0x58, 0x54, 0x5c };
    constexpr uint8_t AM_DR[4]  = { 0x60, 0x68, 0x64, 0x6c };
    constexpr uint8_t SR[4]     = { 0x70, 0x78, 0x74, 0x7c };
    constexpr uint8_t SL_RR[4]  = { 0x80, 0x88, 0x84, 0x8c };
    constexpr uint8_t SSG_EG[4] = { 0x90, 0x98, 0x94, 0x9c };
    constexpr uint8_t FB_ALG = 0xb0;
    constexpr uint8_t LR_AMS_PMS = 0xb4;
    constexpr uint8_t FNUM_L = 0xa0;
    constexpr uint8_t FNUM_H_BLOCK = 0xa4;
}

constexpr int kDefaultAlgorithm = 4;
constexpr int kDefaultFeedback = 5;

// Default piano patch (FMPatch.ts DEFAULT_ENVELOPES / PIANO_PATCH)
const OperatorEnvelope kDefaultEnvelopes[4] = {
    { 31, 8, 0, 7, 12, 20, 0, 2, 7, 0, 0 },
    { 31, 5, 2, 7,  6,  4, 0, 2, 3, 0, 0 },
    { 31, 7, 4, 7, 10, 18, 0, 2, 3, 0, 0 },
    { 31, 0, 4, 7,  0,  0, 0, 2, 0, 0, 0 },
};

// F-numbers per semitone at block 4, 8MHz clock (FMPatch.ts BASE_FNUMS)
constexpr int kBaseFnums[12] = { 617, 654, 693, 734, 778, 824, 873, 925, 980, 1038, 1100, 1165 };

struct NoteFreq { int fnum; int block; };

// Host MIDI note → fnum/block. Matches midiNoteToString (60 = C4) piped into
// FMPatch.noteToFreq: block = octave clamped 0-7, fnum = base table per semitone.
inline NoteFreq midiToFreq (int midiNote)
{
    const int octave = midiNote / 12 - 1;
    const int block = octave < 0 ? 0 : (octave > 7 ? 7 : octave);
    return { kBaseFnums[midiNote % 12], block };
}

inline void setFrequencyRaw (const WriteRegFn& write, int channel, int fnum, int block)
{
    const int port = channel < 3 ? 0 : 1;
    const int ch = channel % 3;
    // Write high byte first (latched until low byte write)
    write (port, (uint8_t) (reg::FNUM_H_BLOCK + ch), (uint8_t) (((block & 7) << 3) | ((fnum >> 8) & 7)));
    write (port, (uint8_t) (reg::FNUM_L + ch), (uint8_t) (fnum & 0xff));
}

inline void setFrequency (const WriteRegFn& write, int channel, int midiNote)
{
    const auto nf = midiToFreq (midiNote);
    setFrequencyRaw (write, channel, nf.fnum, nf.block);
}

inline void keyOn (const WriteRegFn& write, int channel)
{
    const int chBits = channel < 3 ? channel : (channel % 3) | 0x04;
    write (0, reg::KEY_ON, (uint8_t) (0xf0 | chBits));
}

inline void keyOff (const WriteRegFn& write, int channel)
{
    const int chBits = channel < 3 ? channel : (channel % 3) | 0x04;
    write (0, reg::KEY_ON, (uint8_t) chBits);
}

inline void updateOperatorEnvelope (const WriteRegFn& write, int channel, int opIndex, const OperatorEnvelope& e)
{
    if (opIndex < 0 || opIndex > 3)
        return;
    const int port = channel < 3 ? 0 : 1;
    const int ch = channel % 3;

    write (port, (uint8_t) (reg::DT_MUL[opIndex] + ch), (uint8_t) (((e.dt & 0x07) << 4) | (e.mul & 0x0f)));
    write (port, (uint8_t) (reg::KS_AR[opIndex] + ch), (uint8_t) ((e.ks << 6) | (e.ar & 0x1f)));
    write (port, (uint8_t) (reg::AM_DR[opIndex] + ch), (uint8_t) ((e.am << 7) | (e.dr & 0x1f)));
    write (port, (uint8_t) (reg::SR[opIndex] + ch), (uint8_t) (e.sr & 0x1f));
    write (port, (uint8_t) (reg::SL_RR[opIndex] + ch), (uint8_t) (((e.sl & 0x0f) << 4) | (e.rr & 0x0f)));
    write (port, (uint8_t) (reg::TL[opIndex] + ch), (uint8_t) (e.tl & 0x7f));
    write (port, (uint8_t) (reg::SSG_EG[opIndex] + ch), (uint8_t) (e.ssgEg & 0x0f));
}

inline void setAlgorithm (const WriteRegFn& write, int channel, int algorithm, int feedback)
{
    const int port = channel < 3 ? 0 : 1;
    const int ch = channel % 3;
    write (port, (uint8_t) (reg::FB_ALG + ch), (uint8_t) (((feedback & 0x07) << 3) | (algorithm & 0x07)));
}

inline void setLFOGlobal (const WriteRegFn& write, int freq, bool enabled)
{
    write (0, reg::LFO, (uint8_t) ((enabled ? 0x08 : 0x00) | (freq & 0x07)));
}

inline void setPanAmsPms (const WriteRegFn& write, int channel, int pan, int ams, int pms)
{
    const int port = channel < 3 ? 0 : 1;
    const int ch = channel % 3;
    write (port, (uint8_t) (reg::LR_AMS_PMS + ch), (uint8_t) (((pan & 0x03) << 6) | ((ams & 0x03) << 4) | (pms & 0x07)));
}

inline void enable6ChannelMode (const WriteRegFn& write)
{
    write (0, reg::MODE, 0x80);
}

} // namespace fmpatch
