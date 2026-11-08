#pragma once
#include "Project.h"
#include <string>
#include <vector>

namespace mf
{
struct SynthPreset
{
    std::string name;
    std::string era;   // "1960s", "1970s", "1980s"
    SynthParams synth;
    float drive = 0.0f, chorus = 0.0f, wow = 0.0f;
};

const std::vector<SynthPreset>& synthPresets();
void applyPreset (DollSettings& d, const SynthPreset& preset);

struct BuiltInSampleInfo { const char* name; int rootNote; };
const std::vector<BuiltInSampleInfo>& builtInSamples();
} // namespace mf
