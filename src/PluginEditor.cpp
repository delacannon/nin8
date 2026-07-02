#include "PluginEditor.h"

#if ! NINEIGHT_NO_WEBVIEW
#include "bridge/HostEnvSanitizer.h"

NineightAudioProcessorEditor::NineightAudioProcessorEditor (NineightAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      bridge (p)
{
    {
        // The webview helper is forked in this constructor — keep host bundle
        // env vars (Ardour et al.) away from it or webkit fails white
        ScopedHostEnvSanitizer envGuard;
        webView = std::make_unique<juce::WebBrowserComponent> (bridge.makeOptions());
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    for (auto* param : processorRef.getParameters())
        if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            processorRef.parameters().addParameterListener (withID->paramID, this);

    setSize (1280, 800);
    startTimerHz (30);
}

NineightAudioProcessorEditor::~NineightAudioProcessorEditor()
{
    for (auto* param : processorRef.getParameters())
        if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            processorRef.parameters().removeParameterListener (withID->paramID, this);
}

void NineightAudioProcessorEditor::resized()
{
    webView->setBounds (getLocalBounds());
}

void NineightAudioProcessorEditor::timerCallback()
{
    bridge.emitTimerEvents (*webView);

    std::map<juce::String, float> toSend;
    {
        const juce::ScopedLock sl (pendingLock);
        toSend.swap (pendingParams);
    }
    for (const auto& [id, value] : toSend)
        bridge.emitParamChanged (*webView, id, value);
}

void NineightAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    const juce::ScopedLock sl (pendingLock);
    pendingParams[parameterID] = newValue;
}

#endif
