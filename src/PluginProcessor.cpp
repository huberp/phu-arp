#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EditorLogger.h"
#include "../lib/EventSource.h"

PhuArpAudioProcessor::PhuArpAudioProcessor()
    : AudioProcessor(BusesProperties()) // MIDI effect - no audio buses
    , patternTracker(chordTracker)
    , coordinator(chordTracker, patternTracker)
    , editorLogger(std::make_unique<EditorLogger>())
{
    // Register coordinator as listener for DAW global events
    syncGlobals.addEventListener(&coordinator);
    
    // Set our logger as the global JUCE logger
    juce::Logger::setCurrentLogger(editorLogger.get());
    
    // Log initialization
    LOG_MESSAGE("PhuArp plugin initialized");
}

PhuArpAudioProcessor::~PhuArpAudioProcessor() 
{
    // Unregister from events
    syncGlobals.removeEventListener(&coordinator);
    
    // Clear logger if it's ours
    if (juce::Logger::getCurrentLogger() == editorLogger.get())
    {
        juce::Logger::setCurrentLogger(nullptr);
    }
}

void PhuArpAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    syncGlobals.updateSampleRate(sampleRate);
}

void PhuArpAudioProcessor::releaseResources() {}

void PhuArpAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Get playhead position info
    auto playHeadPtr = getPlayHead();
    auto positionInfo = playHeadPtr ? playHeadPtr->getPosition() : juce::Optional<juce::AudioPlayHead::PositionInfo>();
    
    // Update DAW globals
    syncGlobals.updateDAWGlobals(
        buffer,
        midiMessages,
        positionInfo
    );
    
    // Test logging (can be removed later)
    static int blockCount = 0;
    if (++blockCount % 1000 == 0)
    {
        LOG_MESSAGE("Processed " + juce::String(blockCount) + " audio blocks");
    }
    if(syncGlobals.isDawPlaying()) {
        // Process chord pattern coordination
        coordinator.processBlock(midiMessages);
    }
    // Mark end of processing
    syncGlobals.finishRun(buffer.getNumSamples());
}

juce::AudioProcessorEditor* PhuArpAudioProcessor::createEditor() 
{ 
    auto* editor = new PhuArpAudioProcessorEditor(*this);
    
    // Register editor with logger
    if (editorLogger)
    {
        editorLogger->setEditor(editor);
        LOG_MESSAGE("Editor opened");
    }
    
    return editor;
}

bool PhuArpAudioProcessor::hasEditor() const { return true; }

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