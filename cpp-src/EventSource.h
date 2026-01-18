#pragma once

#include <vector>
#include <algorithm>
#include "Event.h"
#include "EventListener.h"

/**
 * Base EventSource template
 * 
 * Provides common listener management functionality for all event sources.
 * Eliminates code duplication between different EventSource types.
 * 
 * @tparam ListenerType The listener interface type (e.g., GlobalsEventListener)
 */
template<typename ListenerType>
class EventSource {
protected:
    std::vector<ListenerType*> listeners;
    
public:
    virtual ~EventSource() = default;
    
    /**
     * Add a listener
     * @param listener Pointer to listener (must outlive this EventSource or be removed)
     * @return The listener pointer (for chaining or storing the handle)
     */
    ListenerType* addEventListener(ListenerType* listener) {
        if (listener && std::find(listeners.begin(), listeners.end(), listener) == listeners.end()) {
            listeners.push_back(listener);
        }
        return listener;
    }
    
    /**
     * Remove a listener
     * @param listener Pointer to listener to remove
     * @return true if listener was found and removed
     */
    bool removeEventListener(ListenerType* listener) {
        auto it = std::find(listeners.begin(), listeners.end(), listener);
        if (it != listeners.end()) {
            listeners.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * Get number of registered listeners
     */
    size_t getListenerCount() const {
        return listeners.size();
    }
};

/**
 * EventSource for GLOBALS events
 * 
 * Manages listeners for BPM, IsPlaying, and SampleRate events.
 * Mirrors the Lua EventSource mixed into GLOBALS table.
 * 
 * Usage:
 *   GlobalsEventSource globals;
 *   globals.addEventListener(&myListener);
 *   globals.fireBPMChanged(bpmEvent);
 */
class GlobalsEventSource : public EventSource<GlobalsEventListener> {
public:
    /**
     * Fire a BPM changed event to all listeners
     * @param event The BPM event to fire
     */
    void fireBPMChanged(const BPMEvent& event) {
        // Iterate by index to handle potential modifications during iteration
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->onBPMChanged(event);
        }
    }
    
    /**
     * Fire an IsPlaying changed event to all listeners
     * @param event The IsPlaying event to fire
     */
    void fireIsPlayingChanged(const IsPlayingEvent& event) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->onIsPlayingChanged(event);
        }
    }
    
    /**
     * Fire a SampleRate changed event to all listeners
     * @param event The SampleRate event to fire
     */
    void fireSampleRateChanged(const SampleRateEvent& event) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->onSampleRateChanged(event);
        }
    }
};

/**
 * EventSource for BUFFERS events
 * 
 * Manages listeners for buffer configuration changes.
 * Mirrors the Lua EventSource mixed into BUFFERS table.
 * 
 * Usage:
 *   BufferEventSource buffers;
 *   buffers.addEventListener(&myListener);
 *   buffers.fireBuffersChanged(buffersEvent);
 */
class BufferEventSource : public EventSource<BufferEventListener> {
public:
    /**
     * Fire a buffers changed event to all listeners
     * @param event The buffer event to fire
     */
    void fireBuffersChanged(const BuffersChangedEvent& event) {
        for (size_t i = 0; i < listeners.size(); ++i) {
            listeners[i]->onBuffersChanged(event);
        }
    }
};
