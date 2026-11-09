#include "SampleData.h"
#include "DspUtils.h"
#include "../engine/Presets.h"
#include <array>
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

// ------------------------------------------------------------------ shared helpers
static double hz (int midi) { return 440.0 * std::pow (2.0, (midi - 69) / 12.0); }

/** Karplus-Strong string added into `out` at `offset`. `damping` near 0.5 = long sustain. */
static void pluck (std::vector<float>& out, double sr, double f, size_t offset, float gain, float damping, uint32_t seed, float brightness = 1.0f)
{
    Rng rng; rng.seed (seed);
    const int n = std::max (2, (int) (sr / f));
    std::vector<float> line ((size_t) n);
    float lp = 0;
    for (auto& x : line) { const float w = rng.bipolar(); lp += brightness * (w - lp); x = lp; }
    int idx = 0;
    for (size_t i = offset; i < out.size(); ++i)
    {
        const float cur = line[(size_t) idx], nxt = line[(size_t) ((idx + 1) % n)];
        line[(size_t) idx] = damping * (cur + nxt);
        idx = (idx + 1) % n;
        out[i] += gain * cur;
    }
}

/** Sum of resonance curves: a crude vocal-tract / instrument-body formant filter in the frequency domain. */
struct Formants
{
    std::vector<std::array<double, 3>> f; // centre, bandwidth, gain
    double gain (double hzv) const
    {
        double g = 0;
        for (auto& x : f) { const double d = (hzv - x[0]) / x[1]; g += x[2] / (1.0 + d * d); }
        return g;
    }
};

/** Additive voice: harmonics shaped by formants, several performers with their own vibrato and detune. */
static std::vector<float> additiveEnsemble (double sr, double seconds, double f0, const Formants& fm, int performers,
                                            double vibDepth, double attack, double release, double tilt)
{
    const size_t len = (size_t) (sr * seconds);
    std::vector<float> out (len, 0.0f);
    for (int v = 0; v < performers; ++v)
    {
        const double det = 1.0 + 0.0045 * (v - (performers - 1) * 0.5);
        const double vibRate = 4.6 + v * 0.41;
        const int maxH = std::min (80, (int) (sr * 0.45 / f0));
        std::vector<double> amp ((size_t) maxH + 1), ph ((size_t) maxH + 1, 0.0);
        for (int h = 1; h <= maxH; ++h) { amp[(size_t) h] = fm.gain (f0 * h) / std::pow ((double) h, tilt); ph[(size_t) h] = v * 1.7 * h; }
        for (size_t i = 0; i < len; ++i)
        {
            const double t = (double) i / sr;
            const double vib = 1.0 + vibDepth * std::sin (2.0 * M_PI * vibRate * t + v) * std::min (1.0, t / 0.4);
            const double f = f0 * det * vib;
            double sum = 0;
            for (int h = 1; h <= maxH; ++h)
            {
                ph[(size_t) h] += 2.0 * M_PI * f * h / sr;
                sum += amp[(size_t) h] * std::sin (ph[(size_t) h]);
            }
            const double env = std::min (1.0, t / attack) * std::min (1.0, (seconds - t) / release);
            out[i] += (float) (sum * env);
        }
    }
    return out;
}

/** Inharmonic bell: partial ratios, amplitudes and decays, with slow beating between partials. */
static std::vector<float> bell (double sr, double seconds, double f, const std::vector<std::array<double, 3>>& partials, double strikeNoise)
{
    const size_t len = (size_t) (sr * seconds);
    std::vector<float> out (len, 0.0f);
    int k = 0;
    for (auto& p : partials)
    {
        const double w = 2.0 * M_PI * f * p[0] / sr;
        const double beat = 2.0 * M_PI * (0.6 + k * 0.29) / sr;
        for (size_t i = 0; i < len; ++i)
        {
            const double t = (double) i / sr;
            const double env = std::exp (-t / p[2]) * (1.0 - std::exp (-t * 900.0));
            out[i] += (float) (p[1] * env * std::sin (w * (double) i) * (0.85 + 0.15 * std::cos (beat * (double) i)));
        }
        ++k;
    }
    Rng rng; rng.seed (99);
    SVF lp; lp.set ((float) std::min (8000.0, f * 6.0), 0.2f, (float) sr);
    for (size_t i = 0; i < std::min (len, (size_t) (sr * 0.03)); ++i)
        out[i] += (float) (strikeNoise * std::exp (-(double) i / (sr * 0.006))) * lp.process (rng.bipolar()).low;
    return out;
}

