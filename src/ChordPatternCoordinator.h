#pragma once

#include "ChordNotesTracker.h"
#include "PatternTracker.h"
#include "../lib/SyncGlobalsListener.h"
#include "EditorLogger.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <atomic>
#include <vector>

/**
 * ChordPatternCoordinator
 * 
 * Coordinates the processing of chord notes (channel 1) and rhythm pattern notes (channel 16)
 * to produce output notes (channel 2). This implements the algorithm from the Lua version.
 * 
 * Processing order is critical:
 * 1. Process rhythm pattern note-offs first (to prevent hanging notes during chord changes)
 * 2. Process chord note updates (channel 1 note-on/off)
 * 3. Process rhythm pattern note-ons (to trigger chord notes)
 * 
 * Key concepts:
 * - Channel 1: Chord note input (defines which notes are in the chord)
 * - Channel 16: Rhythm pattern input (triggers chord notes with optional octave offset)
 * - Channel 2: Output notes (actual MIDI output)
 * - RHYTHM_ROOT_NOTE: Base note (e.g., C1 = 24) for computing chord index from pattern
 * 
 * Usage Pattern:
 *   ChordNotesTracker chordTracker;
 *   PatternTracker patternTracker(chordTracker);
 *   ChordPatternCoordinator coordinator(chordTracker, patternTracker);
 *   
 *   // In processBlock:
 *   coordinator.processBlock(midiBuffer);
 */
class ChordPatternCoordinator : public GlobalsEventListener {
public:
    /**
     * Event with sample position for proper timing
     */
    struct TimedEvent {
        juce::MidiMessage message;
        int samplePosition;
        
        TimedEvent(const juce::MidiMessage& msg, int pos)
            : message(msg), samplePosition(pos) {}
    };

private:
    ChordNotesTracker& chordTracker;
    PatternTracker& patternTracker;
    int rhythmRootNote;                    // Root note for rhythm pattern (default: C1 = 24)
    EditorLogger* logger = nullptr;        // Instance-scoped logger (optional)

    // If true, MIDI on channels other than chord/rhythm/output will be preserved.
    // MIDI on chord/rhythm/output channels is consumed/replaced by this processor.
    std::atomic<bool> passThroughOtherMidi { false };

    // MIDI routing channels (1..16)
    int chordInputChannel = 1;
    int rhythmInputChannel = 16;
    int outputChannel = 2;

    // Scratch buffers reused per audio block to avoid heap churn on the audio thread.
    // (Performance/RT-safety improvement: avoids per-block allocations.)
    std::vector<TimedEvent> tempEventBuffer;
    std::vector<TimedEvent> outputEvents;
    
    static constexpr int defaultChordInputChannel = 1;
    static constexpr int defaultRhythmInputChannel = 16;
    static constexpr int defaultOutputChannel = 2;

public:
    /**
     * Constructor
     * @param chordTracker Reference to the ChordNotesTracker
     * @param patternTracker Reference to the PatternTracker
     * @param rootNote Root note for rhythm pattern (default: C1 = 24)
     */
    ChordPatternCoordinator(ChordNotesTracker& chordTracker, 
                           PatternTracker& patternTracker,
                           int rootNote = 24,
                           EditorLogger* loggerToUse = nullptr)
        : chordTracker(chordTracker)
        , patternTracker(patternTracker)
        , rhythmRootNote(rootNote)
        , logger(loggerToUse)
        , chordInputChannel(defaultChordInputChannel)
        , rhythmInputChannel(defaultRhythmInputChannel)
        , outputChannel(defaultOutputChannel)
    {}

    void setLogger(EditorLogger* loggerToUse) noexcept { logger = loggerToUse; }
    EditorLogger* getLogger() const noexcept { return logger; }

    void setChordInputChannel(int channel) noexcept { chordInputChannel = channel; }
    int getChordInputChannel() const noexcept { return chordInputChannel; }

    void setRhythmInputChannel(int channel) noexcept { rhythmInputChannel = channel; }
    int getRhythmInputChannel() const noexcept { return rhythmInputChannel; }

    void setOutputChannel(int channel) noexcept { outputChannel = channel; }
    int getOutputChannel() const noexcept { return outputChannel; }

    void setPassThroughOtherMidi(bool shouldPassThrough) noexcept { passThroughOtherMidi.store(shouldPassThrough, std::memory_order_relaxed); }
    bool getPassThroughOtherMidi() const noexcept { return passThroughOtherMidi.load(std::memory_order_relaxed); }
    
    /**
     * Set the rhythm root note
     * @param rootNote MIDI note number (e.g., 24 for C1)
     */
    void setRhythmRootNote(int rootNote) {
        rhythmRootNote = rootNote;
    }
    
    /**
     * Get the rhythm root note
     * @return MIDI note number
     */
    int getRhythmRootNote() const {
        return rhythmRootNote;
    }
    
