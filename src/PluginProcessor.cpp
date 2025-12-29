#include "PluginProcessor.h"

PhuArpAudioProcessor::PhuArpAudioProcessor()
    : AudioProcessor(BusesProperties()) // MIDI effect - no audio buses
{
}

PhuArpAudioProcessor::~PhuArpAudioProcessor() {}

void PhuArpAudioProcessor::prepareToPlay(double, int) {}
void PhuArpAudioProcessor::releaseResources() {}

void PhuArpAudioProcessor::processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer& midiMessages)
{
    // MIDI processing only
}

juce::AudioProcessorEditor* PhuArpAudioProcessor::createEditor() { return nullptr; }
bool PhuArpAudioProcessor::hasEditor() const { return false; }

const juce::String PhuArpAudioProcessor::getName() const { return "PhuArp"; }
bool PhuArpAudioProcessor::acceptsMidi() const { return true; }
bool PhuArpAudioProcessor::producesMidi() const { return true; }
bool PhuArpAudioProcessor::isMidiEffect() const { return true; }
double PhuArpAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PhuArpAudioProcessor::getNumPrograms() { return 1; }
int PhuArpAudioProcessor::getCurrentProgram() { return 0; }
void PhuArpAudioProcessor::setCurrentProgram(int) {}
const juce::String PhuArpAudioProcessor::getProgramName(int) { return "Default"; }
void PhuArpAudioProcessor::changeProgramName(int, const juce::String&) {}

void PhuArpAudioProcessor::getStateInformation(juce::MemoryBlock&) {}
void PhuArpAudioProcessor::setStateInformation(const void*, int) {}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhuArpAudioProcessor();
}