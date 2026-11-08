#pragma once
#include "Theme.h"

namespace mf
{
class MatryoshkaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MatryoshkaLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return theme::body (13.0f); }
    juce::Font getPopupMenuFont() override { return theme::body (14.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return theme::body (juce::jmin (14.0f, h * 0.55f), true); }
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>&, const juce::String&) override;
};
} // namespace mf
