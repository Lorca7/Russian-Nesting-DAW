#include "FugueEngine.h"

namespace mf
{
using namespace dsp;

// ===================================================================================== DollTrack
void DollTrack::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    bus.assign ((size_t) std::max (1, maxBlock), 0.0f);
    for (auto& v : synths) v.prepare (sampleRate);
    for (auto& v : samplers) v.prepare (sampleRate);
    wowLine.prepare ((int) (sampleRate * 0.04));
    chorusLine.prepare ((int) (sampleRate * 0.04));
    reset();
}

void DollTrack::reset()
{
    for (auto& v : synths) v.kill();
    for (auto& v : samplers) v.kill();
    wowLine.clear();
    chorusLine.clear();
    scrubPos = 0.0;
    lastSynthHz = 0;
    ageLp = ageEnv = crushHold = 0;
    crushCount = 0;
}

int DollTrack::activeVoices() const
{
    int n = 0;
    for (auto& v : synths) n += v.isActive() ? 1 : 0;
    for (auto& v : samplers) n += v.isActive() ? 1 : 0;
    return n;
}

template <typename Array>
static auto& pickVoice (Array& voices)
{
    for (auto& v : voices)
        if (! v.isActive())
            return v;
    auto* oldest = &voices[0];
    for (auto& v : voices)
        if (v.order < oldest->order)
            oldest = &v;
    return *oldest;
}

void DollTrack::trigger (const Project& p, int doll, const SampleData* sample, const NoteEvent& e, double samplesPerBeat)
{
    const auto& d = p.dolls[(size_t) doll];
    const int gate = (int) std::max (1.0, e.length * samplesPerBeat);
    const float chordGain = 1.0f / std::sqrt ((float) std::max (1, e.numNotes));

    for (int c = 0; c < e.numNotes; ++c)
    {
        if (d.source == Source::Synth)
        {
            auto& v = pickVoice (synths);
            v.order = ++orderCounter;
            const float from = (e.numNotes == 1 && d.synth.glide > 0.0005f) ? lastSynthHz : 0.0f;
            v.start (d.synth, e.notes[c], e.velocity * chordGain, gate, seedCounter++, from);
            if (c == 0) lastSynthHz = dsp::midiToHz ((float) e.notes[0]);
        }
        else
        {
            const auto& sp = d.sampler;
            double startFrac = sp.start, endFrac = 0.0;
            if (sp.mode == SampleMode::Slice)
            {
                const int slices = std::max (1, sp.slices);
                int idx = (e.fractalDegree - p.subject[0].degree + c * 2) % slices;
                if (idx < 0) idx += slices;
                startFrac = (double) idx / slices;
                endFrac = (double) (idx + 1) / slices;
            }
            else if (sp.mode == SampleMode::Scrub)
            {
                startFrac = scrubPos;
            }
            auto& v = pickVoice (samplers);
            v.order = ++orderCounter;
            v.start (sample, sp, e.notes[c], e.velocity * chordGain, gate, startFrac, endFrac);
        }
    }
}

void DollTrack::renderVoices (int start, int num)
{
    if (num <= 0) return;
    float* out = bus.data() + start;
    for (auto& v : synths) v.render (out, num);
    for (auto& v : samplers) v.render (out, num);
}

