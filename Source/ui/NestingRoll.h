#pragma once
#include "Theme.h"
#include "../engine/FractalFugue.h"

namespace mf
{
/** A scrolling piano roll of all four dolls, past and (because the fugue is deterministic) future. */
class NestingRoll : public juce::Component
{
public:
    void update (const Project& p, double beat, bool playing) { project = p; playhead = beat; isPlaying = playing; repaint(); }
    void paint (juce::Graphics&) override;

private:
    Project project;
    double playhead = 0;
    bool isPlaying = false;
};
} // namespace mf
