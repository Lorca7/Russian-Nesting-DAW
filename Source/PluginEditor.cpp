#include "PluginEditor.h"

using namespace mf;
using namespace mf::theme;

static const int kExportBars[] = { 4, 8, 16, 32, 64, 128 };

// Offline WAV rendering on a background thread with a progress window.
class MatryoshkaEditor::RenderJob : public juce::ThreadWithProgressWindow
{
public:
    RenderJob (MatryoshkaEditor& e, const Project& p, const SampleSet& s, double beats, juce::File f)
        : ThreadWithProgressWindow ("Rendering the fugue to WAV...", true, true), editor (e), project (p), samples (s), lengthBeats (beats), file (std::move (f)) {}

    void run() override
    {
        ok = renderWavFile (project, samples, lengthBeats, 48000.0, file, [this] (float prog) {
            setProgress (prog);
            return ! threadShouldExit();
        }, error);
    }
    void threadComplete (bool userPressedCancel) override
    {
        auto* ed = &editor;
        const bool success = ok && ! userPressedCancel;
        const auto msg = success ? "Rendered " + file.getFileName() + " (" + file.getParentDirectory().getFullPathName() + ")" : "WAV export stopped: " + error;
        const auto target = success ? file : juce::File();
        juce::MessageManager::callAsync ([ed, msg, target] { ed->setStatus (msg, target); ed->renderJob.reset(); });
    }

private:
    MatryoshkaEditor& editor;
    Project project;
    SampleSet samples;
    double lengthBeats;
    juce::File file;
    bool ok = false;
    juce::String error;
};

MatryoshkaEditor::MatryoshkaEditor (MatryoshkaProcessor& p)
    : AudioProcessorEditor (&p), proc (p), shelf (p), law (p), subject (p), inspector (p)
{
    setLookAndFeel (&lnf);
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);

    playButton.onClick = [this] { proc.setPlaying (! proc.isPlaying()); };
    playButton.setColour (juce::TextButton::buttonOnColourId, leaf);
    rewindButton.setTooltip ("Back to the start");
    rewindButton.onClick = [this] { proc.rewind(); };
    recordButton.setColour (juce::TextButton::buttonOnColourId, cinnabar);
    recordButton.setTooltip ("Record what you hear to a WAV take, plus a MIDI file of every doll's notes. Saved to Music/Matryoshka Fugue.");
    recordButton.onClick = [this] { toggleRecord(); };

    tempo.setSliderStyle (juce::Slider::LinearHorizontal);
    tempo.setRange (30.0, 220.0, 1.0);
    tempo.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    tempo.textFromValueFunction = [] (double v) { return juce::String ((int) v) + " BPM"; };
    tempo.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    tempo.onValueChange = [this] { const double v = tempo.getValue(); proc.editProject ([v] (Project& q) { q.bpm = v; }); };

    for (int i = 0; i < 6; ++i) exportLength.box.addItem (juce::String (kExportBars[i]) + " bars", i + 1);
    exportLength.box.setSelectedId (3, juce::dontSendNotification);
    exportWavButton.setTooltip ("Render the fugue offline to a 24-bit WAV file.");
    exportMidiButton.setTooltip ("Write a 4-track MIDI file (one track and channel per doll).");
    exportWavButton.onClick = [this] { exportWav(); };
    exportMidiButton.onClick = [this] { exportMidi(); };
    openButton.onClick = [this] { openProject(); };
    saveButton.onClick = [this] { saveProject(); };
    folderButton.onClick = [] { defaultOutputFolder().revealToUser(); };
    revealButton.onClick = [this] { if (revealTarget.exists()) revealTarget.revealToUser(); };

    status.setFont (body (12.5f));
    status.setColour (juce::Label::textColourId, creamDim);
    status.setText ("Click a doll to edit it. Space bar plays and stops.", juce::dontSendNotification);

    shelf.onSelect = [this] (int d) { inspector.setDoll (d); };
    auto edited = [this] { subject.update (proc.getProject()); };
    law.onEdited = edited;
    subject.onEdited = [this] { subject.update (proc.getProject()); law.refresh(); };
    inspector.onEdited = edited;
    inspector.onStatus = [this] (const juce::String& s) { setStatus (s); };

    for (juce::Component* c : std::initializer_list<juce::Component*> { &playButton, &rewindButton, &recordButton, &tempo, &exportLength,
                                                                         &exportWavButton, &exportMidiButton, &openButton, &saveButton,
                                                                         &folderButton, &status, &revealButton, &shelf, &roll, &law,
                                                                         &subject, &inspector })
        addAndMakeVisible (c);
    revealButton.setVisible (false); // shown once there is a file to reveal

    setResizable (true, true);
    setResizeLimits (1220, 740, 2600, 1700);
    setSize (1280, 800);
    setWantsKeyboardFocus (true);
    refreshAll();
    startTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    startTimerHz (30);
}

