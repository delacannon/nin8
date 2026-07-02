#include "PluginEditor.h"

#if ! NINEIGHT_NO_WEBVIEW

NineightAudioProcessorEditor::NineightAudioProcessorEditor (NineightAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      bridge (p),
      webView (bridge.makeOptions())
{
    addAndMakeVisible (webView);
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

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
    webView.setBounds (getLocalBounds());
}

void NineightAudioProcessorEditor::timerCallback()
{
    bridge.emitTimerEvents (webView);
}

void NineightAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::MessageManager::callAsync ([this, parameterID, newValue]
    {
        bridge.emitParamChanged (webView, parameterID, newValue);
    });
}

#endif
