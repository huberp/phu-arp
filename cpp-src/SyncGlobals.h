#pragma once

#include "EventSource.h"
#include <cstddef>

/**
 * SyncGlobals - Singleton that tracks DAW global state
 * 
 * This is a C++ translation of the Lua SyncGlobals module.
 * Tracks BPM, sample rate, and playing state, firing events when they change.
 * 
 * Usage:
 *   auto& globals = SyncGlobals::getInstance();
 *   globals.addEventListener(&myListener);
 *   globals.updateDAWGlobals(samples, numSamples, midiBuffer, position);
 */
class SyncGlobals : public GlobalsEventSource {
private:
    // PPQ (Pulses Per Quarter) base values
    struct PPQBaseValue {
        double msec = 60000.0;        // milliseconds per minute
        double noteNum = 1.0;
        double noteDenom = 4.0;
        double ratio = 0.25;           // noteNum / noteDenom
    } ppqBase;
    
    // Global state
    int runs = 0;                      // Number of processBlock calls
    long long samplesCount = 0;        // Total samples processed
    int sampleRate = -1;
    double sampleRateByMsec = -1.0;    // Samples per millisecond
    bool isPlaying = false;
    double bpm = 0.0;
    double msecPerBeat = 0.0;          // Based on whole note
    double samplesPerBeat = 0.0;       // Based on whole note
    
    // Singleton instance
    static SyncGlobals instance;
    
    // Private constructor for singleton
    SyncGlobals() = default;
    
public:
    // Delete copy constructor and assignment
    SyncGlobals(const SyncGlobals&) = delete;
    SyncGlobals& operator=(const SyncGlobals&) = delete;
    
    /**
     * Get singleton instance
     */
    static SyncGlobals& getInstance() {
        return instance;
    }
    
    /**
     * Mark end of processing run
     * @param numSamples Number of samples processed in this block
     */
    void finishRun(int numSamples) {
        runs++;
        samplesCount += numSamples;
    }
    
    /**
     * Get total samples processed
     */
    long long getCurrentSampleCount() const {
        return samplesCount;
    }
    
    /**
     * Get current BPM
     */
    double getBPM() const {
        return bpm;
    }
    
    /**
     * Get current sample rate
     */
    int getSampleRate() const {
        return sampleRate;
    }
    
    /**
     * Check if DAW is playing
     */
    bool isDawPlaying() const {
        return isPlaying;
    }
    
    /**
     * Update sample rate (fires event if changed)
     * @param newSampleRate New sample rate in Hz
     */
    void updateSampleRate(int newSampleRate) {
        if (newSampleRate != sampleRate) {
            int oldSampleRate = sampleRate;
            sampleRate = newSampleRate;
            sampleRateByMsec = newSampleRate / 1000.0;
            
            // Recalculate samples per beat if we have a BPM
            if (bpm > 0.0) {
                samplesPerBeat = msecPerBeat * sampleRateByMsec;
            }
            
            // Fire event
            SampleRateEvent event;
            event.source = this;
            event.oldRate = oldSampleRate;
            event.newRate = newSampleRate;
            
            fireSampleRateChanged(event);
        }
    }
    
    /**
     * Update DAW globals at the beginning of a new frame
     * This mirrors the Lua updateDAWGlobals function
     * 
     * @param samples Pointer to sample buffer
     * @param numSamples Number of samples in this frame
     * @param midiBuffer Pointer to MIDI buffer
     * @param position Pointer to DAW position info
     * @return Context object for this frame
     */
    Event::Context updateDAWGlobals(void* samples, int numSamples, 
                                     void* midiBuffer, void* position) {
        // Create context for this frame
        Event::Context ctx;
        ctx.samples = samples;
        ctx.numberOfSamplesInFrame = numSamples;
        ctx.midiBuffer = midiBuffer;
        ctx.dawPosition = position;
        ctx.epoch = runs;
        
        // In a real implementation, you would extract BPM and isPlaying from position
        // For now, this is a placeholder structure
        // double newBPM = extractBPM(position);
        // bool newIsPlaying = extractIsPlaying(position);
        
        // Example: Check BPM change
        // if (newBPM != bpm) {
        //     BPMEvent event;
        //     event.source = this;
        //     event.context = ctx;
        //     event.oldValues = {bpm, msecPerBeat, samplesPerBeat};
        //     
        //     // Update values
        //     bpm = newBPM;
        //     msecPerBeat = ppqBase.msec / newBPM;
        //     samplesPerBeat = msecPerBeat * sampleRateByMsec;
        //     
        //     event.newValues = {bpm, msecPerBeat, samplesPerBeat};
        //     fireBPMChanged(event);
        // }
        
        // Example: Check playing state change
        // if (newIsPlaying != isPlaying) {
        //     IsPlayingEvent event;
        //     event.source = this;
        //     event.context = ctx;
        //     event.oldValue = isPlaying;
        //     event.newValue = newIsPlaying;
        //     isPlaying = newIsPlaying;
        //     fireIsPlayingChanged(event);
        // }
        
        return ctx;
    }
};

// Define the singleton instance
inline SyncGlobals SyncGlobals::instance;
