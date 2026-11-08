#pragma once
#include "DspUtils.h"
#include "SampleData.h"
#include "../engine/Project.h"

namespace mf
{
/** Virtual-analogue / FM voice. Mono output, rendered additively into a track bus. */
class SynthVoice
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; }
    void start (const SynthParams& params, int midiNote, float velocity, int gateSamples, uint32_t seed);
    void render (float* out, int numSamples);
    void kill() { amp.stage = dsp::Envelope::Idle; amp.value = 0; }
    bool isActive() const { return amp.active(); }
    uint64_t order = 0;

private:
    float sr = 48000.0f;
    SynthParams p;
    int note = 60;
    float vel = 1.0f, baseHz = 440.0f;
    int gateLeft = 0;
    float ph1 = 0, ph2 = 0, phSub = 0, phMod = 0, phVib = 0, phPwm = 0;
    float driftCents = 0, driftWalk = 0, modEnv = 0, modDecayCoef = 0;
    float pitchMul = 1.0f;
    int counter = 0;
    dsp::Envelope amp, fil;
    dsp::SVF svf;
    dsp::Ladder ladder;
    dsp::Rng rng;
    float filterEnvValue = 0;

    float osc (int wave, float phase, float dt, float pw, float pm);
    void updateFilter();
};

/** One sampler playhead. */
class SamplerVoice
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; }
    /** startFraction: where in the sample to begin (0..1). endFraction limits a slice (<= 0 = whole sample). */
    void start (const SampleData* sample, const SamplerParams& params, int midiNote, float velocity,
                int gateSamples, double startFraction, double endFraction);
    void render (float* out, int numSamples);
    void kill() { env.stage = dsp::Envelope::Idle; env.value = 0; }
    bool isActive() const { return env.active() && sample != nullptr; }
    uint64_t order = 0;

private:
    float sr = 48000.0f;
    const SampleData* sample = nullptr;
    SamplerParams p;
    double pos = 0, inc = 1, startPos = 0, endPos = 0;
    int gateLeft = 0;
    float vel = 1.0f;
    dsp::Envelope env;
    dsp::SVF tone;
};
} // namespace mf
