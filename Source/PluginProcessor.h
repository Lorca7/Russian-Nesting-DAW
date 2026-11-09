#pragma once
#include "dsp/FugueEngine.h"
#include "io/Exporters.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

class MatryoshkaProcessor : public juce::AudioProcessor,
                            private mf::FugueEngine::Listener,
                            private juce::Timer
{
public:
    MatryoshkaProcessor();
    ~MatryoshkaProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Matryoshka Fugue"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- project (message thread)
    mf::Project getProject() const;
    void editProject (const std::function<void (mf::Project&)>& fn);
    void replaceProject (const mf::Project& p);

    // ---- transport
    void setPlaying (bool shouldPlay);
    bool isPlaying() const { return playing.load(); }
    void rewind() { rewindRequested = true; }
    double getBeat() const { return beatPosition.load(); }
    bool isFollowingHost() const { return wrapperType != wrapperType_Standalone; }

    // ---- samples (message thread)
    void useBuiltInSample (int doll, int builtInIndex);
    /** Loads one of the Russian scores: the whole project plus its built-in samples. */
    void loadScore (int scoreIndex);
    bool loadSampleFile (int doll, const juce::File& file, juce::String& error);
    void shareSampleWithAll (int fromDoll);
    const mf::SampleData* getSample (int doll) const { return currentSamples[(size_t) doll].load(); }
    juce::String getSampleLabel (int doll) const;
    mf::SampleSet getSampleSet() const;
    double getScrubPosition (int doll) const { return scrubPositions[(size_t) doll].load(); }

    // ---- live take recording (WAV + MIDI of what you hear)
    bool startRecording (juce::String& error);
    void stopRecording();
    bool isRecording() const { return activeWriter.load() != nullptr; }
    double getRecordedSeconds() const;
    juce::File getLastTakeWav() const { return lastTakeWav; }
    juce::File getLastTakeMidi() const { return lastTakeMidi; }

    // ---- UI feedback
    float getDollPulse (int doll) const { return pulses[(size_t) doll].load(); }
    int getDollStep (int doll) const { return currentSteps[(size_t) doll].load(); }
    float getPeak() const { return peakLevel.load(); }

    juce::AudioFormatManager formatManager;

    // Persist and restore whole projects (including sample references).
    std::unique_ptr<juce::XmlElement> createStateXml() const;
    void restoreFromStateXml (const juce::XmlElement& xml);

private:
    void noteStarted (const mf::NoteEvent& e, int sampleOffset) override;
    void timerCallback() override;
    const mf::SampleData* addToLibrary (std::unique_ptr<mf::SampleData> s);

    // project hand-off: the UI writes `project` under the lock; the audio thread copies it when dirty
    mutable juce::SpinLock projectLock;
    mf::Project project;
    std::atomic<bool> projectDirty { true };
    mf::Project audioProject;

    // samples: owned by the library for the lifetime of the processor, audio reads raw pointers
    std::vector<std::unique_ptr<mf::SampleData>> library;
    const mf::SampleData* builtIn (int index); // synthesised on first use
    std::vector<const mf::SampleData*> builtIns;
    std::array<std::atomic<const mf::SampleData*>, mf::kNumDolls> currentSamples {};
    std::array<int, mf::kNumDolls> sampleBuiltInIndex { 0, 0, 0, 0 };
    std::array<juce::String, mf::kNumDolls> sampleFilePaths;

    mf::FugueEngine engine;
    double currentSampleRate = 48000.0;
    std::atomic<bool> playing { false }, rewindRequested { false };
    std::atomic<double> beatPosition { 0.0 };
    bool hostWasPlaying = false;
    juce::AudioBuffer<float> renderBuffer;

    // feedback for the UI
    std::array<std::atomic<float>, mf::kNumDolls> pulses {};
    std::array<std::atomic<int>, mf::kNumDolls> currentSteps {};
    std::array<std::atomic<double>, mf::kNumDolls> scrubPositions {};
    std::atomic<float> peakLevel { 0.0f };

    // MIDI out
    juce::MidiBuffer* currentMidiOut = nullptr;
    double blockStartBeat = 0.0, samplesPerBeat = 1.0;
    struct PendingOff { int channel, note; double beat; };
    std::array<PendingOff, 1024> pendingOffs {};
    int numPendingOffs = 0;

    // live recording
    juce::TimeSliceThread writerThread { "Matryoshka take writer" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::SpinLock writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    std::atomic<juce::int64> recordedSamples { 0 };
    std::atomic<double> recordStartBeat { 0.0 };
    std::atomic<bool> recordStartPending { false };
    juce::AbstractFifo noteFifo { 4096 };
    std::array<mf::RecordedNote, 4096> noteFifoData {};
    std::vector<mf::RecordedNote> recordedNotes;
    double recordBpm = 120.0;
    juce::File lastTakeWav, lastTakeMidi, pendingTakeBase;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatryoshkaProcessor)
};
