#pragma once
#include <array>
#include <cstdint>

namespace mf
{
constexpr int kNumDolls = 4;
constexpr int kMaxSteps = 16;
constexpr int kMaxChord = 4;

// All enums are stored as plain ints inside Project so the whole struct stays trivially copyable
// (the audio thread copies it without allocating).
namespace Wave       { enum { Saw, Square, Pulse, Triangle, Sine, Off, count }; }
namespace FilterType { enum { Ladder24, SEM12, Polivoks, HighPass, BandPass, count }; }
namespace Transform  { enum { Prime, Inversion, Retrograde, RetroInversion, count }; }
namespace Gate       { enum { All, Cantor, Sierpinski, Fibonacci, count }; }
namespace Source     { enum { Synth, Sampler, count }; }
namespace SampleMode { enum { Pitched, Slice, Scrub, count }; }
namespace Ratio      { enum { Octave, Fifth, Golden, Twelfth, Fourth, Tritone, count }; }

struct Step
{
    int degree = 0;      // scale degree relative to the tonic
    int length = 1;      // in pulses (1..4)
    float velocity = 0.9f;
    bool on = true;      // false = rest
};

struct SynthParams
{
    int wave1 = Wave::Saw;
    int wave2 = Wave::Saw;
    float osc2Semis = 0.0f;   // -24..24
    float osc2Detune = 7.0f;  // cents
    float oscMix = 0.5f;      // 0 = osc1 only, 1 = osc2 only
    float subLevel = 0.0f;
    float noiseLevel = 0.0f;
    float pulseWidth = 0.5f;
    float pwmDepth = 0.0f;
    float pwmRate = 0.6f;
    float fmIndex = 0.0f;     // phase-modulation index (applies to sine oscillators)
    float fmRatio = 1.0f;
    float fmDecay = 0.6f;     // seconds
    int filterType = FilterType::Ladder24;
    float cutoff = 1800.0f;
    float resonance = 0.25f;
    float envAmount = 2.0f;   // octaves
    float keyTrack = 0.5f;
    float ampA = 0.005f, ampD = 0.3f, ampS = 0.7f, ampR = 0.35f;
    float filA = 0.005f, filD = 0.4f, filS = 0.2f, filR = 0.4f;
    float vibratoRate = 5.0f;
    float vibratoDepth = 0.0f; // cents
    float drift = 3.0f;        // cents of analogue instability
    float level = 0.7f;
};

struct SamplerParams
{
    int mode = SampleMode::Pitched;
    int rootNote = 57;         // the pitch at which the sample plays back unchanged
    float start = 0.0f;        // 0..1
    int slices = 8;
    float scanSpeed = 1.0f;    // scrub playhead speed for the smallest doll (x realtime)
    float tone = 16000.0f;     // low-pass cutoff
    float attack = 0.003f;
    float release = 0.25f;
    bool pitchFollow = true;
    bool reverse = false;
    float level = 0.8f;
};

struct DollSettings
{
    int source = Source::Synth;
    SynthParams synth;
    SamplerParams sampler;
    int octave = 3;
    int transform = Transform::Prime;
    int gate = Gate::All;
    int chordSize = 1;
    float legato = 0.85f;
    float volume = 0.8f;
    float pan = 0.0f;
    float drive = 0.0f;
    float chorus = 0.0f;
    float wow = 0.0f;
    float reverbSend = 0.25f;
    float echoSend = 0.0f;
    bool mute = false;
    bool solo = false;
};

struct Project
{
    double bpm = 92.0;
    int root = 2;               // pitch class, 2 = D
    int scaleIndex = 1;         // Natural minor
    int ratio = Ratio::Octave;
    double pulseBeats = 0.25;   // the smallest doll's pulse (a 16th note)
    int answerInterval = 4;     // degrees: subject / answer alternate like a fugue exposition
    float coupling = 1.0f;      // 0 = strict canon, 1 = each doll rides on its parent's current note
    bool fold = true;           // keep the fractal drift within an octave of the subject
    bool staggerEntries = true;
    double entryGapBeats = 4.0;

    std::array<Step, kMaxSteps> subject {};
    int subjectLength = 8;

    std::array<DollSettings, kNumDolls> dolls {};

    // the hall
    float reverbSize = 0.82f;
    float reverbLevel = 0.35f;
    double echoBeats = 0.75;
    float echoFeedback = 0.35f;
    float masterVolume = 0.8f;

    Project();
};

/** A fresh, musical default project. */
Project makeDefaultProject();
} // namespace mf

namespace mf
{
/** UTF-8 names for the four dolls, largest first. */
inline const char* dollRussianName (int d)
{
    static const char* names[] = { "\xD0\x91\xD0\xB0\xD0\xB1\xD1\x83\xD1\x88\xD0\xBA\xD0\xB0",          // Бабушка
                                   "\xD0\x9C\xD0\xB0\xD0\xBC\xD0\xB0",                                  // Мама
                                   "\xD0\x94\xD0\xBE\xD1\x87\xD0\xBA\xD0\xB0",                          // Дочка
                                   "\xD0\x9C\xD0\xB0\xD0\xBB\xD1\x8B\xD1\x88\xD0\xBA\xD0\xB0" };        // Малышка
    return names[d & 3];
}
inline const char* dollLatinName (int d)
{
    static const char* names[] = { "Babushka", "Mama", "Dochka", "Malyshka" };
    return names[d & 3];
}
inline const char* dollRole (int d)
{
    static const char* roles[] = { "Largest", "Second largest", "Second smallest", "Smallest" };
    return roles[d & 3];
}
} // namespace mf
