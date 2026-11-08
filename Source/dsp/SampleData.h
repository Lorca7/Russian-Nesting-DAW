#pragma once
#include <string>
#include <vector>

namespace mf
{
/** A mono sample held in memory. Instances are owned by the processor's sample library and never
    freed while audio might be reading them. */
struct SampleData
{
    std::vector<float> data;
    double sampleRate = 48000.0;
    std::string name;
    int rootNote = 60;
    double durationSeconds() const { return data.empty() ? 0.0 : (double) data.size() / sampleRate; }
};

/** Synthesises the built-in samples (balalaika, church bell, choir, bayan). */
SampleData makeBuiltInSample (int index, double sampleRate = 48000.0);
} // namespace mf
