#include "Scores.h"
#include "FractalFugue.h"
#include "Presets.h"

namespace mf
{
namespace
{
std::vector<std::string>& problems()
{
    static std::vector<std::string> p;
    return p;
}

constexpr int R = 99; // marks a rest in a subject

/** Small builder so each score reads like a short score sheet. */
struct Sheet
{
    Score s;

    Sheet (const char* name, const char* group, const char* basedOn, const char* description)
    {
        s.name = name; s.group = group; s.basedOn = basedOn; s.description = description;
        s.project = Project();
        for (auto& d : s.project.dolls) d = DollSettings();
    }

    Sheet& key (int root, const char* scaleName, double bpm)
    {
        auto& p = s.project;
        p.root = root;
        p.bpm = bpm;
        const int i = findScale (scaleName);
        if (scaleAt (i).name != scaleName) problems().push_back (std::string ("scale: ") + scaleName);
        p.scaleIndex = i;
        return *this;
    }

    Sheet& law (int ratio, double pulseBeats, int answer, float coupling, bool stagger, double entryGap, bool fold = true)
    {
        auto& p = s.project;
        p.ratio = ratio; p.pulseBeats = pulseBeats; p.answerInterval = answer; p.coupling = coupling;
        p.staggerEntries = stagger; p.entryGapBeats = entryGap; p.fold = fold;
        return *this;
    }

    Sheet& subject (std::initializer_list<int> degrees, std::initializer_list<int> lengths)
    {
        auto& p = s.project;
        if (degrees.size() != lengths.size() || degrees.size() > (size_t) kMaxSteps)
            problems().push_back (s.name + ": subject degrees and lengths differ");
        p.subjectLength = (int) std::min (degrees.size(), (size_t) kMaxSteps);
        auto di = degrees.begin();
        auto li = lengths.begin();
        for (int i = 0; i < kMaxSteps; ++i)
        {
            auto& st = p.subject[(size_t) i];
            st = Step {};
            if (i < p.subjectLength)
            {
                st.on = *di != R;
                st.degree = *di == R ? (i > 0 ? p.subject[(size_t) i - 1].degree : 0) : *di;
                st.length = li != lengths.end() ? *li : 1;
                st.velocity = i == 0 ? 1.0f : 0.82f;
                ++di;
                if (li != lengths.end()) ++li;
            }
        }
        return *this;
    }

    Sheet& synth (int d, const char* preset, int octave)
    {
        auto& doll = s.project.dolls[(size_t) d];
        doll.source = Source::Synth;
        doll.octave = octave;
        if (const auto* pr = findPreset (preset)) applyPreset (doll, *pr);
        else problems().push_back (s.name + ": preset " + preset);
        return *this;
    }

    Sheet& sample (int d, const char* sampleName, int octave, int mode = SampleMode::Pitched)
    {
        auto& doll = s.project.dolls[(size_t) d];
        doll.source = Source::Sampler;
        doll.octave = octave;
        doll.sampler.mode = mode;
        const int i = findBuiltInSample (sampleName);
        if (i < 0) { problems().push_back (s.name + ": sample " + sampleName); return *this; }
        s.samples[(size_t) d] = i;
        doll.sampler.rootNote = builtInSamples()[(size_t) i].rootNote;
        doll.sampler.pitchFollow = ! builtInSamples()[(size_t) i].percussive;
        return *this;
    }

    Sheet& shape (int d, int transform, int gate = Gate::All, int chord = 1, float legato = 0.85f)
    {
        auto& doll = s.project.dolls[(size_t) d];
        doll.transform = transform; doll.gate = gate; doll.chordSize = chord; doll.legato = legato;
        return *this;
    }

    Sheet& mix (int d, float volume, float pan, float reverb, float echo = 0.0f)
    {
        auto& doll = s.project.dolls[(size_t) d];
        doll.volume = volume; doll.pan = pan; doll.reverbSend = reverb; doll.echoSend = echo;
        return *this;
    }

