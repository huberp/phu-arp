#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <algorithm>

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
 *   // Remove from chord
 *   tracker.removeChordNote(60);
 */
class ChordNotesTracker {
private:
    std::vector<juce::MidiMessage> chordNotes;  // Sorted list of chord notes
    
public:
    /**
     * Get the number of notes in the chord
     */
    size_t getChordSize() const {
        return chordNotes.size();
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
     * @return Pointer to MidiMessage or nullptr if not found
     */
    const juce::MidiMessage* getChordNoteByIndex(int chordIndex) const {
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
     * @param channel MIDI channel (1-16)
     */
    void insertChordNote(int noteNumber, int velocity, int channel = 1) {
        auto newNote = juce::MidiMessage::noteOn(channel, noteNumber, static_cast<juce::uint8>(velocity));
        chordNotes.push_back(newNote);
        std::sort(chordNotes.begin(), chordNotes.end(),
            [](const juce::MidiMessage& a, const juce::MidiMessage& b) {
                return a.getNoteNumber() < b.getNoteNumber();
            });
    }
    
    /**
     * Remove a chord note by its MIDI note number
     * 
     * @param noteNumber MIDI note number to remove
     * @return true if note was found and removed
     */
    bool removeChordNote(int noteNumber) {
        auto it = std::find_if(chordNotes.begin(), chordNotes.end(),
            [noteNumber](const juce::MidiMessage& msg) { 
                return msg.getNoteNumber() == noteNumber; 
            });
        
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
     * Get all chord notes
     */
    const std::vector<juce::MidiMessage>& getChordNotes() const {
        return chordNotes;
    }
};
