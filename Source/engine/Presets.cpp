#include "Presets.h"

namespace mf
{
namespace
{
template <typename Fn>
SynthPreset make (const char* name, const char* era, float drive, float chorus, float wow, Fn&& edit)
{
    SynthPreset p;
    p.name = name;
    p.era = era;
    p.drive = drive;
    p.chorus = chorus;
    p.wow = wow;
    edit (p.synth);
    return p;
}
} // namespace

const std::vector<SynthPreset>& synthPresets()
{
    static const std::vector<SynthPreset> presets {
        // ------------------------------------------------------------------ 1960s
        make ("Modular Bass", "1960s", 0.25f, 0.0f, 0.05f, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Square; s.osc2Semis = -12; s.osc2Detune = 4; s.oscMix = 0.45f;
            s.filterType = FilterType::Ladder24; s.cutoff = 380; s.resonance = 0.45f; s.envAmount = 3.2f;
            s.filA = 0.002f; s.filD = 0.28f; s.filS = 0.1f; s.filR = 0.2f;
            s.ampA = 0.003f; s.ampD = 0.4f; s.ampS = 0.75f; s.ampR = 0.15f; s.drift = 5; s.level = 0.75f; }),
        make ("Theremin Wail", "1960s", 0.0f, 0.0f, 0.1f, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Triangle; s.oscMix = 0.2f; s.osc2Detune = 0;
            s.filterType = FilterType::SEM12; s.cutoff = 4000; s.resonance = 0.05f; s.envAmount = 0;
            s.ampA = 0.12f; s.ampD = 0.2f; s.ampS = 0.95f; s.ampR = 0.5f;
            s.vibratoRate = 6.2f; s.vibratoDepth = 28; s.drift = 8; s.level = 0.7f; }),
        make ("Tape-Loop Strings", "1960s", 0.1f, 0.25f, 0.55f, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Detune = 11; s.oscMix = 0.5f; s.noiseLevel = 0.04f;
            s.filterType = FilterType::SEM12; s.cutoff = 2400; s.resonance = 0.1f; s.envAmount = 0.6f;
            s.filA = 0.2f; s.filD = 1.0f; s.filS = 0.6f;
            s.ampA = 0.18f; s.ampD = 0.6f; s.ampS = 0.85f; s.ampR = 0.6f; s.drift = 6; s.level = 0.6f; }),
        make ("Combo Organ", "1960s", 0.3f, 0.15f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Square; s.wave2 = Wave::Square; s.osc2Semis = 12; s.osc2Detune = 2; s.oscMix = 0.4f; s.subLevel = 0.3f;
            s.filterType = FilterType::SEM12; s.cutoff = 3200; s.resonance = 0.1f; s.envAmount = 0;
            s.ampA = 0.004f; s.ampD = 0.1f; s.ampS = 1.0f; s.ampR = 0.06f;
            s.vibratoRate = 6.8f; s.vibratoDepth = 9; s.drift = 1; s.level = 0.55f; }),
        // ------------------------------------------------------------------ 1970s
        make ("Mono Lead", "1970s", 0.35f, 0.0f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Detune = 9; s.oscMix = 0.5f; s.subLevel = 0.2f;
            s.filterType = FilterType::Ladder24; s.cutoff = 900; s.resonance = 0.35f; s.envAmount = 2.6f;
            s.filA = 0.01f; s.filD = 0.5f; s.filS = 0.35f; s.filR = 0.3f;
            s.ampA = 0.005f; s.ampD = 0.4f; s.ampS = 0.8f; s.ampR = 0.25f; s.vibratoRate = 5.2f; s.vibratoDepth = 6; s.drift = 4; }),
        make ("String Ensemble", "1970s", 0.0f, 0.75f, 0.05f, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Pulse; s.osc2Semis = 12; s.pulseWidth = 0.35f; s.pwmDepth = 0.25f; s.pwmRate = 0.4f; s.oscMix = 0.35f;
            s.filterType = FilterType::SEM12; s.cutoff = 3000; s.resonance = 0.05f; s.envAmount = 0.2f;
            s.ampA = 0.35f; s.ampD = 0.5f; s.ampS = 0.9f; s.ampR = 0.9f; s.drift = 3; s.level = 0.55f; }),
        make ("Poly Brass", "1970s", 0.2f, 0.2f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Saw; s.wave2 = Wave::Saw; s.osc2Detune = 12; s.oscMix = 0.5f;
            s.filterType = FilterType::Ladder24; s.cutoff = 700; s.resonance = 0.15f; s.envAmount = 2.4f;
            s.filA = 0.08f; s.filD = 0.6f; s.filS = 0.45f; s.filR = 0.3f;
            s.ampA = 0.06f; s.ampD = 0.4f; s.ampS = 0.8f; s.ampR = 0.35f; s.drift = 4; s.level = 0.6f; }),
        make ("Polivoks Snarl", "1970s", 0.6f, 0.0f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Square; s.wave2 = Wave::Saw; s.osc2Semis = 7; s.osc2Detune = 6; s.oscMix = 0.4f;
            s.filterType = FilterType::Polivoks; s.cutoff = 600; s.resonance = 0.7f; s.envAmount = 3.5f;
            s.filA = 0.001f; s.filD = 0.22f; s.filS = 0.15f; s.filR = 0.2f;
            s.ampA = 0.002f; s.ampD = 0.3f; s.ampS = 0.6f; s.ampR = 0.15f; s.drift = 7; s.level = 0.6f; }),
        make ("ANS Photo-Drone", "1970s", 0.0f, 0.4f, 0.2f, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Sine; s.osc2Semis = 7; s.osc2Detune = 14; s.oscMix = 0.45f; s.noiseLevel = 0.03f;
            s.filterType = FilterType::BandPass; s.cutoff = 1200; s.resonance = 0.3f; s.envAmount = 0.8f;
            s.filA = 1.0f; s.filD = 2.0f; s.filS = 0.5f; s.filR = 1.5f;
            s.ampA = 0.6f; s.ampD = 1.0f; s.ampS = 0.9f; s.ampR = 1.6f; s.vibratoRate = 0.3f; s.vibratoDepth = 10; s.drift = 10; s.level = 0.75f; }),
        // ------------------------------------------------------------------ 1980s
        make ("FM Tine Piano", "1980s", 0.0f, 0.3f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Sine; s.osc2Semis = 0; s.osc2Detune = 4; s.oscMix = 0.3f;
            s.fmIndex = 3.2f; s.fmRatio = 1.0f; s.fmDecay = 0.45f;
            s.filterType = FilterType::SEM12; s.cutoff = 9000; s.resonance = 0.0f; s.envAmount = 0;
            s.ampA = 0.002f; s.ampD = 1.4f; s.ampS = 0.15f; s.ampR = 0.5f; s.drift = 0.5f; s.level = 0.65f; }),
        make ("FM Glass Bell", "1980s", 0.0f, 0.2f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Sine; s.wave2 = Wave::Sine; s.osc2Semis = 12; s.oscMix = 0.25f;
            s.fmIndex = 4.5f; s.fmRatio = 3.5f; s.fmDecay = 1.2f;
            s.filterType = FilterType::SEM12; s.cutoff = 12000; s.envAmount = 0;
            s.ampA = 0.001f; s.ampD = 2.2f; s.ampS = 0.0f; s.ampR = 1.6f; s.drift = 0; s.level = 0.55f; }),
        make ("Chorus Pad", "1980s", 0.0f, 0.85f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Pulse; s.wave2 = Wave::Saw; s.pulseWidth = 0.4f; s.pwmDepth = 0.35f; s.pwmRate = 0.7f; s.oscMix = 0.4f; s.subLevel = 0.15f;
            s.filterType = FilterType::Ladder24; s.cutoff = 1500; s.resonance = 0.2f; s.envAmount = 1.0f;
            s.filA = 0.4f; s.filD = 1.0f; s.filS = 0.6f; s.filR = 0.8f;
            s.ampA = 0.3f; s.ampD = 0.8f; s.ampS = 0.85f; s.ampR = 1.0f; s.drift = 2; s.level = 0.55f; }),
        make ("Synth-Pop Bass", "1980s", 0.15f, 0.15f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Pulse; s.wave2 = Wave::Saw; s.pulseWidth = 0.3f; s.osc2Detune = 5; s.oscMix = 0.5f; s.subLevel = 0.4f;
            s.filterType = FilterType::Ladder24; s.cutoff = 500; s.resonance = 0.3f; s.envAmount = 2.8f;
            s.filA = 0.001f; s.filD = 0.18f; s.filS = 0.05f; s.filR = 0.12f;
            s.ampA = 0.001f; s.ampD = 0.25f; s.ampS = 0.6f; s.ampR = 0.1f; s.drift = 1; s.level = 0.7f; }),
        make ("Digital Hoover Lead", "1980s", 0.25f, 0.6f, 0.0f, [] (SynthParams& s) {
            s.wave1 = Wave::Pulse; s.wave2 = Wave::Saw; s.pulseWidth = 0.2f; s.pwmDepth = 0.4f; s.pwmRate = 2.5f; s.osc2Semis = -12; s.osc2Detune = 18; s.oscMix = 0.5f;
            s.filterType = FilterType::SEM12; s.cutoff = 2600; s.resonance = 0.25f; s.envAmount = 1.0f;
            s.ampA = 0.01f; s.ampD = 0.5f; s.ampS = 0.8f; s.ampR = 0.3f; s.vibratoRate = 5.5f; s.vibratoDepth = 12; s.drift = 2; s.level = 0.55f; }),
    };
    return presets;
}

void applyPreset (DollSettings& d, const SynthPreset& preset)
{
    d.synth = preset.synth;
    d.drive = preset.drive;
    d.chorus = preset.chorus;
    d.wow = preset.wow;
}

const std::vector<BuiltInSampleInfo>& builtInSamples()
{
    static const std::vector<BuiltInSampleInfo> s {
        { "Balalaika pluck", 57 },
        { "Kolokol church bell", 48 },
        { "Choir 'Ah'", 57 },
        { "Bayan accordion", 57 },
    };
    return s;
}
} // namespace mf
