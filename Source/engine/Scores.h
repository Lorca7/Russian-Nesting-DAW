#pragma once
#include "Project.h"
#include <array>
#include <string>
#include <vector>

namespace mf
{
/** A complete fugue setup drawn from Russian chant, bell ringing, folk practice or a Russian / Soviet
    composer's technique: key, scale, fractal law, subject, and a voice for every doll. */
struct Score
{
    std::string name;
    std::string group;
    std::string basedOn;      // the musical source in a phrase
    std::string description;  // what you hear and where the fractal logic lies
    Project project;
    std::array<int, kNumDolls> samples { -1, -1, -1, -1 }; // built-in sample per doll (-1 = none)
};

const std::vector<Score>& scores();

/** Names of presets, samples or scales that a score refers to but that don't exist (for tests). */
const std::vector<std::string>& scoreProblems();
} // namespace mf
