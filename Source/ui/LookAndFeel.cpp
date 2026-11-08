#include "LookAndFeel.h"

namespace mf
{
using namespace theme;

MatryoshkaLookAndFeel::MatryoshkaLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, lacquer);
    setColour (juce::ComboBox::backgroundColourId, ink);
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, goldDim);
    setColour (juce::ComboBox::arrowColourId, gold);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, cinnabar);
    setColour (juce::PopupMenu::highlightedTextColourId, cream);
    setColour (juce::PopupMenu::headerTextColourId, gold);
    setColour (juce::TextButton::buttonColourId, ink);
    setColour (juce::TextButton::buttonOnColourId, cinnabar);
    setColour (juce::TextButton::textColourOffId, cream);
    setColour (juce::TextButton::textColourOnId, cream);
    setColour (juce::Label::textColourId, cream);
    setColour (juce::Slider::textBoxTextColourId, cream);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, gold);
    setColour (juce::Slider::thumbColourId, gold);
    setColour (juce::Slider::trackColourId, cinnabar);
    setColour (juce::Slider::backgroundColourId, ink);
    setColour (juce::TooltipWindow::backgroundColourId, panelHi);
    setColour (juce::TooltipWindow::textColourId, cream);
    setColour (juce::TooltipWindow::outlineColourId, gold);
    setColour (juce::BubbleComponent::backgroundColourId, panelHi);
    setColour (juce::BubbleComponent::outlineColourId, gold);
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, cream);
    setColour (juce::AlertWindow::outlineColourId, gold);
    setColour (juce::ProgressBar::foregroundColourId, cinnabar);
    setColour (juce::ProgressBar::backgroundColourId, ink);
    setColour (juce::TextEditor::backgroundColourId, ink);
    setColour (juce::TextEditor::textColourId, cream);
    setColour (juce::TextEditor::outlineColourId, goldDim);
    setColour (juce::ScrollBar::thumbColourId, goldDim);
}

void MatryoshkaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                              float start, float end, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

    // value arc
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0, start, end, true);
    value.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0, start, angle, true);
    g.setColour (ink.brighter (0.15f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // lacquered wooden knob with a gold rim of beads
    const float kr = r - 6.0f;
    juce::ColourGradient grad (panelHi.brighter (0.25f), c.x - kr * 0.5f, c.y - kr * 0.6f, lacquer, c.x + kr * 0.5f, c.y + kr, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (gold.withAlpha (s.isEnabled() ? 0.9f : 0.3f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.2f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * i / 8.0f;
        const float br = kr - 3.0f;
        g.fillEllipse (c.x + br * std::sin (a) - 0.9f, c.y - br * std::cos (a) - 0.9f, 1.8f, 1.8f);
    }

    // pointer
    juce::Path ptr;
    ptr.addRoundedRectangle (-1.4f, -kr + 3.0f, 2.8f, kr * 0.62f, 1.4f);
    g.setColour (cream);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void MatryoshkaLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float,
                                              float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s);
        return;
    }
    const float cy = (float) y + (float) h * 0.5f;
    auto track = juce::Rectangle<float> ((float) x, cy - 3.0f, (float) w, 6.0f);
    g.setColour (ink.brighter (0.1f));
    g.fillRoundedRectangle (track, 3.0f);
    g.setColour (s.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (track.withRight (pos), 3.0f);
    g.setColour (gold);
    g.fillEllipse (pos - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    g.setColour (ink);
    g.fillEllipse (pos - 2.5f, cy - 2.5f, 5.0f, 5.0f);
}

void MatryoshkaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const float rad = juce::jmin (r.getHeight() * 0.5f, 12.0f);
    auto fill = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : b.findColour (juce::TextButton::buttonColourId);
    if (over) fill = fill.brighter (0.12f);
    if (down) fill = fill.darker (0.2f);
    if (! b.isEnabled()) fill = fill.withAlpha (0.4f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, rad);
    g.setColour (b.getToggleState() ? gold : goldDim.brighter (over ? 0.4f : 0.0f));
    g.drawRoundedRectangle (r, rad, 1.0f);
}

void MatryoshkaLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 2), juce::Justification::centred, 1);
}

void MatryoshkaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto box = r.removeFromLeft (r.getHeight()).reduced (4.0f);
    g.setColour (ink);
    g.fillEllipse (box);
    g.setColour (over ? gold : goldDim);
    g.drawEllipse (box, 1.0f);
    if (b.getToggleState())
    {
        // a little five-petal flower
        const auto c = box.getCentre();
        const float pr = box.getWidth() * 0.18f;
        g.setColour (cinnabar);
        for (int i = 0; i < 5; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * i / 5.0f;
            g.fillEllipse (c.x + std::sin (a) * pr * 1.3f - pr, c.y - std::cos (a) * pr * 1.3f - pr, pr * 2, pr * 2);
        }
        g.setColour (gold);
        g.fillEllipse (c.x - pr * 0.8f, c.y - pr * 0.8f, pr * 1.6f, pr * 1.6f);
    }
    g.setColour (cream.withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.setFont (body (13.0f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedLeft (4), juce::Justification::centredLeft, 1);
}

void MatryoshkaLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (ink);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.hasKeyboardFocus (true) ? gold : goldDim);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    juce::Path tri;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    tri.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (gold);
    g.fillPath (tri);
}

juce::Label* MatryoshkaLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (body (12.0f));
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

void MatryoshkaLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setFont (display (13.0f));
    g.setColour (gold);
    g.drawFittedText (name, area.reduced (10, 0), juce::Justification::bottomLeft, 1);
}
} // namespace mf
