#pragma once

#include "PluginProcessor.h"
#include "UI/SpectrumDisplay.h"
#include "UI/NeonKnob.h"
#include "UI/ChannelStrip.h"
#include "UI/AboutDialog.h"

class Sa16SpectrumEditor final : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit Sa16SpectrumEditor (Sa16SpectrumProcessor&);
    ~Sa16SpectrumEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

private:
    void finishUiSetup();
    void timerCallback() override;
    void syncFromProcessor();
    void applyChannelState (int index, Sa16::ChannelState state);
    void syncViewModeToUi (Sa16::ViewMode mode);
    void showAboutDialog();
    void clearButtonLookAndFeels();
    juce::String formatFreq (float x01) const;
    void setParamFloat (const juce::String& id, float value);
    float getParamFloat (const juce::String& id, float fallback) const;
    void setParamChoice (const juce::String& id, int index);
    void setParamBool (const juce::String& id, bool value);

    Sa16SpectrumProcessor& processor;
    bool uiReady = false;

    std::unique_ptr<Sa16::SpectrumDisplay> display;
    std::array<std::unique_ptr<Sa16::ChannelStrip>, Sa16::kMaxChannels> strips;
    std::array<std::unique_ptr<juce::TextButton>, (int) Sa16::ViewMode::NumModes> modeButtons;
    std::array<std::unique_ptr<juce::TextButton>, Sa16::kNumFftSizes> fftButtons;
    std::array<std::unique_ptr<juce::TextButton>, (int) Sa16::WindowType::NumWindows> windowButtons;

    std::unique_ptr<juce::TextButton> freezeBtn, linBtn, inspectBtn, measureBtn;
    std::unique_ptr<juce::TextButton> bypassBtn, aboutBtn;
    std::unique_ptr<Sa16::AboutDialog> aboutDialog;

    std::unique_ptr<Sa16::NeonKnob> sideChannel, sideReactivity, sideZoom;
    std::unique_ptr<Sa16::NeonKnob> knSmoothing, knPreamp;

    bool inspect = false;
    bool measurement = false;
    float inspectX = 0.5f;
    juce::String freqReadout { "20 Hz" };
    juce::String levelReadout { "-110.0 dB" };

    juce::Rectangle<int> headerBounds, plotBounds, dbAxisBounds, rightBounds, freqAxisBounds;
    juce::Rectangle<int> toggleBounds, stripBounds, knobBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sa16SpectrumEditor)
};
