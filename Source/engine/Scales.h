#pragma once
#include <string>
#include <vector>

namespace mf
{
/** A scale is a list of semitone offsets inside one period (normally an octave of 12).
    Some Russian chant scales (Obikhod) repeat at the fourth instead, so the period is explicit. */
struct Scale
{
    std::string name;
    std::string group;
    std::vector<int> steps;
    int period = 12;
    std::string note;
};

const std::vector<Scale>& allScales();
int numScales();
const Scale& scaleAt (int index);
int findScale (const std::string& name);

/** Maps a (possibly negative) scale degree to a semitone offset from the tonic. */
int degreeToSemitone (const Scale& scale, int degree);

const char* noteName (int pitchClass);
} // namespace mf