    /**
     * Process a block of MIDI events
     *
     * Purpose:
     * - Consume incoming chord input (ch 1) and rhythm trigger input (ch 16)
     * - Produce output note events (ch 2) with deterministic timing and robust note-off matching
     *
     * Edge cases this implementation is designed to tackle (see ChordPatternCoordinator_ProblemCases.md):
     * 1) Rhythm note On + Off in the same block (must respect time order)
     * 2) Rhythm note Off + On in the same block (must not create phantom/stuck notes)
     * 3) Chord updates between rhythm On and Off in the same block (avoid "time travel")
     * 4) Chord changes while a rhythm note is held across blocks (note-off must match the note-on)
     * 5) Chord index shifts while held (insert/remove notes changes indices)
     * 6) Chord cleared / chord too small before note-off (must still be able to stop)
     * 8) Repeated rhythm NoteOn without NoteOff (retrigger should be deterministic)
     * 9) Rhythm note below root (negative-relative mapping must be correct)
     * 10) Off-by-one timing hack (avoid shifting note-ons earlier to "fix" ordering)
     *
     * Notes:
     * - Some cases (e.g. "same output pitch from multiple triggers") require voice ownership/ref-counting
     *   beyond what plain MIDI note-on/off semantics can guarantee.
     *
    * @param midiBuffer The MIDI buffer to process.
    *                  If passThroughOtherMidi is false, it will be cleared and filled with output events.
    *                  If passThroughOtherMidi is true, only events on the chord/rhythm/output channels are removed
    *                  and other channels are preserved.
     */
    void processBlock(juce::MidiBuffer& midiBuffer) {
        const bool passThrough = getPassThroughOtherMidi();

        // Step 1: Copy all events to temporary buffer for ordered processing
        // We need to do this because the DAW might provide events sorted by channel,
        // but we need to process them in a specific order
        tempEventBuffer.clear();
        if (tempEventBuffer.capacity() < static_cast<size_t>(midiBuffer.getNumEvents())) {
            tempEventBuffer.reserve(midiBuffer.getNumEvents());
        }
        
        for (const auto metadata : midiBuffer) {
            tempEventBuffer.emplace_back(metadata.getMessage(), metadata.samplePosition);
        }
        
        // Prepare output events buffer
        outputEvents.clear();
        if (outputEvents.capacity() < tempEventBuffer.size()) {
            outputEvents.reserve(tempEventBuffer.size());
        }

        // Step 2: Make event processing time-causal.
        // This directly addresses edge cases 1, 2, 3 by ensuring we never reorder events
        // across time within the audio block.
        // Sort by sample position, and for events at the same sample position apply a stable priority:
        // 1) Rhythm note-offs
        // 2) Chord updates
        // 3) Rhythm note-ons
        auto isNoteOffLike = [](const juce::MidiMessage& msg) {
            // Treat NoteOn velocity=0 as NoteOff (common MIDI encoding).
            return msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0);
        };
        auto phasePriority = [&](const juce::MidiMessage& msg) -> int {
            const int ch = msg.getChannel();
            if (ch == rhythmInputChannel) {
                if (isNoteOffLike(msg)) {
                    return 0;
                }
                if (msg.isNoteOn()) {
                    return 2;
                }
            }
            if (ch == chordInputChannel) {
                if (msg.isNoteOn() || isNoteOffLike(msg)) {
                    return 1;
                }
            }
            return 3;
        };

        // Stable lexicographic ordering: (samplePosition, phasePriority).
        // Prevents edge cases 1, 2, 3 (and removes the need for edge-case-10 timestamp hacks).
        std::stable_sort(tempEventBuffer.begin(), tempEventBuffer.end(),
            [&](const TimedEvent& a, const TimedEvent& b) {
                if (a.samplePosition != b.samplePosition) {
                    return a.samplePosition < b.samplePosition;
                }
                return phasePriority(a.message) < phasePriority(b.message);
            });

        auto stopRhythmOwnedNotes = [&](int samplePosition, int rhythmNoteNumber) {
            // Ownership-based stopping: the note-off is derived from what was actually turned on.
            // Prevents edge cases 4, 5, 6 (and makes retriggers for edge case 8 deterministic).
            auto stoppedNotes = patternTracker.stopPlayingNotesForRhythmOwner(rhythmNoteNumber);
            for (const auto& stopped : stoppedNotes) {
                auto noteOffMsg = juce::MidiMessage::noteOff(
                    outputChannel,
                    stopped.getNoteNumber(),
                    static_cast<juce::uint8>(stopped.getVelocity())
                );
                outputEvents.emplace_back(noteOffMsg, samplePosition);
            }
        };

