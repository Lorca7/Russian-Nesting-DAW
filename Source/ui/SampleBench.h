#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"

namespace mf
{
/** Waveform of the selected doll's sample with every doll's playhead drawn on it. */
class SampleBench : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit SampleBench (MatryoshkaProcessor& p) : proc (p) {}
    void setDoll (int d) { doll = d; cachedFor = nullptr; repaint(); }
    void update (const Project& p) { project = p; repaint(); }
    void paint (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override { if (onFileDropped && ! files.isEmpty()) onFileDropped (juce::File (files[0])); }
    std::function<void (const juce::File&)> onFileDropped;

private:
    MatryoshkaProcessor& proc;
    Project project;
    int doll = 0;
    const SampleData* cachedFor = nullptr;
    std::vector<float> peaks;
};
} // namespace mf
