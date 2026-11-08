#include "Project.h"
#include "Presets.h"

namespace mf
{
Project::Project()
{
    const int deg[] = { 0, 4, 3, 2, 1, 2, -1, 0 };
    const int len[] = { 2, 1, 1, 1, 1, 1, 1, 2 };
    subjectLength = 8;
    for (int i = 0; i < kMaxSteps; ++i)
    {
        subject[(size_t) i] = Step {};
        if (i < subjectLength)
        {
            subject[(size_t) i].degree = deg[i];
            subject[(size_t) i].length = len[i];
            subject[(size_t) i].velocity = i == 0 ? 1.0f : 0.8f;
        }
    }
    for (int d = 0; d < kNumDolls; ++d)
        dolls[(size_t) d].octave = 2 + d;
}

static const SynthPreset* presetNamed (const char* name)
{
    for (auto& p : synthPresets())
        if (p.name == name)
            return &p;
    return &synthPresets().front();
}

Project makeDefaultProject()
{
    Project p;

    auto& big = p.dolls[0];
    applyPreset (big, *presetNamed ("Modular Bass"));
    big.octave = 2; big.reverbSend = 0.12f; big.volume = 0.75f;

    auto& mid = p.dolls[1];
    applyPreset (mid, *presetNamed ("String Ensemble"));
    mid.octave = 3; mid.chordSize = 2; mid.pan = -0.2f; mid.reverbSend = 0.35f; mid.volume = 0.55f;

    auto& small = p.dolls[2];
    small.source = Source::Sampler;
    small.sampler.rootNote = builtInSamples()[0].rootNote;
    small.octave = 4; small.pan = 0.3f; small.reverbSend = 0.25f; small.volume = 0.7f;
    applyPreset (small, *presetNamed ("Mono Lead"));

    auto& tiny = p.dolls[3];
    applyPreset (tiny, *presetNamed ("FM Glass Bell"));
    tiny.octave = 5; tiny.gate = Gate::Fibonacci; tiny.pan = -0.35f; tiny.echoSend = 0.3f; tiny.reverbSend = 0.4f; tiny.volume = 0.45f;

    return p;
}
} // namespace mf