void DollTrack::processFx (const DollSettings& d, bool audible, int num,
                           float* L, float* R, float* revL, float* revR, float* echoL, float* echoR)
{
    const float target = audible ? d.volume : 0.0f;
    const float smooth = 1.0f - std::exp (-1.0f / (0.01f * (float) sr));
    const float panAngle = (std::clamp (d.pan, -1.0f, 1.0f) + 1.0f) * pi * 0.25f;
    const float panL = std::cos (panAngle), panR = std::sin (panAngle);
    const float driveGain = 1.0f + d.drive * 8.0f;
    const float msec = (float) sr * 0.001f;
    const float wowInc = 0.55f / (float) sr, flutterInc = 6.5f / (float) sr, chorusInc = (d.ensemble ? 0.6f : 0.45f) / (float) sr;
    const float vibInc = 6.3f / (float) sr;
    const int crushHoldLen = 1 + (int) (d.crush * 11.0f);
    const float crushSteps = std::pow (2.0f, 15.0f - d.crush * 11.0f);
    const float ageCoef = 1.0f - std::exp (-twoPi * (18000.0f - d.age * 14500.0f) / (float) sr);
    const float envRel = std::exp (-1.0f / (0.15f * (float) sr));
    constexpr float third = 1.0f / 3.0f;

    for (int i = 0; i < num; ++i)
    {
        float x = bus[(size_t) i];

        if (d.drive > 0.001f)
            x = (1.0f - d.drive) * x + d.drive * 0.7f * fastTanh (x * driveGain);

        if (d.crush > 0.001f)
        {
            if (crushCount++ % crushHoldLen == 0)
                crushHold = std::round (x * crushSteps) / crushSteps;
            x = crushHold;
        }

        if (d.age > 0.001f)
        {
            ageEnv = std::max (std::abs (x), ageEnv * envRel);
            x = fastTanh (x * (1.0f + d.age)) / (1.0f + 0.5f * d.age);
            x += hissRng.bipolar() * d.age * 0.06f * ageEnv;
            ageLp += ageCoef * (x - ageLp);
            x = ageLp;
        }

        wowLine.push (x);
        if (d.wow > 0.001f)
        {
            const float dl = (7.0f + d.wow * (3.5f * std::sin (twoPi * wowPhase) + 0.6f * std::sin (twoPi * flutterPhase))) * msec;
            x = wowLine.read (dl);
        }
        wowPhase += wowInc; wowPhase -= std::floor (wowPhase);
        flutterPhase += flutterInc; flutterPhase -= std::floor (flutterPhase);

        chorusLine.push (x);
        float l = x, r = x;
        if (d.chorus > 0.001f && d.ensemble)
        {
            // String-machine ensemble: three delay taps, each swept by a slow and a fast LFO 120 degrees apart.
            float taps[3];
            for (int k = 0; k < 3; ++k)
            {
                const float off = (float) k * third;
                const float slow = std::sin (twoPi * (chorusPhase + off)), fast = std::sin (twoPi * (vibPhase + off));
                taps[k] = chorusLine.read ((7.0f + 2.6f * slow + 0.3f * fast) * msec);
            }
            const float dry = 1.0f - 0.6f * d.chorus;
            l = dry * x + d.chorus * 0.7f * (taps[0] + 0.5f * taps[1]);
            r = dry * x + d.chorus * 0.7f * (taps[2] + 0.5f * taps[1]);
        }
        else if (d.chorus > 0.001f)
        {
            const float a = std::sin (twoPi * chorusPhase), b = std::cos (twoPi * chorusPhase);
            const float wl = chorusLine.read ((9.0f + 3.0f * a) * msec);
            const float wr = chorusLine.read ((9.0f + 3.0f * b) * msec);
            const float dry = 1.0f - 0.45f * d.chorus;
            l = dry * x + d.chorus * 0.8f * wl;
            r = dry * x + d.chorus * 0.8f * wr;
        }
        chorusPhase += chorusInc; chorusPhase -= std::floor (chorusPhase);
        vibPhase += vibInc; vibPhase -= std::floor (vibPhase);

        gainSmoothed += (target - gainSmoothed) * smooth;
        l *= gainSmoothed * panL * 1.41f;
        r *= gainSmoothed * panR * 1.41f;

        L[i] += l; R[i] += r;
        revL[i] += l * d.reverbSend; revR[i] += r * d.reverbSend;
        echoL[i] += l * d.echoSend; echoR[i] += r * d.echoSend;
    }
}

// ===================================================================================== FugueEngine
void FugueEngine::prepare (double sampleRate, int block)
{
    sr = sampleRate;
    maxBlock = std::max (1, block);
    for (auto& t : tracks) t.prepare (sampleRate, maxBlock);
    revL.assign ((size_t) maxBlock, 0.0f); revR = revL; echoL = revL; echoR = revL;
    reverb.setSampleRate (sampleRate);
    echoLineL.prepare ((int) (sampleRate * 4.1));
    echoLineR.prepare ((int) (sampleRate * 4.1));
    reset();
}

void FugueEngine::reset()
{
    for (auto& t : tracks) t.reset();
    reverb.reset();
    echoLineL.clear(); echoLineR.clear();
    echoDampL = echoDampR = 0;
    limiterEnv = 0;
}

