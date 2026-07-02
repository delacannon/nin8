#pragma once

#include <array>
#include <atomic>
#include <cstdint>

// Post-mix mono ring buffer feeding the UI oscilloscope (replaces the web
// AnalyserNode's getByteTimeDomainData). Audio thread writes, GUI timer reads
// the most recent kWindow samples.
class WaveformTap
{
public:
    static constexpr int kWindow = 2048; // matches web AnalyserNode fftSize

    void push (const float* left, const float* right, int numSamples)
    {
        auto pos = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            buffer[pos & kMask] = 0.5f * (left[i] + right[i]);
            ++pos;
        }
        writePos.store (pos, std::memory_order_release);
    }

    // GUI thread: copy latest window as AnalyserNode-style bytes (128 = zero)
    void readBytes (uint8_t* dest) const
    {
        const auto end = writePos.load (std::memory_order_acquire);
        const auto start = end - kWindow;
        for (int i = 0; i < kWindow; ++i)
        {
            const float s = buffer[(start + (uint64_t) i) & kMask];
            const int v = (int) (s * 128.0f) + 128;
            dest[i] = (uint8_t) (v < 0 ? 0 : (v > 255 ? 255 : v));
        }
    }

private:
    static constexpr uint64_t kMask = 4095;
    std::array<float, 4096> buffer {};
    std::atomic<uint64_t> writePos { kWindow };
};
