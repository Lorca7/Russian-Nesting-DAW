#include "Panels.h"
#include "../engine/Presets.h"

namespace mf
{
using namespace theme;

// ============================================================================ FugueLawPanel
static const double kPulseValues[] = { 0.125, 0.25, 0.5, 1.0 };
static const char* kPulseNames[] = { "1/32 note", "1/16 note", "1/8 note", "1/4 note" };
static const double kEchoValues[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
static const char* kEchoNames[] = { "1/16", "1/8", "Dotted 1/8", "1/4", "Dotted 1/4", "1/2" };

FugueLawPanel::FugueLawPanel (MatryoshkaProcessor& p) : proc (p), bind (p)
{
    bind.onEdited = [this] { if (onEdited) onEdited(); };

    for (int i = 0; i < 12; ++i) key.box.addItem (noteName (i), i + 1);
    std::string lastGroup;
    for (int i = 0; i < numScales(); ++i)
    {
        const auto& s = scaleAt (i);
        if (s.group != lastGroup) { scale.box.addSectionHeading (s.group); lastGroup = s.group; }
        scale.box.addItem (s.name, i + 1);
    }
    for (int i = 0; i < Ratio::count; ++i) ratio.box.addItem (FractalFugue::ratioName (i), i + 1);
    const std::pair<int, const char*> answers[] = { { 0, "Unison" }, { 1, "2nd" }, { 2, "3rd" }, { 3, "4th" }, { 4, "5th (fugal answer)" }, { 5, "6th" }, { 7, "Octave" } };
    for (auto& [v, n] : answers) answer.box.addItem (n, v + 1);
    for (int i = 0; i < 4; ++i) pulse.box.addItem (kPulseNames[i], i + 1);
    for (int i = 0; i < 6; ++i) echoTime.box.addItem (kEchoNames[i], i + 1);

    bind.combo (key.box, [] (Project& q) -> int& { return q.root; });
    bind.combo (scale.box, [] (Project& q) -> int& { return q.scaleIndex; });
    bind.combo (ratio.box, [] (Project& q) -> int& { return q.ratio; });
    bind.combo (answer.box, [] (Project& q) -> int& { return q.answerInterval; });
    scale.box.onChange = [this, prev = scale.box.onChange] { prev(); updateScaleNote(); };

    pulse.box.onChange = [this] {
        const int i = pulse.box.getSelectedItemIndex();
        if (i >= 0) { proc.editProject ([&] (Project& q) { q.pulseBeats = kPulseValues[i]; }); if (onEdited) onEdited(); }
    };
    echoTime.box.onChange = [this] {
        const int i = echoTime.box.getSelectedItemIndex();
        if (i >= 0) { proc.editProject ([&] (Project& q) { q.echoBeats = kEchoValues[i]; }); if (onEdited) onEdited(); }
    };

    bind.knob<float> (coupling, [] (Project& q) -> float& { return q.coupling; });
    coupling.slider.textFromValueFunction = [] (double v) { return v < 0.01 ? juce::String ("Strict canon") : pctText (v); };
    coupling.setTooltip ("How far each doll rides on its parent's current note. 0 = strict canon, 100% = full fractal.");
    bind.knob<double> (entryGap, [] (Project& q) -> double& { return q.entryGapBeats; });
    entryGap.slider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " beats"; };
    bind.toggle (fold, [] (Project& q) -> bool& { return q.fold; });
    bind.toggle (stagger, [] (Project& q) -> bool& { return q.staggerEntries; });
    fold.setTooltip ("Keep the accumulated fractal drift within an octave of the subject.");
    stagger.setTooltip ("Dolls enter one after another, largest first, like a fugue exposition.");

    bind.knob<float> (revSize, [] (Project& q) -> float& { return q.reverbSize; });
    bind.knob<float> (revLevel, [] (Project& q) -> float& { return q.reverbLevel; });
    bind.knob<float> (echoFb, [] (Project& q) -> float& { return q.echoFeedback; });
    bind.knob<float> (master, [] (Project& q) -> float& { return q.masterVolume; });
    for (auto* k : { &coupling, &revSize, &revLevel, &echoFb, &master })
        if (k != &coupling) k->slider.textFromValueFunction = pctText;

    scaleNote.setFont (body (11.5f));
    scaleNote.setColour (juce::Label::textColourId, creamDim);
    scaleNote.setJustificationType (juce::Justification::topLeft);
    scaleNote.setMinimumHorizontalScale (1.0f);

    for (juce::Component* c : std::initializer_list<juce::Component*> { &key, &scale, &ratio, &answer, &pulse, &echoTime, &coupling, &entryGap,
                                                                         &fold, &stagger, &revSize, &revLevel, &echoFb, &master, &scaleNote })
        addAndMakeVisible (c);
    refresh();
}

void FugueLawPanel::refresh()
{
    bind.refresh();
    const auto p = proc.getProject();
    for (int i = 0; i < 4; ++i) if (std::abs (p.pulseBeats - kPulseValues[i]) < 1e-6) pulse.box.setSelectedItemIndex (i, juce::dontSendNotification);
    for (int i = 0; i < 6; ++i) if (std::abs (p.echoBeats - kEchoValues[i]) < 1e-6) echoTime.box.setSelectedItemIndex (i, juce::dontSendNotification);
    updateScaleNote();
}

void FugueLawPanel::updateScaleNote()
{
    const auto& s = scaleAt (proc.getProject().scaleIndex);
    scaleNote.setText (juce::String (s.note), juce::dontSendNotification);
}

void FugueLawPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto hall = r.removeFromBottom (122);
    drawPanel (g, r.withTrimmedBottom (6), "Fugue Law", juce::CharPointer_UTF8 ("\xD0\x97\xD0\xB0\xD0\xBA\xD0\xBE\xD0\xBD \xD1\x84\xD1\x83\xD0\xB3\xD0\xB8"));
    drawPanel (g, hall, "The Hall", juce::CharPointer_UTF8 ("\xD0\x97\xD0\xB0\xD0\xBB"));
}

