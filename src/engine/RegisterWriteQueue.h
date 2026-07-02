#pragma once

#include <juce_core/juce_core.h>
#include <array>

// SPSC queue: message thread (WebView bridge / param listeners) → audio thread.
// Mirrors the AudioWorklet postMessage channel of the web app.
struct EngineCommand
{
    enum Type : int { writeReg, noteOn, noteOff, allNotesOff, resetChip };
    Type type = writeReg;
    int a = 0; // writeReg: port  | noteOn/noteOff: midi note
    int b = 0; // writeReg: addr
    int c = 0; // writeReg: data
};

class CommandQueue
{
public:
    bool push (const EngineCommand& cmd)
    {
        const auto scope = fifo.write (1);
        if (scope.blockSize1 == 0)
            return false;
        buffer[(size_t) scope.startIndex1] = cmd;
        return true;
    }

    template <typename Fn>
    void drain (Fn&& fn)
    {
        const auto ready = fifo.getNumReady();
        const auto scope = fifo.read (ready);
        for (int i = 0; i < scope.blockSize1; ++i)
            fn (buffer[(size_t) (scope.startIndex1 + i)]);
        for (int i = 0; i < scope.blockSize2; ++i)
            fn (buffer[(size_t) (scope.startIndex2 + i)]);
    }

private:
    static constexpr int kCapacity = 8192;
    juce::AbstractFifo fifo { kCapacity };
    std::array<EngineCommand, kCapacity> buffer;
};