// ------------------------------------------------------------------ strings
static std::vector<float> bassBalalaika (double sr)
{
    std::vector<float> out ((size_t) (sr * 2.6), 0.0f);
    pluck (out, sr, hz (40), 0, 1.0f, 0.4993f, 71, 0.6f);
    pluck (out, sr, hz (40) * 1.002, (size_t) (sr * 0.004), 0.5f, 0.4993f, 72, 0.6f);
    SVF body; body.set (160.0f, 0.5f, (float) sr);
    SVF wood; wood.set (900.0f, 0.4f, (float) sr);
    for (auto& x : out) x = x + 1.2f * body.process (x).band + 0.3f * wood.process (x).band;
    normalise (out);
    return out;
}

static std::vector<float> domraTremolo (double sr)
{
    // A round-bodied lute played with rapid alternating pick strokes: one string re-excited ~18 times a second.
    const size_t len = (size_t) (sr * 1.8);
    std::vector<float> out (len, 0.0f);
    const double f = hz (69);
    const int n = (int) (sr / f);
    std::vector<float> line ((size_t) n, 0.0f);
    Rng rng; rng.seed (55);
    const size_t strokeEvery = (size_t) (sr * 0.056);
    int idx = 0;
    for (size_t i = 0; i < len; ++i)
    {
        if (i % strokeEvery == 0 && i < len - (size_t) (sr * 0.25))
        {
            const float g = ((i / strokeEvery) % 2 == 0 ? 0.7f : 0.5f);
            for (int k = 0; k < n; ++k) line[(size_t) k] += g * rng.bipolar() * (0.5f + 0.5f * std::sin (dsp::pi * (float) k / (float) n));
        }
        const float cur = line[(size_t) idx], nxt = line[(size_t) ((idx + 1) % n)];
        line[(size_t) idx] = 0.4975f * (cur + nxt);
        idx = (idx + 1) % n;
        out[i] = cur;
    }
    SVF body; body.set (500.0f, 0.45f, (float) sr);
    for (auto& x : out) x = x + 0.6f * body.process (x).band;
    normalise (out);
    return out;
}

static std::vector<float> gusli (double sr)
{
    // Psaltery: a bright pluck near the bridge with the open strings ringing in sympathy.
    std::vector<float> out ((size_t) (sr * 4.0), 0.0f);
    const double f = hz (62);
    pluck (out, sr, f, 0, 1.0f, 0.4996f, 11, 0.95f);
    const double sym[] = { 1.5, 2.0, 1.25, 3.0 };
    for (int k = 0; k < 4; ++k)
        pluck (out, sr, f * sym[k], (size_t) (sr * (0.02 + 0.01 * k)), 0.12f, 0.4997f, (uint32_t) (20 + k), 0.4f);
    SVF board; board.set (2600.0f, 0.3f, (float) sr);
    for (auto& x : out) x = x + 0.35f * board.process (x).band;
    normalise (out);
    return out;
}

// ------------------------------------------------------------------ winds and reeds
static std::vector<float> zhaleika (double sr)
{
    // Single-reed hornpipe with a cow-horn bell: odd harmonics, a nasal formant, a pitch scoop and breath.
    const double secs = 1.7, f0 = hz (72);
    const size_t len = (size_t) (sr * secs);
    std::vector<float> out (len, 0.0f);
    Formants fm { { { 1400, 300, 1.0 }, { 3000, 600, 0.45 }, { 600, 250, 0.5 } } };
    double ph[24] {};
    Rng rng; rng.seed (7);
    SVF breath; breath.set (2500.0f, 0.4f, (float) sr);
    for (size_t i = 0; i < len; ++i)
    {
        const double t = (double) i / sr;
        const double scoop = std::pow (2.0, -0.3 * std::exp (-t / 0.05) / 12.0);
        const double wob = 1.0 + 0.004 * std::sin (2.0 * M_PI * 4.3 * t) + 0.002 * std::sin (2.0 * M_PI * 7.1 * t);
        const double f = f0 * scoop * wob;
        double sum = 0;
        for (int h = 1; h <= 23; ++h)
        {
            if (f * h > sr * 0.45) break;
            ph[h] += 2.0 * M_PI * f * h / sr;
            sum += (h % 2 == 1 ? 1.0 : 0.25) * fm.gain (f0 * h) * std::sin (ph[h]) / std::sqrt ((double) h);
        }
        const double env = std::min (1.0, t / 0.03) * std::min (1.0, (secs - t) / 0.15);
        out[i] = (float) ((sum + 0.15 * breath.process (rng.bipolar()).band) * env);
    }
    normalise (out);
    return out;
}

