#include "EditorLogger.h"
#include "PluginEditor.h"

void EditorLogger::setEditor(PhuArpAudioProcessorEditor* newEditor)
{
    juce::ScopedLock lock(messageLock);
    editor = newEditor;
}

void EditorLogger::clearEditor()
{
    juce::ScopedLock lock(messageLock);
    editor = nullptr;
}

void EditorLogger::logMessage(const juce::String& message)
{
    // This can be called from any thread
    juce::ScopedLock lock(messageLock);
    pendingMessages.add(message);
    
    // Trigger async update to flush on message thread
    triggerAsyncUpdate();
}

void EditorLogger::handleAsyncUpdate()
{
    // This is called on the message thread
    juce::StringArray messages;
    
    // Swap out pending messages atomically
    {
        juce::ScopedLock lock(messageLock);
        messages.swapWith(pendingMessages);
    }
    
    // Send all messages to the editor
    if (editor != nullptr)
    {
        for (const auto& msg : messages)
        {
            editor->addLogMessage(msg);
        }
    }
}
