# ChordPatternCoordinator

## Overview

`ChordPatternCoordinator` implements the MIDI event processing algorithm from the Lua Protoplug version. It coordinates the interaction between chord notes (channel 1) and rhythm pattern notes (channel 16) to produce output MIDI notes (channel 2).

## Architecture Decision: MidiMessage vs MidiMessageMetadata

**Decision: Use MidiMessage in ChordNotesTracker and PatternTracker, but handle sample positions in ChordPatternCoordinator.**

### Rationale:

1. **ChordNotesTracker**: Stores chord note templates without timing information. These are just the notes that make up the current chord. ✅ MidiMessage is sufficient.

2. **PatternTracker**: Tracks which chord notes are currently playing, but doesn't need to store exact timing - just which notes are active. ✅ MidiMessage is sufficient.

3. **ChordPatternCoordinator**: Processes MIDI buffers and needs sample-accurate timing within each audio block. It works with sample positions directly when:
   - Reading events from input buffer (using `MidiBuffer::Iterator` which provides both message and position)
   - Creating output events (using `MidiBuffer::addEvent(message, samplePosition)`)
   
   ✅ Uses sample positions but doesn't need to refactor other classes.

### JUCE MIDI Buffer Processing

In JUCE, when iterating over a `MidiBuffer`, you get:
```cpp
for (const auto metadata : midiBuffer) {
    const juce::MidiMessage& message = metadata.getMessage();
    int samplePosition = metadata.samplePosition;
    // Process...
}
```

The `metadata` is essentially a `MidiMessageMetadata` struct, but we don't need to store it - we just extract the components we need.

## Algorithm Overview

The Lua version's key insight was **processing order and time causality matter**.

### Processing Steps (current implementation)

1. **Copy all events to a temporary buffer**
   - Needed because hosts may deliver events grouped/sorted in non-time-causal ways

2. **Sort events by `samplePosition` (time-causal)**
   - Primary key: `samplePosition`
   - Tie-breaker at the same position (stable priority):
     1) rhythm note-offs (ch 16)
     2) chord updates (ch 1)
     3) rhythm note-ons (ch 16)
   - Treats **note-on with velocity 0** as note-off

3. **Apply events in order**
   - Chord updates mutate `ChordNotesTracker`
   - Rhythm note-ons compute chord index + octave offset and emit output note-ons
   - Rhythm note-offs emit output note-offs

4. **Write output events to the MIDI buffer (Channel 2)**
   - Input buffer is cleared and replaced with generated output events
   - Sample positions are preserved exactly (no “pos-1” hacks)

## Key Concepts

### MIDI Channels

- **Channel 1**: Chord note input (defines the chord)
- **Channel 16**: Rhythm pattern input (triggers chord notes)
- **Channel 2**: Output (actual sound)

### Rhythm Root Note

Default: **C1 (MIDI note 24)**