static std::vector<float> svirel (double sr)
{
    // End-blown shepherd's flute: near-sine tone, breath noise tuned to the pitch, a chiff at the start.
    const double secs = 1.9, f0 = hz (79);
    const size_t len = (size_t) (sr * secs);
    std::vector<float> out (len, 0.0f);
    Rng rng; rng.seed (13);
    SVF tuned; tuned.set ((float) f0, 0.92f, (float) sr);
    SVF chiff; chiff.set (5000.0f, 0.2f, (float) sr);
    double ph = 0;
    for (size_t i = 0; i < len; ++i)
    {
        const double t = (double) i / sr;
        const double vib = 1.0 + 0.008 * std::sin (2.0 * M_PI * 5.2 * t) * std::min (1.0, std::max (0.0, (t - 0.3) / 0.4));
        ph += 2.0 * M_PI * f0 * vib / sr;
        const double tone = std::sin (ph) + 0.15 * std::sin (2 * ph) + 0.05 * std::sin (3 * ph);
        const float n = rng.bipolar();
        const double env = std::min (1.0, t / 0.06) * std::min (1.0, (secs - t) / 0.2);
        out[i] = (float) ((tone + 0.25 * tuned.process (n).band + 0.5 * std::exp (-t / 0.03) * chiff.process (n).band) * env);
    }
    normalise (out);
    return out;
}

static std::vector<float> reedOrgan (double sr, double secs, double f0, const std::vector<double>& reeds, double tiltPow, double swell)
{
    const size_t len = (size_t) (sr * secs);
    std::vector<float> out (len, 0.0f);
    for (size_t r = 0; r < reeds.size(); ++r)
        for (int h = 1; h <= 28; ++h)
        {
            const double f = f0 * reeds[r] * h;
            if (f > sr * 0.45) break;
            const double a = (r == 0 ? 1.0 : 0.85) * (h % 2 == 1 ? 1.0 : 0.65) / std::pow ((double) h, tiltPow);
            const double w = 2.0 * M_PI * f / sr;
            for (size_t i = 0; i < len; ++i) out[i] += (float) (a * std::sin (w * (double) i + h + r));
        }
    for (size_t i = 0; i < len; ++i)
    {
        const double t = (double) i / sr;
        out[i] *= (float) (std::min (1.0, t / 0.05) * (1.0 - swell + swell * std::sin (M_PI * std::min (1.0, t / secs))) * std::min (1.0, (secs - t) / 0.2));
    }
    normalise (out);
    return out;
}

static std::vector<float> volynka (double sr)
{
    // Bagpipe: a buzzy chanter over a continuous drone an octave below, with bag-pressure wobble.
    const double secs = 3.0, f0 = hz (57);
    const size_t len = (size_t) (sr * secs);
    std::vector<float> out (len, 0.0f);
    Formants fm { { { 1200, 400, 1.0 }, { 2800, 700, 0.4 } } };
    double phC[30] {}, phD[30] {};
    for (size_t i = 0; i < len; ++i)
    {
        const double t = (double) i / sr;
        const double press = 1.0 + 0.003 * std::sin (2.0 * M_PI * 0.7 * t);
        double sum = 0;
        for (int h = 1; h < 30; ++h)
        {
            if (f0 * h > sr * 0.45) break;
            phC[h] += 2.0 * M_PI * f0 * press * h / sr;
            phD[h] += 2.0 * M_PI * f0 * 0.5 * h / sr;
            sum += fm.gain (f0 * h) * std::sin (phC[h]) / std::sqrt ((double) h);
            sum += 0.5 * fm.gain (f0 * 0.5 * h) * std::sin (phD[h]) / std::sqrt ((double) h);
        }
        out[i] = (float) (sum * std::min (1.0, t / 0.12) * std::min (1.0, (secs - t) / 0.3));
    }
    normalise (out);
    return out;
}

// ------------------------------------------------------------------ voices
static std::vector<float> oktavist (double sr)
{
    // Russian basso profundo ("oktavist") on a dark 'o' vowel, three singers.
    Formants fm { { { 420, 70, 1.0 }, { 780, 90, 0.55 }, { 2600, 200, 0.12 } } };
    auto out = additiveEnsemble (sr, 3.0, hz (38), fm, 3, 0.005, 0.35, 0.5, 0.3);
    normalise (out);
    return out;
}

// ------------------------------------------------------------------ bells
static std::vector<float> zazvonnye (double sr)
{
    auto out = bell (sr, 2.2, hz (79), { { 1.0, 1.0, 0.9 }, { 2.03, 0.6, 0.6 }, { 2.74, 0.45, 0.45 }, { 3.9, 0.3, 0.3 }, { 5.3, 0.2, 0.2 }, { 0.5, 0.25, 1.1 } }, 0.4);
    normalise (out);
    return out;
}

static std::vector<float> blagovest (double sr)
{
    auto out = bell (sr, 7.0, hz (36), { { 0.5, 0.8, 7.0 }, { 1.0, 0.9, 5.5 }, { 1.19, 0.75, 4.8 }, { 1.51, 0.4, 3.5 }, { 2.0, 0.85, 4.0 },
                                         { 2.51, 0.35, 2.4 }, { 2.66, 0.3, 2.0 }, { 3.01, 0.25, 1.6 }, { 4.16, 0.2, 1.1 }, { 5.43, 0.12, 0.7 } }, 0.8);
    normalise (out);
    return out;
}