MatryoshkaEditor::~MatryoshkaEditor()
{
    stopTimer();
    renderJob.reset();
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void MatryoshkaEditor::refreshAll()
{
    const auto p = proc.getProject();
    tempo.setValue (p.bpm, juce::dontSendNotification);
    law.refresh();
    subject.update (p);
    inspector.refresh();
}

void MatryoshkaEditor::setStatus (const juce::String& s, juce::File reveal)
{
    status.setText (s, juce::dontSendNotification);
    revealTarget = reveal;
    revealButton.setVisible (reveal.exists());
    resized();
}

void MatryoshkaEditor::timerCallback()
{
    const auto p = proc.getProject();
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime;
    shelf.tick (p, now);
    roll.update (p, proc.getBeat(), proc.isPlaying());
    subject.update (p);
    inspector.tick (p);

    playButton.setButtonText (proc.isPlaying() ? "Stop" : "Play");
    playButton.setToggleState (proc.isPlaying(), juce::dontSendNotification);
    recordButton.setToggleState (proc.isRecording(), juce::dontSendNotification);
    if (proc.isRecording())
    {
        const int secs = (int) proc.getRecordedSeconds();
        recordButton.setButtonText (juce::String::formatted ("Rec %d:%02d", secs / 60, secs % 60));
    }
    else
        recordButton.setButtonText ("Record");
    if (! tempo.isMouseButtonDown() && std::abs (tempo.getValue() - p.bpm) > 0.5)
        tempo.setValue (p.bpm, juce::dontSendNotification);
}

bool MatryoshkaEditor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey)
    {
        proc.setPlaying (! proc.isPlaying());
        return true;
    }
    return false;
}

void MatryoshkaEditor::paint (juce::Graphics& g)
{
    g.fillAll (lacquer);

    // title cartouche
    auto top = getLocalBounds().removeFromTop (58).reduced (14, 6);
    auto title = top.removeFromLeft (226);
    g.setColour (gold);
    g.setFont (display (22.0f));
    g.drawText ("Matryoshka Fugue", title.removeFromTop (28), juce::Justification::bottomLeft);
    g.setColour (cinnabar.brighter (0.3f));
    g.setFont (body (12.0f));
    g.drawText (juce::CharPointer_UTF8 ("\xD0\x9C\xD0\xB0\xD1\x82\xD1\x80\xD1\x91\xD1\x88\xD0\xBA\xD0\xB0-\xD1\x84\xD1\x83\xD0\xB3\xD0\xB0  \xC2\xB7  a fractal fugue in four dolls"),
                title, juce::Justification::topLeft);

    // gold rule with beads under the top bar
    const float y = 60.0f;
    g.setColour (goldDim);
    g.drawHorizontalLine ((int) y, 14.0f, (float) getWidth() - 14.0f);
    for (float x = 20.0f; x < (float) getWidth() - 14.0f; x += 26.0f)
    {
        g.setColour (((int) (x / 26.0f)) % 4 == 0 ? cinnabar : gold);
        g.fillEllipse (x - 2.0f, y - 2.0f, 4.0f, 4.0f);
    }

    // output meter
    auto meter = juce::Rectangle<float> ((float) getWidth() - 22.0f, 12.0f, 6.0f, 38.0f);
    g.setColour (ink);
    g.fillRoundedRectangle (meter, 3.0f);
    const float pk = juce::jlimit (0.0f, 1.0f, proc.getPeak());
    g.setColour (pk > 0.9f ? cinnabar : leaf);
    g.fillRoundedRectangle (meter.withTrimmedTop (meter.getHeight() * (1.0f - pk)), 3.0f);
}

