#include "PluginEditor.h"

NineightAudioProcessorEditor::NineightAudioProcessorEditor (NineightAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (1280, 800);
}

void NineightAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff260e04));
    g.setColour (juce::Colour (0xff7b5e57));
    g.setFont (24.0f);
    g.drawText ("NINEIGHT — UI pending (Phase 3)", getLocalBounds(), juce::Justification::centred);
}
