#pragma once

#include <cstdint>
#include "OpnaChip.h"

// Verbatim port of the AudioWorklet resampling loop (opna-processor.js process()),
// including its t = accumulator interpolation quirk, to keep output identical to
// the web app: accumulator linear interpolation, scale 1/24576 (-3dB headroom).
class Resampler
{
public:
    void prepare (double nativeRate, double outputRate)
    {
        ratio = nativeRate / outputRate;
        accumulator = 0.0;
        prevL = prevR = lastL = lastR = 0;
    }

    void renderNextSample (OpnaChip& chip, float& outL, float& outR)
    {
        accumulator += ratio;
        while (accumulator >= 1.0)
        {
            prevL = lastL;
            prevR = lastR;
            chip.generate (lastL, lastR);
            accumulator -= 1.0;
        }
        const float t = (float) accumulator;
        constexpr float scale = 1.0f / 24576.0f;
        outL = ((1.0f - t) * (float) prevL + t * (float) lastL) * scale;
        outR = ((1.0f - t) * (float) prevR + t * (float) lastR) * scale;
    }

private:
    double ratio = 1.0;
    double accumulator = 0.0;
    int32_t prevL = 0, prevR = 0, lastL = 0, lastR = 0;
};
