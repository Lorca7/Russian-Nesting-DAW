#include "ProjectIO.h"

namespace mf
{
namespace
{
// A tiny visitor so saving and loading use one list of fields.
struct Writer
{
    juce::XmlElement& x;
    void f (const char* k, float v)  { x.setAttribute (k, (double) v); }
    void f (const char* k, double v) { x.setAttribute (k, v); }
    void f (const char* k, int v)    { x.setAttribute (k, v); }
    void f (const char* k, bool v)   { x.setAttribute (k, v ? 1 : 0); }
};
struct Reader
{
    const juce::XmlElement& x;
    void f (const char* k, float& v)  { if (x.hasAttribute (k)) v = (float) x.getDoubleAttribute (k); }
    void f (const char* k, double& v) { if (x.hasAttribute (k)) v = x.getDoubleAttribute (k); }
    void f (const char* k, int& v)    { if (x.hasAttribute (k)) v = x.getIntAttribute (k); }
    void f (const char* k, bool& v)   { if (x.hasAttribute (k)) v = x.getIntAttribute (k) != 0; }
};

template <typename V, typename S> void visitSynth (V& v, S& s)
{
    v.f ("wave1", s.wave1); v.f ("wave2", s.wave2); v.f ("osc2Semis", s.osc2Semis); v.f ("osc2Detune", s.osc2Detune);
    v.f ("oscMix", s.oscMix); v.f ("sub", s.subLevel); v.f ("noise", s.noiseLevel); v.f ("pw", s.pulseWidth);
    v.f ("pwmDepth", s.pwmDepth); v.f ("pwmRate", s.pwmRate); v.f ("fmIndex", s.fmIndex); v.f ("fmRatio", s.fmRatio);
    v.f ("fmDecay", s.fmDecay); v.f ("filterType", s.filterType); v.f ("cutoff", s.cutoff); v.f ("res", s.resonance);
    v.f ("envAmt", s.envAmount); v.f ("keyTrack", s.keyTrack); v.f ("ampA", s.ampA); v.f ("ampD", s.ampD);
    v.f ("ampS", s.ampS); v.f ("ampR", s.ampR); v.f ("filA", s.filA); v.f ("filD", s.filD); v.f ("filS", s.filS);
    v.f ("filR", s.filR); v.f ("vibRate", s.vibratoRate); v.f ("vibDepth", s.vibratoDepth); v.f ("drift", s.drift);
    v.f ("level", s.level); v.f ("unison", s.unison); v.f ("unisonSpread", s.unisonSpread);
    v.f ("glide", s.glide); v.f ("sync", s.sync);
}
template <typename V, typename S> void visitSampler (V& v, S& s)
{
    v.f ("mode", s.mode); v.f ("root", s.rootNote); v.f ("start", s.start); v.f ("slices", s.slices);
    v.f ("scan", s.scanSpeed); v.f ("tone", s.tone); v.f ("attack", s.attack); v.f ("release", s.release);
    v.f ("pitchFollow", s.pitchFollow); v.f ("reverse", s.reverse); v.f ("level", s.level);
}
template <typename V, typename D> void visitDoll (V& v, D& d)
{
    v.f ("source", d.source); v.f ("octave", d.octave); v.f ("transform", d.transform); v.f ("gate", d.gate);
    v.f ("chord", d.chordSize); v.f ("legato", d.legato); v.f ("volume", d.volume); v.f ("pan", d.pan);
    v.f ("drive", d.drive); v.f ("chorus", d.chorus); v.f ("wow", d.wow); v.f ("reverbSend", d.reverbSend);
    v.f ("echoSend", d.echoSend); v.f ("mute", d.mute); v.f ("solo", d.solo);
    v.f ("ensemble", d.ensemble); v.f ("crush", d.crush); v.f ("age", d.age);
}
template <typename V, typename P> void visitProject (V& v, P& p)
{
    v.f ("bpm", p.bpm); v.f ("root", p.root); v.f ("scale", p.scaleIndex); v.f ("ratio", p.ratio);
    v.f ("pulseBeats", p.pulseBeats); v.f ("answer", p.answerInterval); v.f ("coupling", p.coupling);
    v.f ("fold", p.fold); v.f ("stagger", p.staggerEntries); v.f ("entryGap", p.entryGapBeats);
    v.f ("subjectLength", p.subjectLength); v.f ("reverbSize", p.reverbSize); v.f ("reverbLevel", p.reverbLevel);
    v.f ("echoBeats", p.echoBeats); v.f ("echoFeedback", p.echoFeedback); v.f ("master", p.masterVolume);
}
} // namespace

std::unique_ptr<juce::XmlElement> projectToXml (const Project& p)
{
    auto root = std::make_unique<juce::XmlElement> ("MatryoshkaFugue");
    root->setAttribute ("version", 1);
    Writer w { *root };
    visitProject (w, p);

    auto* subj = root->createNewChildElement ("Subject");
    for (int i = 0; i < kMaxSteps; ++i)
    {
        auto* s = subj->createNewChildElement ("Step");
        const auto& st = p.subject[(size_t) i];
        s->setAttribute ("degree", st.degree);
        s->setAttribute ("length", st.length);
        s->setAttribute ("velocity", (double) st.velocity);
        s->setAttribute ("on", st.on ? 1 : 0);
    }
    for (int d = 0; d < kNumDolls; ++d)
    {
        auto* de = root->createNewChildElement ("Doll");
        de->setAttribute ("index", d);
        Writer dw { *de };
        visitDoll (dw, p.dolls[(size_t) d]);
        auto* se = de->createNewChildElement ("Synth");
        Writer sw { *se };
        visitSynth (sw, p.dolls[(size_t) d].synth);
        auto* sa = de->createNewChildElement ("Sampler");
        Writer aw { *sa };
        visitSampler (aw, p.dolls[(size_t) d].sampler);
    }
    return root;
}

void projectFromXml (const juce::XmlElement& xml, Project& p)
{
    Reader r { xml };
    visitProject (r, p);
    p.subjectLength = juce::jlimit (1, kMaxSteps, p.subjectLength);
    p.scaleIndex = juce::jmax (0, p.scaleIndex);

    if (auto* subj = xml.getChildByName ("Subject"))
    {
        int i = 0;
        for (auto* s : subj->getChildWithTagNameIterator ("Step"))
        {
            if (i >= kMaxSteps) break;
            auto& st = p.subject[(size_t) i++];
            st.degree = s->getIntAttribute ("degree", st.degree);
            st.length = juce::jlimit (1, 13, s->getIntAttribute ("length", st.length));
            st.velocity = (float) s->getDoubleAttribute ("velocity", st.velocity);
            st.on = s->getIntAttribute ("on", st.on ? 1 : 0) != 0;
        }
    }
    for (auto* de : xml.getChildWithTagNameIterator ("Doll"))
    {
        const int d = de->getIntAttribute ("index", -1);
        if (d < 0 || d >= kNumDolls) continue;
        auto& doll = p.dolls[(size_t) d];
        Reader dr { *de };
        visitDoll (dr, doll);
        if (auto* se = de->getChildByName ("Synth")) { Reader sr { *se }; visitSynth (sr, doll.synth); }
        if (auto* sa = de->getChildByName ("Sampler")) { Reader ar { *sa }; visitSampler (ar, doll.sampler); }
    }
}
} // namespace mf
