#pragma once

#include <JuceHeader.h>

class PhuArpAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    PhuArpAudioProcessorEditor(PhuArpAudioProcessor&);
    ~PhuArpAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    PhuArpAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhuArpAudioProcessorEditor)
};