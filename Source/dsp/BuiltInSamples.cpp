#include "SampleData.h"
#include "DspUtils.h"
#include "../engine/Presets.h"
#include <cmath>

namespace mf
{
using namespace dsp;

static void normalise (std::vector<float>& v, float peak = 0.9f)
{
    float m = 0;
    for (float x : v) m = std::max (m, std::abs (x));
    if (m > 0) for (float& x : v) x *= peak / m;
}

static std::vector<float> balalaika (double sr)
{
    // Karplus-Strong, two slightly detuned courses plucked a hair apart, through a small body resonance.
    const size_t len = (size_t) (sr * 1.8);
    std::vector<float> out (len, 0.0f);
    Rng rng; rng.seed (1234);
    const double f = midiToHz (57);
    for (int course = 0; course < 2; ++course)
    {
        const double period = sr / (f * (course == 0 ? 1.0 : 1.0035));
        const int n = (int) period;
        std::vector<float> line ((size_t) n);
        for (auto& x : line) x = rng.bipolar();
        const size_t offset = (size_t) (course * sr * 0.012);
        int idx = 0;
        float last = 0;
        for (size_t i = offset; i < len; ++i)
        {
            const float cur = line[(size_t) idx];
            const float nxt = line[(size_t) ((idx + 1) % n)];
            const float y = 0.4985f * (cur + nxt);
            line[(size_t) idx] = y;
            idx = (idx + 1) % n;
            out[i] += 0.6f * cur + 0.4f * last;
            last = cur;
        }
    }
    SVF body; body.set (420.0f, 0.55f, (float) sr);
    SVF bright; bright.set (2400.0f, 0.3f, (float) sr);
    for (auto& x : out) { const float b = body.process (x).band; const float h = bright.process (x).band; x = x + 0.8f * b + 0.4f * h; }
    normalise (out);
    return out;
}

static std::vector<float> kolokol (double sr)
{
    // Russian church bell: inharmonic partials (hum, prime, minor tierce, quint, nominal...) with long decays.
    const size_t len = (size_t) (sr * 5.0);
    std::vector<float> out (len, 0.0f);
    const double f = midiToHz (48);
    const double partials[] = { 0.5, 1.0, 1.19, 1.51, 2.0, 2.51, 2.66, 3.01, 4.16, 5.43, 6.79 };
    const double amps[]     = { 0.6, 0.8, 0.7,  0.35, 0.9, 0.4,  0.35, 0.3,  0.25, 0.18, 0.12 };
    const double decays[]   = { 4.5, 3.2, 2.8,  2.0,  2.5, 1.4,  1.2,  1.0,  0.7,  0.5,  0.35 };
    for (int p = 0; p < 11; ++p)
    {
        const double w = 2.0 * M_PI * f * partials[p] / sr;
        const double beat = 2.0 * M_PI * (0.7 + p * 0.31) / sr;
        for (size_t i = 0; i < len; ++i)
        {
            const double t = (double) i / sr;
            const double env = std::exp (-t / decays[p]) * (1.0 - std::exp (-t * 900.0));
            out[i] += (float) (amps[p] * env * std::sin (w * (double) i) * (0.85 + 0.15 * std::cos (beat * (double) i)));
        }
    }
    normalise (out);
    return out;
}

static std::vector<float> choir (double sr)
{
    // Additive "Ah" vowel: harmonics shaped by three formants, three singers with their own vibrato.
    const size_t len = (size_t) (sr * 2.4);
    std::vector<float> out (len, 0.0f);
    const double f0 = midiToHz (57);
    const double formants[3][3] = { { 800, 80, 1.0 }, { 1150, 90, 0.5 }, { 2900, 120, 0.25 } };
    auto gain = [&] (double hz) {
        double g = 0;
        for (auto& fm : formants) { const double d = (hz - fm[0]) / fm[1]; g += fm[2] / (1.0 + d * d); }
        return g;
    };
    const double detunes[] = { 1.0, 1.006, 0.995 };
    for (int v = 0; v < 3; ++v)
    {
        double phase[40] {};
        const double vibRate = 5.1 + v * 0.37;
        for (size_t i = 0; i < len; ++i)
        {
            const double t = (double) i / sr;
            const double vib = 1.0 + 0.006 * std::sin (2.0 * M_PI * vibRate * t + v);
            const double f = f0 * detunes[v] * vib;
            double s = 0;
            for (int h = 1; h <= 40; ++h)
            {
                const double hz = f * h;
                if (hz > sr * 0.45) break;
                phase[h - 1] += 2.0 * M_PI * hz / sr;
                s += gain (hz) * std::sin (phase[h - 1]) / std::sqrt ((double) h);
            }
            const double env = std::min (1.0, t / 0.18) * std::min (1.0, (2.4 - t) / 0.4);
            out[i] += (float) (s * env);
        }
    }
    normalise (out);
    return out;
}

static std::vector<float> bayan (double sr)
{
    // Free-reed accordion: bright reed spectrum, two reeds detuned for the musette beating, bellows swell.
    const size_t len = (size_t) (sr * 2.0);
    std::vector<float> out (len, 0.0f);
    const double f0 = midiToHz (57);
    const double reeds[] = { 1.0, 1.0045, 0.997 };
    const double reedAmp[] = { 1.0, 0.8, 0.5 };
    for (int r = 0; r < 3; ++r)
        for (int h = 1; h <= 24; ++h)
        {
            const double hz = f0 * reeds[r] * h;
            if (hz > sr * 0.45) break;
            const double a = reedAmp[r] * (h % 2 == 1 ? 1.0 : 0.6) / std::pow ((double) h, 0.9);
            const double w = 2.0 * M_PI * hz / sr;
            for (size_t i = 0; i < len; ++i)
                out[i] += (float) (a * std::sin (w * (double) i + h));
        }
    for (size_t i = 0; i < len; ++i)
    {
        const double t = (double) i / sr;
        const double env = std::min (1.0, t / 0.06) * (0.85 + 0.15 * std::sin (2.0 * M_PI * 1.3 * t)) * std::min (1.0, (2.0 - t) / 0.25);
        out[i] *= (float) env;
    }
    normalise (out);
    return out;
}

SampleData makeBuiltInSample (int index, double sr)
{
    SampleData s;
    s.sampleRate = sr;
    const auto& info = builtInSamples();
    const int i = std::clamp (index, 0, (int) info.size() - 1);
    s.name = info[(size_t) i].name;
    s.rootNote = info[(size_t) i].rootNote;
    switch (i)
    {
        case 0: s.data = balalaika (sr); break;
        case 1: s.data = kolokol (sr); break;
        case 2: s.data = choir (sr); break;
        default: s.data = bayan (sr); break;
    }
    return s;
}
} // namespace mf
