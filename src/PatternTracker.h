#pragma once

#include "ChordNotesTracker.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <algorithm>
#include <cmath>

/**
 * PatternTracker
 * 
 * Manages pattern playback by tracking which chord notes are currently playing.
 * Handles the mapping between rhythm patterns and chord notes.
 * 
 * Key responsibilities:
 * - Track currently playing notes from the chord
 * - Start/stop notes with octave offsets
 * - Compute chord index and octave offset from rhythm patterns
 * 
 * Usage Pattern:
 *   ChordNotesTracker chordTracker;
 *   PatternTracker patternTracker(chordTracker);
 *   
 *   // Start playing chord notes
 *   patternTracker.startPlayingNote(0, 12);  // Play first chord note + octave
 *   patternTracker.stopPlayingNote(0, 12);   // Stop first chord note + octave
 */
class PatternTracker {
public:
    /**
     * Represents a note that is currently playing
     * Includes the octave offset applied when it was triggered
     */
    struct PlayingNote {
        juce::MidiMessage message;      // JUCE MidiMessage containing note information
        int originalChordIndex;         // Index in chord that triggered this (at note-on time)
        int octaveOffset;               // Octave offset used when triggered (at note-on time)
        int ownerRhythmNote;            // Rhythm input note number that owns this note (-1 if unknown)
        
        PlayingNote(const juce::MidiMessage& msg,
                    int chordIdx = -1,
                    int octaveOffsetSemitones = 0,
                    int rhythmOwnerNote = -1)
            : message(msg)
            , originalChordIndex(chordIdx)
            , octaveOffset(octaveOffsetSemitones)
            , ownerRhythmNote(rhythmOwnerNote)
        {}
        
        // Helper methods for easy access
        int getNoteNumber() const { return message.getNoteNumber(); }
        int getVelocity() const { return message.getVelocity(); }
        int getChannel() const { return message.getChannel(); }
    };

private:
    ChordNotesTracker& chordTracker;       // Reference to chord tracker
    std::vector<PlayingNote> playingNotes; // Currently playing notes
    
public:
    /**
     * Constructor
     * @param tracker Reference to the ChordNotesTracker
     */
    explicit PatternTracker(ChordNotesTracker& tracker)
        : chordTracker(tracker) {}
    
    /**
     * Get the number of notes currently playing
     */
    size_t getPlayingNotesCount() const {
        return playingNotes.size();
    }
    
    /**
     * Start playing a chord note with an octave offset
     * Adds to the tracking list of currently playing notes
     * 
     * @param chordIndex Index in chord (0-based)
     * @param octaveOffset Octave offset in semitones (typically multiples of 12)
     * @return The actual MIDI note number being played, or -1 if chord index invalid
     */
    int startPlayingNote(int chordIndex, int octaveOffset = 0) {
        const juce::MidiMessage* chordNote = chordTracker.getChordNoteByIndex(chordIndex);
        if (chordNote == nullptr) {
            return -1;
        }
        
        int actualNote = chordNote->getNoteNumber() + octaveOffset;
        auto playingMessage = juce::MidiMessage::noteOn(
            2, 
            actualNote, 
            static_cast<juce::uint8>(chordNote->getVelocity())
        );
        PlayingNote playingNote(playingMessage, chordIndex, octaveOffset, -1);
        playingNotes.push_back(playingNote);
        
        return actualNote;
    }

    /**
     * Start playing a note owned by a rhythm input note.
     * This stores the concrete output note number so it can be stopped later even if the chord changes.
     *
     * @param rhythmNoteNumber Rhythm input note number (channel 16 note)
     * @param actualNote Concrete output MIDI note number
     * @param velocity Velocity to use for the note-on
     * @param channel Output channel (default: 2)
     * @param chordIndex Index in chord at note-on time (optional, for diagnostics)
     * @param octaveOffset Octave offset at note-on time (optional, for diagnostics)
     */
    void startPlayingRhythmOwnedNote(int rhythmNoteNumber,
                                    int actualNote,
                                    juce::uint8 velocity,
                                    int channel = 2,
                                    int chordIndex = -1,
                                    int octaveOffset = 0) {
        auto playingMessage = juce::MidiMessage::noteOn(channel, actualNote, velocity);
        playingNotes.emplace_back(playingMessage, chordIndex, octaveOffset, rhythmNoteNumber);
    }
    