// ------------------------------------------------------------------ percussion and the jaw harp
static std::vector<float> vargan (double sr)
{
    // Jaw harp: a decaying buzzy lamella, its overtones picked out by the player's moving mouth cavity.
    const double secs = 1.7, f0 = hz (50);
    const size_t len = (size_t) (sr * secs);
    std::vector<float> out (len, 0.0f);
    SVF mouth;
    double ph = 0;
    for (size_t i = 0; i < len; ++i)
    {
        const double t = (double) i / sr;
        ph += f0 / sr; ph -= std::floor (ph);
        const float buzz = (float) (2.0 * ph - 1.0) * (float) std::exp (-t / 0.9);
        if (i % 32 == 0)
            mouth.set ((float) (900.0 + 1300.0 * (0.5 - 0.5 * std::cos (2.0 * M_PI * 1.6 * t))), 0.9f, (float) sr);
        out[i] = 0.25f * buzz + mouth.process (buzz).band;
    }
    normalise (out);
    return out;
}

static void woodClack (std::vector<float>& out, double sr, size_t at, float gain, uint32_t seed, float f1, float f2)
{
    Rng rng; rng.seed (seed);
    SVF a; a.set (f1, 0.85f, (float) sr);
    SVF b; b.set (f2, 0.8f, (float) sr);
    for (size_t i = at; i < std::min (out.size(), at + (size_t) (sr * 0.08)); ++i)
    {
        const double t = (double) (i - at) / sr;
        const float n = t < 0.0015 ? rng.bipolar() : 0.0f;
        out[i] += gain * (float) std::exp (-t / 0.018) * (a.process (n * 8.0f).band + 0.7f * b.process (n * 8.0f).band);
    }
}

static std::vector<float> lozhki (double sr)
{
    std::vector<float> out ((size_t) (sr * 0.35), 0.0f);
    woodClack (out, sr, 0, 1.0f, 3, 1100.0f, 2400.0f);
    woodClack (out, sr, (size_t) (sr * 0.075), 0.7f, 4, 1250.0f, 2700.0f);
    normalise (out);
    return out;
}

static std::vector<float> treshchotka (double sr)
{
    std::vector<float> out ((size_t) (sr * 0.45), 0.0f);
    double t = 0;
    for (int k = 0; k < 8; ++k)
    {
        woodClack (out, sr, (size_t) (sr * t), 1.0f - 0.07f * (float) k, (uint32_t) (40 + k), 1700.0f + 60.0f * (float) k, 3200.0f);
        t += 0.034 - 0.0015 * k;
    }
    normalise (out);
    return out;
}

static std::vector<float> buben (double sr)
{
    // Frame drum with jingles: a falling membrane thump plus shimmering metal discs.
    const size_t len = (size_t) (sr * 0.9);
    std::vector<float> out (len, 0.0f);
    Rng rng; rng.seed (17);
    SVF hp; hp.set (6500.0f, 0.3f, (float) sr);
    double ph = 0;
    for (size_t i = 0; i < len; ++i)
    {
        const double t = (double) i / sr;
        const double f = 100.0 + 45.0 * std::exp (-t / 0.03);
        ph += 2.0 * M_PI * f / sr;
        const double drum = std::sin (ph) * std::exp (-t / 0.18) + 0.3 * rng.bipolar() * std::exp (-t / 0.01);
        const double jingle = hp.process (rng.bipolar()).high * std::exp (-t / 0.25) * (0.6 + 0.4 * std::sin (2.0 * M_PI * 23.0 * t));
        out[i] = (float) (drum + 0.6 * jingle);
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
        case 0:  s.data = balalaika (sr); break;
        case 1:  s.data = kolokol (sr); break;
        case 2:  s.data = choir (sr); break;
        case 3:  s.data = bayan (sr); break;
        case 4:  s.data = bassBalalaika (sr); break;
        case 5:  s.data = domraTremolo (sr); break;
        case 6:  s.data = gusli (sr); break;
        case 7:  s.data = zhaleika (sr); break;
        case 8:  s.data = svirel (sr); break;
        case 9:  s.data = reedOrgan (sr, 2.0, hz (64), { 1.0, 1.0087, 0.9945 }, 0.6, 0.15); break; // Tula garmon: bright, "wet" tuning
        case 10: s.data = volynka (sr); break;
        case 11: s.data = oktavist (sr); break;
        case 12: s.data = zazvonnye (sr); break;
        case 13: s.data = blagovest (sr); break;
        case 14: s.data = vargan (sr); break;
        case 15: s.data = lozhki (sr); break;
        case 16: s.data = buben (sr); break;
        default: s.data = treshchotka (sr); break;
    }
    return s;
}
} // namespace mf
