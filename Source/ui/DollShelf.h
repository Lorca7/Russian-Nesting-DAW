#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"

namespace mf
{
/** Four painted matryoshka dolls on a shelf, largest to smallest. They sing when they play. */
class DollShelf : public juce::Component
{
public:
    explicit DollShelf (MatryoshkaProcessor& p) : proc (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void tick (const Project& project, double timeSeconds);

    int selected = 0;
    std::function<void (int)> onSelect;

    static void drawDoll (juce::Graphics& g, juce::Rectangle<float> r, int index, float pulse, bool blink, float wobble);

private:
    juce::Rectangle<float> dollBounds (int d) const;
    MatryoshkaProcessor& proc;
    Project cached;
    double now = 0;
    std::array<double, kNumDolls> nextBlink { 1.0, 2.3, 3.1, 4.4 };
    std::array<double, kNumDolls> blinkUntil {};
    juce::Random rng;
};
} // namespace mf