void FugueLawPanel::resized()
{
    auto r = getLocalBounds();
    auto hall = r.removeFromBottom (122);
    r = r.withTrimmedBottom (6).withTrimmedTop (30).reduced (10, 2);

    auto row = [&] (int h) { auto x = r.removeFromTop (h); r.removeFromTop (4); return x; };
    auto two = row (42);
    key.setBounds (two.removeFromLeft (70));
    two.removeFromLeft (8);
    scale.setBounds (two);
    scaleNote.setBounds (row (32));
    auto rr = row (42);
    ratio.setBounds (rr.removeFromLeft (rr.getWidth() / 2 - 4));
    rr.removeFromLeft (8);
    answer.setBounds (rr);
    auto pr = row (42);
    pulse.setBounds (pr.removeFromLeft (pr.getWidth() / 2 - 4));
    pr.removeFromLeft (8);
    echoTime.setBounds (pr);
    auto kr = row (juce::jmin (78, r.getHeight() - 30));
    auto toggles = kr.removeFromRight (kr.getWidth() - 150);
    coupling.setBounds (kr.removeFromLeft (75));
    entryGap.setBounds (kr);
    fold.setBounds (toggles.removeFromTop (toggles.getHeight() / 2).reduced (0, 4));
    stagger.setBounds (toggles.reduced (0, 4));

    hall = hall.withTrimmedTop (30).reduced (10, 2);
    layoutGrid (hall, { &revSize, &revLevel, &echoFb, &master }, 60, hall.getHeight());
}

// ============================================================================ SubjectPanel
SubjectPanel::SubjectPanel (MatryoshkaProcessor& p) : proc (p), editor (p)
{
    editor.onEdited = [this] { if (onEdited) onEdited(); };
    for (int g = 0; g < Generator::count; ++g) generator.box.addItem (generatorName (g), g + 1);
    generator.box.setSelectedId (1, juce::dontSendNotification);
    generate.setTooltip ("Write a new subject with the chosen generator.");
    generate.onClick = [this] {
        seed = seed * 1664525u + 1013904223u;
        const int gen = generator.box.getSelectedId() - 1;
        proc.editProject ([&] (Project& q) {
            generateSubject (q, gen, seed);
            if (gen == Generator::DSCH) { q.root = 0; q.scaleIndex = findScale ("Harmonic Minor"); }
        });
        if (onEdited) onEdited();
    };
    nest.setTooltip ("Largest doll plays 4-note chords, then 3, 2, and the smallest a single line.");
    nest.onClick = [this] { proc.editProject ([] (Project& q) { nestChords (q); }); if (onEdited) onEdited(); };

    steps.setSliderStyle (juce::Slider::LinearHorizontal);
    steps.setRange (1, kMaxSteps, 1);
    steps.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 64, 20);
    steps.textFromValueFunction = [] (double v) { return juce::String ((int) v) + " steps"; };
    steps.onValueChange = [this] {
        const int n = (int) steps.getValue();
        proc.editProject ([&] (Project& q) { q.subjectLength = n; });
        if (onEdited) onEdited();
    };
    for (juce::Component* c : std::initializer_list<juce::Component*> { &editor, &generator, &generate, &nest, &steps })
        addAndMakeVisible (c);
}

