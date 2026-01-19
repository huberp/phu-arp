#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Base Event class - mirrors the Lua event table structure
 * All events carry a source pointer and context information
 */
struct Event {
    virtual ~Event() = default;
    
    // Source that fired this event
    const void* source = nullptr;
    
    // Context from DAW (position, samples, etc.)
    // This mirrors the CONTEXT field in Lua events
    struct Context {
        const juce::AudioBuffer<float>* buffer = nullptr;
        int numberOfSamplesInFrame = 0;
        const juce::MidiBuffer* midiBuffer = nullptr;
        const juce::Optional<juce::AudioPlayHead::PositionInfo>* positionInfo = nullptr;
        int epoch = 0;
    } context;
    
protected:
    Event() = default;
};

/**
 * BPM Changed Event
 * Fired when DAW tempo changes
 */
struct BPMEvent : public Event {
    struct Values {
        double bpm = 0.0;
        double msecPerBeat = 0.0;
        double samplesPerBeat = 0.0;
    };
    
    Values oldValues;
    Values newValues;
};

/**
 * Playing State Changed Event
 * Fired when DAW starts/stops playback
 */
struct IsPlayingEvent : public Event {
    bool oldValue = false;
    bool newValue = false;
};

/**
 * Sample Rate Changed Event
 * Fired when audio sample rate changes
 */
struct SampleRateEvent : public Event {
    double oldRate = 0.0;
    double newRate = 0.0;
};

/**
 * Buffers Changed Event
 * Fired when audio buffers are reconfigured
 */
struct BuffersChangedEvent : public Event {
    int numBeats = 0;
    int globalSize = 0;
    double samplesPerBeat = 0.0;
    
    // Additional buffer-specific data can be added here
    void* bufferData = nullptr;
};
