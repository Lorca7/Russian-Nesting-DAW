#include "SampleBench.h"

namespace mf
{
using namespace theme;

void SampleBench::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ink);
    g.fillRoundedRectangle (r, 6.0f);
    r = r.reduced (4.0f);

    const auto* s = proc.getSample (doll);
    if (s == nullptr || s->data.empty())
    {
        g.setColour (creamDim);
        g.drawText ("Drop an audio file here", r, juce::Justification::centred);
        return;
    }

    const int cols = juce::jmax (1, (int) r.getWidth());
    if (cachedFor != s || (int) peaks.size() != cols)
    {
        peaks.assign ((size_t) cols, 0.0f);
        const size_t per = juce::jmax<size_t> (1, s->data.size() / (size_t) cols);
        for (int c = 0; c < cols; ++c)
        {
            float m = 0;
            for (size_t i = (size_t) c * per; i < juce::jmin (s->data.size(), (size_t) (c + 1) * per); ++i)
                m = juce::jmax (m, std::abs (s->data[i]));
            peaks[(size_t) c] = m;
        }
        cachedFor = s;
    }

    const auto& sp = project.dolls[(size_t) doll].sampler;
    if (sp.mode == SampleMode::Slice)
        for (int i = 0; i < sp.slices; ++i)
        {
            g.setColour (i % 2 == 0 ? panelHi : panel);
            const float x0 = r.getX() + r.getWidth() * (float) i / (float) sp.slices;
            g.fillRect (x0, r.getY(), r.getWidth() / (float) sp.slices, r.getHeight());
        }

    const float mid = r.getCentreY();
    g.setColour (dollColour (doll).withAlpha (0.85f));
    for (int c = 0; c < cols; ++c)
    {
        const float h = peaks[(size_t) c] * r.getHeight() * 0.48f;
        g.drawVerticalLine ((int) r.getX() + c, mid - h, mid + h);
    }

    // a playhead for every doll that reads this same sample
    for (int d = 0; d < kNumDolls; ++d)
    {
        if (proc.getSample (d) != s || project.dolls[(size_t) d].source != Source::Sampler) continue;
        const auto& dp = project.dolls[(size_t) d].sampler;
        double pos = dp.start;
        if (dp.mode == SampleMode::Scrub) pos = proc.getScrubPosition (d);
        const float x = r.getX() + (float) pos * r.getWidth();
        g.setColour (dollColour (d).brighter (0.3f));
        g.drawLine (x, r.getY(), x, r.getBottom(), 2.0f);
        juce::Path tri;
        tri.addTriangle (x - 5, r.getY(), x + 5, r.getY(), x, r.getY() + 7);
        g.fillPath (tri);
    }

    g.setColour (cream);
    g.setFont (body (11.0f));
    g.drawText (proc.getSampleLabel (doll), r.reduced (4, 2), juce::Justification::bottomLeft);
}
} // namespace mf