void SubjectPanel::update (const Project& p)
{
    editor.update (p);
    steps.setValue (p.subjectLength, juce::dontSendNotification);
}

void SubjectPanel::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds(), "The Subject", juce::CharPointer_UTF8 ("\xD0\xA2\xD0\xB5\xD0\xBC\xD0\xB0 \xC2\xB7 click to write, click a note for a rest, bottom row = length & velocity"));
}

void SubjectPanel::resized()
{
    auto r = getLocalBounds().withTrimmedTop (30).reduced (10, 6);
    auto top = r.removeFromTop (42);
    generator.setBounds (top.removeFromLeft (160));
    top.removeFromLeft (8);
    generate.setBounds (top.removeFromLeft (90).withTrimmedTop (15));
    top.removeFromLeft (8);
    nest.setBounds (top.removeFromRight (150).withTrimmedTop (15));
    top.removeFromRight (8);
    steps.setBounds (top.withTrimmedTop (15));
    r.removeFromTop (6);
    editor.setBounds (r);
}

// ============================================================================ DollInspector
DollInspector::DollInspector (MatryoshkaProcessor& p) : proc (p), bind (p), bench (p)
{
    bind.onEdited = [this] { if (onEdited) onEdited(); };

    // ---- source and page buttons
    playsSynth.setClickingTogglesState (false);
    playsSampler.setClickingTogglesState (false);
    playsSynth.onClick = [this] { proc.editProject ([&] (Project& q) { q.dolls[(size_t) doll()].source = Source::Synth; }); showPage (SynthPage); refresh(); if (onEdited) onEdited(); };
    playsSampler.onClick = [this] { proc.editProject ([&] (Project& q) { q.dolls[(size_t) doll()].source = Source::Sampler; }); showPage (SamplerPage); refresh(); if (onEdited) onEdited(); };
    pageSynth.onClick = [this] { showPage (SynthPage); };
    pageSampler.onClick = [this] { showPage (SamplerPage); };
    pageNest.onClick = [this] { showPage (NestPage); };
    for (auto* b : { &playsSynth, &playsSampler, &pageSynth, &pageSampler, &pageNest }) addAndMakeVisible (b);

    // ---- synth page
    std::string lastEra;
    const auto& presets = synthPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (presets[i].era != lastEra) { preset.box.addSectionHeading (presets[i].era); lastEra = presets[i].era; }
        preset.box.addItem (presets[i].name, (int) i + 1);
    }
    preset.box.setTextWhenNothingSelected ("Choose a vintage preset...");
    preset.box.onChange = [this] {
        const int i = preset.box.getSelectedId() - 1;
        if (i < 0) return;
        proc.editProject ([&] (Project& q) { applyPreset (q.dolls[(size_t) doll()], synthPresets()[(size_t) i]); });
        refresh();
        if (onEdited) onEdited();
    };
    const char* waves[] = { "Saw", "Square", "Pulse (PWM)", "Triangle", "Sine (FM)", "Off" };
    for (int i = 0; i < Wave::count; ++i) { wave1.box.addItem (waves[i], i + 1); wave2.box.addItem (waves[i], i + 1); }
    const char* filters[] = { "Ladder 24 dB", "SEM 12 dB", "Polivoks (gritty)", "High-pass", "Band-pass" };
    for (int i = 0; i < FilterType::count; ++i) filter.box.addItem (filters[i], i + 1);
    bind.combo (wave1.box, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].synth.wave1; });
    bind.combo (wave2.box, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].synth.wave2; });
    bind.combo (filter.box, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].synth.filterType; });

    struct KS { const char* name; double lo, hi, step, skew; float SynthParams::* field; std::function<juce::String (double)> fmt; };
    auto semis = [] (double v) { return (v > 0 ? "+" : "") + juce::String ((int) v) + " st"; };
    auto cents = [] (double v) { return juce::String ((int) v) + " ct"; };
    auto plain = [] (double v) { return juce::String (v, 2); };
    auto hz = [] (double v) { return juce::String (v, 2) + " Hz"; };
    auto oct = [] (double v) { return juce::String (v, 1) + " oct"; };
    const std::vector<KS> synthSpec {
        { "Osc2 pitch", -24, 24, 1, 0, &SynthParams::osc2Semis, semis },
        { "Detune", 0, 50, 0.1, 0, &SynthParams::osc2Detune, cents },
        { "Osc mix", 0, 1, 0, 0, &SynthParams::oscMix, pctText },
        { "Sub osc", 0, 1, 0, 0, &SynthParams::subLevel, pctText },
        { "Noise", 0, 0.5, 0, 0, &SynthParams::noiseLevel, pctText },
        { "Pulse width", 0.05, 0.95, 0, 0, &SynthParams::pulseWidth, pctText },
        { "PWM depth", 0, 1, 0, 0, &SynthParams::pwmDepth, pctText },
        { "PWM rate", 0.05, 8, 0, 1, &SynthParams::pwmRate, hz },
        { "FM index", 0, 10, 0, 2, &SynthParams::fmIndex, plain },
        { "FM ratio", 0.5, 8, 0.5, 0, &SynthParams::fmRatio, plain },
        { "FM decay", 0.02, 4, 0, 0.6, &SynthParams::fmDecay, secText },
        { "Cutoff", 40, 18000, 0, 1200, &SynthParams::cutoff, hzText },
        { "Resonance", 0, 1, 0, 0, &SynthParams::resonance, pctText },
        { "Env amount", -2, 6, 0, 0, &SynthParams::envAmount, oct },
        { "Key track", 0, 1, 0, 0, &SynthParams::keyTrack, pctText },
        { "Amp attack", 0.001, 4, 0, 0.3, &SynthParams::ampA, secText },
        { "Amp decay", 0.01, 4, 0, 0.5, &SynthParams::ampD, secText },
        { "Amp sustain", 0, 1, 0, 0, &SynthParams::ampS, pctText },
        { "Amp release", 0.01, 5, 0, 0.6, &SynthParams::ampR, secText },
        { "Filt attack", 0.001, 4, 0, 0.3, &SynthParams::filA, secText },
        { "Filt decay", 0.01, 4, 0, 0.5, &SynthParams::filD, secText },
        { "Filt sustain", 0, 1, 0, 0, &SynthParams::filS, pctText },
        { "Filt release", 0.01, 5, 0, 0.6, &SynthParams::filR, secText },
        { "Vibrato rate", 0.1, 12, 0, 0, &SynthParams::vibratoRate, hz },
        { "Vibrato", 0, 60, 0, 0, &SynthParams::vibratoDepth, cents },
        { "Drift", 0, 20, 0, 0, &SynthParams::drift, cents },
        { "Level", 0, 1, 0, 0, &SynthParams::level, pctText },
    };
    for (auto& k : synthSpec)
    {
        auto knob = std::make_unique<Knob> (k.name, k.lo, k.hi, k.step, k.skew);
        knob->slider.textFromValueFunction = k.fmt;
        const auto field = k.field;
        bind.knob<float> (*knob, [this, field] (Project& q) -> float& { return q.dolls[(size_t) doll()].synth.*field; });
        synthKnobs.push_back (std::move (knob));
    }

    // ---- sampler page
    for (size_t i = 0; i < builtInSamples().size(); ++i) builtIn.box.addItem (builtInSamples()[i].name, (int) i + 1);
    builtIn.box.setTextWhenNothingSelected ("Your own file");
    builtIn.box.onChange = [this] {
        const int i = builtIn.box.getSelectedId() - 1;
        if (i < 0) return;
        proc.useBuiltInSample (doll(), i);
        bench.setDoll (doll());
        refresh();
        if (onEdited) onEdited();
    };
    loadFile.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Load a sample into " + juce::String (dollLatinName (doll())),
                                                       juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                       proc.formatManager.getWildcardForAllFormats());
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) loadSample (fc.getResult()); });
    };
    shareAll.setTooltip ("Give every doll this sample, so four playheads read the same sound in fugue.");
    shareAll.onClick = [this] {
        proc.shareSampleWithAll (doll());
        proc.editProject ([] (Project& q) { for (auto& d : q.dolls) d.source = Source::Sampler; });
        refresh();
        if (onEdited) onEdited();
        if (onStatus) onStatus ("All four dolls now read the same sample with their own playheads.");
    };
    bench.onFileDropped = [this] (const juce::File& f) { loadSample (f); };
    const char* modes[] = { "Pitched (one-shot)", "Slice (fractal chop)", "Scrub (fugue playheads)" };
    for (int i = 0; i < SampleMode::count; ++i) sampleMode.box.addItem (modes[i], i + 1);
    bind.combo (sampleMode.box, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].sampler.mode; });
    bind.toggle (pitchFollow, [this] (Project& q) -> bool& { return q.dolls[(size_t) doll()].sampler.pitchFollow; });
    bind.toggle (reverse, [this] (Project& q) -> bool& { return q.dolls[(size_t) doll()].sampler.reverse; });

    {
        auto root = std::make_unique<Knob> ("Root note", 24, 96, 1);
        root->slider.textFromValueFunction = [] (double v) { return juce::String (noteName ((int) v)) + juce::String ((int) v / 12 - 1); };
        bind.knob<int> (*root, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].sampler.rootNote; });
        samplerKnobs.push_back (std::move (root));
        auto slices = std::make_unique<Knob> ("Slices", 1, 32, 1);
        bind.knob<int> (*slices, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].sampler.slices; });
        samplerKnobs.push_back (std::move (slices));
        struct SK { const char* name; double lo, hi, skew; float SamplerParams::* f; std::function<juce::String (double)> fmt; };
        const std::vector<SK> spec {
            { "Start", 0, 1, 0, &SamplerParams::start, pctText },
            { "Scan speed", 0.05, 4, 1, &SamplerParams::scanSpeed, [] (double v) { return "x" + juce::String (v, 2); } },
            { "Tone", 200, 18000, 3000, &SamplerParams::tone, hzText },
            { "Attack", 0.001, 1, 0.1, &SamplerParams::attack, secText },
            { "Release", 0.005, 4, 0.5, &SamplerParams::release, secText },
            { "Level", 0, 1, 0, &SamplerParams::level, pctText },
        };
        for (auto& k : spec)
        {
            auto knob = std::make_unique<Knob> (k.name, k.lo, k.hi, 0.0, k.skew);
            knob->slider.textFromValueFunction = k.fmt;
            const auto f = k.f;
            bind.knob<float> (*knob, [this, f] (Project& q) -> float& { return q.dolls[(size_t) doll()].sampler.*f; });
            samplerKnobs.push_back (std::move (knob));
        }
    }

    // ---- nest page
    const char* transforms[] = { "Prime (as written)", "Inversion (mirror)", "Retrograde (backwards)", "Retrograde inversion" };
    for (int i = 0; i < Transform::count; ++i) transform.box.addItem (transforms[i], i + 1);
    const char* gates[] = { "Every note", "Cantor set", "Sierpinski triangle", "Fibonacci word" };
    for (int i = 0; i < Gate::count; ++i) gate.box.addItem (gates[i], i + 1);
    bind.combo (transform.box, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].transform; });
    bind.combo (gate.box, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].gate; });
    bind.toggle (mute, [this] (Project& q) -> bool& { return q.dolls[(size_t) doll()].mute; });
    bind.toggle (solo, [this] (Project& q) -> bool& { return q.dolls[(size_t) doll()].solo; });
    {
        auto octave = std::make_unique<Knob> ("Octave", 0, 7, 1);
        bind.knob<int> (*octave, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].octave; });
        nestKnobs.push_back (std::move (octave));
        auto chord = std::make_unique<Knob> ("Chord notes", 1, 4, 1);
        bind.knob<int> (*chord, [this] (Project& q) -> int& { return q.dolls[(size_t) doll()].chordSize; });
        nestKnobs.push_back (std::move (chord));
        struct NK { const char* name; double lo, hi; float DollSettings::* f; std::function<juce::String (double)> fmt; };
        const std::vector<NK> spec {
            { "Legato", 0.1, 1.5, &DollSettings::legato, pctText },
            { "Volume", 0, 1, &DollSettings::volume, pctText },
            { "Pan", -1, 1, &DollSettings::pan, [] (double v) { return std::abs (v) < 0.02 ? juce::String ("C") : (v < 0 ? "L" : "R") + juce::String ((int) std::round (std::abs (v) * 100)); } },
            { "Drive", 0, 1, &DollSettings::drive, pctText },
            { "Chorus", 0, 1, &DollSettings::chorus, pctText },
            { "Tape wow", 0, 1, &DollSettings::wow, pctText },
            { "Hall send", 0, 1, &DollSettings::reverbSend, pctText },
            { "Echo send", 0, 1, &DollSettings::echoSend, pctText },
        };
        for (auto& k : spec)
        {
            auto knob = std::make_unique<Knob> (k.name, k.lo, k.hi);
            knob->slider.textFromValueFunction = k.fmt;
            const auto f = k.f;
            bind.knob<float> (*knob, [this, f] (Project& q) -> float& { return q.dolls[(size_t) doll()].*f; });
            nestKnobs.push_back (std::move (knob));
        }
    }

    for (auto* c : synthComps()) addChildComponent (c);
    for (auto* c : samplerComps()) addChildComponent (c);
    for (auto* c : nestComps()) addChildComponent (c);
    setDoll (0);
    showPage (SynthPage);
}

