#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "../PluginProcessor.h"

// JS ↔ C++ bridge for the embedded NINEIGHT web UI.
// Native fns mirror the AudioWorklet message channel of the web app plus
// param/note entry points; see JuceBackend.ts on the web side.
class WebViewBridge
{
public:
    explicit WebViewBridge (NineightAudioProcessor& p) : processor (p) {}

    juce::WebBrowserComponent::Options makeOptions();

    // 30 Hz GUI timer: waveform + host-MIDI note events → JS
    void emitTimerEvents (juce::WebBrowserComponent& webView);

    // APVTS → JS knob updates (host automation / preset load)
    void emitParamChanged (juce::WebBrowserComponent& webView, const juce::String& id, float value);

private:
    juce::var buildReadyInfo();
    void handlePost (const juce::var& msg);
    void setParamFromUi (const juce::String& id, float value);

    NineightAudioProcessor& processor;
    std::unique_ptr<juce::ZipFile> uiZip;
    juce::CriticalSection zipLock;
};
