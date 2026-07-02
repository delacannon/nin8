// Offline render check: default piano patch, C4 for 1s + 0.5s release → WAV.
// Usage: render_test <rhythm_rom.bin> <out.wav> [sampleRate]
#include <cstdio>
#include <cstring>
#include <vector>
#include "../src/engine/Synth.h"

static void writeWav (const char* path, const std::vector<float>& interleaved, int sampleRate)
{
    FILE* f = fopen (path, "wb");
    const uint32_t dataSize = (uint32_t) (interleaved.size() * 2);
    const uint32_t riffSize = 36 + dataSize;
    const uint16_t channels = 2, bits = 16, blockAlign = 4;
    const uint32_t byteRate = (uint32_t) sampleRate * blockAlign;
    fwrite ("RIFF", 1, 4, f); fwrite (&riffSize, 4, 1, f); fwrite ("WAVEfmt ", 1, 8, f);
    uint32_t fmtSize = 16; uint16_t pcm = 1;
    fwrite (&fmtSize, 4, 1, f); fwrite (&pcm, 2, 1, f); fwrite (&channels, 2, 1, f);
    fwrite (&sampleRate, 4, 1, f); fwrite (&byteRate, 4, 1, f);
    fwrite (&blockAlign, 2, 1, f); fwrite (&bits, 2, 1, f);
    fwrite ("data", 1, 4, f); fwrite (&dataSize, 4, 1, f);
    for (float s : interleaved)
    {
        const int v = (int) (s * 32767.0f);
        const int16_t i16 = (int16_t) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
        fwrite (&i16, 2, 1, f);
    }
    fclose (f);
}

int main (int argc, char** argv)
{
    if (argc < 3) { fprintf (stderr, "usage: %s rom.bin out.wav [rate]\n", argv[0]); return 1; }
    const int rate = argc > 3 ? atoi (argv[3]) : 44100;

    FILE* rf = fopen (argv[1], "rb");
    if (! rf) { fprintf (stderr, "no rom\n"); return 1; }
    std::vector<uint8_t> rom (8192);
    rom.resize (fread (rom.data(), 1, rom.size(), rf));
    fclose (rf);

    Synth synth;
    synth.prepare (rate, rom.data(), rom.size());
    synth.noteOn (60); // C4

    const int noteSamples = rate, tailSamples = rate / 2;
    std::vector<float> l ((size_t) (noteSamples + tailSamples)), r (l.size());
    synth.render (l.data(), r.data(), noteSamples);
    synth.noteOff (60);
    synth.render (l.data() + noteSamples, r.data() + noteSamples, tailSamples);

    double sumSq = 0, peak = 0;
    for (size_t i = 0; i < l.size(); ++i)
    {
        sumSq += l[i] * l[i];
        if (std::fabs (l[i]) > peak) peak = std::fabs (l[i]);
    }
    printf ("rms=%.5f peak=%.5f (%zu samples @ %d Hz)\n",
            std::sqrt (sumSq / (double) l.size()), peak, l.size(), rate);

    std::vector<float> inter (l.size() * 2);
    for (size_t i = 0; i < l.size(); ++i) { inter[i * 2] = l[i]; inter[i * 2 + 1] = r[i]; }
    writeWav (argv[2], inter, rate);
    return peak > 0.01 ? 0 : 2; // fail if silent
}
