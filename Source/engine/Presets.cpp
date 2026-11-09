#include "Presets.h"

namespace mf
{
namespace
{
struct Fx { float drive = 0, chorus = 0, wow = 0; bool ensemble = false; float crush = 0, age = 0; };

template <typename Fn>
SynthPreset make (const char* name, const char* era, const char* note, Fx fx, Fn&& edit)
{
    SynthPreset p;
    p.name = name; p.era = era; p.note = note;
    p.drive = fx.drive; p.chorus = fx.chorus; p.wow = fx.wow;
    p.ensemble = fx.ensemble; p.crush = fx.crush; p.age = fx.age;
    edit (p.synth);
    return p;
}
} // namespace

// Sound design notes: Soviet studios and stages had a distinct palette. Photo-optical and valve
// instruments in the 50s-60s (ANS, Ekvodin, the theremin), transistor combo organs and string
// machines in the 70s estrada and VIA bands, the Moscow electronic studio's Synthi 100, and in the
// 80s the home-grown Polivoks, Aelita and Estradin monosynths alongside imported FM keyboards.
// Tape age, wow and hiss are part of how that music reached listeners, so they are part of the presets.
const std::vector<SynthPreset>& synthPresets()
{
    static const std::vector<SynthPreset> presets {
        // ------------------------------------------------------------------ 1960s
        make ("Termenvox Theremin", "1960s", "Leon Theremin's termenvox: a pure, gliding voice with a singer's vibrato.",
              Fx { 0.0f, 0.0f, 0.12f, false, 0.0f, 0.3f }, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Triangle; s.oscMix = 0.18f; s.osc2Detune = 0;
            s.filterType = FilterType::SEM12; s.cutoff = 5000; s.resonance = 0.05f; s.envAmount = 0;
            s.ampA = 0.14f; s.ampD = 0.2f; s.ampS = 0.95f; s.ampR = 0.45f;
            s.vibratoRate = 6.1f; s.vibratoDepth = 26; s.drift = 7; s.glide = 0.16f; s.level = 0.75f; }),
        make ("ANS Photo-Cluster", "1960s", "Murzin's photoelectronic ANS (named after Scriabin): dense banks of sine tones drawn on glass.",
              Fx { 0.0f, 0.35f, 0.08f, false, 0.0f, 0.25f }, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Sine; s.osc2Semis = 7; s.osc2Detune = 0; s.oscMix = 0.35f;
            s.unison = 7; s.unisonSpread = 38; s.noiseLevel = 0.015f;
            s.filterType = FilterType::SEM12; s.cutoff = 3200; s.resonance = 0.1f; s.envAmount = 0.4f;
            s.filA = 1.2f; s.filD = 2.5f; s.filS = 0.6f; s.filR = 2.0f;
            s.ampA = 0.9f; s.ampD = 1.5f; s.ampS = 0.85f; s.ampR = 2.2f; s.drift = 9; s.level = 0.7f; }),
        make ("Ekvodin Film Lead", "1960s", "Volodin's Ekvodin, heard in early-60s Soviet film scores: reedy pulse tone, valve vibrato, tape.",
              Fx { 0.15f, 0.0f, 0.18f, false, 0.0f, 0.45f }, [] (SynthParams& s) {
            s.wave1 = Wave::Pulse; s.wave2 = Wave::Saw; s.osc2Semis = 12; s.oscMix = 0.25f; s.pulseWidth = 0.32f; s.pwmDepth = 0.15f; s.pwmRate = 0.3f;
            s.filterType = FilterType::SEM12; s.cutoff = 2200; s.resonance = 0.3f; s.envAmount = 0.6f; s.keyTrack = 0.7f;
            s.ampA = 0.04f; s.ampD = 0.3f; s.ampS = 0.85f; s.ampR = 0.3f;
            s.vibratoRate = 5.6f; s.vibratoDepth = 14; s.drift = 6; s.glide = 0.04f; s.level = 0.65f; }),
        make ("Gosteleradio Tape Strings", "1960s", "State radio orchestra strings bounced to tape: soft saws, wow and flutter, worn top end.",
              Fx { 0.1f, 0.5f, 0.55f, true, 0.0f, 0.6f }, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Detune = 9; s.oscMix = 0.5f; s.unison = 3; s.unisonSpread = 14;
            s.filterType = FilterType::SEM12; s.cutoff = 2100; s.resonance = 0.08f; s.envAmount = 0.5f;
            s.filA = 0.3f; s.filD = 1.0f; s.filS = 0.6f;
            s.ampA = 0.25f; s.ampD = 0.6f; s.ampS = 0.85f; s.ampR = 0.7f; s.drift = 6; s.level = 0.55f; }),
        make ("Sputnik Telemetry", "1960s", "Space-race beeps: short square blips with a little FM shimmer.",
              Fx { 0.0f, 0.0f, 0.05f, false, 0.0f, 0.35f }, [] (SynthParams& s) {
            s.wave1 = Wave::Square; s.wave2 = Wave::Sine; s.osc2Semis = 12; s.oscMix = 0.3f; s.fmIndex = 1.5f; s.fmRatio = 2.0f; s.fmDecay = 0.08f;
            s.filterType = FilterType::SEM12; s.cutoff = 4200; s.resonance = 0.2f; s.envAmount = 0;
            s.ampA = 0.001f; s.ampD = 0.09f; s.ampS = 0.0f; s.ampR = 0.05f; s.drift = 2; s.level = 0.6f; }),
        // ------------------------------------------------------------------ 1970s
        make ("Junost Combo Organ", "1970s", "Soviet transistor 'divide-down' combo organ of the estrada and VIA bands, through a rotary-style chorus.",
              Fx { 0.25f, 0.3f, 0.0f, false, 0.0f, 0.2f }, [] (SynthParams& s) {
            s.wave1 = Wave::Organ; s.wave2 = Wave::Square; s.osc2Semis = 12; s.oscMix = 0.2f;
            s.filterType = FilterType::SEM12; s.cutoff = 4500; s.resonance = 0.05f; s.envAmount = 0;
            s.ampA = 0.004f; s.ampD = 0.1f; s.ampS = 1.0f; s.ampR = 0.06f;
            s.vibratoRate = 6.6f; s.vibratoDepth = 7; s.drift = 1; s.level = 0.5f; }),
        make ("VIA String Ensemble", "1970s", "The string machine of 70s Soviet pop bands: divide-down saws through a three-phase ensemble.",
              Fx { 0.0f, 0.8f, 0.0f, true, 0.0f, 0.15f }, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Semis = 12; s.oscMix = 0.35f; s.unison = 2; s.unisonSpread = 6;
            s.filterType = FilterType::SEM12; s.cutoff = 3400; s.resonance = 0.05f; s.envAmount = 0.1f;
            s.ampA = 0.3f; s.ampD = 0.5f; s.ampS = 0.9f; s.ampR = 0.9f; s.drift = 2; s.level = 0.5f; }),
        make ("Synthi Stalker Drone", "1970s", "Moscow Electronic Music Studio, late 70s: noise and saws in a slowly swept resonant band.",
              Fx { 0.1f, 0.2f, 0.15f, false, 0.0f, 0.3f }, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Sine; s.osc2Semis = -12; s.oscMix = 0.4f; s.noiseLevel = 0.25f; s.unison = 3; s.unisonSpread = 20;
            s.filterType = FilterType::BandPass; s.cutoff = 700; s.resonance = 0.6f; s.envAmount = 1.5f; s.keyTrack = 0.3f;
            s.filA = 2.0f; s.filD = 3.0f; s.filS = 0.3f; s.filR = 2.0f;
            s.ampA = 1.4f; s.ampD = 2.0f; s.ampS = 0.85f; s.ampR = 2.5f; s.vibratoRate = 0.2f; s.vibratoDepth = 12; s.drift = 12; s.level = 0.8f; }),
        make ("Solaris Organ Halo", "1970s", "Organ chorale dissolving into electronic haze, in the spirit of the Solaris soundtrack.",
              Fx { 0.0f, 0.4f, 0.2f, true, 0.0f, 0.35f }, [] (SynthParams& s) {
            s.wave1 = Wave::Organ; s.wave2 = Wave::Sine; s.osc2Semis = 19; s.oscMix = 0.25f; s.unison = 5; s.unisonSpread = 22;
            s.filterType = FilterType::SEM12; s.cutoff = 2600; s.resonance = 0.1f; s.envAmount = 0.3f;
            s.ampA = 0.8f; s.ampD = 1.0f; s.ampS = 0.9f; s.ampR = 2.0f; s.drift = 5; s.level = 0.55f; }),
        make ("Kosmos Mono Lead", "1970s", "Space-age monosynth lead with a touch of glide, the sound of Soviet sci-fi broadcasts.",
              Fx { 0.3f, 0.15f, 0.0f, false, 0.0f, 0.15f }, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Detune = 8; s.oscMix = 0.5f; s.subLevel = 0.2f;
            s.filterType = FilterType::Ladder24; s.cutoff = 950; s.resonance = 0.4f; s.envAmount = 2.4f;
            s.filA = 0.01f; s.filD = 0.5f; s.filS = 0.35f; s.filR = 0.3f;
            s.ampA = 0.005f; s.ampD = 0.4f; s.ampS = 0.8f; s.ampR = 0.25f; s.vibratoRate = 5.2f; s.vibratoDepth = 6; s.drift = 4; s.glide = 0.06f; }),
        // ------------------------------------------------------------------ 1980s
        make ("Polivoks Bass", "1980s", "Kuzmin's Polivoks (1982): a raw two-op-amp 12 dB filter that snarls when pushed.",
              Fx { 0.55f, 0.0f, 0.0f, false, 0.0f, 0.0f }, [] (SynthParams& s) {
            s.wave1 = Wave::Square; s.wave2 = Wave::Saw; s.osc2Semis = -12; s.osc2Detune = 5; s.oscMix = 0.45f;
            s.filterType = FilterType::Polivoks; s.cutoff = 420; s.resonance = 0.65f; s.envAmount = 3.4f;
            s.filA = 0.001f; s.filD = 0.2f; s.filS = 0.12f; s.filR = 0.15f;
            s.ampA = 0.002f; s.ampD = 0.3f; s.ampS = 0.65f; s.ampR = 0.12f; s.drift = 6; s.level = 0.65f; }),
        make ("Polivoks Scream Lead", "1980s", "The Polivoks filter at the edge of self-oscillation, with hard sync and a quick glide.",
              Fx { 0.6f, 0.0f, 0.0f, false, 0.0f, 0.0f }, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Semis = 7; s.sync = true; s.oscMix = 0.6f;
            s.filterType = FilterType::Polivoks; s.cutoff = 900; s.resonance = 0.85f; s.envAmount = 2.6f;
            s.filA = 0.01f; s.filD = 0.35f; s.filS = 0.3f; s.filR = 0.2f;
            s.ampA = 0.005f; s.ampD = 0.3f; s.ampS = 0.8f; s.ampR = 0.2f; s.drift = 5; s.glide = 0.05f; s.level = 0.5f; }),
        make ("Aelita Brass", "1980s", "Murom's Aelita (1980): stacked oscillators through a ladder filter with a brassy swell.",
              Fx { 0.2f, 0.25f, 0.0f, false, 0.0f, 0.05f }, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Detune = 10; s.oscMix = 0.5f; s.unison = 3; s.unisonSpread = 10;
            s.filterType = FilterType::Ladder24; s.cutoff = 650; s.resonance = 0.15f; s.envAmount = 2.6f;
            s.filA = 0.09f; s.filD = 0.6f; s.filS = 0.45f; s.filR = 0.3f;
            s.ampA = 0.06f; s.ampD = 0.4f; s.ampS = 0.8f; s.ampR = 0.35f; s.drift = 4; s.level = 0.55f; }),
        make ("Estradin Fat Bass", "1980s", "Estradin-230 / Altair-231 three-oscillator mono: heavy low end and a sub.",
              Fx { 0.35f, 0.0f, 0.0f, false, 0.0f, 0.0f }, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Square; s.osc2Semis = -12; s.osc2Detune = 4; s.oscMix = 0.45f; s.unison = 2; s.unisonSpread = 7; s.subLevel = 0.35f;
            s.filterType = FilterType::Ladder24; s.cutoff = 360; s.resonance = 0.45f; s.envAmount = 3.0f;
            s.filA = 0.002f; s.filD = 0.28f; s.filS = 0.15f; s.filR = 0.2f;
            s.ampA = 0.003f; s.ampD = 0.4f; s.ampS = 0.75f; s.ampR = 0.15f; s.drift = 4; s.level = 0.7f; }),
        make ("Estrada FM Piano", "1980s", "The imported FM electric piano that defined 80s Soviet estrada ballads.",
              Fx { 0.0f, 0.35f, 0.0f, false, 0.0f, 0.0f }, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Sine; s.osc2Detune = 5; s.oscMix = 0.3f;
            s.fmIndex = 3.2f; s.fmRatio = 1.0f; s.fmDecay = 0.45f;
            s.filterType = FilterType::SEM12; s.cutoff = 9000; s.envAmount = 0;
            s.ampA = 0.002f; s.ampD = 1.4f; s.ampS = 0.15f; s.ampR = 0.5f; s.drift = 0.5f; s.level = 0.65f; }),
        make ("Estrada FM Bells", "1980s", "Glassy FM bells over the estrada string pads.",
              Fx { 0.0f, 0.25f, 0.0f, false, 0.0f, 0.0f }, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Sine; s.osc2Semis = 12; s.oscMix = 0.25f;
            s.fmIndex = 4.5f; s.fmRatio = 3.5f; s.fmDecay = 1.2f;
            s.filterType = FilterType::SEM12; s.cutoff = 12000; s.envAmount = 0;
            s.ampA = 0.001f; s.ampD = 2.2f; s.ampS = 0.0f; s.ampR = 1.6f; s.drift = 0; s.level = 0.55f; }),
        make ("Leningrad Chorus Bass", "1980s", "Post-punk new-wave bass of the mid-80s rock clubs: pulse wave, wide chorus.",
              Fx { 0.15f, 0.6f, 0.0f, false, 0.0f, 0.05f }, [] (SynthParams& s) {
            s.wave1 = Wave::Pulse; s.wave2 = Wave::Saw; s.pulseWidth = 0.3f; s.osc2Detune = 5; s.oscMix = 0.4f; s.subLevel = 0.3f;
            s.filterType = FilterType::Ladder24; s.cutoff = 600; s.resonance = 0.25f; s.envAmount = 2.2f;
            s.filA = 0.001f; s.filD = 0.2f; s.filS = 0.25f; s.filR = 0.12f;
            s.ampA = 0.001f; s.ampD = 0.3f; s.ampS = 0.7f; s.ampR = 0.1f; s.drift = 1; s.level = 0.7f; }),
        make ("Elektronika Toy Keys", "1980s", "Cheap home keyboard of the late USSR: thin square voice, gritty low-bit digital.",
              Fx { 0.0f, 0.1f, 0.0f, false, 0.55f, 0.1f }, [] (SynthParams& s) {
            s.wave1 = Wave::Square; s.wave2 = Wave::Pulse; s.pulseWidth = 0.25f; s.osc2Semis = 12; s.oscMix = 0.3f;
            s.filterType = FilterType::SEM12; s.cutoff = 6000; s.resonance = 0.0f; s.envAmount = 0;
            s.ampA = 0.002f; s.ampD = 0.5f; s.ampS = 0.5f; s.ampR = 0.15f; s.vibratoRate = 5.0f; s.vibratoDepth = 4; s.drift = 0; s.level = 0.5f; }),
        make ("Perestroika Synth-Pop Pad", "1980s", "Late-80s synth-pop pad: chorused PWM with a slow filter bloom.",
              Fx { 0.0f, 0.85f, 0.0f, true, 0.0f, 0.0f }, [] (SynthParams& s) {
            s.wave1 = Wave::Pulse; s.wave2 = Wave::Saw; s.pulseWidth = 0.4f; s.pwmDepth = 0.35f; s.pwmRate = 0.7f; s.oscMix = 0.4f; s.subLevel = 0.15f;
            s.filterType = FilterType::Ladder24; s.cutoff = 1500; s.resonance = 0.2f; s.envAmount = 1.0f;
            s.filA = 0.4f; s.filD = 1.0f; s.filS = 0.6f; s.filR = 0.8f;
            s.ampA = 0.3f; s.ampD = 0.8f; s.ampS = 0.85f; s.ampR = 1.0f; s.drift = 2; s.level = 0.5f; }),
    };
    return presets;
}