    /**
     * Stop playing note(s) with a specific chord index and octave offset
     * Removes matching notes from the playing list
     * 
     * @param chordIndex Index in chord (0-based)
     * @param octaveOffset Octave offset in semitones
     * @return Number of notes stopped
     */
    int stopPlayingNote(int chordIndex, int octaveOffset = 0) {
        // Remove all playing notes that were started with this chordIndex + octaveOffset.
        // This is intentionally independent from the *current* chord state.
        auto oldSize = playingNotes.size();
        playingNotes.erase(
            std::remove_if(playingNotes.begin(), playingNotes.end(),
                [chordIndex, octaveOffset](const PlayingNote& pn) {
                    return pn.originalChordIndex == chordIndex && pn.octaveOffset == octaveOffset;
                }),
            playingNotes.end());
        
        return static_cast<int>(oldSize - playingNotes.size());
    }

    /**
     * Stop all notes owned by a specific rhythm input note.
     * Returns the notes that were stopped (so the caller can emit matching note-offs).
     */
    std::vector<PlayingNote> stopPlayingNotesForRhythmOwner(int rhythmNoteNumber) {
        std::vector<PlayingNote> stopped;
        stopped.reserve(4);

        std::vector<PlayingNote> remaining;
        remaining.reserve(playingNotes.size());

        for (auto& note : playingNotes) {
            if (note.ownerRhythmNote == rhythmNoteNumber) {
                stopped.push_back(note);
            } else {
                remaining.push_back(note);
            }
        }

        playingNotes = std::move(remaining);
        return stopped;
    }
    
    /**
     * Stop all currently playing notes
     * @return Number of notes that were playing
     */
    int stopAllPlayingNotes() {
        int count = static_cast<int>(playingNotes.size());
        playingNotes.clear();
        return count;
    }
    
    /**
     * Get all currently playing notes as note-off messages
     * Useful for generating note-offs before clearing (e.g., when DAW stops)
     * 
     * @param channel MIDI channel for the note-off events (default: 2, the output channel)
     * @return Vector of note-off MIDI messages for all playing notes
     */
    std::vector<juce::MidiMessage> getAllPlayingNotesAsNoteOffs(int channel = 2) const {
        std::vector<juce::MidiMessage> noteOffs;
        noteOffs.reserve(playingNotes.size());
        
        for (const auto& playingNote : playingNotes) {
            auto noteOff = juce::MidiMessage::noteOff(
                channel,
                playingNote.getNoteNumber(),
                static_cast<juce::uint8>(playingNote.getVelocity())
            );
            noteOffs.push_back(noteOff);
        }
        
        return noteOffs;
    }
    
    /**
     * Get all currently playing notes
     */
    const std::vector<PlayingNote>& getPlayingNotes() const {
        return playingNotes;
    }
    
    /**
     * Compute chord note index from a rhythm note
     * 
     * @param rhythmNote MIDI note number from rhythm pattern
     * @param rootNote Root note to calculate relative to (e.g., 24 for C1)
     * @return Chord note index (0-based, 0-11)
     */
    static int computeChordIndex(int rhythmNote, int rootNote) {
        int relativeNoteIndex = rhythmNote - rootNote;
        int mod = relativeNoteIndex % 12;
        if (mod < 0) {
            mod += 12;
        }
        return mod;  // 0-based [0..11]
    }
    
    /**
     * Compute octave offset from a rhythm note
     * 
     * @param rhythmNote MIDI note number from rhythm pattern
     * @param rootNote Root note to calculate relative to (e.g., 24 for C1)
     * @return Octave offset in semitones (multiples of 12)
     */
    static int computeOctaveOffset(int rhythmNote, int rootNote) {
        int relativeNoteIndex = rhythmNote - rootNote;
        return static_cast<int>(std::floor(relativeNoteIndex / 12.0)) * 12;
    }
};
