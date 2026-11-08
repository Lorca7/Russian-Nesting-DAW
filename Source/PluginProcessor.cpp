#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "engine/Presets.h"
#include "io/ProjectIO.h"

MatryoshkaProcessor::MatryoshkaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
    project = mf::makeDefaultProject();
    audioProject = project;

    for (int i = 0; i < (int) mf::builtInSamples().size() && i < 4; ++i)
        builtIns[(size_t) i] = addToLibrary (std::make_unique<mf::SampleData> (mf::makeBuiltInSample (i)));
    for (int d = 0; d < mf::kNumDolls; ++d)
        currentSamples[(size_t) d] = builtIns[0];

    writerThread.startThread();
    startTimerHz (30);
}

MatryoshkaProcessor::~MatryoshkaProcessor()
{
    stopTimer();
    stopRecording();
    writerThread.stopThread (2000);
}

const mf::SampleData* MatryoshkaProcessor::addToLibrary (std::unique_ptr<mf::SampleData> s)
{
    library.push_back (std::move (s));
    return library.back().get();
}

bool MatryoshkaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().isDisabled();
}

void MatryoshkaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, juce::jmax (64, samplesPerBlock));
    renderBuffer.setSize (2, juce::jmax (64, samplesPerBlock));
    numPendingOffs = 0;
}

// ===================================================================================== project
mf::Project MatryoshkaProcessor::getProject() const
{
    const juce::SpinLock::ScopedLockType sl (projectLock);
    return project;
}

void MatryoshkaProcessor::editProject (const std::function<void (mf::Project&)>& fn)
{
    {
        const juce::SpinLock::ScopedLockType sl (projectLock);
        fn (project);
    }
    projectDirty = true;
}

void MatryoshkaProcessor::replaceProject (const mf::Project& p)
{
    editProject ([&] (mf::Project& dst) { dst = p; });
}

void MatryoshkaProcessor::setPlaying (bool shouldPlay)
{
    if (shouldPlay && ! playing.load())
        rewindRequested = true;
    playing = shouldPlay;
}

// ===================================================================================== audio
void MatryoshkaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int num = buffer.getNumSamples();

    if (projectDirty.load())
    {
        const juce::SpinLock::ScopedTryLockType tl (projectLock);
        if (tl.isLocked())
        {
            audioProject = project;
            projectDirty = false;
        }
    }
    for (int d = 0; d < mf::kNumDolls; ++d)
        engine.setSample (d, currentSamples[(size_t) d].load());

    double beat = beatPosition.load();
    bool isPlayingNow = playing.load();

    // In a DAW, follow the host transport and tempo while it is running.
    if (isFollowingHost())
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (pos->getIsPlaying())
                {
                    if (auto ppq = pos->getPpqPosition()) beat = juce::jmax (0.0, *ppq);
                    if (auto bpm = pos->getBpm()) audioProject.bpm = *bpm;
                    isPlayingNow = true;
                    hostWasPlaying = true;
                }
                else if (hostWasPlaying)
                {
                    hostWasPlaying = false;
                    isPlayingNow = false;
                    playing = false;
                }
            }

    if (rewindRequested.exchange (false))
    {
        beat = 0.0;
        engine.reset();
        numPendingOffs = 0;
    }

    samplesPerBeat = currentSampleRate * 60.0 / juce::jlimit (20.0, 400.0, audioProject.bpm);
    blockStartBeat = beat;
    currentMidiOut = &midi;

    if (recordStartPending.exchange (false))
    {
        recordStartBeat = beat;
        recordBpm = audioProject.bpm;
    }

    // MIDI note-offs that fall inside this block
    const double blockEndBeat = beat + num / samplesPerBeat;
    for (int i = 0; i < numPendingOffs;)
    {
        auto& o = pendingOffs[(size_t) i];
        if (o.beat < blockEndBeat || ! isPlayingNow)
        {
            const int off = juce::jlimit (0, num - 1, (int) ((o.beat - beat) * samplesPerBeat));
            midi.addEvent (juce::MidiMessage::noteOff (o.channel, o.note), isPlayingNow ? off : 0);
            o = pendingOffs[(size_t) --numPendingOffs];
        }
        else
            ++i;
    }

    if (renderBuffer.getNumSamples() < num)
        renderBuffer.setSize (2, num, false, false, true);

    engine.process (audioProject, buffer.getWritePointer (0), buffer.getWritePointer (1), num, isPlayingNow, beat, this);
    currentMidiOut = nullptr;

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, num);

    beatPosition = beat;
    peakLevel = engine.peakLevel();
    for (int d = 0; d < mf::kNumDolls; ++d)
        scrubPositions[(size_t) d] = engine.scrubPosition (d);

    // live take
    {
        const juce::SpinLock::ScopedTryLockType tl (writerLock);
        if (tl.isLocked())
            if (auto* w = activeWriter.load())
            {
                const float* chans[] = { buffer.getReadPointer (0), buffer.getReadPointer (1) };
                w->write (chans, num);
                recordedSamples += num;
            }
    }
}