This is the "root" note for computing which chord note to play:
- Note 24 (C1) → Chord index 0 (first chord note)
- Note 25 (C#1) → Chord index 1 (second chord note)
- Note 26 (D1) → Chord index 2 (third chord note)
- ...
- Note 35 (B1) → Chord index 11 (twelfth chord note)

### Octave Offset

Pattern notes above the root octave trigger chord notes in higher octaves:
- Note 24 (C1) → Chord index 0, octave 0 → If chord[0] = 60, plays 60
- Note 36 (C2) → Chord index 0, octave +12 → If chord[0] = 60, plays 72
- Note 48 (C3) → Chord index 0, octave +24 → If chord[0] = 60, plays 84

Formula:
```cpp
int relativeNote = rhythmNote - rhythmRootNote;
int chordIndex = abs(relativeNote % 12);        // 0-11
int octaveOffset = (relativeNote / 12) * 12;    // 0, 12, 24, ...
```

Implementation note: for rhythm notes below the root note, the modulo is normalized into the range `[0..11]` and the octave offset uses `floor(relative / 12)` so negative values behave correctly.

## Usage Example

```cpp
#include "ChordPatternCoordinator.h"

// Setup
ChordNotesTracker chordTracker;
PatternTracker patternTracker(chordTracker);
ChordPatternCoordinator coordinator(chordTracker, patternTracker);

// Build a chord (C major: C-E-G)
chordTracker.insertChordNote(60, 100, 1);  // C4
chordTracker.insertChordNote(64, 100, 1);  // E4
chordTracker.insertChordNote(67, 100, 1);  // G4

// In your audio processing callback:
void processBlock(juce::MidiBuffer& midiBuffer, int numSamples) {
    // Process the buffer
    coordinator.processBlock(midiBuffer);
    
    // midiBuffer now contains output notes on channel 2
}
```

## Class Structure

```
ChordNotesTracker
├── Stores chord notes (sorted by note number)
├── Methods: insertChordNote(), removeChordNote(), getChordNoteByIndex()
└── Uses: juce::MidiMessage

PatternTracker
├── Tracks currently playing notes
├── Methods:
│   ├── startPlayingRhythmOwnedNote()  # store concrete output note per rhythm trigger
│   ├── stopPlayingNotesForRhythmOwner()
│   └── stopAllPlayingNotes()
├── Static utilities: computeChordIndex(), computeOctaveOffset()
└── Uses: juce::MidiMessage + (originalChordIndex, octaveOffset, ownerRhythmNote)

ChordPatternCoordinator
├── Coordinates chord and pattern processing
├── Implements the main algorithm with proper event ordering
├── Methods: processBlock()
└── Uses: Sample positions with juce::MidiMessage
```

## Implementation Notes

### Avoiding Unnecessary Copying

The implementation minimizes MIDI message copying:

1. **Input events**: Copied once to temporary buffer for ordering
   - Necessary because processing order matters
   - Lua version also does this

2. **Output events**: Built incrementally in `outputEvents` vector
   - Uses `emplace_back` to construct in-place
   - Only copied once when adding to final MidiBuffer

3. **Chord notes**: Stored as templates in ChordNotesTracker
   - Not copied per-event
   - Looked up by index when needed

### Thread Safety

Not thread-safe. Designed to be called from a single audio thread. If you need multi-threaded access, add appropriate locking.

### Performance Considerations

- Temporary event buffer size is pre-reserved based on input buffer size
- Output events use `std::vector` with efficient memory allocation
- Chord lookups are O(1) by index
- Playing note tracking uses linear search (acceptable for typical note counts)

## Differences from Lua Version

1. **Type Safety**: C++ with JUCE types vs Lua tables
2. **Channel Constants**: Named constants instead of magic numbers
3. **Error Handling**: Explicit nullptr checks
4. **Sample Position**: Events are processed time-causally within a block and output preserves the original `samplePosition`
5. **Note-off Robustness**: Note-offs are derived from notes actually emitted at note-on time (ownership-based), not from the current chord state
6. **Separation of Concerns**: Three classes instead of global state

## Testing

See `ChordPatternCoordinatorExample.cpp` for comprehensive examples:

1. **example1_BasicChordPattern**: Basic chord pattern processing
2. **example2_OctaveOffset**: Octave offset demonstration
3. **example3_ChordChangesDuringPlayback**: Chord changes with playing notes
4. **example4_MultipleSimultaneousNotes**: Multiple overlapping notes

## Future Enhancements

Possible improvements:
- Support for configurable input/output channels
- Velocity mapping/transformation
- Note duration limiting
- MIDI CC pass-through
- Multiple rhythm root notes

## Known limitations

- **Duplicate chord notes**: `ChordNotesTracker` currently allows duplicate note numbers if the host sends repeated note-ons.
- **Same output pitch from multiple triggers**: MIDI note-off has no voice identity; if two different rhythm triggers produce the same pitch on the same output channel, releasing one trigger may silence the other depending on the target synth.
