#include "PluginEditor.h"
#include "PluginProcessor.h"

PhuArpAudioProcessorEditor::PhuArpAudioProcessorEditor(PhuArpAudioProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {}
PhuArpAudioProcessorEditor::~PhuArpAudioProcessorEditor() {}

void PhuArpAudioProcessorEditor::paint(juce::Graphics& g) {}
void PhuArpAudioProcessorEditor::resized() {}