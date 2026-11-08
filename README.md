# Matryoshka Fugue

Matryoshka Fugue is a generative instrument for macOS. Four nesting dolls play one musical subject at four sizes: Babushka (the largest), Mama (second largest), Dochka (second smallest) and Malyshka (the smallest). Each doll is a fractal copy of the others in pitch, note combinations, tempo and rhythm, so together they sound like a fugue.

It runs as a **standalone macOS app** (Sonoma 14.6 or later, Apple silicon and Intel). It also builds as **VST3 and Audio Unit** plug-ins from the same code. In the app you can:

- record what you play, as a WAV file plus a MIDI file of every doll's notes
- export the fugue offline to a 24-bit WAV
- export the sequence as a 4-track MIDI file (one track and channel per doll)

## How the dolls relate

All four dolls state the same **subject**, the theme you write in the Subject grid. Each doll is a scaled copy of its parent, the next larger doll:

| Dimension | Rule |
|---|---|
| **Tempo** | Each doll's pulse is *ratio* times faster than the next larger doll's. With the default ratio of 2 the pulses are 1/16 → 1/8 → 1/4 → 1/2 notes. Fugues call this augmentation and diminution. You can also pick 3:2, 4:3, 3:1, √2 or the golden ratio. Irrational ratios never line up again, so the piece never repeats. |
| **Pitch** | Each statement of the subject is transposed so its first note lands on the note the parent is sounding at that moment. The melody is the subject replayed inside each note of itself. *Coupling* blends from a strict canon (0) to the full fractal (100%). *Fold drift* keeps the wander within an octave. |
| **Fugal answer** | The second largest and the smallest dolls sit an *answer interval* away from the others (a 5th by default), giving subject / answer / subject / answer as in a fugue exposition. *Staggered entries* brings them in one after another, largest first. |
| **Note combinations** | Each doll can stack scale thirds into chords of 1 to 4 notes. **Nest chords 4-3-2-1** gives the largest doll the largest chord. |
| **Rhythmic sequencing** | Each doll can play the subject as written, inverted, backwards or both. A fractal **rhythm gate** can thin out its notes: a Cantor set, a Sierpinski triangle or a Fibonacci word. |

Everything is a pure function of the project and the beat. That is why the piano roll can show the future, and why playback, WAV export and MIDI export agree note for note.

## Sound sources

You can put either a synth or a sampler on each of the four dolls.

**Synth.** Two oscillators (saw, square, PWM pulse, triangle, sine with FM), a sub oscillator, noise and analogue drift. Filters: a Moog-style 24 dB ladder, a SEM-style 12 dB state-variable filter, a gritty Polivoks-style filter, high-pass and band-pass. It has amp and filter envelopes and vibrato. Each doll also has drive, chorus and tape wow. Presets by era:

- **1960s:** Modular Bass, Theremin Wail, Tape-Loop Strings, Combo Organ
- **1970s:** Mono Lead, String Ensemble, Poly Brass, Polivoks Snarl, ANS Photo-Drone
- **1980s:** FM Tine Piano, FM Glass Bell, Chorus Pad, Synth-Pop Bass, Digital Hoover Lead

**Sampler.** Load WAV, AIFF, FLAC, MP3 or M4A, or use one of the built-in samples, all synthesised in code: a balalaika pluck, a kolokol church bell, a choir "Ah" and a bayan accordion. **Use on all dolls** gives every doll the same sample, so four playheads read one sound like fugue voices. There are three playhead modes:

- **Pitched:** one-shot playback, repitched to each note.
- **Slice:** the sample is cut into slices, and the doll's fractal melody chooses which slice plays.
- **Scrub:** each doll's playhead travels through the sample at its own fractal speed (the smallest doll fastest), and every note grabs the sound under its playhead.

## Scales

| Group | Scales |
|---|---|
| Common | Major, natural / harmonic / melodic minor, major and minor pentatonic, blues, chromatic |
| Modes | Dorian, Phrygian, Lydian, Mixolydian, Locrian, Phrygian dominant, Lydian dominant |
| Russian & Slavic | Rimsky-Korsakov octatonic (both forms), Glinka whole-tone (Chernomor), Obikhod / Znamenny chant (repeats at the fourth, not the octave), Scriabin Prometheus, Tcherepnin nine-step, Shostakovich modes (Phrygian ♭4, Locrian ♭4), Russian Romani minor, Ukrainian Dorian, Russian variable mode |
| Symmetric & world | Hungarian minor, double harmonic, augmented, Hirajoshi |

The subject generators include a fractal contour (midpoint displacement), a random walk, a Russian bell peal (zvon), a Znamenny chant line and Shostakovich's DSCH motif.

## Getting the app

### Download a build

Every push runs the **Build macOS app** workflow (GitHub → Actions) on a macOS 14 Sonoma runner. The workflow compiles a universal binary, runs the tests and uploads `Matryoshka-Fugue-macOS.dmg` as an artifact. The DMG contains the app, the VST3 and AU plug-ins, and install notes.

The app has an ad-hoc signature but is **not notarised**, so the first launch needs a right-click → **Open**. See [docs/INSTALL-macOS.txt](docs/INSTALL-macOS.txt).

### Build it yourself

You need Xcode (or the Command Line Tools) and CMake 3.22 or later (`brew install cmake`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 8
ctest --test-dir build --output-on-failure
open "build/MatryoshkaFugue_artefacts/Release/Standalone/Matryoshka Fugue.app"
```

The build downloads JUCE 8.0.9 the first time. To build offline, pass `-DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE`.

## Using it

- **Play / Stop** (or the space bar) starts and stops the fugue. **|<** goes back to the start.
- **Click a doll** to edit it in the inspector on the right:
  - **Synth** page: presets and sound controls
  - **Sampler** page: samples, playhead mode and the waveform with each doll's playhead
  - **Nest & Mix** page: octave, transform, gate, chord size, sends, mute and solo
- **The Subject** grid: click to write a note, click a note to make it a rest, drag to paint. In the bottom row, click to change a step's length and drag up or down to change its velocity. The coloured beads show where each doll is in the subject.
- **Fugue Law**: key, scale, fractal ratio, answer interval, coupling, entries. **The Hall** sets the cathedral reverb, the echo and the master volume.
- **Record** captures the live output, and any changes you make while it plays. Press it again to stop. You get `Matryoshka-Take-<date>.wav` and a matching `.mid` in `~/Music/Matryoshka Fugue/`.
- **Export WAV / Export MIDI** render the chosen number of bars, from the start.
- **Save / Open** store projects as `.matryoshka` files. Projects keep references to your sample files, not copies of them.

As a plug-in it follows the host's tempo and transport. It also sends each doll's notes as MIDI on channels 1 to 4, so in your DAW you can route the dolls to other instruments.

## Code layout

```
Source/engine/   pure C++ fractal-fugue engine, scales, presets (no JUCE; unit tested)
Source/dsp/      synth and sampler voices, filters, per-doll effects, the mixing engine, built-in samples
Source/io/       project XML, MIDI file export, offline WAV render
Source/ui/       the matryoshka look: painted dolls, piano roll, subject grid, panels
tests/           EngineTests (engine) and RenderTests (headless audio / MIDI / WAV rendering)
```
