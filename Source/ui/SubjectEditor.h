#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"

namespace mf
{
/** Edits the fugue subject: columns are steps (width = length), rows are scale degrees.
    Click to set a note, click a note to turn it into a rest, drag to paint. Bottom lane: click to
    cycle the length, drag up/down for velocity. Coloured beads show where each doll is in the subject. */
class SubjectEditor : public juce::Component
{
public:
    explicit SubjectEditor (MatryoshkaProcessor& p) : proc (p) {}
    void update (const Project& p) { project = p; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void()> onEdited;

    static constexpr int kTopDegree = 9, kBottomDegree = -6;

private:
    juce::Rectangle<float> gridArea() const;
    juce::Rectangle<float> laneArea() const;
    int stepAt (float x) const;
    int degreeAt (float y) const;
    float stepX (int step) const;

    MatryoshkaProcessor& proc;
    Project project;
    int dragStep = -1;
    bool dragOnLane = false, paintMode = false;
    float dragStartVel = 0, dragStartY = 0;
};
} // namespace mf
