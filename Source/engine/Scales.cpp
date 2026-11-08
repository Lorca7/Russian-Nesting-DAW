#include "Scales.h"
#include <algorithm>

namespace mf
{
const std::vector<Scale>& allScales()
{
    static const std::vector<Scale> scales {
        // ---- Common
        { "Major (Ionian)",        "Common", { 0, 2, 4, 5, 7, 9, 11 }, 12, "The everyday major scale." },
        { "Natural Minor (Aeolian)","Common", { 0, 2, 3, 5, 7, 8, 10 }, 12, "Relative minor. The backbone of most Russian folk song." },
        { "Harmonic Minor",        "Common", { 0, 2, 3, 5, 7, 8, 11 }, 12, "Raised 7th: the classic fugue-subject minor." },
        { "Melodic Minor",         "Common", { 0, 2, 3, 5, 7, 9, 11 }, 12, "Jazz/ascending melodic minor." },
        { "Major Pentatonic",      "Common", { 0, 2, 4, 7, 9 }, 12, "Five notes, no semitones." },
        { "Minor Pentatonic",      "Common", { 0, 3, 5, 7, 10 }, 12, "Five-note minor." },
        { "Blues",                 "Common", { 0, 3, 5, 6, 7, 10 }, 12, "Minor pentatonic plus the flat fifth." },
        { "Chromatic",             "Common", { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, 12, "All twelve notes." },
        // ---- Modes
        { "Dorian",                "Modes", { 0, 2, 3, 5, 7, 9, 10 }, 12, "Minor with a bright 6th." },
        { "Phrygian",              "Modes", { 0, 1, 3, 5, 7, 8, 10 }, 12, "Minor with a dark flat 2nd." },
        { "Lydian",                "Modes", { 0, 2, 4, 6, 7, 9, 11 }, 12, "Major with a raised 4th." },
        { "Mixolydian",            "Modes", { 0, 2, 4, 5, 7, 9, 10 }, 12, "Major with a flat 7th." },
        { "Locrian",               "Modes", { 0, 1, 3, 5, 6, 8, 10 }, 12, "Diminished-fifth mode." },
        { "Phrygian Dominant",     "Modes", { 0, 1, 4, 5, 7, 8, 10 }, 12, "5th mode of harmonic minor (Freygish)." },
        { "Lydian Dominant",       "Modes", { 0, 2, 4, 6, 7, 9, 10 }, 12, "Acoustic / overtone scale." },
        // ---- Russian & Slavic
        { "Rimsky-Korsakov Octatonic (H-W)", "Russian & Slavic", { 0, 1, 3, 4, 6, 7, 9, 10 }, 12,
          "Half-whole octatonic. Russian theory calls it the Rimsky-Korsakov scale; Stravinsky leaned on it." },
        { "Octatonic (W-H)",       "Russian & Slavic", { 0, 2, 3, 5, 6, 8, 9, 11 }, 12, "Whole-half form of the Rimsky-Korsakov scale." },
        { "Glinka Whole-Tone (Chernomor)", "Russian & Slavic", { 0, 2, 4, 6, 8, 10 }, 12,
          "Glinka used the whole-tone scale for the sorcerer Chernomor in Ruslan and Lyudmila." },
        { "Obikhod (Znamenny Chant)", "Russian & Slavic", { 0, 2, 4 }, 5,
          "Orthodox chant scale built from tone-tone-semitone trichords. It repeats at the fourth, not the octave." },
        { "Scriabin Prometheus (Mystic)", "Russian & Slavic", { 0, 2, 4, 6, 9, 10 }, 12, "Scriabin's mystic-chord scale." },
        { "Tcherepnin Nine-Step",  "Russian & Slavic", { 0, 1, 3, 4, 5, 7, 8, 9, 11 }, 12, "Alexander Tcherepnin's symmetric nine-note scale." },
        { "Shostakovich (Phrygian b4)", "Russian & Slavic", { 0, 1, 3, 4, 7, 8, 10 }, 12, "Phrygian with a lowered 4th, a Shostakovich fingerprint." },
        { "Shostakovich (Locrian b4)", "Russian & Slavic", { 0, 1, 3, 4, 6, 8, 10 }, 12, "Locrian with a lowered 4th." },
        { "Russian Romani Minor",  "Russian & Slavic", { 0, 2, 3, 6, 7, 8, 11 }, 12, "Gypsy minor of the Russian romance and Romani choirs." },
        { "Ukrainian Dorian",      "Russian & Slavic", { 0, 2, 3, 6, 7, 9, 10 }, 12, "Dorian with a raised 4th, common in East Slavic folk." },
        { "Russian Peremenny (Variable)", "Russian & Slavic", { 0, 2, 3, 5, 7, 9, 10 }, 12,
          "Folk 'variable mode' that hovers between minor and its relative major (natural minor with a raised 6th)." },
        // ---- Symmetric & world
        { "Hungarian Minor",       "Symmetric & World", { 0, 2, 3, 6, 7, 8, 11 }, 12, "Double harmonic minor." },
        { "Double Harmonic",       "Symmetric & World", { 0, 1, 4, 5, 7, 8, 11 }, 12, "Byzantine / Arabic double harmonic." },
        { "Augmented",             "Symmetric & World", { 0, 3, 4, 7, 8, 11 }, 12, "Hexatonic augmented scale." },
        { "Hirajoshi",             "Symmetric & World", { 0, 2, 3, 7, 8 }, 12, "Japanese pentatonic." },
    };
    return scales;
}

int numScales() { return (int) allScales().size(); }

const Scale& scaleAt (int index)
{
    const auto& s = allScales();
    return s[(size_t) std::clamp (index, 0, (int) s.size() - 1)];
}

int findScale (const std::string& name)
{
    const auto& s = allScales();
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i].name == name)
            return (int) i;
    return 1;
}

int degreeToSemitone (const Scale& scale, int degree)
{
    const int n = (int) scale.steps.size();
    if (n == 0)
        return 0;
    int oct = degree / n;
    int idx = degree % n;
    if (idx < 0) { idx += n; --oct; }
    return oct * scale.period + scale.steps[(size_t) idx];
}

const char* noteName (int pc)
{
    static const char* names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names[((pc % 12) + 12) % 12];
}
} // namespace mf
