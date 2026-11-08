#pragma once
#include "Project.h"
#include "Scales.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mf
{
/** One note (or chord) played by one doll. Times are in beats (quarter notes). */
struct NoteEvent
{
    int doll = 0;
    double beat = 0.0;
    double length = 0.0;          // sounding length in beats (legato applied)
    int notes[kMaxChord] {};
    int numNotes = 0;
    float velocity = 0.0f;
    int fractalDegree = 0;        // degree before the answer transposition (drives sample slicing)
    int step = 0;                 // index into the (transformed) subject
    int64_t iteration = 0;        // which statement of the subject this is
};

/**
    The fractal fugue.

    Every doll states the same subject. Doll 0 is the largest (slowest, lowest) and doll 3 the
    smallest (fastest, highest). The relationships are self-similar:

      tempo   - doll k's pulse is ratio^(3-k) times the smallest doll's pulse (augmentation /
                diminution, the way a fugue states its subject in longer or shorter values).
      pitch   - each doll's subject is transposed so its first note lands on whatever note its
                parent (the next-larger doll) is sounding at that moment. With coupling = 1 this
                makes the melody a true fractal: the subject replayed inside each note of itself.
                Dolls 1 and 3 also sit a fugal "answer" interval away (subject / answer / subject / answer).
      notes   - the chord size per doll stacks scale thirds, so larger dolls can hold larger chords.
      rhythm  - each doll can be gated by a fractal sequence (Cantor set, Sierpinski triangle,
                Fibonacci word), and can be inverted / reversed like a fugal subject.

    Everything is a pure function of the project and the beat, so playback, the piano roll,
    MIDI export and offline WAV rendering all agree exactly.
*/
class FractalFugue
{
public:
    static double ratioValue (int ratio);
    static const char* ratioName (int ratio);

    static double pulseBeats (const Project& p, int doll)
    {
        return p.pulseBeats * std::pow (ratioValue (p.ratio), (double) (kNumDolls - 1 - doll));
    }

    static int subjectUnits (const Project& p)
    {
        int u = 0;
        for (int i = 0; i < clampLength (p); ++i)
            u += std::max (1, p.subject[(size_t) i].length);
        return std::max (1, u);
    }

    static double cycleBeats (const Project& p, int doll) { return subjectUnits (p) * pulseBeats (p, doll); }

    static double entryBeat (const Project& p, int doll)
    {
        return p.staggerEntries ? p.entryGapBeats * doll : 0.0;
    }

    /** The subject after the doll's fugal transform, with cumulative positions in pulses. */
    struct Transformed
    {
        Step steps[kMaxSteps];
        int pos[kMaxSteps];
        int length = 0;
        int units = 1;
        int anchor = 0;
    };

    static Transformed transformed (const Project& p, int doll)
    {
        Transformed t;
        t.length = clampLength (p);
        t.anchor = p.subject[0].degree;
        const int tr = p.dolls[(size_t) doll].transform;
        const bool retro = (tr == Transform::Retrograde || tr == Transform::RetroInversion);
        const bool inv = (tr == Transform::Inversion || tr == Transform::RetroInversion);
        int acc = 0;
        for (int i = 0; i < t.length; ++i)
        {
            Step s = p.subject[(size_t) (retro ? t.length - 1 - i : i)];
            s.length = std::clamp (s.length, 1, 8);
            if (inv)
                s.degree = t.anchor - (s.degree - t.anchor);
            t.steps[i] = s;
            t.pos[i] = acc;
            acc += s.length;
        }
        t.units = std::max (1, acc);
        return t;
    }

    /** The doll's own (transformed) subject degree at a beat, ignoring gates and rests. */
    static int ownDegreeAt (const Project& p, int doll, double beat)
    {
        const auto t = transformed (p, doll);
        const double start = entryBeat (p, doll);
        if (beat < start)
            return t.anchor;
        const double pb = pulseBeats (p, doll);
        const double phaseUnits = std::fmod ((beat - start) / pb + 1.0e-7, (double) t.units);
        int idx = 0;
        for (int i = 0; i < t.length; ++i)
            if (t.pos[i] <= phaseUnits)
                idx = i;
        return t.steps[idx].degree;
    }

    /** The doll's fractal degree: its own subject riding on its parent's current note. */
    static int fractalDegreeAt (const Project& p, int doll, double beat)
    {
        return combine (p, doll, ownDegreeAt (p, doll, beat), beat);
    }

    static int combine (const Project& p, int doll, int ownDegree, double beat)
    {
        if (doll == 0)
            return ownDegree;
        const int anchor = p.subject[0].degree;
        const int parent = fractalDegreeAt (p, doll - 1, beat + 1.0e-7);
        int off = ownDegree + (int) std::lround (p.coupling * (double) (parent - anchor));
        if (p.fold)
        {
            const int n = (int) scaleAt (p.scaleIndex).steps.size();
            int rel = off - anchor;
            while (rel > n) rel -= n;
            while (rel < -n) rel += n;
            off = anchor + rel;
        }
        return off;
    }

    static int answerOffset (const Project& p, int doll) { return (doll % 2 == 1) ? p.answerInterval : 0; }

    static int degreeToMidi (const Project& p, int doll, int degree)
    {
        const auto& sc = scaleAt (p.scaleIndex);
        const int midi = 12 * (p.dolls[(size_t) doll].octave + 1) + p.root + degreeToSemitone (sc, degree);
        return std::clamp (midi, 0, 127);
    }

    static bool gateOpen (int gate, int64_t globalStep, int64_t iteration, int stepIndex)
    {
        switch (gate)
        {
            case Gate::Cantor:
            {
                int64_t g = globalStep % 27;
                for (int d = 0; d < 3; ++d) { if (g % 3 == 1) return false; g /= 3; }
                return true;
            }
            case Gate::Sierpinski:
            {
                const int row = 15 - (int) (iteration % 16);
                return (stepIndex & row) == stepIndex;
            }
            case Gate::Fibonacci:
            {
                constexpr double phi = 1.6180339887498949;
                const auto a = (int64_t) std::floor ((double) (globalStep + 2) / phi);
                const auto b = (int64_t) std::floor ((double) (globalStep + 1) / phi);
                return (a - b) == 1;
            }
            default: return true;
        }
    }

    /** Calls fn(const NoteEvent&) for every note doll `doll` starts in [beatFrom, beatTo). Allocation free. */
    template <typename Fn>
    static void forEachEvent (const Project& p, int doll, double beatFrom, double beatTo, Fn&& fn)
    {
        const auto& d = p.dolls[(size_t) doll];
        const auto t = transformed (p, doll);
        const double start = entryBeat (p, doll);
        const double pb = pulseBeats (p, doll);
        const double cyc = t.units * pb;
        if (beatTo <= start || cyc <= 0.0)
            return;

        auto n = (int64_t) std::floor ((std::max (beatFrom, start) - start) / cyc);
        n = std::max<int64_t> (0, n - 1);

        for (;; ++n)
        {
            const double cycleStart = start + (double) n * cyc;
            if (cycleStart >= beatTo)
                break;

            for (int i = 0; i < t.length; ++i)
            {
                const double beat = cycleStart + t.pos[i] * pb;
                if (beat < beatFrom || beat >= beatTo)
                    continue;
                const auto& s = t.steps[i];
                if (! s.on)
                    continue;
                if (! gateOpen (d.gate, n * t.length + i, n, i))
                    continue;

                NoteEvent e;
                e.doll = doll;
                e.beat = beat;
                e.length = std::max (0.01, s.length * pb * (double) d.legato);
                e.step = i;
                e.iteration = n;
                e.fractalDegree = combine (p, doll, s.degree, beat);
                const int deg = e.fractalDegree + answerOffset (p, doll);
                e.numNotes = std::clamp (d.chordSize, 1, kMaxChord);
                for (int c = 0; c < e.numNotes; ++c)
                    e.notes[c] = degreeToMidi (p, doll, deg + 2 * c);
                e.velocity = std::clamp (s.velocity * (i == 0 ? 1.0f : 0.86f), 0.0f, 1.0f);
                fn (e);
            }
        }
    }

private:
    static int clampLength (const Project& p) { return std::clamp (p.subjectLength, 1, kMaxSteps); }
};

// ---- subject generators
namespace Generator { enum { FractalMidpoint, RandomWalk, BellPeal, ZnamennyChant, DSCH, count }; }
const char* generatorName (int g);
void generateSubject (Project& p, int generator, uint32_t seed);

/** Sets chord sizes 4,3,2,1 so the largest doll holds the largest chord. */
void nestChords (Project& p);
} // namespace mf
