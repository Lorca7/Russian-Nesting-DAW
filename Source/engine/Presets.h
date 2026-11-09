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
    std::string note;  // what it is modelled on
    SynthParams synth;
    float drive = 0.0f, chorus = 0.0f, wow = 0.0f;
    bool ensemble = false;
    float crush = 0.0f, age = 0.0f;
};

const std::vector<SynthPreset>& synthPresets();
const SynthPreset* findPreset (const std::string& name);
void applyPreset (DollSettings& d, const SynthPreset& preset);

struct BuiltInSampleInfo
{
    const char* name;
    const char* group;
    int rootNote;
    bool percussive;
};
const std::vector<BuiltInSampleInfo>& builtInSamples();
int findBuiltInSample (const std::string& name);
} // namespace mf