void MatryoshkaEditor::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (58).reduced (14, 8);
    top.removeFromLeft (232);
    top.removeFromRight (16); // meter
    auto place = [&] (juce::Component& c, int w) { c.setBounds (top.removeFromLeft (w).withTrimmedTop (14)); top.removeFromLeft (6); };
    place (rewindButton, 36);
    place (playButton, 72);
    place (recordButton, 88);
    tempo.setBounds (top.removeFromLeft (150).withTrimmedTop (14));
    top.removeFromLeft (12);
    exportLength.setBounds (top.removeFromLeft (96));
    top.removeFromLeft (6);
    place (exportWavButton, 96);
    place (exportMidiButton, 98);
    saveButton.setBounds (top.removeFromRight (72).withTrimmedTop (14));
    top.removeFromRight (6);
    openButton.setBounds (top.removeFromRight (72).withTrimmedTop (14));
    top.removeFromRight (6);
    folderButton.setBounds (top.removeFromRight (104).withTrimmedTop (14));

    r.removeFromTop (6);
    r = r.reduced (12, 0);
    auto statusRow = r.removeFromBottom (26);
    if (revealButton.isVisible())
        revealButton.setBounds (statusRow.removeFromRight (120).reduced (0, 2));
    status.setBounds (statusRow);

    // dolls on the left, the nesting roll beside them
    auto stage = r.removeFromTop (juce::jlimit (230, 380, (int) (r.getHeight() * 0.36)));
    shelf.setBounds (stage.removeFromLeft ((int) (stage.getWidth() * 0.55f)));
    stage.removeFromLeft (8);
    roll.setBounds (stage.withTrimmedBottom (6));
    r.removeFromTop (8);

    auto bottom = r.withTrimmedBottom (4);
    law.setBounds (bottom.removeFromLeft (300));
    bottom.removeFromLeft (8);
    inspector.setBounds (bottom.removeFromRight (juce::jlimit (500, 660, bottom.getWidth() / 2)));
    bottom.removeFromRight (8);
    subject.setBounds (bottom);
}

double MatryoshkaEditor::exportBeats() const
{
    const int i = juce::jmax (0, exportLength.box.getSelectedItemIndex());
    return kExportBars[i] * 4.0;
}

void MatryoshkaEditor::exportWav()
{
    if (renderJob != nullptr) return;
    const auto beats = exportBeats();
    chooser = std::make_unique<juce::FileChooser> ("Export the fugue as WAV",
                                                   defaultOutputFolder().getChildFile (timestampedName ("Matryoshka-Fugue") + ".wav"), "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, beats] (const juce::FileChooser& fc) {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              f = f.withFileExtension ("wav");
                              renderJob = std::make_unique<RenderJob> (*this, proc.getProject(), proc.getSampleSet(), beats, f);
                              renderJob->launchThread();
                          });
}

void MatryoshkaEditor::exportMidi()
{
    const auto beats = exportBeats();
    chooser = std::make_unique<juce::FileChooser> ("Export the fugue as MIDI",
                                                   defaultOutputFolder().getChildFile (timestampedName ("Matryoshka-Fugue") + ".mid"), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, beats] (const juce::FileChooser& fc) {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              f = f.withFileExtension ("mid");
                              if (writeMidiFile (buildMidiFile (proc.getProject(), beats), f))
                                  setStatus ("Wrote " + f.getFileName() + ": 4 tracks, dolls on MIDI channels 1-4.", f);
                              else
                                  setStatus ("Couldn't write " + f.getFullPathName());
                          });
}

void MatryoshkaEditor::toggleRecord()
{
    if (proc.isRecording())
    {
        proc.stopRecording();
        setStatus ("Saved take: " + proc.getLastTakeWav().getFileName() + " and " + proc.getLastTakeMidi().getFileName(), proc.getLastTakeWav());
        return;
    }
    juce::String err;
    if (proc.startRecording (err))
        setStatus ("Recording to " + proc.getLastTakeWav().getFullPathName() + ". Press Record again to stop.");
    else
        setStatus ("Recording failed: " + err);
}

void MatryoshkaEditor::saveProject()
{
    chooser = std::make_unique<juce::FileChooser> ("Save project", defaultOutputFolder().getChildFile ("My Fugue.matryoshka"), "*.matryoshka");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc) {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              f = f.withFileExtension ("matryoshka");
                              if (auto xml = proc.createStateXml(); xml != nullptr && xml->writeTo (f))
                                  setStatus ("Saved " + f.getFileName(), f);
                              else
                                  setStatus ("Couldn't save " + f.getFullPathName());
                          });
}

void MatryoshkaEditor::openProject()
{
    chooser = std::make_unique<juce::FileChooser> ("Open project", defaultOutputFolder(), "*.matryoshka");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc) {
                              const auto f = fc.getResult();
                              if (! f.existsAsFile()) return;
                              if (auto xml = juce::XmlDocument::parse (f); xml != nullptr && xml->hasTagName ("MatryoshkaFugue"))
                              {
                                  proc.restoreFromStateXml (*xml);
                                  refreshAll();
                                  inspector.setDoll (shelf.selected);
                                  setStatus ("Opened " + f.getFileName());
                              }
                              else
                                  setStatus (f.getFileName() + " isn't a Matryoshka Fugue project.");
                          });
}

juce::AudioProcessorEditor* MatryoshkaProcessor::createEditor()
{
    return new MatryoshkaEditor (*this);
}
