#pragma once
#include "../engine/FractalFugue.h"
#include "../dsp/SampleData.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <functional>

namespace mf
{
constexpr int kMidiTicksPerQuarter = 960;

/** A note as captured during live recording (beats relative to the start of the take). */
struct RecordedNote
{
    int doll = 0;
    int notes[kMaxChord] {};
    int numNotes = 0;
    float velocity = 0;
    double beat = 0, length = 0;
};

/** Four MIDI tracks (one per doll, channels 1-4) plus a tempo track. */
juce::MidiFile buildMidiFile (const Project& p, double lengthBeats);
juce::MidiFile buildMidiFile (const std::vector<RecordedNote>& notes, double bpm);
bool writeMidiFile (const juce::MidiFile& midi, const juce::File& file);

using SampleSet = std::array<const SampleData*, kNumDolls>;

/** Renders the fugue offline (faster than real time) to a 24-bit stereo WAV.
    `progress` is called with 0..1 and may return false to cancel. Adds a reverb tail. */
bool renderWavFile (const Project& p, const SampleSet& samples, double lengthBeats, double sampleRate,
                    const juce::File& file, const std::function<bool (float)>& progress, juce::String& error);

/** ~/Music/Matryoshka Fugue (created on demand). */
juce::File defaultOutputFolder();
juce::String timestampedName (const juce::String& prefix);
} // namespace mf
