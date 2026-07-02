#pragma once

#include "PluginProcessor.h"

#if ! NINEIGHT_NO_WEBVIEW
#include "bridge/WebViewBridge.h"

class NineightAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer,
                                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit NineightAudioProcessorEditor (NineightAudioProcessor&);
    ~NineightAudioProcessorEditor() override;

    void resized() override;

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    NineightAudioProcessor& processorRef;
    WebViewBridge bridge;
    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NineightAudioProcessorEditor)
};

#else

// Fallback editor for hosts where the WebView backend is unavailable
class NineightAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NineightAudioProcessorEditor (NineightAudioProcessor& p)
        : AudioProcessorEditor (&p), generic (p)
    {
        addAndMakeVisible (generic);
        setSize (600, 800);
    }

    void resized() override { generic.setBounds (getLocalBounds()); }

private:
    juce::GenericAudioProcessorEditor generic;
};

#endif
