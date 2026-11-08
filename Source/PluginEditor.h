#pragma once
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/DollShelf.h"
#include "ui/NestingRoll.h"
#include "ui/Panels.h"

class MatryoshkaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MatryoshkaEditor (MatryoshkaProcessor&);
    ~MatryoshkaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;
    void refreshAll();
    void setStatus (const juce::String& s, juce::File reveal = {});
    double exportBeats() const;
    void exportWav();
    void exportMidi();
    void toggleRecord();
    void saveProject();
    void openProject();

    MatryoshkaProcessor& proc;
    mf::MatryoshkaLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    juce::TextButton playButton { "Play" }, rewindButton { "|<" }, recordButton { "Record" };
    juce::Slider tempo;
    mf::Picker exportLength { "Export length" };
    juce::TextButton exportWavButton { "Export WAV" }, exportMidiButton { "Export MIDI" };
    juce::TextButton openButton { "Open..." }, saveButton { "Save..." }, folderButton { "Takes folder" };
    juce::Label status;
    juce::TextButton revealButton { "Show in Finder" };
    juce::File revealTarget;

    mf::DollShelf shelf;
    mf::NestingRoll roll;
    mf::FugueLawPanel law;
    mf::SubjectPanel subject;
    mf::DollInspector inspector;

    std::unique_ptr<juce::FileChooser> chooser;
    class RenderJob;
    std::unique_ptr<RenderJob> renderJob;
    double startTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatryoshkaEditor)
};
