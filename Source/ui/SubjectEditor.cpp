#include "SubjectEditor.h"

namespace mf
{
using namespace theme;

juce::Rectangle<float> SubjectEditor::gridArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromLeft (34.0f);
    r.removeFromTop (12.0f);
    r.removeFromBottom (34.0f);
    return r;
}

juce::Rectangle<float> SubjectEditor::laneArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromLeft (34.0f);
    return r.removeFromBottom (30.0f);
}

float SubjectEditor::stepX (int step) const
{
    const auto g = gridArea();
    int units = 0, before = 0;
    for (int i = 0; i < project.subjectLength; ++i)
    {
        if (i == step) before = units;
        units += project.subject[(size_t) i].length;
    }
    if (step >= project.subjectLength) before = units;
    return g.getX() + g.getWidth() * (float) before / (float) juce::jmax (1, units);
}

int SubjectEditor::stepAt (float x) const
{
    for (int i = 0; i < project.subjectLength; ++i)
        if (x < stepX (i + 1))
            return i;
    return project.subjectLength - 1;
}

int SubjectEditor::degreeAt (float y) const
{
    const auto g = gridArea();
    const int rows = kTopDegree - kBottomDegree + 1;
    const int row = juce::jlimit (0, rows - 1, (int) ((y - g.getY()) / g.getHeight() * rows));
    return kTopDegree - row;
}

void SubjectEditor::paint (juce::Graphics& g)
{
    const auto grid = gridArea();
    const int rows = kTopDegree - kBottomDegree + 1;
    const float rowH = grid.getHeight() / (float) rows;
    const auto& scale = scaleAt (project.scaleIndex);

    g.setColour (ink);
    g.fillRoundedRectangle (grid.expanded (2.0f), 6.0f);

    for (int deg = kTopDegree; deg >= kBottomDegree; --deg)
    {
        const float y = grid.getY() + (float) (kTopDegree - deg) * rowH;
        const int n = (int) scale.steps.size();
        const bool tonic = ((deg % n) + n) % n == 0;
        if (tonic)
        {
            g.setColour (gold.withAlpha (0.1f));
            g.fillRect (grid.getX(), y, grid.getWidth(), rowH);
        }
        const int pc = ((project.root + degreeToSemitone (scale, deg)) % 12 + 12) % 12;
        g.setColour (tonic ? gold : creamDim);
        g.setFont (body (10.5f, tonic));
        g.drawText (noteName (pc), juce::Rectangle<float> (0, y, 30, rowH), juce::Justification::centredRight);
        g.setColour (cream.withAlpha (0.05f));
        g.drawHorizontalLine ((int) y, grid.getX(), grid.getRight());
    }

    for (int i = 0; i < project.subjectLength; ++i)
    {
        const auto& s = project.subject[(size_t) i];
        const float x0 = stepX (i), x1 = stepX (i + 1);
        g.setColour (cream.withAlpha (0.12f));
        g.drawVerticalLine ((int) x0, grid.getY(), grid.getBottom());

        const int deg = juce::jlimit (kBottomDegree, kTopDegree, s.degree);
        auto cell = juce::Rectangle<float> (x0 + 2, grid.getY() + (float) (kTopDegree - deg) * rowH + 1, x1 - x0 - 4, rowH - 2);
        if (s.on)
        {
            g.setColour (cinnabar.interpolatedWith (gold, 1.0f - s.velocity).withAlpha (0.95f));
            g.fillRoundedRectangle (cell, juce::jmin (5.0f, rowH * 0.45f));
            g.setColour (gold);
            g.drawRoundedRectangle (cell, juce::jmin (5.0f, rowH * 0.45f), 1.0f);
        }
        else
        {
            g.setColour (creamDim.withAlpha (0.6f));
            g.drawRoundedRectangle (cell, juce::jmin (5.0f, rowH * 0.45f), 1.0f);
        }

        // length / velocity lane
        auto lane = laneArea();
        auto lr = juce::Rectangle<float> (x0 + 2, lane.getY(), x1 - x0 - 4, lane.getHeight());
        g.setColour (panelHi);
        g.fillRoundedRectangle (lr, 4.0f);
        g.setColour (gold.withAlpha (0.5f));
        g.fillRoundedRectangle (lr.withTrimmedTop (lr.getHeight() * (1.0f - s.velocity)), 4.0f);
        g.setColour (cream);
        g.setFont (body (11.0f, true));
        g.drawText (juce::String (s.length), lr, juce::Justification::centred);
    }

    // where each doll currently is inside the subject
    if (proc.isPlaying())
        for (int d = 0; d < kNumDolls; ++d)
        {
            const auto t = FractalFugue::transformed (project, d);
            int step = juce::jlimit (0, project.subjectLength - 1, proc.getDollStep (d));
            const bool retro = project.dolls[(size_t) d].transform == Transform::Retrograde
                               || project.dolls[(size_t) d].transform == Transform::RetroInversion;
            if (retro) step = project.subjectLength - 1 - step;
            juce::ignoreUnused (t);
            const float x = stepX (step) + 6.0f + (float) d * 7.0f;
            g.setColour (dollColour (d));
            g.fillEllipse (x - 3.0f, 2.0f, 7.0f, 7.0f);
        }
}

void SubjectEditor::mouseDown (const juce::MouseEvent& e)
{
    dragStep = stepAt (e.position.x);
    if (dragStep < 0) return;
    dragOnLane = laneArea().contains (e.position);
    dragStartY = e.position.y;
    dragStartVel = project.subject[(size_t) dragStep].velocity;

    if (dragOnLane)
        return; // a click (no drag) on the lane cycles the length in mouseUp

    const int deg = degreeAt (e.position.y);
    proc.editProject ([&] (Project& p) {
        auto& s = p.subject[(size_t) dragStep];
        if (s.degree == deg && s.on && ! e.mods.isShiftDown())
        {
            s.on = false;
            paintMode = false;
        }
        else
        {
            s.degree = deg;
            s.on = true;
            paintMode = true;
        }
    });
    project = proc.getProject();
    if (onEdited) onEdited();
    repaint();
}

void SubjectEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStep < 0) return;
    if (dragOnLane)
    {
        const float dv = (dragStartY - e.position.y) / 40.0f;
        const float vel = juce::jlimit (0.1f, 1.0f, dragStartVel + dv);
        const int step = dragStep;
        proc.editProject ([&] (Project& p) { p.subject[(size_t) step].velocity = vel; });
    }
    else if (paintMode)
    {
        const int step = stepAt (e.position.x), deg = degreeAt (e.position.y);
        proc.editProject ([&] (Project& p) { p.subject[(size_t) step].degree = deg; p.subject[(size_t) step].on = true; });
    }
    project = proc.getProject();
    if (onEdited) onEdited();
    repaint();
}
void SubjectEditor::mouseUp (const juce::MouseEvent& e)
{
    if (dragStep >= 0 && dragOnLane && e.getDistanceFromDragStart() < 4)
    {
        const int step = dragStep;
        proc.editProject ([&] (Project& p) {
            auto& s = p.subject[(size_t) step];
            s.length = s.length % 8 + 1;
        });
        project = proc.getProject();
        if (onEdited) onEdited();
        repaint();
    }
    dragStep = -1;
}
} // namespace mf