std::vector<juce::Component*> DollInspector::synthComps()
{
    std::vector<juce::Component*> v { &preset, &wave1, &wave2, &filter };
    for (auto& k : synthKnobs) v.push_back (k.get());
    return v;
}
std::vector<juce::Component*> DollInspector::samplerComps()
{
    std::vector<juce::Component*> v { &builtIn, &sampleMode, &loadFile, &shareAll, &pitchFollow, &reverse, &bench };
    for (auto& k : samplerKnobs) v.push_back (k.get());
    return v;
}
std::vector<juce::Component*> DollInspector::nestComps()
{
    std::vector<juce::Component*> v { &transform, &gate, &mute, &solo };
    for (auto& k : nestKnobs) v.push_back (k.get());
    return v;
}

void DollInspector::showPage (Page p)
{
    page = p;
    for (auto* c : synthComps()) c->setVisible (p == SynthPage);
    for (auto* c : samplerComps()) c->setVisible (p == SamplerPage);
    for (auto* c : nestComps()) c->setVisible (p == NestPage);
    pageSynth.setToggleState (p == SynthPage, juce::dontSendNotification);
    pageSampler.setToggleState (p == SamplerPage, juce::dontSendNotification);
    pageNest.setToggleState (p == NestPage, juce::dontSendNotification);
    resized();
}

