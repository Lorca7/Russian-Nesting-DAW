#include "DollShelf.h"

namespace mf
{
using namespace theme;

static const float kDollScale[] = { 1.0f, 0.82f, 0.67f, 0.54f };

juce::Rectangle<float> DollShelf::dollBounds (int d) const
{
    auto area = getLocalBounds().toFloat().reduced (12.0f, 6.0f);
    area.removeFromBottom (58.0f); // captions
    const float maxH = area.getHeight();
    const float slotW = area.getWidth() / kNumDolls;
    const float h = maxH * kDollScale[d];
    const float w = juce::jmin (slotW * 0.9f, h * 0.64f);
    const float cx = area.getX() + slotW * ((float) d + 0.5f);
    return { cx - w * 0.5f, area.getBottom() - h, w, h };
}

void DollShelf::tick (const Project& project, double timeSeconds)
{
    cached = project;
    now = timeSeconds;
    for (int d = 0; d < kNumDolls; ++d)
        if (now > nextBlink[(size_t) d])
        {
            blinkUntil[(size_t) d] = now + 0.13;
            nextBlink[(size_t) d] = now + 2.0 + rng.nextDouble() * 4.0;
        }
    repaint();
}

void DollShelf::mouseDown (const juce::MouseEvent& e)
{
    const float slotW = (float) getWidth() / kNumDolls;
    const int d = juce::jlimit (0, kNumDolls - 1, (int) (e.position.x / slotW));
    selected = d;
    if (onSelect) onSelect (d);
    repaint();
}

void DollShelf::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // wooden shelf plank
    const float shelfY = bounds.getBottom() - 58.0f - 6.0f;
    juce::ColourGradient wood (juce::Colour (0xff6b3f22), 0, shelfY, juce::Colour (0xff3a2112), 0, shelfY + 14, false);
    g.setGradientFill (wood);
    g.fillRoundedRectangle (bounds.getX() + 6, shelfY, bounds.getWidth() - 12, 12.0f, 4.0f);
    g.setColour (gold.withAlpha (0.35f));
    g.drawHorizontalLine ((int) shelfY, bounds.getX() + 10, bounds.getRight() - 10);

    for (int d = 0; d < kNumDolls; ++d)
    {
        const auto r = dollBounds (d);
        const float pulse = proc.getDollPulse (d);
        const auto& ds = cached.dolls[(size_t) d];
        const bool silent = ds.mute || (std::any_of (cached.dolls.begin(), cached.dolls.end(), [] (auto& x) { return x.solo; }) && ! ds.solo);

        if (d == selected)
        {
            juce::ColourGradient halo (dollColour (d).withAlpha (0.45f), r.getCentreX(), r.getCentreY(),
                                       dollColour (d).withAlpha (0.0f), r.getCentreX() + r.getWidth() * 0.9f, r.getCentreY(), true);
            g.setGradientFill (halo);
            g.fillEllipse (r.expanded (r.getWidth() * 0.45f, 8.0f));
        }

        const float wobble = (float) std::sin (now * 11.0 + d) * pulse * 0.06f;
        juce::Graphics::ScopedSaveState ss (g);
        if (silent) g.setOpacity (0.4f);
        drawDoll (g, r, d, pulse, now < blinkUntil[(size_t) d], wobble);
    }

    // captions
    const float slotW = bounds.getWidth() / kNumDolls;
    for (int d = 0; d < kNumDolls; ++d)
    {
        auto cap = juce::Rectangle<float> (slotW * d, bounds.getBottom() - 56.0f, slotW, 56.0f).reduced (4, 0);
        const auto& ds = cached.dolls[(size_t) d];
        g.setColour (d == selected ? gold : cream);
        g.setFont (display (16.0f));
        g.drawText (juce::String (juce::CharPointer_UTF8 (dollRussianName (d))), cap.removeFromTop (19), juce::Justification::centred);
        g.setColour (dollColour (d).brighter (0.35f));
        g.setFont (body (11.0f, true));
        g.drawText (juce::String (dollRole (d)).toUpperCase(), cap.removeFromTop (14), juce::Justification::centred);

        const double speed = FractalFugue::pulseBeats (cached, kNumDolls - 1) / FractalFugue::pulseBeats (cached, d);
        static const char* tr[] = { "Prime", "Inversion", "Retrograde", "Retro-inv" };
        const juce::String speedText = speed >= 0.999 ? juce::String ("x1") : "x1/" + juce::String (1.0 / speed, (std::abs (1.0 / speed - std::round (1.0 / speed)) < 0.01) ? 0 : 2);
        juce::String info = speedText + "  oct " + juce::String (ds.octave) + "  " + tr[ds.transform & 3]
                            + (ds.source == Source::Sampler ? "  sampler" : "  synth");
        g.setColour (creamDim);
        g.setFont (body (11.0f));
        g.drawText (info, cap.removeFromTop (16), juce::Justification::centred);
    }
}

