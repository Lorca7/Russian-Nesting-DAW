// Renders the editor offscreen to PNG files (used by CI to check the UI without a screen).
// Usage: UISnapshot <output-folder>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    auto proc = std::make_unique<MatryoshkaProcessor>();
    proc->prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (proc->createEditor());

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    proc->setPlaying (true);

    auto shoot = [&] (const juce::String& name, int w, int h)
    {
        editor->setSize (w, h);
        // let the fugue play for a while so the dolls sing and the roll scrolls
        for (int i = 0; i < 40; ++i)
        {
            for (int b = 0; b < 4; ++b) proc->processBlock (buffer, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
        }
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto file = outDir.getChildFile (name + ".png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        png.writeImageToStream (img, out);
        std::printf ("wrote %s\n", file.getFullPathName().toRawUTF8());
    };

    shoot ("editor-1280x800", 1280, 800);
    shoot ("editor-1600x1000", 1600, 1000);
    editor.reset();
    return 0;
}