void DollInspector::setDoll (int d)
{
    currentDoll = juce::jlimit (0, kNumDolls - 1, d);
    const auto c = dollColour (currentDoll);
    for (auto& k : synthKnobs) k->slider.setColour (juce::Slider::rotarySliderFillColourId, c.brighter (0.3f));
    for (auto& k : samplerKnobs) k->slider.setColour (juce::Slider::rotarySliderFillColourId, c.brighter (0.3f));
    for (auto& k : nestKnobs) k->slider.setColour (juce::Slider::rotarySliderFillColourId, c.brighter (0.3f));
    for (auto* b : { &playsSynth, &playsSampler, &pageSynth, &pageSampler, &pageNest })
        b->setColour (juce::TextButton::buttonOnColourId, c.darker (0.2f));
    bench.setDoll (currentDoll);
    preset.box.setSelectedId (0, juce::dontSendNotification);
    refresh();
    const auto p = proc.getProject();
    showPage (p.dolls[(size_t) currentDoll].source == Source::Sampler ? SamplerPage : SynthPage);
    repaint();
}

void DollInspector::refresh()
{
    bind.refresh();
    const auto p = proc.getProject();
    const bool sampler = p.dolls[(size_t) doll()].source == Source::Sampler;
    playsSynth.setToggleState (! sampler, juce::dontSendNotification);
    playsSampler.setToggleState (sampler, juce::dontSendNotification);
    builtIn.box.setSelectedId (0, juce::dontSendNotification);
    if (auto* s = proc.getSample (doll()))
        for (size_t i = 0; i < builtInSamples().size(); ++i)
            if (s->name == builtInSamples()[i].name)
                builtIn.box.setSelectedId ((int) i + 1, juce::dontSendNotification);
    for (auto& k : synthKnobs) k->repaint();
    for (auto& k : samplerKnobs) k->repaint();
    for (auto& k : nestKnobs) k->repaint();
    repaint();
}

