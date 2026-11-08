#include "Voices.h"

namespace mf
{
using namespace dsp;

void SynthVoice::start (const SynthParams& params, int midiNote, float velocity, int gateSamples, uint32_t seed)
{
    p = params;
    note = midiNote;
    vel = velocity;
    baseHz = midiToHz ((float) midiNote);
    gateLeft = std::max (1, gateSamples);
    rng.seed (seed * 2654435761u + 17u);
    ph1 = rng.unipolar(); ph2 = rng.unipolar(); phSub = 0; phMod = 0;
    phVib = rng.unipolar(); phPwm = rng.unipolar();
    driftCents = rng.bipolar() * p.drift;
    driftWalk = 0;
    modEnv = 1.0f;
    modDecayCoef = std::exp (-1.0f / std::max (1.0f, p.fmDecay * sr));
    amp.set (p.ampA, p.ampD, p.ampS, p.ampR, sr);
    fil.set (p.filA, p.filD, p.filS, p.filR, sr);
    amp.value = 0; fil.value = 0;
    amp.noteOn(); fil.noteOn();
    svf.reset(); ladder.reset();
    counter = 0;
    filterEnvValue = 0;
    updateFilter();
}

void SynthVoice::updateFilter()
{
    const float key = p.keyTrack * ((float) note - 60.0f) / 12.0f;
    const float cents = (rng.bipolar() * 0.02f);
    const float hz = p.cutoff * std::pow (2.0f, p.envAmount * filterEnvValue + key + cents);
    if (p.filterType == FilterType::Ladder24)
        ladder.set (hz, p.resonance, sr);
    else
        svf.set (hz, p.filterType == FilterType::Polivoks ? std::min (1.0f, p.resonance * 1.05f + 0.1f) : p.resonance, sr);
}

float SynthVoice::osc (int wave, float phase, float dt, float pw, float pm)
{
    switch (wave)
    {
        case Wave::Saw:      return sawBlep (phase, dt);
        case Wave::Square:   return pulseBlep (phase, dt, 0.5f);
        case Wave::Pulse:    return pulseBlep (phase, dt, pw);
        case Wave::Triangle: return triangle (phase);
        case Wave::Sine:     return std::sin (twoPi * phase + pm);
        default:             return 0.0f;
    }
}

void SynthVoice::render (float* out, int numSamples)
{
    if (! amp.active())
        return;

    const float osc2Mul = std::pow (2.0f, (p.osc2Semis * 100.0f + p.osc2Detune) / 1200.0f);
    const float mix2 = std::clamp (p.oscMix, 0.0f, 1.0f), mix1 = 1.0f - mix2;
    const float vibInc = p.vibratoRate / sr, pwmInc = p.pwmRate / sr;
    const bool fm = p.fmIndex > 0.001f;

    for (int i = 0; i < numSamples; ++i)
    {
        if ((counter++ & 15) == 0)
        {
            driftWalk = std::clamp (driftWalk + rng.bipolar() * 0.05f * p.drift, -p.drift, p.drift);
            const float vib = std::sin (twoPi * phVib) * p.vibratoDepth;
            pitchMul = std::pow (2.0f, (vib + driftCents + driftWalk) / 1200.0f);
            updateFilter();
        }
        if (gateLeft > 0 && --gateLeft == 0) { amp.noteOff(); fil.noteOff(); }

        const float f1 = baseHz * pitchMul;
        const float f2 = f1 * osc2Mul;
        const float dt1 = std::min (0.45f, f1 / sr), dt2 = std::min (0.45f, f2 / sr);
        const float pw = std::clamp (p.pulseWidth + p.pwmDepth * 0.45f * std::sin (twoPi * phPwm), 0.05f, 0.95f);

        float pm = 0.0f;
        if (fm)
        {
            pm = p.fmIndex * modEnv * std::sin (twoPi * phMod);
            modEnv *= modDecayCoef;
            phMod += f1 * p.fmRatio / sr; phMod -= std::floor (phMod);
        }

        float s = mix1 * osc (p.wave1, ph1, dt1, pw, pm) + mix2 * osc (p.wave2, ph2, dt2, pw, pm);
        if (p.subLevel > 0)  s += p.subLevel * pulseBlep (phSub, dt1 * 0.5f, 0.5f);
        if (p.noiseLevel > 0) s += p.noiseLevel * rng.bipolar();

        ph1 += dt1; ph1 -= std::floor (ph1);
        ph2 += dt2; ph2 -= std::floor (ph2);
        phSub += dt1 * 0.5f; phSub -= std::floor (phSub);
        phVib += vibInc; phVib -= std::floor (phVib);
        phPwm += pwmInc; phPwm -= std::floor (phPwm);

        filterEnvValue = fil.next();
        float y;
        switch (p.filterType)
        {
            case FilterType::Ladder24: y = ladder.process (s * 0.8f); break;
            case FilterType::SEM12:    y = svf.process (s).low; break;
            case FilterType::Polivoks: y = 0.8f * svf.process (fastTanh (s * 1.6f), true).low; break;
            case FilterType::HighPass: y = svf.process (s).high; break;
            default:                   y = svf.process (s).band * 1.5f; break;
        }
        out[i] += y * amp.next() * vel * p.level * 0.5f;
    }
}

void SamplerVoice::start (const SampleData* s, const SamplerParams& params, int midiNote, float velocity,
                          int gateSamples, double startFraction, double endFraction)
{
    sample = s;
    if (sample == nullptr || sample->data.size() < 4)
    {
        sample = nullptr;
        return;
    }
    p = params;
    vel = velocity;
    gateLeft = std::max (1, gateSamples);
    const double n = (double) sample->data.size() - 2.0;
    const double ratio = p.pitchFollow ? std::pow (2.0, (midiNote - p.rootNote) / 12.0) : 1.0;
    inc = ratio * sample->sampleRate / (double) sr;
    startPos = std::clamp (startFraction, 0.0, 1.0) * n;
    endPos = endFraction > 0.0 ? std::clamp (endFraction, 0.0, 1.0) * n : n;
    if (endPos <= startPos + 8) endPos = n;
    if (p.reverse) { std::swap (startPos, endPos); inc = -inc; }
    pos = startPos;
    env.set (std::max (0.001f, p.attack), 0.05f, 1.0f, std::max (0.005f, p.release), sr);
    env.value = 0;
    env.noteOn();
    tone.reset();
    tone.set (p.tone, 0.1f, sr);
}

void SamplerVoice::render (float* out, int numSamples)
{
    if (! isActive())
        return;
    const auto& d = sample->data;
    const double lo = std::min (startPos, endPos), hi = std::max (startPos, endPos);
    for (int i = 0; i < numSamples; ++i)
    {
        if (gateLeft > 0 && --gateLeft == 0) env.noteOff();
        if (pos < lo || pos >= hi)
        {
            // fade the tail of a slice instead of clicking
            env.noteOff();
            if (env.value < 0.002f) { kill(); return; }
        }
        const double cp = std::clamp (pos, 0.0, (double) d.size() - 3.0);
        const auto i0 = (size_t) cp;
        const float frac = (float) (cp - (double) i0);
        // 4-point Hermite interpolation
        const float xm1 = d[i0 > 0 ? i0 - 1 : 0], x0 = d[i0], x1 = d[i0 + 1], x2 = d[i0 + 2];
        const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        const float s = ((c3 * frac + c2) * frac + c1) * frac + x0;
        out[i] += tone.process (s).low * env.next() * vel * p.level;
        pos += inc;
        if (! env.active()) return;
    }
}
} // namespace mf
