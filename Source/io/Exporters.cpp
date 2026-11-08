#include "Exporters.h"
#include "../dsp/FugueEngine.h"

namespace mf
{
static void addNote (juce::MidiMessageSequence& seq, int channel, int note, float velocity, double beat, double length)
{
    const double on = beat * kMidiTicksPerQuarter;
    const double off = (beat + std::max (0.01, length)) * kMidiTicksPerQuarter;
    const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::lround (velocity * 127.0f));
    seq.addEvent (juce::MidiMessage::noteOn (channel, note, vel), on);
    seq.addEvent (juce::MidiMessage::noteOff (channel, note), off);
}

static juce::MidiMessageSequence tempoTrack (double bpm)
{
    juce::MidiMessageSequence t;
    auto name = juce::MidiMessage::textMetaEvent (3, "Matryoshka Fugue");
    name.setTimeStamp (0);
    t.addEvent (name);
    t.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / bpm)), 0);
    t.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    return t;
}

static juce::MidiMessageSequence dollTrack (int d)
{
    juce::MidiMessageSequence seq;
    const auto name = juce::String (juce::CharPointer_UTF8 (dollRussianName (d))) + " (" + dollRole (d) + ")";
    auto meta = juce::MidiMessage::textMetaEvent (3, name);
    meta.setTimeStamp (0);
    seq.addEvent (meta);
    return seq;
}

juce::MidiFile buildMidiFile (const Project& p, double lengthBeats)
{
    juce::MidiFile file;
    file.setTicksPerQuarterNote (kMidiTicksPerQuarter);
    auto tempo = tempoTrack (p.bpm);
    tempo.addEvent (juce::MidiMessage::endOfTrack(), lengthBeats * kMidiTicksPerQuarter);
    file.addTrack (tempo);

    for (int d = 0; d < kNumDolls; ++d)
    {
        auto seq = dollTrack (d);
        FractalFugue::forEachEvent (p, d, 0.0, lengthBeats, [&] (const NoteEvent& e) {
            for (int c = 0; c < e.numNotes; ++c)
                addNote (seq, d + 1, e.notes[c], e.velocity, e.beat, std::min (e.length, lengthBeats - e.beat));
        });
        seq.updateMatchedPairs();
        file.addTrack (seq);
    }
    return file;
}

juce::MidiFile buildMidiFile (const std::vector<RecordedNote>& notes, double bpm)
{
    juce::MidiFile file;
    file.setTicksPerQuarterNote (kMidiTicksPerQuarter);
    file.addTrack (tempoTrack (bpm));
    for (int d = 0; d < kNumDolls; ++d)
    {
        auto seq = dollTrack (d);
        for (auto& n : notes)
            if (n.doll == d)
                for (int c = 0; c < n.numNotes; ++c)
                    addNote (seq, d + 1, n.notes[c], n.velocity, n.beat, n.length);
        seq.updateMatchedPairs();
        file.addTrack (seq);
    }
    return file;
}

bool writeMidiFile (const juce::MidiFile& midi, const juce::File& file)
{
    file.deleteFile();
    juce::FileOutputStream out (file);
    if (! out.openedOk())
        return false;
    return midi.writeTo (out, 1);
}

bool renderWavFile (const Project& p, const SampleSet& samples, double lengthBeats, double sampleRate,
                    const juce::File& file, const std::function<bool (float)>& progress, juce::String& error)
{
    constexpr int block = 512;
    auto engine = std::make_unique<FugueEngine>();
    engine->prepare (sampleRate, block);
    for (int d = 0; d < kNumDolls; ++d)
        engine->setSample (d, samples[(size_t) d]);

    file.deleteFile();
    auto stream = file.createOutputStream();
    if (stream == nullptr)
    {
        error = "Could not write to " + file.getFullPathName();
        return false;
    }
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (stream.release());
    auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}
                                                .withSampleRate (sampleRate)
                                                .withNumChannels (2)
                                                .withBitsPerSample (24));
    if (writer == nullptr)
    {
        error = "Could not create the WAV writer.";
        return false;
    }

    const double secondsPerBeat = 60.0 / std::clamp (p.bpm, 20.0, 400.0);
    const auto musicSamples = (juce::int64) std::ceil (lengthBeats * secondsPerBeat * sampleRate);
    const auto tailSamples = (juce::int64) (sampleRate * 4.0);
    const auto total = musicSamples + tailSamples;

    juce::AudioBuffer<float> buffer (2, block);
    double beat = 0.0;
    for (juce::int64 done = 0; done < total; done += block)
    {
        const int n = (int) std::min<juce::int64> (block, total - done);
        const bool playing = done < musicSamples;
        int nPlay = playing ? (int) std::min<juce::int64> (n, musicSamples - done) : 0;
        if (nPlay > 0)
            engine->process (p, buffer.getWritePointer (0), buffer.getWritePointer (1), nPlay, true, beat, nullptr);
        if (nPlay < n)
            engine->process (p, buffer.getWritePointer (0) + nPlay, buffer.getWritePointer (1) + nPlay, n - nPlay, false, beat, nullptr);
        if (! writer->writeFromAudioSampleBuffer (buffer, 0, n))
        {
            error = "Writing the WAV file failed (is the disk full?).";
            return false;
        }
        if (progress && (done / block) % 64 == 0 && ! progress ((float) done / (float) total))
        {
            writer.reset();
            file.deleteFile();
            error = "Cancelled.";
            return false;
        }
    }
    writer.reset();
    if (progress) progress (1.0f);
    return true;
}

juce::File defaultOutputFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Matryoshka Fugue");
    dir.createDirectory();
    return dir;
}

juce::String timestampedName (const juce::String& prefix)
{
    return prefix + "-" + juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
}
} // namespace mf
