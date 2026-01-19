#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

/**
 * ChordNotesTracker
 * 
 * Manages a collection of chord notes, allowing insertion, removal, and queries.
 * This is a C++ translation of the CHORD_EVENTS functionality from the Lua code.
 * 
 * Key responsibilities:
 * - Store chord notes in sorted order (by MIDI note number)
 * - Add/remove notes dynamically
 * - Query notes by index
 * - Track which chord notes are currently playing
 * 
 * Usage Pattern:
 *   ChordNotesTracker tracker;
 *   
 *   // Build chord
 *   tracker.insertChordNote(60, 100);  // C4, velocity 100
 *   tracker.insertChordNote(64, 100);  // E4
 *   tracker.insertChordNote(67, 100);  // G4
 *   
 *   // Query chord notes
 *   auto* note = tracker.getChordNoteByIndex(0);  // Get first note (C4)
 *   
 *   // Track playing notes
 *   tracker.startPlayingNote(0, 12);  // Play first chord note + octave
 *   tracker.stopPlayingNote(0, 12);   // Stop first chord note + octave
 *   
 *   // Remove from chord
 *   tracker.removeChordNote(60);
 */
class ChordNotesTracker {
public:
    /**
     * Represents a MIDI note with number and velocity
     */
    struct Note {
        int noteNumber;      // MIDI note number (0-127)
        int velocity;        // MIDI velocity (0-127)
        int channel;         // MIDI channel (1-16)
        
        Note(int note = 0, int vel = 0, int chan = 1) 
            : noteNumber(note), velocity(vel), channel(chan) {}
        
        bool operator<(const Note& other) const {
            return noteNumber < other.noteNumber;
        }
        
        bool operator==(const Note& other) const {
            return noteNumber == other.noteNumber;
        }
    };
    
    /**
     * Represents a note that is currently playing
     * Includes the octave offset applied when it was triggered
     */
    struct PlayingNote {
        int noteNumber;      // Actual MIDI note being played
        int velocity;
        int channel;
        int originalChordIndex;  // Index in chord that triggered this
        
        PlayingNote(int note, int vel, int chan, int chordIdx = -1)
            : noteNumber(note), velocity(vel), channel(chan), originalChordIndex(chordIdx) {}
    };
    
private:
    std::vector<Note> chordNotes;         // Sorted list of chord notes
    std::vector<PlayingNote> playingNotes; // Currently playing notes
    
public:
    /**
     * Get the number of notes in the chord
     */
    size_t getChordSize() const {
        return chordNotes.size();
    }
    
    /**
     * Get the number of notes currently playing
     */
    size_t getPlayingNotesCount() const {
        return playingNotes.size();
    }
    
    /**
     * Check if chord is empty
     */
    bool isChordEmpty() const {
        return chordNotes.empty();
    }
    
    /**
     * Get a chord note by its index (0-based)
     * Returns nullptr if index is out of bounds
     * 
     * @param chordIndex Index in chord (0-based)
     * @return Pointer to Note or nullptr if not found
     */
    const Note* getChordNoteByIndex(int chordIndex) const {
        if (chordIndex < 0 || chordIndex >= static_cast<int>(chordNotes.size())) {
            return nullptr;
        }
        return &chordNotes[chordIndex];
    }
    
    /**
     * Insert a chord note
     * Always adds a new note to the chord and keeps them sorted by MIDI note number
     * Note: This allows duplicate note numbers if inserted multiple times
     * 
     * @param noteNumber MIDI note number (0-127)
     * @param velocity MIDI velocity (0-127)
     * @param channel MIDI channel (default: 1)
     */
    void insertChordNote(int noteNumber, int velocity, int channel = 1) {
        Note newNote(noteNumber, velocity, channel);
        chordNotes.push_back(newNote);
        std::sort(chordNotes.begin(), chordNotes.end());
    }
    
    /**
     * Remove a chord note by its MIDI note number
     * 
     * @param noteNumber MIDI note number to remove
     * @return true if note was found and removed
     */
    bool removeChordNote(int noteNumber) {
        auto it = std::find_if(chordNotes.begin(), chordNotes.end(),
            [noteNumber](const Note& n) { return n.noteNumber == noteNumber; });
        
        if (it != chordNotes.end()) {
            chordNotes.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * Clear all chord notes
     */
    void clearChord() {
        chordNotes.clear();
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
        const Note* chordNote = getChordNoteByIndex(chordIndex);
        if (chordNote == nullptr) {
            return -1;
        }
        
        int actualNote = chordNote->noteNumber + octaveOffset;
        PlayingNote playingNote(actualNote, chordNote->velocity, chordNote->channel, chordIndex);
        playingNotes.push_back(playingNote);
        
        return actualNote;
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
        const Note* chordNote = getChordNoteByIndex(chordIndex);
        if (chordNote == nullptr) {
            return 0;
        }
        
        int noteToStop = chordNote->noteNumber + octaveOffset;
        
        // Remove all playing notes with this note number
        auto oldSize = playingNotes.size();
        playingNotes.erase(
            std::remove_if(playingNotes.begin(), playingNotes.end(),
                [noteToStop](const PlayingNote& pn) { return pn.noteNumber == noteToStop; }),
            playingNotes.end());
        
        return static_cast<int>(oldSize - playingNotes.size());
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
     * Get all currently playing notes
     */
    const std::vector<PlayingNote>& getPlayingNotes() const {
        return playingNotes;
    }
    
    /**
     * Get all chord notes
     */
    const std::vector<Note>& getChordNotes() const {
        return chordNotes;
    }
    
    /**
     * Compute chord note index and octave offset from a rhythm note
     * 
     * @param rhythmNote MIDI note number from rhythm pattern
     * @param rootNote Root note to calculate relative to (e.g., 24 for C1)
     * @param outChordIndex Output: chord note index (0-based)
     * @param outOctaveOffset Output: octave offset in semitones
     */
    static void computeChordIndexAndOctave(int rhythmNote, int rootNote, 
                                           int& outChordIndex, int& outOctaveOffset) {
        int relativeNoteIndex = rhythmNote - rootNote;
        outOctaveOffset = static_cast<int>(std::floor(relativeNoteIndex / 12.0)) * 12;
        outChordIndex = std::abs(relativeNoteIndex % 12);  // 0-based
    }
};
