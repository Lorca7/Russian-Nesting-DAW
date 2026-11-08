#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace mf::theme
{
// Lacquer box palette: black lacquer ground, gold leaf, cinnabar, plus one colour per doll.
const juce::Colour lacquer   { 0xff15100e };
const juce::Colour panel     { 0xff221713 };
const juce::Colour panelHi   { 0xff2e201a };
const juce::Colour gold      { 0xffe2ad3b };
const juce::Colour goldDim   { 0xff7d6128 };
const juce::Colour cream     { 0xfff4e4c1 };
const juce::Colour creamDim  { 0xffb9a888 };
const juce::Colour cinnabar  { 0xffc81f2d };
const juce::Colour rose      { 0xffe8768a };
const juce::Colour ink       { 0xff241510 };
const juce::Colour leaf      { 0xff2f8f4e };

inline juce::Colour dollColour (int d)
{
    static const juce::Colour c[] = { juce::Colour (0xffc81f2d), juce::Colour (0xff2456b3), juce::Colour (0xff1f8a55), juce::Colour (0xfff0a01a) };
    return c[d & 3];
}
inline juce::Colour scarfColour (int d)
{
    static const juce::Colour c[] = { juce::Colour (0xfff0b324), juce::Colour (0xffeae2cf), juce::Colour (0xffe8768a), juce::Colour (0xffc81f2d) };
    return c[d & 3];
}

inline juce::Font display (float size) { return juce::Font (juce::FontOptions ("Georgia", size, juce::Font::bold)); }
inline juce::Font body (float size, bool bold = false) { return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)); }
inline juce::Font mono (float size) { return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain)); }

/** A tiny gold curl ornament (Khokhloma-style) used in panel headers. */
inline void drawCurl (juce::Graphics& g, juce::Point<float> c, float s, juce::Colour col)
{
    juce::Path p;
    p.startNewSubPath (c.x - s, c.y);
    p.cubicTo (c.x - s * 0.5f, c.y - s, c.x + s * 0.5f, c.y - s, c.x + s * 0.2f, c.y - s * 0.2f);
    p.cubicTo (c.x, c.y + s * 0.2f, c.x - s * 0.3f, c.y - s * 0.1f, c.x - s * 0.1f, c.y - s * 0.4f);
    g.setColour (col);
    g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.fillEllipse (c.x + s * 0.55f, c.y - s * 0.15f, s * 0.3f, s * 0.3f);
}

/** Panel background with a gold hairline and a title. Returns the content area. */
inline juce::Rectangle<int> drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title,
                                       const juce::String& russian, juce::Colour accent = gold)
{
    auto rf = r.toFloat().reduced (0.5f);
    g.setColour (panel);
    g.fillRoundedRectangle (rf, 10.0f);
    g.setColour (accent.withAlpha (0.55f));
    g.drawRoundedRectangle (rf, 10.0f, 1.0f);
    auto head = r.removeFromTop (30).reduced (12, 0);
    drawCurl (g, { (float) head.getX() + 7.0f, (float) head.getCentreY() + 3.0f }, 6.0f, accent);
    head.removeFromLeft (20);
    g.setColour (cream);
    g.setFont (display (15.0f));
    g.drawText (title, head, juce::Justification::centredLeft);
    const int tw = juce::GlyphArrangement::getStringWidthInt (display (15.0f), title);
    g.setColour (accent.withAlpha (0.8f));
    g.setFont (body (12.0f));
    g.drawText (russian, head.withTrimmedLeft (tw + 10), juce::Justification::centredLeft);
    return r.reduced (10, 4);
}
} // namespace mf::theme
