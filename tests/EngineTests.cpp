// Plain C++ tests for the fractal fugue engine (no JUCE needed).
#include "../Source/engine/FractalFugue.h"
#include "../Source/engine/Presets.h"
#include "../Source/engine/Scores.h"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

static int failures = 0;
#define CHECK(cond) do { if (! (cond)) { std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

using namespace mf;

static std::vector<NoteEvent> collect (const Project& p, int doll, double a, double b)
{
    std::vector<NoteEvent> v;
    FractalFugue::forEachEvent (p, doll, a, b, [&] (const NoteEvent& e) { v.push_back (e); });
    return v;
}

int main()
{
    // ---- scales
    const auto& major = scaleAt (0);
    CHECK (degreeToSemitone (major, 0) == 0);
    CHECK (degreeToSemitone (major, 7) == 12);
    CHECK (degreeToSemitone (major, -1) == -1);
    CHECK (degreeToSemitone (major, -7) == -12);
    const auto& obikhod = scaleAt (findScale ("Obikhod (Znamenny Chant)"));
    CHECK (obikhod.period == 5);
    // G A B c d e f g a bb : 0 2 4 5 7 9 10 12 14 15
    const int expectObikhod[] = { 0, 2, 4, 5, 7, 9, 10, 12, 14, 15 };
    for (int i = 0; i < 10; ++i)
        CHECK (degreeToSemitone (obikhod, i) == expectObikhod[i]);
    std::set<std::string> groups;
    for (auto& s : allScales()) groups.insert (s.group);
    CHECK (groups.count ("Russian & Slavic") == 1);
    for (auto& s : allScales())
    {
        CHECK (! s.steps.empty());
        CHECK (s.steps.front() == 0);
        for (size_t i = 1; i < s.steps.size(); ++i) CHECK (s.steps[i] > s.steps[i - 1]);
        CHECK (s.steps.back() < s.period);
    }

    // ---- tempo relationship: each larger doll's pulse is `ratio` times the next smaller one
    Project p;
    p.staggerEntries = false;
    p.coupling = 0.0f;
    p.answerInterval = 0;
    for (int d = 0; d < kNumDolls - 1; ++d)
        CHECK (std::abs (FractalFugue::pulseBeats (p, d) / FractalFugue::pulseBeats (p, d + 1) - 2.0) < 1e-9);
    p.ratio = Ratio::Golden;
    CHECK (std::abs (FractalFugue::pulseBeats (p, 0) / FractalFugue::pulseBeats (p, 1) - 1.6180339887) < 1e-6);
    p.ratio = Ratio::Octave;

    // ---- note counts scale with the ratio (smallest plays 8x as many notes as the largest)
    const double span = FractalFugue::cycleBeats (p, 0) * 2;
    const auto big = collect (p, 0, 0, span);
    const auto tiny = collect (p, 3, 0, span);
    CHECK (big.size() == (size_t) (2 * p.subjectLength));
    CHECK (tiny.size() == big.size() * 8);

    // ---- strict canon (coupling 0, no answer) gives the same pitch classes in every doll
    for (size_t i = 0; i < (size_t) p.subjectLength; ++i)
        CHECK ((big[i].notes[0] - tiny[i].notes[0]) % 12 == 0);

    // ---- fractal coupling: the second doll's first note of each statement lands on the parent's current note
    p.coupling = 1.0f;
    p.fold = false;
    const auto mid = collect (p, 1, 0, FractalFugue::cycleBeats (p, 0));
    for (auto& e : mid)
        if (e.step == 0)
        {
            const int parent = FractalFugue::fractalDegreeAt (p, 0, e.beat);
            CHECK (e.fractalDegree == parent);
        }

    // ---- inversion mirrors the subject around its first note
    p.coupling = 0.0f;
    p.dolls[0].transform = Transform::Inversion;
    const auto inv = collect (p, 0, 0, FractalFugue::cycleBeats (p, 0));
    for (int i = 0; i < p.subjectLength; ++i)
        CHECK (inv[(size_t) i].fractalDegree == 2 * p.subject[0].degree - p.subject[(size_t) i].degree);
    p.dolls[0].transform = Transform::Retrograde;
    const auto ret = collect (p, 0, 0, FractalFugue::cycleBeats (p, 0));
    CHECK (ret.front().fractalDegree == p.subject[(size_t) p.subjectLength - 1].degree);
    p.dolls[0].transform = Transform::Prime;

    // ---- chords stack thirds
    p.dolls[0].chordSize = 3;
    const auto chord = collect (p, 0, 0, 0.01);
    CHECK (chord.size() == 1 && chord[0].numNotes == 3);
    CHECK (chord[0].notes[1] > chord[0].notes[0] && chord[0].notes[2] > chord[0].notes[1]);
    nestChords (p);
    CHECK (p.dolls[0].chordSize == 4 && p.dolls[3].chordSize == 1);

    // ---- staggered entries
    p.staggerEntries = true;
    p.entryGapBeats = 4;
    const auto late = collect (p, 3, 0, 100);
    CHECK (! late.empty() && late.front().beat >= 12.0 - 1e-9);

    // ---- block boundaries never duplicate or drop events
    {
        std::vector<NoteEvent> whole = collect (p, 3, 0, 64);
        size_t pieces = 0;
        for (double b = 0; b < 64; b += 0.0107)
            pieces += collect (p, 3, b, std::min (64.0, b + 0.0107)).size();
        CHECK (pieces == whole.size());
    }

    // ---- gates are fractal: Cantor keeps 8 of every 27 pulses
    {
        int on = 0;
        for (int g = 0; g < 27; ++g) on += FractalFugue::gateOpen (Gate::Cantor, g, 0, 0) ? 1 : 0;
        CHECK (on == 8);
        int fib = 0;
        for (int g = 0; g < 1000; ++g) fib += FractalFugue::gateOpen (Gate::Fibonacci, g, 0, 0) ? 1 : 0;
        CHECK (fib > 600 && fib < 640);
    }

    // ---- generators produce valid subjects
    for (int g = 0; g < Generator::count; ++g)
        for (uint32_t seed = 1; seed < 20; ++seed)
        {
            Project q;
            generateSubject (q, g, seed);
            CHECK (q.subjectLength >= 1 && q.subjectLength <= kMaxSteps);
            for (int i = 0; i < q.subjectLength; ++i)
                CHECK (q.subject[(size_t) i].length >= 1 && q.subject[(size_t) i].length <= 4);
        }

    // ---- notes stay in MIDI range for every scale and ratio
    {
        Project q = makeDefaultProject();
        for (int s = 0; s < numScales(); ++s)
            for (int r = 0; r < Ratio::count; ++r)
            {
                q.scaleIndex = s; q.ratio = r;
                for (int d = 0; d < kNumDolls; ++d)
                    FractalFugue::forEachEvent (q, d, 0, 32, [&] (const NoteEvent& e) {
                        for (int c = 0; c < e.numNotes; ++c) CHECK (e.notes[c] >= 0 && e.notes[c] <= 127);
                        CHECK (e.length > 0);
                    });
            }
    }

    CHECK (synthPresets().size() >= 18);
    for (auto& pr : synthPresets()) CHECK (! pr.note.empty());

    // ---- drone gate: one held note per statement, as long as the statement
    {
        Project q;
        q.staggerEntries = false;
        q.dolls[0].gate = Gate::Drone;
        const auto ev = collect (q, 0, 0, FractalFugue::cycleBeats (q, 0) * 3);
        CHECK (ev.size() == 3);
        for (auto& e : ev) CHECK (e.length > FractalFugue::cycleBeats (q, 0) * 0.9);
    }

    // ---- built-in samples: unique names, valid roots
    {
        std::set<std::string> names;
        for (auto& b : builtInSamples()) { names.insert (b.name); CHECK (b.rootNote > 20 && b.rootNote < 100); }
        CHECK (names.size() == builtInSamples().size());
        CHECK (builtInSamples().size() >= 18);
        CHECK (std::string (builtInSamples()[0].name) == "Balalaika pluck"); // saved projects depend on indices
    }

    // ---- scores: everything they reference exists, every doll plays, notes stay in range
    {
        for (auto& p : scoreProblems()) std::printf ("score problem: %s\n", p.c_str());
        CHECK (scoreProblems().empty());
        CHECK (scores().size() >= 15);
        std::set<std::string> names;
        for (auto& sc : scores())
        {
            names.insert (sc.name);
            CHECK (! sc.description.empty() && ! sc.basedOn.empty());
            for (int d = 0; d < kNumDolls; ++d)
            {
                int count = 0;
                FractalFugue::forEachEvent (sc.project, d, 0, 128, [&] (const NoteEvent& e) {
                    ++count;
                    for (int c = 0; c < e.numNotes; ++c) CHECK (e.notes[c] >= 12 && e.notes[c] <= 108);
                });
                if (count == 0) std::printf ("silent doll %d in %s\n", d, sc.name.c_str());
                CHECK (count > 0);
                if (sc.project.dolls[(size_t) d].source == Source::Sampler)
                    CHECK (sc.samples[(size_t) d] >= 0);
            }
        }
        CHECK (names.size() == scores().size());
    }

    if (failures == 0)
        std::printf ("All engine tests passed.\n");
    return failures == 0 ? 0 : 1;
}
