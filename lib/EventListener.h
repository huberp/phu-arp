#pragma once

#include "Event.h"

/**
 * Listener interface for GLOBALS events (BPM, IsPlaying, SampleRate)
 * 
 * Objects that need to respond to DAW global state changes should inherit
 * from this interface and implement the relevant callbacks.
 * 
 * Mirrors the Lua pattern:
 *   GLOBALS:addEventListener(function(inEvent) ... end)
 */
class GlobalsEventListener {
public:
    virtual ~GlobalsEventListener() = default;
    
    /**
     * Called when BPM changes
     * @param event Contains old and new BPM values plus derived timing info
     */
    virtual void onBPMChanged(const BPMEvent& event) {
        // Default empty implementation - override if needed
        (void)event; // Suppress unused warning
    }
    
    /**
     * Called when playback starts or stops
     * @param event Contains old and new playing state
     */
    virtual void onIsPlayingChanged(const IsPlayingEvent& event) {
        // Default empty implementation - override if needed
        (void)event;
    }
    
    /**
     * Called when sample rate changes
     * @param event Contains old and new sample rate
     */
    virtual void onSampleRateChanged(const SampleRateEvent& event) {
        // Default empty implementation - override if needed
        (void)event;
    }
};

/**
 * Listener interface for BUFFERS events
 * 
 * Objects that need to respond to buffer changes should inherit
 * from this interface.
 * 
 * Mirrors the Lua pattern:
 *   BUFFERS:addEventListener(function(inEvent) ... end)
 */
class BufferEventListener {
public:
    virtual ~BufferEventListener() = default;
    
    /**
     * Called when buffers are reconfigured
     * @param event Contains buffer configuration details
     */
    virtual void onBuffersChanged(const BuffersChangedEvent& event) = 0;
};

/**
 * Example: A consumer that listens to both GLOBALS and BUFFERS
 * 
 * This demonstrates the multi-source listening pattern used in your Lua code:
 *   GLOBALS:addEventListener(function(inEvent) BUFFERS:listenToGlobalsChange(inEvent) end)
 *   BUFFERS:addEventListener(function(inEvent) CLIENT_PATHS:listenToBufferChanges(inEvent) end)
 */
class MultiSourceListener : public GlobalsEventListener, public BufferEventListener {
public:
    // Can override any or all methods from both interfaces
    void onBPMChanged(const BPMEvent& event) override {
        // Handle BPM changes
    }
    
    void onBuffersChanged(const BuffersChangedEvent& event) override {
        // Handle buffer changes
    }
    
    // onIsPlayingChanged and onSampleRateChanged use default implementations
};