void DollInspector::loadSample (const juce::File& f)
{
    juce::String err;
    if (proc.loadSampleFile (doll(), f, err))
    {
        proc.editProject ([&] (Project& q) { q.dolls[(size_t) doll()].source = Source::Sampler; });
        bench.setDoll (doll());
        refresh();
        if (onEdited) onEdited();
        if (onStatus) onStatus ("Loaded " + f.getFileName() + " into " + dollLatinName (doll()) + ". Set its Root note to the pitch it was recorded at.");
    }
    else if (onStatus)
        onStatus (err);
}

void DollInspector::paint (juce::Graphics& g)
{
    const auto c = dollColour (currentDoll);
    auto r = drawPanel (g, getLocalBounds(), juce::String (dollRole (currentDoll)) + " doll",
                        juce::String (juce::CharPointer_UTF8 (dollRussianName (currentDoll))) + juce::String (juce::CharPointer_UTF8 (" \xC2\xB7 ")) + dollLatinName (currentDoll), c.brighter (0.3f));
    juce::ignoreUnused (r);
    g.setColour (creamDim);
    g.setFont (body (11.0f));
    g.drawText ("Plays", getLocalBounds().withTrimmedTop (30).reduced (12, 6).removeFromTop (26), juce::Justification::centredLeft);
}

