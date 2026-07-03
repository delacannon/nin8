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

    // Resizable, 16:10 like the 1280x800 design; the Phaser canvas scale-fits
    setResizable (true, true);
    setResizeLimits (640, 400, 2560, 1600);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (1280.0 / 800.0);
    setSize (processorRef.editorWidth.load(), processorRef.editorHeight.load());
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
    processorRef.editorWidth.store (getWidth());
    processorRef.editorHeight.store (getHeight());
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
