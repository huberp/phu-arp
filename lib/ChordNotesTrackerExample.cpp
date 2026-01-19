/**
 * ChordNotesTracker Usage Examples
 * 
 * Demonstrates the C++ translation of the Lua CHORD_EVENTS functionality
 */

#include "ChordNotesTracker.h"
#include <iostream>
#include <iomanip>

void printChord(const ChordNotesTracker& tracker) {
    std::cout << "Chord: [";
    const auto& notes = tracker.getChordNotes();
    for (size_t i = 0; i < notes.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << notes[i].noteNumber << "(" << notes[i].velocity << ")";
    }
    std::cout << "]" << std::endl;
}

void printPlayingNotes(const ChordNotesTracker& tracker) {
    std::cout << "Playing: [";
    const auto& notes = tracker.getPlayingNotes();
    for (size_t i = 0; i < notes.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << notes[i].noteNumber;
    }
    std::cout << "]" << std::endl;
}

/**
 * Example 1: Basic chord building and queries
 */
void example1_BasicChordOperations() {
    std::cout << "\n=== Example 1: Basic Chord Operations ===" << std::endl;
    
    ChordNotesTracker tracker;
    
    // Build a C major chord (C4, E4, G4)
    std::cout << "Building C major chord..." << std::endl;
    tracker.insertChordNote(60, 100);  // C4
    tracker.insertChordNote(64, 100);  // E4
    tracker.insertChordNote(67, 100);  // G4
    
    printChord(tracker);
    std::cout << "Chord size: " << tracker.getChordSize() << std::endl;
    
    // Query notes by index
    std::cout << "\nQuerying notes by index:" << std::endl;
    for (int i = 0; i < static_cast<int>(tracker.getChordSize()); ++i) {
        const auto* note = tracker.getChordNoteByIndex(i);
        if (note) {
            std::cout << "  Index " << i << ": Note " << note->noteNumber 
                      << ", Vel " << note->velocity << std::endl;
        }
    }
    
    // Remove middle note
    std::cout << "\nRemoving E4 (64)..." << std::endl;
    tracker.removeChordNote(64);
    printChord(tracker);
    
    // Add it back (notes stay sorted)
    std::cout << "\nAdding E4 back..." << std::endl;
    tracker.insertChordNote(64, 100);
    printChord(tracker);
}

/**
 * Example 2: Playing and stopping notes with octave offsets
 * This mirrors the Lua playChordNote and stopChordNote functions
 */
void example2_PlayingNotes() {
    std::cout << "\n=== Example 2: Playing Notes with Octave Offsets ===" << std::endl;
    
    ChordNotesTracker tracker;
    
    // Build a chord
    tracker.insertChordNote(60, 100);  // C4
    tracker.insertChordNote(64, 100);  // E4
    tracker.insertChordNote(67, 100);  // G4
    
    std::cout << "Chord: C major (C4, E4, G4)" << std::endl;
    
    // Play first note (index 0) at original octave
    std::cout << "\nPlaying chord note 0 (C4) at original octave..." << std::endl;
    int playedNote = tracker.startPlayingNote(0, 0);
    std::cout << "Started playing note: " << playedNote << std::endl;
    printPlayingNotes(tracker);
    
    // Play second note (index 1) one octave up
    std::cout << "\nPlaying chord note 1 (E4) one octave up..." << std::endl;
    playedNote = tracker.startPlayingNote(1, 12);
    std::cout << "Started playing note: " << playedNote << " (E5)" << std::endl;
    printPlayingNotes(tracker);
    
    // Play third note (index 2) two octaves down
    std::cout << "\nPlaying chord note 2 (G4) two octaves down..." << std::endl;
    playedNote = tracker.startPlayingNote(2, -24);
    std::cout << "Started playing note: " << playedNote << " (G2)" << std::endl;
    printPlayingNotes(tracker);
    
    // Stop first note
    std::cout << "\nStopping chord note 0 at original octave..." << std::endl;
    int stopped = tracker.stopPlayingNote(0, 0);
    std::cout << "Stopped " << stopped << " note(s)" << std::endl;
    printPlayingNotes(tracker);
    
    // Stop all
    std::cout << "\nStopping all playing notes..." << std::endl;
    stopped = tracker.stopAllPlayingNotes();
    std::cout << "Stopped " << stopped << " note(s)" << std::endl;
    printPlayingNotes(tracker);
}

/**
 * Example 3: Computing chord index and octave from rhythm notes
 * This mirrors the Lua logic for processing rhythm pattern triggers
 */