void DollInspector::resized()
{
    auto r = getLocalBounds().withTrimmedTop (30).reduced (12, 6);
    auto head = r.removeFromTop (26);
    head.removeFromLeft (40);
    playsSynth.setBounds (head.removeFromLeft (76));
    head.removeFromLeft (4);
    playsSampler.setBounds (head.removeFromLeft (76));
    pageNest.setBounds (head.removeFromRight (96));
    head.removeFromRight (4);
    pageSampler.setBounds (head.removeFromRight (76));
    head.removeFromRight (4);
    pageSynth.setBounds (head.removeFromRight (64));
    r.removeFromTop (8);

    if (page == SynthPage)
    {
        auto top = r.removeFromTop (42);
        const int w = top.getWidth();
        preset.setBounds (top.removeFromLeft (w * 2 / 5 - 4)); top.removeFromLeft (6);
        wave1.setBounds (top.removeFromLeft (w / 5 - 4)); top.removeFromLeft (6);
        wave2.setBounds (top.removeFromLeft (w / 5 - 4)); top.removeFromLeft (6);
        filter.setBounds (top);
        r.removeFromTop (4);
        std::vector<juce::Component*> ks;
        for (auto& k : synthKnobs) ks.push_back (k.get());
        const int cols = juce::jmax (1, r.getWidth() / 62);
        const int rows = ((int) ks.size() + cols - 1) / cols;
        layoutGrid (r, ks, 62, juce::jmin (76, r.getHeight() / juce::jmax (1, rows)));
    }
    else if (page == SamplerPage)
    {
        auto top = r.removeFromTop (42);
        builtIn.setBounds (top.removeFromLeft (170)); top.removeFromLeft (6);
        sampleMode.setBounds (top.removeFromLeft (180)); top.removeFromLeft (6);
        loadFile.setBounds (top.withTrimmedTop (15));
        r.removeFromTop (6);
        auto row = r.removeFromTop (26);
        shareAll.setBounds (row.removeFromLeft (150));
        row.removeFromLeft (10);
        pitchFollow.setBounds (row.removeFromLeft (120));
        reverse.setBounds (row.removeFromLeft (100));
        r.removeFromTop (6);
        bench.setBounds (r.removeFromTop (juce::jmax (60, r.getHeight() - 84)));
        r.removeFromTop (4);
        std::vector<juce::Component*> ks;
        for (auto& k : samplerKnobs) ks.push_back (k.get());
        layoutGrid (r, ks, juce::jmax (52, r.getWidth() / (int) ks.size()), r.getHeight());
    }
    else
    {
        auto top = r.removeFromTop (42);
        transform.setBounds (top.removeFromLeft (200)); top.removeFromLeft (8);
        gate.setBounds (top.removeFromLeft (170)); top.removeFromLeft (8);
        auto tg = top.withTrimmedTop (15);
        mute.setBounds (tg.removeFromLeft (tg.getWidth() / 2));
        solo.setBounds (tg);
        r.removeFromTop (8);
        std::vector<juce::Component*> ks;
        for (auto& k : nestKnobs) ks.push_back (k.get());
        layoutGrid (r, ks, 76, juce::jmin (84, r.getHeight() / 2));
    }
}
} // namespace mf
