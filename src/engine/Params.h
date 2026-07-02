#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FMPatch.h"

// APVTS layout: 8 globals + 4 ops × 11 = 52 automatable params.
// Ranges/defaults mirror the web app patch model (FMPatch.ts piano patch).
namespace params
{

inline juce::String opId (int op, const char* name) { return "op" + juce::String (op + 1) + "_" + name; }

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using IntParam = juce::AudioParameterInt;
    using FloatParam = juce::AudioParameterFloat;
    using BoolParam = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<IntParam> ("alg", "Algorithm", 0, 7, fmpatch::kDefaultAlgorithm));
    layout.add (std::make_unique<IntParam> ("fb", "Feedback", 0, 7, fmpatch::kDefaultFeedback));
    layout.add (std::make_unique<IntParam> ("lfo_freq", "LFO Freq", 0, 7, 0));
    layout.add (std::make_unique<IntParam> ("lfo_ams", "LFO AMS", 0, 3, 0));
    layout.add (std::make_unique<IntParam> ("lfo_pms", "LFO PMS", 0, 7, 0));
    layout.add (std::make_unique<FloatParam> ("master_gain", "Master Gain",
                                              juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<FloatParam> ("pan", "Pan",
                                              juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<BoolParam> ("poly", "Polyphony", true));

    for (int op = 0; op < 4; ++op)
    {
        const auto& d = fmpatch::kDefaultEnvelopes[op];
        layout.add (std::make_unique<IntParam> (opId (op, "ar"), opId (op, "AR"), 0, 31, d.ar));
        layout.add (std::make_unique<IntParam> (opId (op, "dr"), opId (op, "DR"), 0, 31, d.dr));
        layout.add (std::make_unique<IntParam> (opId (op, "sr"), opId (op, "SR"), 0, 31, d.sr));
        layout.add (std::make_unique<IntParam> (opId (op, "rr"), opId (op, "RR"), 0, 15, d.rr));
        layout.add (std::make_unique<IntParam> (opId (op, "sl"), opId (op, "SL"), 0, 15, d.sl));
        layout.add (std::make_unique<IntParam> (opId (op, "tl"), opId (op, "TL"), 0, 127, d.tl));
        layout.add (std::make_unique<IntParam> (opId (op, "ks"), opId (op, "KS"), 0, 3, d.ks));
        layout.add (std::make_unique<IntParam> (opId (op, "mul"), opId (op, "MUL"), 0, 15, d.mul));
        layout.add (std::make_unique<IntParam> (opId (op, "dt"), opId (op, "DT"), 0, 7, d.dt));
        layout.add (std::make_unique<BoolParam> (opId (op, "am"), opId (op, "AM"), d.am != 0));
        layout.add (std::make_unique<IntParam> (opId (op, "ssgeg"), opId (op, "SSG-EG"), 0, 15, d.ssgEg));
    }

    return layout;
}

} // namespace params
