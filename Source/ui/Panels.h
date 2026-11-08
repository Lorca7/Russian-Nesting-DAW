#pragma once
#include "Controls.h"
#include "SubjectEditor.h"
#include "SampleBench.h"

namespace mf
{
/** Key, scale, the fractal ratio and the fugal rules, plus the hall (reverb / echo / master). */
class FugueLawPanel : public juce::Component
{
public:
    explicit FugueLawPanel (MatryoshkaProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
    std::function<void()> onEdited;

private:
    void updateScaleNote();
    MatryoshkaProcessor& proc;
    Bindings bind;
    Picker key { "Key" }, scale { "Scale" }, ratio { "Fractal ratio" }, answer { "Answer interval" }, pulse { "Smallest pulse" }, echoTime { "Echo time" };
    Knob coupling { "Coupling", 0.0, 1.0 }, entryGap { "Entry gap", 0.0, 16.0, 0.5 };
    juce::ToggleButton fold { "Fold drift" }, stagger { "Staggered entries" };
    Knob revSize { "Hall size", 0.0, 1.0 }, revLevel { "Hall level", 0.0, 1.0 }, echoFb { "Echo fdbk", 0.0, 0.9 }, master { "Master", 0.0, 1.0 };
    juce::Label scaleNote;
};

/** The subject editor plus generators. */
class SubjectPanel : public juce::Component
{
public:
    explicit SubjectPanel (MatryoshkaProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void update (const Project& p);
    std::function<void()> onEdited;

private:
    MatryoshkaProcessor& proc;
    SubjectEditor editor;
    Picker generator { "Generator" };
    juce::TextButton generate { "Generate" };
    juce::Slider steps;
    uint32_t seed = 7;
};

/** Everything about the selected doll: synth, sampler, nesting and mix. */
class DollInspector : public juce::Component
{
public:
    explicit DollInspector (MatryoshkaProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setDoll (int d);
    void refresh();
    void tick (const Project& p) { bench.update (p); }
    std::function<void()> onEdited;
    std::function<void (const juce::String&)> onStatus;

private:
    enum Page { SynthPage, SamplerPage, NestPage };
    void showPage (Page p);
    void loadSample (const juce::File& f);
    int doll() const { return currentDoll; }
    std::vector<juce::Component*> synthComps() ;
    std::vector<juce::Component*> samplerComps();
    std::vector<juce::Component*> nestComps();

    MatryoshkaProcessor& proc;
    Bindings bind;
    int currentDoll = 0;
    Page page = SynthPage;

    juce::TextButton playsSynth { "Synth" }, playsSampler { "Sampler" };
    juce::TextButton pageSynth { "Synth" }, pageSampler { "Sampler" }, pageNest { "Nest & Mix" }; // which page to edit

    // synth page
    Picker preset { "Preset (by era)" }, wave1 { "Osc 1" }, wave2 { "Osc 2" }, filter { "Filter" };
    std::vector<std::unique_ptr<Knob>> synthKnobs;

    // sampler page
    Picker builtIn { "Built-in sample" }, sampleMode { "Playhead mode" };
    juce::TextButton loadFile { "Load audio file..." }, shareAll { "Use on all dolls" };
    juce::ToggleButton pitchFollow { "Follow pitch" }, reverse { "Reverse" };
    std::vector<std::unique_ptr<Knob>> samplerKnobs;
    SampleBench bench;
    std::unique_ptr<juce::FileChooser> chooser;

    // nest page
    Picker transform { "Fugal transform" }, gate { "Rhythm gate" };
    juce::ToggleButton mute { "Mute" }, solo { "Solo" };
    juce::TextButton nestAll { "Nest chords 4-3-2-1" };
    std::vector<std::unique_ptr<Knob>> nestKnobs;
};
} // namespace mf