    Sheet& hall (float size, float level, double echoBeats = 0.75, float echoFeedback = 0.3f)
    {
        auto& p = s.project;
        p.reverbSize = size; p.reverbLevel = level; p.echoBeats = echoBeats; p.echoFeedback = echoFeedback;
        return *this;
    }

    operator Score() const { return s; }
};

// pitch classes
enum { C = 0, Db = 1, D = 2, Eb = 3, E = 4, F = 5, Gb = 6, G = 7, Ab = 8, A = 9, Bb = 10, B = 11 };
using T = int;
constexpr T Prime = Transform::Prime, Inv = Transform::Inversion, Retro = Transform::Retrograde, RetroInv = Transform::RetroInversion;
} // namespace

const std::vector<Score>& scores()
{
    static const std::vector<Score> list {
        // ===================================================================== Sacred & chant
        Sheet ("Znamenny Canon", "Sacred & chant",
               "Znamenny chant on the Obikhod scale",
               "Stepwise chant neumes on the Obikhod scale, which repeats at the fourth rather than the octave. "
               "The oktavists hold the ison drone while two choirs sing the chant in augmentation and diminution; "
               "small bells answer on a Cantor-set gate.")
            .key (G, "Obikhod (Znamenny Chant)", 66)
            .law (Ratio::Octave, 0.5, 0, 0.5f, true, 8)
            .subject ({ 0, 1, 2, 1, 0, -1, 0, 1, 2, 3, 2, 1, 0 }, { 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 3 })
            .sample (0, "Oktavist basso 'O'", 2).shape (0, Prime, Gate::Drone).mix (0, 0.75f, 0.0f, 0.4f)
            .sample (1, "Choir 'Ah'", 3).shape (1, Prime, Gate::All, 1, 1.0f).mix (1, 0.7f, -0.25f, 0.5f)
            .sample (2, "Choir 'Ah'", 4).shape (2, Prime, Gate::All, 1, 0.95f).mix (2, 0.6f, 0.25f, 0.5f)
            .sample (3, "Zazvonnye small bells", 5).shape (3, Prime, Gate::Cantor).mix (3, 0.3f, 0.4f, 0.6f)
            .hall (0.92f, 0.5f),

        Sheet ("Troestrochie (Three-Line Chant)", "Sacred & chant",
               "Russian three-voice strochnoe singing (niz, put', verkh)",
               "The put' carries the chant as a cantus firmus; the verkh sits a second above and the niz runs low and "
               "melismatic beneath. The clashing seconds are the point: by Western rules this polyphony is dissonant. "
               "Here the low line is also the fast one, so the fractal roles cut across the registers.")
            .key (D, "Dorian", 72)
            .law (Ratio::Octave, 0.5, 1, 0.35f, true, 4)
            .subject ({ 0, 0, 1, 2, 1, 0, 2, 3, 4, 3, 2, 1, 0 }, { 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 3 })
            .sample (0, "Choir 'Ah'", 3).shape (0, Prime, Gate::All, 1, 1.0f).mix (0, 0.75f, 0.0f, 0.45f)
            .sample (1, "Choir 'Ah'", 4).shape (1, Prime, Gate::All, 1, 0.95f).mix (1, 0.6f, 0.3f, 0.45f)
            .sample (2, "Oktavist basso 'O'", 2).shape (2, Inv).mix (2, 0.7f, -0.3f, 0.4f)
            .sample (3, "Kolokol church bell", 4).shape (3, Prime, Gate::Cantor).mix (3, 0.25f, 0.2f, 0.6f)
            .hall (0.9f, 0.45f),

        Sheet ("Vigil Oktavists", "Sacred & chant",
               "Rachmaninoff, All-Night Vigil (1915), the Nunc dimittis",
               "A slow descending scale that lands on the low B-flat the oktavists famously reach at the end of the "
               "Nunc dimittis. Every doll sings the same descent; the middle voice sings it upside down, so it rises "
               "while the others fall.")
            .key (Bb, "Russian Peremenny (Variable)", 56)
            .law (Ratio::Octave, 0.5, 0, 0.25f, true, 6)
            .subject ({ 7, 6, 5, 4, 3, 2, 1, 0 }, { 1, 1, 1, 1, 1, 1, 1, 4 })
            .sample (0, "Oktavist basso 'O'", 1).shape (0, Prime, Gate::All, 1, 1.0f).mix (0, 0.85f, 0.0f, 0.4f)
            .sample (1, "Choir 'Ah'", 3).shape (1, Prime, Gate::All, 2, 1.0f).mix (1, 0.6f, -0.3f, 0.5f)
            .sample (2, "Choir 'Ah'", 4).shape (2, Inv, Gate::All, 1, 1.0f).mix (2, 0.5f, 0.3f, 0.5f)
            .sample (3, "Zazvonnye small bells", 5).shape (3, Prime, Gate::Fibonacci).mix (3, 0.25f, 0.0f, 0.7f)
            .hall (0.95f, 0.55f),

        // ===================================================================== Bells (zvon)
        Sheet ("Perezvon Bell Tower", "Bells (zvon)",
               "Russian Orthodox bell ringing: blagovest, perezvon, trezvon",
               "A bell tower is already a fractal: one great bell strikes slowly (blagovest), the middle bells ring in "
               "turn from largest to smallest (perezvon), and the small bells chatter fastest, ending in three strokes "
               "of everything together (trezvon). Bells don't bend pitch, so coupling is off.")
            .key (D, "Major Pentatonic", 96)
            .law (Ratio::Octave, 0.25, 0, 0.0f, true, 4)
            .subject ({ 0, 1, 2, 3, 4, R, 0, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1, 2 })
            .sample (0, "Blagovest great bell", 2).shape (0, Prime, Gate::Drone).mix (0, 0.85f, 0.0f, 0.35f)
            .sample (1, "Kolokol church bell", 3).shape (1, Prime).mix (1, 0.6f, -0.35f, 0.4f)
            .sample (2, "Zazvonnye small bells", 4).shape (2, Prime).mix (2, 0.45f, 0.35f, 0.45f)
            .sample (3, "Zazvonnye small bells", 5).shape (3, Prime, Gate::Sierpinski, 3).mix (3, 0.35f, 0.0f, 0.5f)
            .hall (0.85f, 0.45f),

        Sheet ("Perebor Funeral Toll", "Bells (zvon)",
               "The perebor: each bell once in turn, then all together",
               "Every bell sounds once, one after another, then all strike together. Sources disagree on whether the "
               "order runs up or down, so one doll plays it backwards. The 3:2 ratio lets the tolls drift slowly out of "
               "phase and back.")
            .key (A, "Natural Minor (Aeolian)", 48)
            .law (Ratio::Fifth, 0.5, 0, 0.0f, true, 6)
            .subject ({ 7, 6, 5, 4, 3, 2, 1, 0, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1, 4 })
            .sample (0, "Blagovest great bell", 2).shape (0, Prime).mix (0, 0.8f, 0.0f, 0.4f)
            .sample (1, "Kolokol church bell", 3).shape (1, Retro).mix (1, 0.55f, -0.4f, 0.45f)
            .sample (2, "Kolokol church bell", 4).shape (2, Prime).mix (2, 0.45f, 0.4f, 0.45f)
            .sample (3, "Zazvonnye small bells", 5).shape (3, Prime, Gate::Drone, 4).mix (3, 0.35f, 0.0f, 0.55f)
            .hall (0.9f, 0.5f),

        // ===================================================================== Folk
        Sheet ("Kamarinskaya Backgrounds", "Folk",
               "Glinka, Kamarinskaya (1848): 'changing backgrounds'",
               "Glinka kept the folk dance tune intact and varied only what surrounds it. Here the dance cell never "
               "changes (coupling is off); each doll is a different background: bass balalaika in long notes, garmon "
               "chords on a Sierpinski gate, the balalaika upside down, the domra backwards on a Fibonacci gate.")
            .key (G, "Major (Ionian)", 132)
            .law (Ratio::Octave, 0.25, 0, 0.0f, true, 2)
            .subject ({ 4, 4, 3, 2, 3, 1, 2, 0 }, { 1, 1, 1, 1, 1, 1, 1, 1 })
            .sample (0, "Bass balalaika", 2).shape (0, Prime).mix (0, 0.85f, 0.0f, 0.15f)
            .sample (1, "Garmon (Tula accordion)", 3).shape (1, Prime, Gate::Sierpinski, 2).mix (1, 0.5f, -0.3f, 0.2f)
            .sample (2, "Balalaika pluck", 4).shape (2, Inv).mix (2, 0.6f, 0.3f, 0.2f)
            .sample (3, "Domra tremolo", 5).shape (3, Retro, Gate::Fibonacci).mix (3, 0.45f, 0.1f, 0.25f)
            .hall (0.5f, 0.2f),

        Sheet ("Protyazhnaya with Podgoloski", "Folk",
               "The drawn-out lyrical song and its heterophonic sub-voices",
               "One long-breathed melody sung by everyone at once, each singer bending it a little: Russian "
               "heterophony (podgolosochnaya polyphony). Low coupling keeps the variants close to the tune, and the 3:2 "
               "ratio lets them drift in and out of unison over a bagpipe drone.")
            .key (E, "Natural Minor (Aeolian)", 54)
            .law (Ratio::Fifth, 0.5, 0, 0.35f, true, 3)
            .subject ({ 0, 2, 4, 5, 4, 3, 2, 3, 1, 0 }, { 3, 1, 2, 1, 1, 1, 2, 1, 1, 4 })
            .sample (0, "Volynka (bagpipe drone)", 2).shape (0, Prime, Gate::Drone).mix (0, 0.5f, 0.0f, 0.3f)
            .sample (1, "Choir 'Ah'", 3).shape (1, Prime, Gate::All, 1, 1.05f).mix (1, 0.7f, -0.2f, 0.45f)
            .sample (2, "Zhaleika (hornpipe)", 4).shape (2, Prime, Gate::All, 1, 0.95f).mix (2, 0.45f, 0.3f, 0.4f)
            .sample (3, "Svirel (shepherd flute)", 5).shape (3, Prime, Gate::All, 1, 1.1f).mix (3, 0.35f, -0.4f, 0.5f)
            .hall (0.8f, 0.4f),

        Sheet ("Burlak Work Song", "Folk",
               "Volga barge-haulers' call-and-response work songs",
               "A heavy pentatonic heave sung low, answered a fourth higher, with the jaw harp and frame drum marking "
               "the pull on Cantor and Sierpinski gates.")
            .key (A, "Minor Pentatonic", 60)
            .law (Ratio::Octave, 0.5, 2, 0.5f, true, 4)
            .subject ({ 0, 0, 2, 1, 0, -1, 0 }, { 2, 1, 1, 2, 1, 1, 4 })
            .sample (0, "Oktavist basso 'O'", 2).shape (0, Prime, Gate::All, 1, 1.0f).mix (0, 0.85f, 0.0f, 0.3f)
            .sample (1, "Bass balalaika", 3).shape (1, Prime).mix (1, 0.6f, -0.3f, 0.2f)
            .sample (2, "Vargan (jaw harp)", 3).shape (2, Prime, Gate::Cantor).mix (2, 0.5f, 0.3f, 0.2f)
            .sample (3, "Buben (frame drum)", 4).shape (3, Prime, Gate::Sierpinski).mix (3, 0.55f, 0.0f, 0.15f)
            .hall (0.6f, 0.3f),

        // ===================================================================== Classical
        Sheet ("White-Key Fugue", "Classical",
               "After Shostakovich, Prelude and Fugue Op. 87 No. 1",
               "Shostakovich's C major fugue uses only the white keys and avoids vertical dissonance. A plain diatonic "
               "subject enters doll by doll, answered at the fifth, as a strict canon by augmentation: each doll plays "
               "exactly half as fast as the one inside it.")
            .key (C, "Major (Ionian)", 80)
            .law (Ratio::Octave, 0.5, 4, 0.0f, true, 8)
            .subject ({ 0, 2, 1, 3, 2, 4, 3, 1, 0 }, { 2, 1, 1, 1, 1, 1, 1, 1, 3 })
            .synth (0, "Estrada FM Piano", 2).shape (0, Prime).mix (0, 0.75f, 0.0f, 0.25f)
            .synth (1, "Estrada FM Piano", 3).shape (1, Prime).mix (1, 0.65f, -0.3f, 0.25f)
            .synth (2, "Estrada FM Piano", 4).shape (2, Prime).mix (2, 0.6f, 0.3f, 0.25f)
            .synth (3, "Estrada FM Piano", 5).shape (3, Prime).mix (3, 0.5f, 0.0f, 0.3f)
            .hall (0.6f, 0.25f),

        Sheet ("DSCH Monogram", "Classical",
               "Shostakovich's D-Es-C-H signature, with Taneyev-style invertible counterpoint",
               "The four-note DSCH motif (D, E-flat, C, B) in all four fugal guises at once: prime, inversion, "
               "retrograde and retrograde inversion, one per doll, each at its own fractal speed.")
            .key (C, "Harmonic Minor", 66)
            .law (Ratio::Octave, 0.5, 4, 0.0f, true, 4)
            .subject ({ 1, 2, 0, -1 }, { 2, 2, 2, 2 })
            .synth (0, "Gosteleradio Tape Strings", 2).shape (0, Prime, Gate::All, 2, 1.0f).mix (0, 0.7f, 0.0f, 0.35f)
            .synth (1, "VIA String Ensemble", 3).shape (1, Inv).mix (1, 0.6f, -0.35f, 0.35f)
            .synth (2, "VIA String Ensemble", 4).shape (2, Retro).mix (2, 0.55f, 0.35f, 0.35f)
            .synth (3, "Estrada FM Bells", 5).shape (3, RetroInv).mix (3, 0.4f, 0.0f, 0.45f, 0.2f)
            .hall (0.75f, 0.35f),

        Sheet ("Gubaidulina Fibonacci", "Classical",
               "Sofia Gubaidulina's Fibonacci-proportioned forms (from Perception, 1983)",
               "Note lengths follow the Fibonacci series 1, 1, 2, 3, 5, 8; the dolls relate by the golden ratio and the "
               "bells are gated by the Fibonacci word. The bayan, an instrument Gubaidulina wrote for often, carries a "
               "chromatic wedge that widens from a single note.")
            .key (E, "Chromatic", 76)
            .law (Ratio::Golden, 0.25, 0, 0.6f, true, 5)
            .subject ({ 0, 1, -1, 2, -2, 3 }, { 1, 1, 2, 3, 5, 8 })
            .synth (0, "Synthi Stalker Drone", 2).shape (0, Prime, Gate::Drone).mix (0, 0.6f, 0.0f, 0.4f)
            .sample (1, "Bayan accordion", 3).shape (1, Prime, Gate::All, 1, 1.0f).mix (1, 0.65f, -0.3f, 0.35f)
            .synth (2, "ANS Photo-Cluster", 4).shape (2, Inv).mix (2, 0.45f, 0.3f, 0.45f)
            .sample (3, "Zazvonnye small bells", 5).shape (3, Prime, Gate::Fibonacci).mix (3, 0.3f, 0.0f, 0.6f)
            .hall (0.85f, 0.4f),

        Sheet ("Octatonic Ritual", "Classical",
               "Stravinsky's Russian period: octatonic folk cells and additive rhythm",
               "A tiny folk-like cell on the Rimsky-Korsakov octatonic scale, in additive lengths of 2 and 3, repeated "
               "without development. Each doll layers the same block at a 3:2 speed while spoons and frame drum gate it "
               "into ritual patterns.")
            .key (D, "Rimsky-Korsakov Octatonic (H-W)", 112)
            .law (Ratio::Fifth, 0.25, 2, 0.5f, true, 3)
            .subject ({ 0, 1, 0, -1, 0 }, { 2, 3, 2, 2, 3 })
            .synth (0, "Polivoks Bass", 2).shape (0, Prime).mix (0, 0.75f, 0.0f, 0.15f)
            .sample (1, "Zhaleika (hornpipe)", 4).shape (1, Inv).mix (1, 0.5f, -0.3f, 0.3f)
            .sample (2, "Lozhki (spoons)", 4).shape (2, Prime, Gate::Cantor).mix (2, 0.6f, 0.4f, 0.15f)
            .sample (3, "Buben (frame drum)", 4).shape (3, Prime, Gate::Fibonacci).mix (3, 0.55f, -0.1f, 0.15f)
            .hall (0.45f, 0.25f),

        Sheet ("Scriabin Mystic Bells", "Classical",
               "Scriabin's mystic chord, and the ANS synthesizer named after him",
               "The Prometheus scale arpeggiated in fourths. With full coupling every doll rides on its parent's note, "
               "so the mystic chord unfolds inside itself; the ANS cluster holds the root.")
            .key (C, "Scriabin Prometheus (Mystic)", 70)
            .law (Ratio::Golden, 0.25, 2, 1.0f, true, 5)
            .subject ({ 0, 3, 1, 4, 2, 5 }, { 1, 1, 1, 1, 1, 3 })
            .synth (0, "ANS Photo-Cluster", 2).shape (0, Prime, Gate::Drone).mix (0, 0.6f, 0.0f, 0.45f)
            .synth (1, "Termenvox Theremin", 4).shape (1, Prime, Gate::All, 1, 1.0f).mix (1, 0.5f, -0.3f, 0.4f)
            .synth (2, "Estrada FM Bells", 4).shape (2, Inv).mix (2, 0.45f, 0.3f, 0.5f)
            .sample (3, "Zazvonnye small bells", 5).shape (3, Prime).mix (3, 0.3f, 0.0f, 0.6f, 0.2f)
            .hall (0.85f, 0.45f),

        // ===================================================================== Minimal & electronic
        Sheet ("Sacred Minimalism", "Minimal & electronic",
               "Moscow minimalism (Martynov, Batagov): repetition meets Orthodox chant",
               "A six-note diatonic cell repeated like a chant formula. Full coupling turns the repetition into a "
               "fractal: each doll replays the cell inside every note of the one above it. Two gusli read the same "
               "sample, one plucked and one with a scrubbing fugue playhead.")
            .key (D, "Dorian", 96)
            .law (Ratio::Octave, 0.25, 0, 1.0f, true, 4)
            .subject ({ 0, 2, 4, 2, 1, 3 }, { 1, 1, 1, 1, 1, 1 })
            .synth (0, "Solaris Organ Halo", 2).shape (0, Prime, Gate::Drone).mix (0, 0.5f, 0.0f, 0.4f)
            .sample (1, "Gusli (zither)", 3).shape (1, Prime).mix (1, 0.7f, -0.3f, 0.35f)
            .sample (2, "Gusli (zither)", 4, SampleMode::Scrub).shape (2, Inv).mix (2, 0.5f, 0.3f, 0.4f)
            .sample (3, "Svirel (shepherd flute)", 5).shape (3, Prime, Gate::Fibonacci).mix (3, 0.35f, 0.0f, 0.5f, 0.2f)
            .hall (0.8f, 0.4f),

        Sheet ("Solaris Photo-Drone", "Minimal & electronic",
               "Artemiev's ANS and Synthi 100 scores for Tarkovsky",
               "Slow clusters on Tcherepnin's nine-step scale, the dolls related by a 4:3 ratio so they never line up "
               "the same way twice. A theremin floats over the top.")
            .key (C, "Tcherepnin Nine-Step", 50)
            .law (Ratio::Fourth, 0.5, 0, 0.5f, true, 6)
            .subject ({ 0, 1, 3, 2 }, { 5, 3, 2, 8 })
            .synth (0, "Synthi Stalker Drone", 2).shape (0, Prime, Gate::Drone).mix (0, 0.6f, 0.0f, 0.45f)
            .synth (1, "ANS Photo-Cluster", 3).shape (1, Prime, Gate::All, 1, 1.0f).mix (1, 0.55f, -0.3f, 0.5f)
            .synth (2, "Solaris Organ Halo", 4).shape (2, Inv, Gate::All, 1, 1.0f).mix (2, 0.45f, 0.3f, 0.5f)
            .synth (3, "Termenvox Theremin", 5).shape (3, Retro, Gate::All, 1, 1.2f).mix (3, 0.35f, 0.0f, 0.5f, 0.25f)
            .hall (0.95f, 0.55f, 1.5, 0.4f),

        Sheet ("Kosmos Disco 1980", "Minimal & electronic",
               "Soviet space disco at the turn of the 80s",
               "Octave-jumping disco bass on a Polivoks, string-machine chords on top, FM bells as the smallest doll. "
               "The bass line is the subject, so the arpeggios above are the same line sped up and riding on it.")
            .key (A, "Natural Minor (Aeolian)", 120)
            .law (Ratio::Octave, 0.25, 0, 0.5f, true, 4)
            .subject ({ 0, 0, 7, 0, 3, 0, 5, 4 }, { 1, 1, 1, 1, 1, 1, 1, 1 })
            .synth (0, "Perestroika Synth-Pop Pad", 3).shape (0, Prime, Gate::All, 3, 1.0f).mix (0, 0.45f, 0.0f, 0.35f)
            .synth (1, "Polivoks Bass", 2).shape (1, Prime).mix (1, 0.75f, 0.0f, 0.1f)
            .synth (2, "Aelita Brass", 4).shape (2, Prime, Gate::Sierpinski).mix (2, 0.45f, -0.3f, 0.25f)
            .synth (3, "Estrada FM Bells", 5).shape (3, Prime).mix (3, 0.35f, 0.3f, 0.3f, 0.35f)
            .hall (0.6f, 0.3f, 0.75, 0.35f),

        Sheet ("Leningrad New Wave 1986", "Minimal & electronic",
               "Mid-80s Soviet rock-club new wave",
               "A chorused pulse bass and a cheap toy keyboard play the same minor riff at four scales. The keyboard "
               "drops notes on a Fibonacci gate and the lead glides between them.")
            .key (E, "Natural Minor (Aeolian)", 138)
            .law (Ratio::Octave, 0.25, 4, 0.7f, true, 4)
            .subject ({ 0, 3, 2, 0, -2, 0, 3, 4 }, { 1, 1, 1, 1, 1, 1, 1, 1 })
            .synth (0, "Perestroika Synth-Pop Pad", 3).shape (0, Prime, Gate::All, 2, 1.0f).mix (0, 0.4f, 0.0f, 0.35f)
            .synth (1, "Leningrad Chorus Bass", 2).shape (1, Prime).mix (1, 0.75f, 0.0f, 0.1f)
            .synth (2, "Elektronika Toy Keys", 4).shape (2, Prime, Gate::Fibonacci).mix (2, 0.45f, -0.3f, 0.2f, 0.2f)
            .synth (3, "Kosmos Mono Lead", 5).shape (3, Inv).mix (3, 0.35f, 0.3f, 0.25f, 0.3f)
            .hall (0.55f, 0.3f, 0.75, 0.3f),
    };
    return list;
}

const std::vector<std::string>& scoreProblems()
{
    (void) scores();
    return problems();
}
} // namespace mf
