#include "Voices.h"
#include <array>

namespace mf
{
using namespace dsp;

// Drawbar-style organ tone (16', 8', 5 1/3', 4', 2 2/3', 2'), the sound of transistor combo organs.
static const std::array<float, 2048>& organTable()
{
    static const std::array<float, 2048> table = [] {
        std::array<float, 2048> t {};
        const float harm[] = { 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
        const float amp[] = { 1.0f, 0.75f, 0.45f, 0.55f, 0.3f, 0.25f };
        float peak = 0;
        for (size_t i = 0; i < t.size(); ++i)
        {
            const float ph = twoPi * (float) i / (float) t.size();
            float v = 0;
            for (int h = 0; h < 6; ++h) v += amp[h] * std::sin (harm[h] * ph);
            t[i] = v;
            peak = std::max (peak, std::abs (v));
        }
        for (auto& x : t) x /= peak;
        return t;
    }();
    return table;
}

float SynthVoice::organ (float phase)
{
    const auto& t = organTable();
    const float p = phase * (float) t.size();
    const auto i0 = (size_t) p % t.size();
    const float frac = p - std::floor (p);
    return t[i0] + frac * (t[(i0 + 1) % t.size()] - t[i0]);
}

void SynthVoice::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    (void) organTable(); // build the table off the audio thread
}

void SynthVoice::start (const SynthParams& params, int midiNote, float velocity, int gateSamples, uint32_t seed, float glideFromHz)
{
    p = params;
    note = midiNote;
    vel = velocity;
    baseHz = midiToHz ((float) midiNote);
    gateLeft = std::max (1, gateSamples);
    rng.seed (seed * 2654435761u + 17u);
    numUnison = std::clamp (p.unison, 1, kMaxUnison);
    unisonGain = 1.0f / std::sqrt ((float) numUnison);
    for (int u = 0; u < numUnison; ++u)
    {
        ph1[u] = rng.unipolar(); ph2[u] = rng.unipolar();
        const float spread = numUnison > 1 ? ((float) u / (float) (numUnison - 1) - 0.5f) * p.unisonSpread : 0.0f;
        detune[u] = std::pow (2.0f, (spread + rng.bipolar() * 0.15f * p.unisonSpread / (float) numUnison) / 1200.0f);
    }
    phSub = 0; phMod = 0;
    glideRatio = (glideFromHz > 0.0f && p.glide > 0.0005f) ? glideFromHz / baseHz : 1.0f;
    glideCoef = p.glide > 0.0005f ? std::exp (-16.0f / (p.glide * sr * 0.25f)) : 0.0f;
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
        case Wave::Organ:    return organ (phase);
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
            glideRatio = 1.0f + (glideRatio - 1.0f) * glideCoef;
            pitchMul = std::pow (2.0f, (vib + driftCents + driftWalk) / 1200.0f) * glideRatio;
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

        float s = 0.0f;
        for (int u = 0; u < numUnison; ++u)
        {
            const float d1 = std::min (0.45f, dt1 * detune[u]), d2 = std::min (0.45f, dt2 * detune[u]);
            s += mix1 * osc (p.wave1, ph1[u], d1, pw, pm) + mix2 * osc (p.wave2, ph2[u], d2, pw, pm);
            ph1[u] += d1;
            ph2[u] += d2;
            if (ph1[u] >= 1.0f)
            {
                ph1[u] -= 1.0f;
                if (p.sync) ph2[u] = ph1[u] * d2 / d1; // hard sync: restart osc 2 with osc 1
            }
            ph2[u] -= std::floor (ph2[u]);
        }
        s *= unisonGain;
        if (p.subLevel > 0)  s += p.subLevel * pulseBlep (phSub, dt1 * 0.5f, 0.5f);
        if (p.noiseLevel > 0) s += p.noiseLevel * rng.bipolar();

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
    pos = startPos;
    if (p.reverse) { pos = endPos - 1.0; inc = -inc; }
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
    const double lo = startPos, hi = endPos;
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