void DollShelf::drawDoll (juce::Graphics& g, juce::Rectangle<float> r, int index, float pulse, bool blink, float wobble)
{
    const float w = r.getWidth(), h = r.getHeight(), cx = r.getCentreX(), top = r.getY();
    const auto body = dollColour (index), scarf = scarfColour (index);

    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::rotation (wobble, cx, r.getBottom()));

    // silhouette: round head, narrow neck, full belly
    juce::Path s;
    s.startNewSubPath (cx, top);
    s.cubicTo (cx + 0.31f * w, top, cx + 0.37f * w, top + 0.2f * h, cx + 0.33f * w, top + 0.36f * h);
    s.cubicTo (cx + 0.31f * w, top + 0.43f * h, cx + 0.5f * w, top + 0.55f * h, cx + 0.49f * w, top + 0.8f * h);
    s.cubicTo (cx + 0.48f * w, top + 0.94f * h, cx + 0.4f * w, top + h, cx + 0.3f * w, top + h);
    s.lineTo (cx - 0.3f * w, top + h);
    s.cubicTo (cx - 0.4f * w, top + h, cx - 0.48f * w, top + 0.94f * h, cx - 0.49f * w, top + 0.8f * h);
    s.cubicTo (cx - 0.5f * w, top + 0.55f * h, cx - 0.31f * w, top + 0.43f * h, cx - 0.33f * w, top + 0.36f * h);
    s.cubicTo (cx - 0.37f * w, top + 0.2f * h, cx - 0.31f * w, top, cx, top);
    s.closeSubPath();

    // shadow
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (cx - 0.42f * w, top + h - 0.03f * h, 0.84f * w, 0.06f * h);

    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (s);

        juce::ColourGradient grad (body.brighter (0.35f), cx - 0.3f * w, top + 0.3f * h, body.darker (0.45f), cx + 0.5f * w, top + h, true);
        g.setGradientFill (grad);
        g.fillPath (s);

        // headscarf (platok)
        g.setColour (scarf);
        g.fillEllipse (cx - 0.4f * w, top - 0.04f * h, 0.8f * w, 0.5f * h);
        g.setColour (scarf.darker (0.25f));
        for (int i = 0; i < 9; ++i) // polka dots on the scarf
        {
            const float a = juce::MathConstants<float>::pi * (0.15f + 0.7f * i / 8.0f);
            g.fillEllipse (cx - std::cos (a) * 0.31f * w - 0.018f * w, top + 0.2f * h - std::sin (a) * 0.17f * h - 0.018f * w, 0.036f * w, 0.036f * w);
        }

        // apron
        juce::ColourGradient ap (cream, cx, top + 0.6f * h, cream.darker (0.15f), cx, top + h, false);
        g.setGradientFill (ap);
        g.fillEllipse (cx - 0.31f * w, top + 0.5f * h, 0.62f * w, 0.56f * h);

        // folk flower: petals count grows as the doll gets smaller
        const auto fc = juce::Point<float> (cx, top + 0.72f * h);
        const float pr = 0.075f * w;
        const int petals = 5 + index;
        g.setColour (leaf);
        for (int side = -1; side <= 1; side += 2)
        {
            juce::Path leafPath;
            leafPath.addEllipse (-pr * 0.6f, -pr * 1.6f, pr * 1.2f, pr * 3.2f);
            g.fillPath (leafPath, juce::AffineTransform::rotation (side * 0.9f).translated (fc.x + side * pr * 2.0f, fc.y + pr * 1.4f));
        }
        g.setColour (body);
        for (int i = 0; i < petals; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) petals;
            g.fillEllipse (fc.x + std::sin (a) * pr * 1.25f - pr, fc.y - std::cos (a) * pr * 1.25f - pr, pr * 2, pr * 2);
        }
        g.setColour (gold);
        g.fillEllipse (fc.x - pr * 0.9f, fc.y - pr * 0.9f, pr * 1.8f, pr * 1.8f);
        g.setColour (body.darker (0.3f));
        g.fillEllipse (fc.x - pr * 0.35f, fc.y - pr * 0.35f, pr * 0.7f, pr * 0.7f);
        // buds
        g.setColour (cinnabar.interpolatedWith (body, 0.5f));
        for (int side = -1; side <= 1; side += 2)
            g.fillEllipse (fc.x + side * pr * 3.0f - pr * 0.5f, fc.y - pr * 1.9f, pr, pr);

        // gold trim between scarf and dress
        g.setColour (gold);
        for (int i = 0; i < 11; ++i)
        {
            const float t = (float) i / 10.0f;
            const float x = cx - 0.3f * w + t * 0.6f * w;
            g.fillEllipse (x - 0.012f * w, top + 0.44f * h + std::sin (t * juce::MathConstants<float>::pi) * 0.02f * h, 0.024f * w, 0.024f * w);
        }

        // lacquer shine
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.fillEllipse (cx - 0.3f * w, top + 0.05f * h, 0.14f * w, 0.24f * h);
        g.fillEllipse (cx - 0.44f * w, top + 0.55f * h, 0.1f * w, 0.28f * h);
    }

    // face
    const auto face = juce::Rectangle<float> (cx - 0.2f * w, top + 0.09f * h, 0.4f * w, 0.25f * h);
    g.setColour (juce::Colour (0xfff8dcc0));
    g.fillEllipse (face);
    {
        juce::Graphics::ScopedSaveState clip (g);
        juce::Path fp;
        fp.addEllipse (face);
        g.reduceClipRegion (fp);
        g.setColour (juce::Colour (0xff5a321c)); // hair with a centre parting
        g.fillEllipse (cx - 0.22f * w, top + 0.05f * h, 0.44f * w, 0.12f * h);
        g.setColour (juce::Colour (0xff3a1e10));
        g.drawLine (cx, top + 0.085f * h, cx, top + 0.165f * h, 1.0f);
    }
    g.setColour (ink.withAlpha (0.5f));
    g.drawEllipse (face, 1.0f);

    // eyes
    const float ey = top + 0.205f * h, ex = 0.07f * w, er = 0.022f * w;
    g.setColour (ink);
    for (int side = -1; side <= 1; side += 2)
    {
        if (blink)
            g.drawLine (cx + side * ex - er, ey, cx + side * ex + er, ey, 1.4f);
        else
        {
            g.fillEllipse (cx + side * ex - er, ey - er * 1.2f, er * 2, er * 2.4f);
            g.setColour (juce::Colours::white);
            g.fillEllipse (cx + side * ex - er * 0.2f, ey - er * 0.9f, er * 0.7f, er * 0.7f);
            g.setColour (ink);
        }
    }

    // rosy cheeks glow when singing
    g.setColour (rose.withAlpha (0.45f + 0.5f * pulse));
    const float cr = 0.04f * w * (1.0f + 0.25f * pulse);
    for (int side = -1; side <= 1; side += 2)
        g.fillEllipse (cx + side * 0.115f * w - cr, top + 0.255f * h - cr, cr * 2, cr * 2);

    // mouth opens with the note
    const float mw = 0.05f * w * (1.0f + 0.3f * pulse), mh = 0.012f * h + pulse * 0.03f * h;
    g.setColour (cinnabar.darker (0.2f));
    g.fillEllipse (cx - mw * 0.5f, top + 0.285f * h - mh * 0.3f, mw, mh);

    g.setColour (ink.withAlpha (0.8f));
    g.strokePath (s, juce::PathStrokeType (1.6f));
}
} // namespace mf
