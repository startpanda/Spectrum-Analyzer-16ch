#pragma once

#include "Constants.h"
#include "DSP/SpectrumEngine.h"

class Sa16SpectrumProcessor final : public juce::AudioProcessor
{
public:
    Sa16SpectrumProcessor();
    ~Sa16SpectrumProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    Sa16::SpectrumEngine& getEngine() noexcept { return engine; }
    const Sa16::SpectrumEngine& getEngine() const noexcept { return engine; }

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    std::array<Sa16::ChannelState, Sa16::kMaxChannels>& getChannelStates() noexcept { return channelStates; }
    const std::array<Sa16::ChannelState, Sa16::kMaxChannels>& getChannelStates() const noexcept { return channelStates; }

    void setViewMode (Sa16::ViewMode mode);
    Sa16::ViewMode getViewMode() const noexcept
    {
        return static_cast<Sa16::ViewMode> (viewMode.load (std::memory_order_relaxed));
    }

    void setBypass (bool bypass);
    bool isBypassed() const noexcept { return bypassed.load (std::memory_order_relaxed); }

    double getSampleRateHz() const noexcept { return currentSampleRate.load (std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    Sa16::SpectrumEngine engine;
    juce::AudioProcessorValueTreeState apvts;
    std::array<Sa16::ChannelState, Sa16::kMaxChannels> channelStates {};

    std::atomic<int> viewMode { (int) Sa16::ViewMode::Analyzer };
    std::atomic<bool> bypassed { false };
    std::atomic<double> currentSampleRate { 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sa16SpectrumProcessor)
};
