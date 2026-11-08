#pragma once
#include "Voices.h"
#include "../engine/FractalFugue.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace mf
{
/** One nesting doll: its voices plus its insert effects (drive, tape wow, chorus, pan). */
class DollTrack
{
public:
    static constexpr int kVoices = 16;

    void prepare (double sampleRate, int maxBlock);
    void reset();
    void trigger (const Project& p, int doll, const SampleData* sample, const NoteEvent& e, double samplesPerBeat);
    void renderVoices (int start, int num);
    void clearBus (int num) { std::fill (bus.begin(), bus.begin() + num, 0.0f); }
    /** Adds this track to the master and the reverb / echo sends. */
    void processFx (const DollSettings& d, bool audible, int num,
                    float* L, float* R, float* revL, float* revR, float* echoL, float* echoR);

    double scrubPos = 0.0;
    int activeVoices() const;

private:
    double sr = 48000.0;
    std::vector<float> bus;
    std::array<SynthVoice, kVoices> synths;
    std::array<SamplerVoice, kVoices> samplers;
    uint64_t orderCounter = 0;
    uint32_t seedCounter = 1;
    dsp::DelayLine wowLine, chorusLine;
    float wowPhase = 0, flutterPhase = 0, chorusPhase = 0;
    float gainSmoothed = 0;
};

/** Renders the whole fractal fugue: four dolls, an echo, a cathedral reverb and a limiter. */
class FugueEngine
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void noteStarted (const NoteEvent& e, int sampleOffset) = 0;
    };

    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSample (int doll, const SampleData* s) { samples[(size_t) doll] = s; }
    const SampleData* getSample (int doll) const { return samples[(size_t) doll]; }

    /** Renders `num` samples into L/R (overwriting). When playing, advances `beat`. */
    void process (const Project& p, float* L, float* R, int num, bool playing, double& beat, Listener* listener);

    double scrubPosition (int doll) const { return tracks[(size_t) doll].scrubPos; }
    float peakLevel() const { return peak; }

private:
    double sr = 48000.0;
    int maxBlock = 512;
    std::array<DollTrack, kNumDolls> tracks;
    std::array<const SampleData*, kNumDolls> samples {};
    std::vector<float> revL, revR, echoL, echoR;
    juce::Reverb reverb;
    dsp::DelayLine echoLineL, echoLineR;
    float echoDampL = 0, echoDampR = 0;
    float limiterEnv = 0, peak = 0;
    float lastReverbSize = -1;

    static constexpr int kMaxEventsPerBlock = 256;
    std::array<NoteEvent, kMaxEventsPerBlock> events;
};
} // namespace mf
