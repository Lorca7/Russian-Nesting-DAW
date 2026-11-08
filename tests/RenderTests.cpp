// Exercises the real audio engine and exporters headlessly: renders every synth preset and every
// sampler mode, checks the output is finite, audible and not clipping, and round-trips a project.
#include "../Source/dsp/FugueEngine.h"
#include "../Source/engine/Presets.h"
#include "../Source/io/Exporters.h"
#include "../Source/io/ProjectIO.h"
#include <cstdio>

using namespace mf;
static int failures = 0;
#define CHECK(c) do { if (! (c)) { std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

struct Stats { float peak = 0, rms = 0; bool finite = true; };

static Stats render (const Project& p, const SampleSet& samples, double seconds)
{
    FugueEngine engine;
    engine.prepare (48000.0, 512);
    for (int d = 0; d < kNumDolls; ++d) engine.setSample (d, samples[(size_t) d]);
    std::vector<float> L (512), R (512);
    double beat = 0, sum = 0;
    long n = 0;
    Stats s;
    for (long done = 0; done < (long) (seconds * 48000); done += 512)
    {
        engine.process (p, L.data(), R.data(), 512, true, beat, nullptr);
        for (int i = 0; i < 512; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) s.finite = false;
            s.peak = std::max (s.peak, std::max (std::abs (L[(size_t) i]), std::abs (R[(size_t) i])));
            sum += L[(size_t) i] * L[(size_t) i];
            ++n;
        }
    }
    s.rms = (float) std::sqrt (sum / (double) n);
    return s;
}

int main()
{
    std::vector<SampleData> built;
    for (int i = 0; i < 4; ++i) built.push_back (makeBuiltInSample (i));
    for (auto& b : built) { CHECK (b.data.size() > 48000); CHECK (b.durationSeconds() > 1.0); }
    const SampleSet samples { &built[0], &built[1], &built[2], &built[3] };

    // default project
    {
        const auto s = render (makeDefaultProject(), samples, 6.0);
        std::printf ("default project: peak %.3f rms %.4f\n", s.peak, s.rms);
        CHECK (s.finite); CHECK (s.peak > 0.05f); CHECK (s.peak <= 1.0f);
    }

    // every preset on every doll
    for (auto& preset : synthPresets())
    {
        Project p = makeDefaultProject();
        for (auto& d : p.dolls) { d.source = Source::Synth; applyPreset (d, preset); }
        const auto s = render (p, samples, 3.0);
        std::printf ("%-22s (%s) peak %.3f rms %.4f\n", preset.name.c_str(), preset.era.c_str(), s.peak, s.rms);
        CHECK (s.finite); CHECK (s.peak > 0.02f); CHECK (s.peak <= 1.0f);
    }

    // sampler modes with each built-in sample shared by all four playheads
    for (int b = 0; b < 4; ++b)
        for (int mode = 0; mode < SampleMode::count; ++mode)
        {
            Project p = makeDefaultProject();
            for (auto& d : p.dolls) { d.source = Source::Sampler; d.sampler.mode = mode; d.sampler.rootNote = built[(size_t) b].rootNote; }
            const SampleSet shared { &built[(size_t) b], &built[(size_t) b], &built[(size_t) b], &built[(size_t) b] };
            const auto s = render (p, shared, 3.0);
            std::printf ("sampler %-20s mode %d peak %.3f rms %.4f\n", built[(size_t) b].name.c_str(), mode, s.peak, s.rms);
            CHECK (s.finite); CHECK (s.peak > 0.02f); CHECK (s.peak <= 1.0f);
        }

    // MIDI export has one track per doll plus tempo, and the right number of notes
    {
        Project p = makeDefaultProject();
        auto midi = buildMidiFile (p, 32.0);
        CHECK (midi.getNumTracks() == 5);
        int expected = 0;
        for (int d = 0; d < kNumDolls; ++d)
            FractalFugue::forEachEvent (p, d, 0, 32.0, [&] (const NoteEvent& e) { expected += e.numNotes; });
        int ons = 0;
        for (int t = 1; t < midi.getNumTracks(); ++t)
            for (auto* ev : *midi.getTrack (t))
                if (ev->message.isNoteOn()) ++ons;
        CHECK (ons == expected);
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mf-test.mid");
        CHECK (writeMidiFile (midi, tmp));
        CHECK (tmp.getSize() > 100);
        tmp.deleteFile();
    }

    // offline WAV render writes a readable 24-bit stereo file with the reverb tail
    {
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("mf-test.wav");
        juce::String err;
        float lastProgress = 0;
        Project p = makeDefaultProject();
        CHECK (renderWavFile (p, samples, 8.0, 48000.0, tmp, [&] (float pr) { lastProgress = pr; return true; }, err));
        CHECK (lastProgress == 1.0f);
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (tmp.createInputStream().release(), true));
        CHECK (reader != nullptr);
        if (reader)
        {
            CHECK (reader->numChannels == 2);
            CHECK (reader->bitsPerSample == 24);
            const double expectSecs = 8.0 * 60.0 / p.bpm + 4.0;
            CHECK (std::abs ((double) reader->lengthInSamples / 48000.0 - expectSecs) < 0.05);
        }
        reader.reset();
        tmp.deleteFile();
    }

    // project XML round trip
    {
        Project a = makeDefaultProject();
        a.bpm = 133; a.scaleIndex = findScale ("Obikhod (Znamenny Chant)"); a.ratio = Ratio::Golden;
        a.dolls[2].sampler.mode = SampleMode::Scrub; a.dolls[1].synth.cutoff = 777; a.subject[3].degree = -4; a.subjectLength = 11;
        auto xml = projectToXml (a);
        Project b;
        projectFromXml (*xml, b);
        CHECK (b.bpm == 133); CHECK (b.scaleIndex == a.scaleIndex); CHECK (b.ratio == Ratio::Golden);
        CHECK (b.dolls[2].sampler.mode == SampleMode::Scrub); CHECK (std::abs (b.dolls[1].synth.cutoff - 777) < 0.01);
        CHECK (b.subject[3].degree == -4); CHECK (b.subjectLength == 11);
    }

    if (failures == 0) std::printf ("All render tests passed.\n");
    return failures == 0 ? 0 : 1;
}
