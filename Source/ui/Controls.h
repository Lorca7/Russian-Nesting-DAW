#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"
#include <functional>

namespace mf
{
/** A rotary knob with a caption and its value underneath. */
class Knob : public juce::Component
{
public:
    Knob (const juce::String& caption, double min, double max, double interval = 0.0, double skewCentre = 0.0)
        : name (caption)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setRange (min, max, interval);
        if (skewCentre > 0.0) slider.setSkewFactorFromMidPoint (skewCentre);
        slider.onValueChange = [this] { repaint(); if (onChange) onChange(); };
        addAndMakeVisible (slider);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromBottom (26);
        slider.setBounds (r);
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().removeFromBottom (26);
        g.setColour (theme::creamDim);
        g.setFont (theme::body (11.0f));
        g.drawFittedText (name, r.removeFromTop (13), juce::Justification::centred, 1);
        g.setColour (theme::cream);
        g.setFont (theme::body (11.5f, true));
        g.drawFittedText (slider.getTextFromValue (slider.getValue()), r, juce::Justification::centred, 1);
    }
    juce::Slider slider;
    juce::String name;
    std::function<void()> onChange;
};

/** A combo box with a small caption above. */
class Picker : public juce::Component
{
public:
    explicit Picker (const juce::String& caption) : name (caption) { addAndMakeVisible (box); }
    void resized() override { auto r = getLocalBounds(); r.removeFromTop (15); box.setBounds (r.removeFromTop (26)); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::creamDim);
        g.setFont (theme::body (11.0f));
        g.drawText (name, getLocalBounds().removeFromTop (15), juce::Justification::centredLeft);
    }
    juce::ComboBox box;
    juce::String name;
};

/** Connects controls to fields of the project so the UI stays a thin view over MatryoshkaProcessor. */
class Bindings
{
public:
    template <typename T> using Accessor = std::function<T& (Project&)>;

    explicit Bindings (MatryoshkaProcessor& p) : proc (p) {}
    std::function<void()> onEdited;

    template <typename T>
    void slider (juce::Slider& s, Accessor<T> a)
    {
        s.onValueChange = [this, &s, a, prev = s.onValueChange] {
            if (prev) prev();
            if (refreshing) return;
            proc.editProject ([&] (Project& p) { a (p) = (T) s.getValue(); });
            if (onEdited) onEdited();
        };
        refreshers.push_back ([&s, a] (Project& p) { s.setValue ((double) a (p), juce::dontSendNotification); });
    }
    template <typename T> void knob (Knob& k, Accessor<T> a) { slider<T> (k.slider, a); refreshers.push_back ([&k] (Project&) { k.repaint(); }); }

    /** Combo items must use ids value + 1. */
    void combo (juce::ComboBox& c, Accessor<int> a)
    {
        c.onChange = [this, &c, a] {
            if (refreshing || c.getSelectedId() == 0) return;
            proc.editProject ([&] (Project& p) { a (p) = c.getSelectedId() - 1; });
            if (onEdited) onEdited();
        };
        refreshers.push_back ([&c, a] (Project& p) { c.setSelectedId (a (p) + 1, juce::dontSendNotification); });
    }
    void toggle (juce::Button& b, Accessor<bool> a)
    {
        b.onClick = [this, &b, a] {
            if (refreshing) return;
            proc.editProject ([&] (Project& p) { a (p) = b.getToggleState(); });
            if (onEdited) onEdited();
        };
        refreshers.push_back ([&b, a] (Project& p) { b.setToggleState (a (p), juce::dontSendNotification); });
    }
    void refresh()
    {
        auto p = proc.getProject();
        refreshing = true;
        for (auto& r : refreshers) r (p);
        refreshing = false;
    }

private:
    MatryoshkaProcessor& proc;
    std::vector<std::function<void (Project&)>> refreshers;
    bool refreshing = false;
};

inline void layoutGrid (juce::Rectangle<int> area, const std::vector<juce::Component*>& comps, int cellW, int cellH)
{
    const int cols = juce::jmax (1, area.getWidth() / cellW);
    const int w = area.getWidth() / cols;
    for (size_t i = 0; i < comps.size(); ++i)
    {
        const int col = (int) i % cols, row = (int) i / cols;
        comps[i]->setBounds (area.getX() + col * w, area.getY() + row * cellH, w, cellH);
    }
}

inline juce::String hzText (double v) { return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " kHz" : juce::String ((int) v) + " Hz"; }
inline juce::String secText (double v) { return v < 1.0 ? juce::String ((int) std::round (v * 1000.0)) + " ms" : juce::String (v, 2) + " s"; }
inline juce::String pctText (double v) { return juce::String ((int) std::round (v * 100.0)) + "%"; }
} // namespace mf