void FugueEngine::process (const Project& p, float* L, float* R, int num, bool playing, double& beat, Listener* listener)
{
    // Large host blocks are processed in chunks of maxBlock.
    if (num > maxBlock)
    {
        for (int off = 0; off < num; off += maxBlock)
            process (p, L + off, R + off, std::min (maxBlock, num - off), playing, beat, listener);
        return;
    }

    std::fill (L, L + num, 0.0f);
    std::fill (R, R + num, 0.0f);
    std::fill (revL.begin(), revL.begin() + num, 0.0f);
    std::fill (revR.begin(), revR.begin() + num, 0.0f);
    std::fill (echoL.begin(), echoL.begin() + num, 0.0f);
    std::fill (echoR.begin(), echoR.begin() + num, 0.0f);

    const double bpm = std::clamp (p.bpm, 20.0, 400.0);
    const double samplesPerBeat = sr * 60.0 / bpm;
    const double b0 = beat, b1 = beat + num / samplesPerBeat;

    bool anySolo = false;
    for (auto& d : p.dolls) anySolo = anySolo || d.solo;

    for (int doll = 0; doll < kNumDolls; ++doll)
    {
        auto& track = tracks[(size_t) doll];
        const auto& ds = p.dolls[(size_t) doll];
        const bool audible = ! ds.mute && (! anySolo || ds.solo);
        track.clearBus (num);

        int count = 0;
        if (playing)
            FractalFugue::forEachEvent (p, doll, b0, b1, [&] (const NoteEvent& e) {
                if (count < kMaxEventsPerBlock) events[(size_t) count++] = e;
            });

        int pos = 0;
        for (int k = 0; k < count; ++k)
        {
            const auto& e = events[(size_t) k];
            const int off = std::clamp ((int) ((e.beat - b0) * samplesPerBeat), 0, num - 1);
            track.renderVoices (pos, off - pos);
            pos = off;
            if (audible)
            {
                track.trigger (p, doll, samples[(size_t) doll], e, samplesPerBeat);
                if (listener != nullptr) listener->noteStarted (e, off);
            }
        }
        track.renderVoices (pos, num - pos);

        if (playing)
        {
            if (const auto* s = samples[(size_t) doll]; s != nullptr && s->durationSeconds() > 0.01)
            {
                const double speed = ds.sampler.scanSpeed * std::pow (FractalFugue::ratioValue (p.ratio), (double) (doll - (kNumDolls - 1)));
                track.scrubPos += (num / sr) * speed / s->durationSeconds();
                track.scrubPos -= std::floor (track.scrubPos);
            }
        }

        track.processFx (ds, audible, num, L, R, revL.data(), revR.data(), echoL.data(), echoR.data());
    }

    // ---- echo: tempo-synced ping-pong with a darkening feedback path
    const float echoSamples = (float) std::clamp (p.echoBeats * 60.0 / bpm * sr, 1.0, sr * 4.0);
    const float fb = std::clamp (p.echoFeedback, 0.0f, 0.92f);
    const float damp = 0.35f;
    for (int i = 0; i < num; ++i)
    {
        const float dl = echoLineL.read (echoSamples), dr = echoLineR.read (echoSamples);
        echoDampL += damp * (dl - echoDampL);
        echoDampR += damp * (dr - echoDampR);
        echoLineL.push (echoL[(size_t) i] + fb * echoDampR);
        echoLineR.push (echoR[(size_t) i] + fb * echoDampL);
        L[i] += dl * 0.8f;
        R[i] += dr * 0.8f;
        revL[(size_t) i] += dl * 0.3f;
        revR[(size_t) i] += dr * 0.3f;
    }

    // ---- cathedral reverb
    if (std::abs (lastReverbSize - p.reverbSize) > 1e-4f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = std::clamp (p.reverbSize, 0.0f, 1.0f);
        rp.damping = 0.45f;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
        lastReverbSize = p.reverbSize;
    }
    reverb.processStereo (revL.data(), revR.data(), num);

    // ---- master and a gentle peak limiter
    const float rel = std::exp (-1.0f / (0.12f * (float) sr));
    float blockPeak = 0;
    for (int i = 0; i < num; ++i)
    {
        float l = (L[i] + revL[(size_t) i] * p.reverbLevel) * p.masterVolume;
        float r = (R[i] + revR[(size_t) i] * p.reverbLevel) * p.masterVolume;
        const float a = std::max (std::abs (l), std::abs (r));
        limiterEnv = a > limiterEnv ? a : limiterEnv * rel + a * (1.0f - rel);
        const float g = limiterEnv > 0.92f ? 0.92f / limiterEnv : 1.0f;
        l *= g; r *= g;
        L[i] = std::clamp (l, -1.0f, 1.0f);
        R[i] = std::clamp (r, -1.0f, 1.0f);
        blockPeak = std::max (blockPeak, std::max (std::abs (L[i]), std::abs (R[i])));
    }
    peak = blockPeak;

    if (playing)
        beat = b1;
}
} // namespace mf