const SynthPreset* findPreset (const std::string& name)
{
    for (auto& p : synthPresets())
        if (p.name == name)
            return &p;
    return nullptr;
}

void applyPreset (DollSettings& d, const SynthPreset& preset)
{
    d.synth = preset.synth;
    d.drive = preset.drive;
    d.chorus = preset.chorus;
    d.wow = preset.wow;
    d.ensemble = preset.ensemble;
    d.crush = preset.crush;
    d.age = preset.age;
}

// Indices are stored in saved projects, so new samples are only ever appended.
const std::vector<BuiltInSampleInfo>& builtInSamples()
{
    static const std::vector<BuiltInSampleInfo> s {
        { "Balalaika pluck",           "Strings",          57, false },
        { "Kolokol church bell",       "Bells",            48, false },
        { "Choir 'Ah'",                "Voices",           57, false },
        { "Bayan accordion",           "Winds & reeds",    57, false },
        { "Bass balalaika",            "Strings",          40, false },
        { "Domra tremolo",             "Strings",          69, false },
        { "Gusli (zither)",            "Strings",          62, false },
        { "Zhaleika (hornpipe)",       "Winds & reeds",    72, false },
        { "Svirel (shepherd flute)",   "Winds & reeds",    79, false },
        { "Garmon (Tula accordion)",   "Winds & reeds",    64, false },
        { "Volynka (bagpipe drone)",   "Winds & reeds",    57, false },
        { "Oktavist basso 'O'",        "Voices",           38, false },
        { "Zazvonnye small bells",     "Bells",            79, false },
        { "Blagovest great bell",      "Bells",            36, false },
        { "Vargan (jaw harp)",         "Percussion",       50, false },
        { "Lozhki (spoons)",           "Percussion",       60, true },
        { "Buben (frame drum)",        "Percussion",       60, true },
        { "Treshchotka (rattle)",      "Percussion",       60, true },
    };
    return s;
}

int findBuiltInSample (const std::string& name)
{
    const auto& s = builtInSamples();
    for (size_t i = 0; i < s.size(); ++i)
        if (name == s[i].name)
            return (int) i;
    return -1;
}
} // namespace mf