void example3_RhythmNoteMapping() {
    std::cout << "\n=== Example 3: Rhythm Note to Chord Mapping ===" << std::endl;
    
    ChordNotesTracker tracker;
    const int RHYTHM_ROOT_NOTE = 24;  // C1
    
    // Build a chord
    tracker.insertChordNote(60, 100);  // C4
    tracker.insertChordNote(64, 100);  // E4
    tracker.insertChordNote(67, 100);  // G4
    tracker.insertChordNote(72, 100);  // C5
    
    std::cout << "Chord: Extended C major" << std::endl;
    printChord(tracker);
    
    std::cout << "\nRhythm Root Note: " << RHYTHM_ROOT_NOTE << " (C1)" << std::endl;
    std::cout << "\nProcessing rhythm pattern notes:" << std::endl;
    
    // Test various rhythm notes
    int rhythmNotes[] = {24, 25, 26, 36, 37, 48};  // C1, C#1, D1, C2, C#2, C3
    
    for (int rhythmNote : rhythmNotes) {
        int chordIndex, octaveOffset;
        ChordNotesTracker::computeChordIndexAndOctave(
            rhythmNote, RHYTHM_ROOT_NOTE, chordIndex, octaveOffset);
        
        std::cout << "  Rhythm note " << std::setw(2) << rhythmNote 
                  << " -> Chord index " << chordIndex 
                  << ", Octave offset " << std::setw(3) << octaveOffset;
        
        // Show what would be played
        const auto* chordNote = tracker.getChordNoteByIndex(chordIndex);
        if (chordNote) {
            int resultNote = chordNote->noteNumber + octaveOffset;
            std::cout << " -> Plays note " << resultNote << std::endl;
        } else {
            std::cout << " -> No chord note at index!" << std::endl;
        }
    }
}

/**
 * Example 4: Simulating the Lua pattern - chord changes while notes are playing
 */
void example4_ChordChangesWithPlayingNotes() {
    std::cout << "\n=== Example 4: Chord Changes While Playing ===" << std::endl;
    
    ChordNotesTracker tracker;
    
    // Start with C major
    std::cout << "Building C major chord..." << std::endl;
    tracker.insertChordNote(60, 100);  // C4
    tracker.insertChordNote(64, 100);  // E4
    tracker.insertChordNote(67, 100);  // G4
    printChord(tracker);
    
    // Play some notes
    std::cout << "\nPlaying notes from chord..." << std::endl;
    tracker.startPlayingNote(0, 0);   // C4
    tracker.startPlayingNote(1, 0);   // E4
    tracker.startPlayingNote(2, 12);  // G5
    printPlayingNotes(tracker);
    
    // Change chord to C minor (change E to Eb)
    std::cout << "\nChanging to C minor (E4 -> Eb4)..." << std::endl;
    tracker.removeChordNote(64);      // Remove E4
    tracker.insertChordNote(63, 100); // Add Eb4
    printChord(tracker);
    
    // Note: In real usage, you'd stop playing notes that are no longer in chord
    std::cout << "\nNote: Playing notes list unchanged (would need manual cleanup)" << std::endl;
    printPlayingNotes(tracker);
    
    // Stop all to prevent hanging notes
    std::cout << "\nStopping all notes to prevent hanging notes..." << std::endl;
    tracker.stopAllPlayingNotes();
    printPlayingNotes(tracker);
}

/**
 * Example 5: Edge cases and error handling
 */
void example5_EdgeCases() {
    std::cout << "\n=== Example 5: Edge Cases ===" << std::endl;
    
    ChordNotesTracker tracker;
    
    // Query empty chord
    std::cout << "Querying empty chord..." << std::endl;
    const auto* note = tracker.getChordNoteByIndex(0);
    std::cout << "Result: " << (note ? "Found" : "nullptr") << std::endl;
    
    // Try to play from empty chord
    std::cout << "\nTrying to play from empty chord..." << std::endl;
    int result = tracker.startPlayingNote(0, 0);
    std::cout << "Result: " << result << " (should be -1)" << std::endl;
    
    // Add notes and test out-of-bounds
    tracker.insertChordNote(60, 100);
    tracker.insertChordNote(64, 100);
    
    std::cout << "\nChord has " << tracker.getChordSize() << " notes" << std::endl;
    std::cout << "Querying index -1 (invalid): " 
              << (tracker.getChordNoteByIndex(-1) ? "Found" : "nullptr") << std::endl;
    std::cout << "Querying index 2 (out of bounds): " 
              << (tracker.getChordNoteByIndex(2) ? "Found" : "nullptr") << std::endl;
    
    // Duplicate notes
    std::cout << "\nInserting duplicate note (60)..." << std::endl;
    printChord(tracker);
    tracker.insertChordNote(60, 127);  // Update velocity
    std::cout << "After duplicate insert (velocity updated):" << std::endl;
    printChord(tracker);
}

int main() {
    std::cout << "=== ChordNotesTracker Examples ===" << std::endl;
    std::cout << "C++ translation of Lua CHORD_EVENTS functionality" << std::endl;
    
    example1_BasicChordOperations();
    example2_PlayingNotes();
    example3_RhythmNoteMapping();
    example4_ChordChangesWithPlayingNotes();
    example5_EdgeCases();
    
    std::cout << "\n=== All Examples Complete ===" << std::endl;
    return 0;
}