void MatryoshkaProcessor::noteStarted (const mf::NoteEvent& e, int sampleOffset)
{
    pulses[(size_t) e.doll] = juce::jmax (pulses[(size_t) e.doll].load(), 0.35f + 0.65f * e.velocity);
    currentSteps[(size_t) e.doll] = e.step;

    if (currentMidiOut != nullptr)
        for (int c = 0; c < e.numNotes; ++c)
        {
            const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) (e.velocity * 127.0f));
            currentMidiOut->addEvent (juce::MidiMessage::noteOn (e.doll + 1, e.notes[c], vel), sampleOffset);
            if (numPendingOffs < (int) pendingOffs.size())
                pendingOffs[(size_t) numPendingOffs++] = { e.doll + 1, e.notes[c], e.beat + e.length };
        }

    if (activeWriter.load() != nullptr)
    {
        mf::RecordedNote n;
        n.doll = e.doll;
        n.numNotes = e.numNotes;
        for (int c = 0; c < e.numNotes; ++c) n.notes[c] = e.notes[c];
        n.velocity = e.velocity;
        n.beat = e.beat - recordStartBeat.load();
        n.length = e.length;
        if (n.beat >= 0.0)
        {
            const auto scope = noteFifo.write (1);
            if (scope.blockSize1 > 0) noteFifoData[(size_t) scope.startIndex1] = n;
            else if (scope.blockSize2 > 0) noteFifoData[(size_t) scope.startIndex2] = n;
        }
    }
}

void MatryoshkaProcessor::timerCallback()
{
    // drain recorded notes on the message thread
    const auto scope = noteFifo.read (noteFifo.getNumReady());
    for (int i = 0; i < scope.blockSize1; ++i) recordedNotes.push_back (noteFifoData[(size_t) (scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i) recordedNotes.push_back (noteFifoData[(size_t) (scope.startIndex2 + i)]);

    for (auto& p : pulses)
        p = p.load() * 0.86f;
}

// ===================================================================================== samples
void MatryoshkaProcessor::useBuiltInSample (int doll, int index)
{
    index = juce::jlimit (0, 3, index);
    currentSamples[(size_t) doll] = builtIns[(size_t) index];
    sampleBuiltInIndex[(size_t) doll] = index;
    sampleFilePaths[(size_t) doll].clear();
    const int root = mf::builtInSamples()[(size_t) index].rootNote;
    editProject ([&] (mf::Project& p) { p.dolls[(size_t) doll].sampler.rootNote = root; });
}

bool MatryoshkaProcessor::loadSampleFile (int doll, const juce::File& file, juce::String& error)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "Couldn't read " + file.getFileName() + ". Try WAV, AIFF, FLAC, MP3 or M4A.";
        return false;
    }
    const auto maxSamples = (juce::int64) (reader->sampleRate * 60.0);
    const int len = (int) juce::jmin (reader->lengthInSamples, maxSamples);
    if (len < 16)
    {
        error = file.getFileName() + " is empty.";
        return false;
    }
    juce::AudioBuffer<float> buf ((int) juce::jmax (1u, reader->numChannels), len);
    reader->read (&buf, 0, len, 0, true, true);

    auto s = std::make_unique<mf::SampleData>();
    s->sampleRate = reader->sampleRate;
    s->name = file.getFileNameWithoutExtension().toStdString();
    s->rootNote = 60;
    s->data.resize ((size_t) len);
    const float scale = 1.0f / (float) buf.getNumChannels();
    for (int i = 0; i < len; ++i)
    {
        float v = 0;
        for (int c = 0; c < buf.getNumChannels(); ++c) v += buf.getSample (c, i);
        s->data[(size_t) i] = v * scale;
    }
    currentSamples[(size_t) doll] = addToLibrary (std::move (s));
    sampleBuiltInIndex[(size_t) doll] = -1;
    sampleFilePaths[(size_t) doll] = file.getFullPathName();
    editProject ([&] (mf::Project& p) { p.dolls[(size_t) doll].sampler.rootNote = 60; });
    return true;
}

