#pragma once

#include <array>
#include <cstdint>

// Port of packages/core/src/VoiceAllocator.ts. Notes are MIDI numbers (-1 = free);
// Date.now() becomes a monotonic counter (only ordering matters).
class VoiceAllocator
{
public:
    static constexpr int kNumChannels = 6;

    int allocate (int note)
    {
        ++clock;

        for (int i = 0; i < kNumChannels; ++i)
            if (voices[i].note == note)
            {
                voices[i].timestamp = clock;
                return i;
            }

        for (int i = 0; i < kNumChannels; ++i)
            if (voices[i].note == kFree)
            {
                voices[i] = { note, clock };
                return i;
            }

        // Voice stealing: oldest voice
        int oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < kNumChannels; ++i)
            if (voices[i].timestamp < oldestTime)
            {
                oldestTime = voices[i].timestamp;
                oldest = i;
            }
        voices[oldest] = { note, clock };
        return oldest;
    }

    int release (int note)
    {
        for (int i = 0; i < kNumChannels; ++i)
            if (voices[i].note == note)
            {
                voices[i].note = kFree;
                return i;
            }
        return -1;
    }

    int getChannel (int note) const
    {
        for (int i = 0; i < kNumChannels; ++i)
            if (voices[i].note == note)
                return i;
        return -1;
    }

    void releaseAll()
    {
        for (auto& v : voices)
            v.note = kFree;
    }

private:
    static constexpr int kFree = -1;
    struct Voice { int note = kFree; uint64_t timestamp = 0; };
    std::array<Voice, kNumChannels> voices;
    uint64_t clock = 0;
};
