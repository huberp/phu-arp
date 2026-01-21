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

The rhythm mapping uses a configurable **rhythm root note** (default **C1 = 24**). Notes are mapped by semitone index within the octave plus an octave offset.

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