#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

/** Standalone host: UI first, optional delayed audio open. */
class Sa16StandaloneApp final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "SA16 Spectrum Analyzer"; }
    const juce::String getApplicationVersion() override    { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override { mainWindow = nullptr; }
    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow final : public juce::DocumentWindow,
                             private juce::Timer
    {
    public:
        explicit MainWindow (juce::String name)
            : DocumentWindow (name, juce::Colours::black, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setOpaque (true);
            setContentOwned (new juce::Component(), true);
            centreWithSize (Sa16::kEditorWidth, Sa16::kEditorHeight);
            setVisible (true);
            toFront (true);

            juce::MessageManager::callAsync ([this]
            {
                processor = std::make_unique<Sa16SpectrumProcessor>();
                setContentOwned (processor->createEditor(), true);
                setResizable (false, false);
                startTimer (800);
            });
        }

        ~MainWindow() override
        {
            stopTimer();
            deviceManager.removeAudioCallback (&player);
            player.setProcessor (nullptr);
            clearContentComponent();
            processor = nullptr;
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        void timerCallback() override
        {
            stopTimer();
            openAudio();
        }

        void openAudio()
        {
            // Prefer low-channel shared WASAPI; never block forever on ASIO panel.
            auto setup = deviceManager.getAudioDeviceSetup();
            setup.inputChannels.setRange (0, 2, true);
            setup.outputChannels.setRange (0, 2, true);
            setup.bufferSize = 512;
            setup.sampleRate = 48000.0;

            juce::String error = deviceManager.initialise (2, 2, nullptr, true, {}, &setup);

            if (error.isNotEmpty())
                error = deviceManager.initialise (0, 2, nullptr, true);

            if (error.isEmpty() && processor != nullptr)
            {
                player.setProcessor (processor.get());
                deviceManager.addAudioCallback (&player);
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync (
                    juce::AlertWindow::WarningIcon,
                    "Audio",
                    "Audio device failed:\n" + error + "\nUI will keep running.");
            }
        }

        std::unique_ptr<Sa16SpectrumProcessor> processor;
        juce::AudioDeviceManager deviceManager;
        juce::AudioProcessorPlayer player;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (Sa16StandaloneApp)