        auto startRhythmOwnedNote = [&](int samplePosition, int rhythmNoteNumber) {
            // Ensure retriggers are clean for the same rhythm key.
            // Addresses edge case 8.
            stopRhythmOwnedNotes(samplePosition, rhythmNoteNumber);

            // Correct index mapping even for rhythm notes below the root.
            // Addresses edge case 9.
            const int chordIndex = PatternTracker::computeChordIndex(rhythmNoteNumber, rhythmRootNote);
            const int octaveOffset = PatternTracker::computeOctaveOffset(rhythmNoteNumber, rhythmRootNote);

            const juce::MidiMessage* chordNote = chordTracker.getChordNoteByIndex(chordIndex);
            if (chordNote == nullptr) {
                return;
            }

            const int actualNote = chordNote->getNoteNumber() + octaveOffset;
            const auto velocity = static_cast<juce::uint8>(chordNote->getVelocity());

            // Store the concrete output note for this rhythm trigger so future note-offs do not depend
            // on the *current* chord content/indexing.
            // Prevents edge cases 4, 5, 6.
            patternTracker.startPlayingRhythmOwnedNote(
                rhythmNoteNumber,
                actualNote,
                velocity,
                outputChannel,
                chordIndex,
                octaveOffset
            );

            // Emit note-on at the actual sample position (no -1 shifting).
            // Addresses edge case 10.
            auto noteOnMsg = juce::MidiMessage::noteOn(outputChannel, actualNote, velocity);
            outputEvents.emplace_back(noteOnMsg, samplePosition);
        };

        // Step 3: Process the (now ordered) event stream.
        for (const auto& evt : tempEventBuffer) {
            const auto& msg = evt.message;

            if (msg.getChannel() == rhythmInputChannel) {
                if (isNoteOffLike(msg)) {
                    stopRhythmOwnedNotes(evt.samplePosition, msg.getNoteNumber());
                } else if (msg.isNoteOn()) {
                    startRhythmOwnedNote(evt.samplePosition, msg.getNoteNumber());
                }
                continue;
            }

            if (msg.getChannel() == chordInputChannel) {
                if (msg.isNoteOn() && msg.getVelocity() > 0) {
                    chordTracker.insertChordNote(
                        msg.getNoteNumber(),
                        msg.getVelocity(),
                        msg.getChannel()
                    );
                } else if (isNoteOffLike(msg)) {
                    chordTracker.removeChordNote(msg.getNoteNumber());
                }
                continue;
            }
        }
        
        // Step 5: Write back to the MIDI buffer
        if (passThrough) {
            // Keep everything except chord/rhythm/output channels, then add generated output.
            juce::MidiBuffer filtered;

            for (const auto metadata : midiBuffer) {
                const auto& msg = metadata.getMessage();
                const bool isConsumed =
                    msg.isForChannel(chordInputChannel) ||
                    msg.isForChannel(rhythmInputChannel) ||
                    msg.isForChannel(outputChannel);

                if (!isConsumed) {
                    filtered.addEvent(msg, metadata.samplePosition);
                }
            }

            for (const auto& evt : outputEvents) {
                filtered.addEvent(evt.message, evt.samplePosition);
            }

            midiBuffer.swapWith(filtered);
        } else {
            midiBuffer.clear();
            for (const auto& evt : outputEvents) {
                midiBuffer.addEvent(evt.message, evt.samplePosition);
            }
        }
    }
    
    /**
     * Handle DAW play/stop state changes
     * When DAW stops, send note-offs for all playing notes and clear state
     */
    void onIsPlayingChanged(const IsPlayingEvent& event) override {
        // When DAW stops playing, clear all notes and chord
        if (event.oldValue == true && event.newValue == false)
        {
            LOG_MESSAGE(logger, "DAW stopped - cleaning up notes");
            
            // Get the MIDI buffer from the event context
            auto* midiBuffer = const_cast<juce::MidiBuffer*>(event.context.midiBuffer);
            
            if (midiBuffer)
            {
                midiBuffer->clear();
                // Get note-off events for all playing notes before clearing
                auto noteOffs = patternTracker.getAllPlayingNotesAsNoteOffs(outputChannel);
                
                LOG_MESSAGE(logger, "Sending " + juce::String(noteOffs.size()) + " note-off events");
                
                // Add note-offs directly to the MIDI buffer at sample position 0
                for (const auto& noteOff : noteOffs)
                {
                    midiBuffer->addEvent(noteOff, 0);
                }
            }
            
            // Now stop all currently playing notes (clears internal state)
            patternTracker.stopAllPlayingNotes();
            
            // Clear all stored chord notes
            chordTracker.clearChord();
            LOG_MESSAGE(logger, "Cleared all playing notes and chord");
        }
    }

private:
    // Note: helper logic for processing is now inside processBlock to keep
    // the ordering rules and state transitions close to where the event stream is consumed.
};
