#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Forward declaration
class PhuArpAudioProcessorEditor;

/**
 * EditorLogger
 * 
 * Custom JUCE Logger that forwards log messages to the plugin editor's log view.
 * Thread-safe and uses AsyncUpdater to ensure GUI updates happen on the message thread.
 * 
 * Usage:
 *   Logger::setCurrentLogger(editorLogger.get());
 *   Logger::getCurrentLogger()->writeToLog("Your message");
 *   // or
 *   LOG_MESSAGE("Your message");
 */
class EditorLogger : public juce::Logger,
                     public juce::AsyncUpdater
{
public:
    EditorLogger() = default;
    ~EditorLogger() override = default;
    
    /**
     * Set the editor that will receive log messages
     * @param editor Pointer to the editor (uses SafePointer internally)
     */
    void setEditor(PhuArpAudioProcessorEditor* editor);
    
    /**
     * Clear the editor reference (call when editor is destroyed)
     */
    void clearEditor();
    
    /**
     * Logger override - called from any thread
     * Adds message to queue and triggers async update
     */
    void logMessage(const juce::String& message) override;
    
protected:
    /**
     * AsyncUpdater override - called on message thread
     * Flushes pending messages to the editor
     */
    void handleAsyncUpdate() override;
    
private:
    // Thread-safe message queue
    juce::CriticalSection messageLock;
    juce::StringArray pendingMessages;
    
    // Safe pointer to editor (automatically nulled when editor is destroyed)
    juce::Component::SafePointer<PhuArpAudioProcessorEditor> editor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorLogger)
};

// Convenience macro for logging
#define LOG_MESSAGE(msg) \
    do { \
        if (auto* logger = juce::Logger::getCurrentLogger()) \
            logger->writeToLog(msg); \
    } while(0)
