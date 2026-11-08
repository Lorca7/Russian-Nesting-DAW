#include "FractalFugue.h"
#include <random>
#include <vector>

namespace mf
{
double FractalFugue::ratioValue (int ratio)
{
    switch (ratio)
    {
        case Ratio::Fifth:   return 1.5;
        case Ratio::Golden:  return 1.6180339887498949;
        case Ratio::Twelfth: return 3.0;
        case Ratio::Fourth:  return 4.0 / 3.0;
        case Ratio::Tritone: return 1.4142135623730951;
        default:             return 2.0;
    }
}

const char* FractalFugue::ratioName (int ratio)
{
    switch (ratio)
    {
        case Ratio::Fifth:   return "Fifth 3:2";
        case Ratio::Golden:  return "Golden 1.618";
        case Ratio::Twelfth: return "Twelfth 3:1";
        case Ratio::Fourth:  return "Fourth 4:3";
        case Ratio::Tritone: return "Tritone 1.414";
        default:             return "Octave 2:1";
    }
}

const char* generatorName (int g)
{
    switch (g)
    {
        case Generator::FractalMidpoint: return "Fractal contour";
        case Generator::RandomWalk:      return "Random walk";
        case Generator::BellPeal:        return "Bell peal (Zvon)";
        case Generator::ZnamennyChant:   return "Znamenny chant";
        case Generator::DSCH:            return "DSCH motif";
        default:                         return "?";
    }
}

static void setSubject (Project& p, const std::vector<int>& degrees, const std::vector<int>& lengths)
{
    p.subjectLength = (int) std::min<size_t> (degrees.size(), kMaxSteps);
    for (int i = 0; i < kMaxSteps; ++i)
    {
        auto& s = p.subject[(size_t) i];
        s = Step {};
        if (i < p.subjectLength)
        {
            s.degree = degrees[(size_t) i];
            s.length = i < (int) lengths.size() ? lengths[(size_t) i] : 1;
        }
    }
}

void generateSubject (Project& p, int generator, uint32_t seed)
{
    std::mt19937 rng (seed);
    auto uni = [&] (int lo, int hi) { return std::uniform_int_distribution<int> (lo, hi) (rng); };

    switch (generator)
    {
        case Generator::FractalMidpoint:
        {
            // Midpoint displacement: a self-similar melodic contour over 9 points, roughness halves per level.
            std::vector<double> c (9, 0.0);
            c[8] = uni (-2, 4);
            double spread = 4.0;
            for (int step = 8; step > 1; step /= 2)
            {
                for (int i = step / 2; i < 9; i += step)
                    c[(size_t) i] = 0.5 * (c[(size_t) (i - step / 2)] + c[(size_t) (i + step / 2)])
                                    + std::uniform_real_distribution<double> (-spread, spread) (rng);
                spread *= 0.55;
            }
            std::vector<int> deg, len;
            for (int i = 0; i < 8; ++i)
            {
                deg.push_back ((int) std::lround (c[(size_t) i]));
                len.push_back (i == 7 || uni (0, 5) == 0 ? 2 : 1);
            }
            deg[0] = 0;
            setSubject (p, deg, len);
            break;
        }
        case Generator::RandomWalk:
        {
            const int n = uni (6, 10);
            std::vector<int> deg { 0 }, len { 1 };
            for (int i = 1; i < n; ++i)
            {
                deg.push_back (std::clamp (deg.back() + uni (-2, 2) + (uni (0, 6) == 0 ? 3 : 0), -5, 9));
                len.push_back (uni (0, 4) == 0 ? 2 : 1);
            }
            setSubject (p, deg, len);
            break;
        }
        case Generator::BellPeal:
        {
            // Russian bell ringing: a descending peal rotated each time it repeats, ending on the big bell.
            std::vector<int> peal { 7, 5, 4, 2, 0 };
            const int rot = uni (0, 4);
            std::vector<int> deg, len;
            for (int i = 0; i < 5; ++i) { deg.push_back (peal[(size_t) ((i + rot) % 5)]); len.push_back (1); }
            deg.insert (deg.begin(), 0);
            len.insert (len.begin(), 2);
            deg.push_back (-3); len.push_back (2);
            setSubject (p, deg, len);
            break;
        }
        case Generator::ZnamennyChant:
            setSubject (p, { 0, 1, 2, 1, 0, 1, 2, 3, 2, 1, 0, 0 }, { 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 2 });
            break;
        case Generator::DSCH:
            // D - Eb - C - B in C harmonic minor: degrees 1, 2, 0, -1
            setSubject (p, { 1, 2, 0, -1 }, { 1, 1, 1, 1 });
            p.subject[0].degree = 1;
            break;
        default: break;
    }

    for (int i = 0; i < p.subjectLength; ++i)
        p.subject[(size_t) i].velocity = (i == 0 ? 1.0f : 0.75f + 0.2f * std::uniform_real_distribution<float> (0.0f, 1.0f) (rng));
}

void nestChords (Project& p)
{
    for (int d = 0; d < kNumDolls; ++d)
        p.dolls[(size_t) d].chordSize = kNumDolls - d;
}
} // namespace mf
