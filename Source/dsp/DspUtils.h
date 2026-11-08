#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mf::dsp
{
constexpr float twoPi = 6.28318530717958647692f;
constexpr float pi = 3.14159265358979323846f;

inline float fastTanh (float x)
{
    x = std::clamp (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float sawBlep (float phase, float dt) { return 2.0f * phase - 1.0f - polyBlep (phase, dt); }

inline float pulseBlep (float phase, float dt, float width)
{
    float v = phase < width ? 1.0f : -1.0f;
    v += polyBlep (phase, dt);
    float t2 = phase + 1.0f - width;
    t2 -= std::floor (t2);
    v -= polyBlep (t2, dt);
    return v;
}

inline float triangle (float phase) { return 4.0f * std::abs (phase - 0.5f) - 1.0f; }

struct Rng
{
    uint32_t state = 0x9E3779B9u;
    void seed (uint32_t s) { state = s ? s : 0x9E3779B9u; }
    uint32_t next() { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }
    float bipolar() { return (float) (next() & 0xFFFFFF) / 8388607.5f - 1.0f; }
    float unipolar() { return (float) (next() & 0xFFFFFF) / 16777215.0f; }
};

/** Topology-preserving state-variable filter (Simper). Optional soft clipping of the integrators
    gives the gritty, unstable character of the Soviet Polivoks. */
struct SVF
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;
    void reset() { ic1 = ic2 = 0; }
    void set (float cutoff, float res, float sampleRate)
    {
        const float fc = std::clamp (cutoff, 16.0f, sampleRate * 0.45f);
        const float g = std::tan (pi * fc / sampleRate);
        k = 2.0f - 1.96f * std::clamp (res, 0.0f, 1.0f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    struct Out { float low, band, high; };
    Out process (float v0, bool clip = false)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        if (clip) { ic1 = 1.4f * fastTanh (ic1 * 0.7f); ic2 = 1.4f * fastTanh (ic2 * 0.7f); }
        return { v2, v1, v0 - k * v1 - v2 };
    }
};

/** Four-pole transistor-ladder style low-pass with tanh input stage. */
struct Ladder
{
    float s[4] {}, g = 0.5f, fb = 0.0f;
    void reset() { std::fill (std::begin (s), std::end (s), 0.0f); }
    void set (float cutoff, float res, float sampleRate)
    {
        const float fc = std::clamp (cutoff, 16.0f, sampleRate * 0.42f);
        const float wc = std::tan (pi * fc / sampleRate);
        g = wc / (1.0f + wc);
        fb = 4.2f * std::clamp (res, 0.0f, 1.0f);
    }
    float process (float x)
    {
        const float u = fastTanh (x - fb * s[3]);
        s[0] += 2.0f * g * (u - s[0]);
        s[1] += 2.0f * g * (s[0] - s[1]);
        s[2] += 2.0f * g * (s[1] - s[2]);
        s[3] += 2.0f * g * (s[2] - s[3]);
        return s[3] * (1.0f + 0.45f * fb);
    }
};

/** Linear-interpolated delay line. */
struct DelayLine
{
    std::vector<float> buf;
    int write = 0;
    void prepare (int maxSamples) { buf.assign ((size_t) std::max (4, maxSamples), 0.0f); write = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) { buf[(size_t) write] = x; write = (write + 1) % (int) buf.size(); }
    float read (float delaySamples) const
    {
        const int n = (int) buf.size();
        const float d = std::clamp (delaySamples, 1.0f, (float) (n - 2));
        float rp = (float) write - d;
        while (rp < 0) rp += (float) n;
        const int i0 = (int) rp;
        const float frac = rp - (float) i0;
        const float a = buf[(size_t) i0];
        const float b = buf[(size_t) ((i0 + 1) % n)];
        return a + frac * (b - a);
    }
};

/** Simple ADSR with exponential segments, sample-by-sample. */
struct Envelope
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float value = 0, attackInc = 0, decayCoef = 0, releaseCoef = 0, sustain = 0;
    void set (float a, float d, float s, float r, float sampleRate)
    {
        attackInc = 1.0f / std::max (1.0f, a * sampleRate);
        decayCoef = std::exp (-4.6f / std::max (1.0f, d * sampleRate));
        releaseCoef = std::exp (-4.6f / std::max (1.0f, r * sampleRate));
        sustain = std::clamp (s, 0.0f, 1.0f);
    }
    void noteOn() { stage = Attack; }
    void noteOff() { if (stage != Idle) stage = Release; }
    bool active() const { return stage != Idle; }
    float next()
    {
        switch (stage)
        {
            case Attack:  value += attackInc; if (value >= 1.0f) { value = 1.0f; stage = Decay; } break;
            case Decay:   value = sustain + (value - sustain) * decayCoef; if (std::abs (value - sustain) < 1e-4f) stage = Sustain; break;
            case Sustain: value = sustain; break;
            case Release: value *= releaseCoef; if (value < 1e-4f) { value = 0; stage = Idle; } break;
            case Idle:    value = 0; break;
        }
        return value;
    }
};

inline float midiToHz (float midi) { return 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f); }
} // namespace mf::dsp
