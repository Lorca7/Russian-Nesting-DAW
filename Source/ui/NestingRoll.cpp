#include "NestingRoll.h"

namespace mf
{
using namespace theme;

void NestingRoll::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ink);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (6.0f, 4.0f);

    const double before = 4.0, after = 20.0;
    const double from = isPlaying ? playhead - before : -0.5;
    const double to = from + before + after;

    int lo = 127, hi = 0;
    struct N { int doll; double b, len; int note; float vel; };
    std::vector<N> notes;
    notes.reserve (512);
    for (int d = 0; d < kNumDolls; ++d)
        FractalFugue::forEachEvent (project, d, juce::jmax (0.0, from - 8.0), to, [&] (const NoteEvent& e) {
            for (int c = 0; c < e.numNotes; ++c)
            {
                if (e.beat + e.length < from) continue;
                notes.push_back ({ d, e.beat, e.length, e.notes[c], e.velocity });
                lo = juce::jmin (lo, e.notes[c]);
                hi = juce::jmax (hi, e.notes[c]);
            }
        });
    if (notes.empty()) { lo = 36; hi = 84; }
    lo -= 2; hi += 2;
    const float rowH = r.getHeight() / (float) (hi - lo + 1);
    auto xOf = [&] (double b) { return r.getX() + (float) ((b - from) / (to - from)) * r.getWidth(); };
    auto yOf = [&] (int n) { return r.getBottom() - (float) (n - lo + 1) * rowH; };

    // octave lines and beat grid
    for (int n = lo; n <= hi; ++n)
        if (n % 12 == project.root)
        {
            g.setColour (goldDim.withAlpha (0.35f));
            g.drawHorizontalLine ((int) (yOf (n) + rowH), r.getX(), r.getRight());
        }
    for (double b = std::ceil (from); b < to; b += 1.0)
    {
        const bool bar = std::fmod (b, 4.0) == 0.0;
        g.setColour (cream.withAlpha (bar ? 0.14f : 0.05f));
        g.drawVerticalLine ((int) xOf (b), r.getY(), r.getBottom());
    }

    // notes, largest doll at the back
    for (auto& n : notes)
    {
        const float x0 = juce::jmax (r.getX(), xOf (n.b)), x1 = juce::jmin (r.getRight(), xOf (n.b + n.len));
        if (x1 <= x0) continue;
        auto rect = juce::Rectangle<float> (x0, yOf (n.note), juce::jmax (2.0f, x1 - x0), juce::jmax (2.0f, rowH - 1.0f));
        const bool past = isPlaying && n.b + n.len < playhead;
        const bool sounding = isPlaying && n.b <= playhead && playhead < n.b + n.len;
        g.setColour (dollColour (n.doll).withAlpha (past ? 0.35f : 0.55f + 0.4f * n.vel));
        g.fillRoundedRectangle (rect, juce::jmin (3.0f, rowH * 0.4f));
        if (sounding)
        {
            g.setColour (gold);
            g.drawRoundedRectangle (rect.expanded (0.5f), 3.0f, 1.2f);
        }
    }

    if (isPlaying)
    {
        const float x = xOf (playhead);
        g.setColour (gold);
        g.drawLine (x, r.getY(), x, r.getBottom(), 1.5f);
    }
    else
    {
        g.setColour (creamDim);
        g.setFont (body (11.0f));
        g.drawText ("Preview of the first bars. Press Play to let the dolls sing.", getLocalBounds().reduced (12, 6), juce::Justification::topRight);
    }
}
} // namespace mf