void MatryoshkaProcessor::shareSampleWithAll (int fromDoll)
{
    const auto root = getProject().dolls[(size_t) fromDoll].sampler.rootNote;
    for (int d = 0; d < mf::kNumDolls; ++d)
    {
        currentSamples[(size_t) d] = currentSamples[(size_t) fromDoll].load();
        sampleBuiltInIndex[(size_t) d] = sampleBuiltInIndex[(size_t) fromDoll];
        sampleFilePaths[(size_t) d] = sampleFilePaths[(size_t) fromDoll];
    }
    editProject ([&] (mf::Project& p) {
        for (auto& d : p.dolls) d.sampler.rootNote = root;
    });
}

juce::String MatryoshkaProcessor::getSampleLabel (int doll) const
{
    if (auto* s = getSample (doll))
        return juce::String (s->name) + juce::String::formatted ("  (%.1fs)", s->durationSeconds());
    return "No sample";
}

mf::SampleSet MatryoshkaProcessor::getSampleSet() const
{
    mf::SampleSet s {};
    for (int d = 0; d < mf::kNumDolls; ++d) s[(size_t) d] = getSample (d);
    return s;
}

// ===================================================================================== recording
bool MatryoshkaProcessor::startRecording (juce::String& error)
{
    stopRecording();
    pendingTakeBase = mf::defaultOutputFolder().getChildFile (mf::timestampedName ("Matryoshka-Take"));
    auto file = pendingTakeBase.withFileExtension ("wav");
    auto stream = file.createOutputStream();
    if (stream == nullptr)
    {
        error = "Couldn't create " + file.getFullPathName();
        return false;
    }
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (stream.release());
    auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}
                                               .withSampleRate (currentSampleRate)
                                               .withNumChannels (2)
                                               .withBitsPerSample (24));
    if (writer == nullptr)
    {
        error = "Couldn't create a WAV writer.";
        return false;
    }
    recordedNotes.clear();
    noteFifo.reset();
    recordedSamples = 0;
    recordStartPending = true;
    recordStartBeat = beatPosition.load();
    recordBpm = getProject().bpm;
    threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (writer.release(), writerThread, 32768);
    lastTakeWav = file;
    {
        const juce::SpinLock::ScopedLockType sl (writerLock);
        activeWriter = threadedWriter.get();
    }
    if (! isPlaying())
        setPlaying (true);
    return true;
}

void MatryoshkaProcessor::stopRecording()
{
    if (activeWriter.load() == nullptr)
        return;
    {
        const juce::SpinLock::ScopedLockType sl (writerLock);
        activeWriter = nullptr;
    }
    threadedWriter.reset(); // flushes and closes the WAV
    timerCallback();        // drain the last notes
    lastTakeMidi = pendingTakeBase.withFileExtension ("mid");
    mf::writeMidiFile (mf::buildMidiFile (recordedNotes, recordBpm), lastTakeMidi);
}

double MatryoshkaProcessor::getRecordedSeconds() const
{
    return (double) recordedSamples.load() / currentSampleRate;
}

// ===================================================================================== state
std::unique_ptr<juce::XmlElement> MatryoshkaProcessor::createStateXml() const
{
    auto xml = mf::projectToXml (getProject());
    for (int d = 0; d < mf::kNumDolls; ++d)
    {
        auto* s = xml->createNewChildElement ("Sample");
        s->setAttribute ("doll", d);
        s->setAttribute ("builtIn", sampleBuiltInIndex[(size_t) d]);
        s->setAttribute ("file", sampleFilePaths[(size_t) d]);
    }
    return xml;
}

void MatryoshkaProcessor::restoreFromStateXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("MatryoshkaFugue"))
        return;
    mf::Project p = mf::makeDefaultProject();
    mf::projectFromXml (xml, p);
    replaceProject (p);
    for (auto* s : xml.getChildWithTagNameIterator ("Sample"))
    {
        const int d = s->getIntAttribute ("doll", -1);
        if (d < 0 || d >= mf::kNumDolls) continue;
        const juce::File f (s->getStringAttribute ("file"));
        juce::String err;
        const auto root = p.dolls[(size_t) d].sampler.rootNote;
        if (s->getStringAttribute ("file").isNotEmpty() && f.existsAsFile() && loadSampleFile (d, f, err))
            editProject ([&] (mf::Project& q) { q.dolls[(size_t) d].sampler.rootNote = root; });
        else
        {
            useBuiltInSample (d, juce::jmax (0, s->getIntAttribute ("builtIn", 0)));
            editProject ([&] (mf::Project& q) { q.dolls[(size_t) d].sampler.rootNote = root; });
        }
    }
}

void MatryoshkaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = createStateXml())
        copyXmlToBinary (*xml, destData);
}

void MatryoshkaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        restoreFromStateXml (*xml);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MatryoshkaProcessor();
}
