# phu-arp

JUCE-based **VST3 MIDI effect** that turns:

- **Chord input** (MIDI channel 1)
- **Rhythm/pattern triggers** (MIDI channel 16)

into **generated note output** (MIDI channel 2).

The core MIDI algorithm lives in `ChordPatternCoordinator` and is a C++ translation of the original Lua/Protoplug approach.

## Build (CMake + Visual Studio)

This repo is set up for CMake presets.

- Configure: `cmake --preset vs2026-x64`
- Build Debug: `cmake --build --preset debug`
- Build Release: `cmake --build --preset release`

The target built by the presets is `phu-arp_VST3`.

## MIDI routing

- **Ch 1**: chord definition (note on/off)
- **Ch 16**: rhythm triggers (note on/off)
- **Ch 2**: generated output notes

### How rhythm notes map to chord notes

The rhythm mapping uses a configurable **rhythm root note** (default **C1 = 24**). Each rhythm note triggers a corresponding chord note based on:
1. **Chord index**: determined by the semitone distance from the rhythm root note (modulo 12)
2. **Octave offset**: determined by which octave the rhythm note is in relative to the root

**Example with 3 chord notes (C major triad: C4, E4, G4):**

Rhythm notes in the C1 octave (24-35):
- C1 (24) → chord index 0 → plays C4 (60)
- C#1 (25) → chord index 1 → plays E4 (64)
- D1 (26) → chord index 2 → plays G4 (67)
- D#1 (27) → chord index 3 → no note (chord only has 3 notes)
- E1 (28) → chord index 4 → no note
- F1 (29) → chord index 5 → no note
- F#1 (30) → chord index 6 → no note
- G1 (31) → chord index 7 → no note
- G#1 (32) → chord index 8 → no note
- A1 (33) → chord index 9 → no note
- A#1 (34) → chord index 10 → no note
- B1 (35) → chord index 11 → no note

Rhythm notes in the C2 octave (36-47):
- C2 (36) → chord index 0, octave +12 → plays C5 (72)
- C#2 (37) → chord index 1, octave +12 → plays E5 (76)
- D2 (38) → chord index 2, octave +12 → plays G5 (79)

**Example with 4 chord notes (C major 7th: C4, E4, G4, B4):**

Rhythm notes in the C1 octave (24-35):
- C1 (24) → chord index 0 → plays C4 (60)
- C#1 (25) → chord index 1 → plays E4 (64)
- D1 (26) → chord index 2 → plays G4 (67)
- D#1 (27) → chord index 3 → plays B4 (71)
- E1 (28) → chord index 4 → no note (chord only has 4 notes)
- F1 (29) → chord index 5 → no note
- F#1 (30) → chord index 6 → no note
- G1 (31) → chord index 7 → no note
- G#1 (32) → chord index 8 → no note
- A1 (33) → chord index 9 → no note
- A#1 (34) → chord index 10 → no note
- B1 (35) → chord index 11 → no note

**Formula:**
```
relativeNote = rhythmNote - rhythmRootNote
chordIndex = relativeNote % 12             // 0-11, wraps around chord notes
  (if negative, normalized to 0-11)
octaveOffset = floor(relativeNote / 12) * 12  // octave adjustment
outputNote = chordNote[chordIndex] + octaveOffset
  (no note played if chord doesn't have enough notes)
```

This design allows you to create rhythm patterns that cycle through chord notes, with different octaves accessible by playing higher or lower on your MIDI controller.

## How to setup in Bitwig Studio

phu-arp requires two MIDI sources: one for chords and one for rhythm patterns. Here's how to set this up in Bitwig Studio:

### Step 1: Add the phu-arp plugin
1. Create an **Instrument Track** for your target synthesizer/sound
2. In the FX chain *before* the instrument, add **phu-arp** as a MIDI effect (VST3)
3. phu-arp will process MIDI and pass the result to your instrument

### Step 2: Setup a chord track (MIDI Channel 1)
1. Create a **MIDI track** and name it "Chords"
2. Add a simple instrument to this track (or leave it empty—we just need MIDI)
3. Record or draw chord notes on this track (e.g., C-E-G for C major)
4. Important: Make sure this track sends on **MIDI Channel 1**
5. Route this track's MIDI output to the instrument track with phu-arp:
   - Click the track output selector
   - Select the instrument track as destination
   - Or use Bitwig's "Note Receiver" device (see below)

### Step 3: Setup a rhythm track (MIDI Channel 16)
1. Create another **MIDI track** and name it "Rhythm"
2. Before the track output, add the **Note Receiver** device
3. Configure Note Receiver:
   - Set it to receive notes from phu-arp's output track (the instrument track)
   - This allows the rhythm track to "send" MIDI to the instrument track
4. Important: Set this track's output to **MIDI Channel 16**
   - In Bitwig, you can set the MIDI channel on the track's MIDI output
5. Create your rhythm pattern on this track
   - Use notes starting from C1 (MIDI note 24) and above
   - Each note triggers a chord note according to the mapping described above

### Alternative simplified routing
If Bitwig's routing is complex, use this approach:
1. Create a single instrument track with phu-arp before the synth
2. Use Bitwig's **Note Effect Layer** or **Note FX Grid**:
   - Layer 1: Send chord notes on Channel 1
   - Layer 2: Send rhythm notes on Channel 16
3. Both layers feed into phu-arp, which outputs on Channel 2 to the synth

### Tips
- Use Bitwig's **Clip Launcher** to trigger different chord progressions
- Create rhythm patterns with varying velocities for dynamic expression
- Experiment with different rhythm root notes (default is C1 = 24)
- Layer multiple rhythm patterns at different octaves for complex arpeggios

## Implementation status (high level)

- **Time-causal processing within a block**: events are ordered by `samplePosition` (with a stable priority at the same position).
- **Ownership-based note-offs**: rhythm note-offs stop the exact output pitch(es) produced by that rhythm trigger, even if the chord changes later.
- **Negative/below-root mapping fixed**: rhythm notes below the root map correctly.

Known limitations to be aware of:

- **Duplicate chord note-ons** are allowed by `ChordNotesTracker` (no refcounting).
- **Same output pitch from different triggers** is fundamentally ambiguous in MIDI (note-off has no voice id); if two triggers produce the same pitch on the same channel, releasing one may silence the other depending on the target instrument.

## Where to look

- MIDI algorithm/design notes: `src/ChordPatternCoordinator_README.md`
- Edge cases + regression checklist: `ChordPatternCoordinator_ProblemCases.md`
- Small header-only event system used by the plugin: `lib/README.md`