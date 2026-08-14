#pragma once

#include "Constants.h"
#include <atomic>

namespace Sa16
{
/** Lightweight realtime spectrum engine with runtime-selectable FFT size. */
class SpectrumEngine
{
public:
    static constexpr int kMaxFftSize = 1 << kMaxFftOrder;

    SpectrumEngine();

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void process (const juce::AudioBuffer<float>& buffer);

    void setFftSize (int size);
    void setWindowType (WindowType type);
    void setSmoothing (float amount01);
    void setReactivity (float amount01);
    void setChannelGainDb (float db);
    void setPreampDb (float db);
    void setFrozen (bool frozen);
    void setChannelEnabled (int ch, bool enabled);
    void setChannelHold (int ch, bool hold);
    void clearHolds();

    int getFftSize() const noexcept { return fftSize; }
    AnalyzerSnapshot getSnapshot() const;

private:
    void applyFftOrder (int order);
    void rebuildWindow();
    void pushSample (int ch, float sample);
    void runFft (int ch);

    double sampleRateHz = 48000.0;
    int numChannels = 2;
    int fftOrder = kDefaultFftOrder;
    int fftSize = 1 << kDefaultFftOrder;
    int hopSize = (1 << kDefaultFftOrder) / 4;
    bool prepared = false;
    WindowType activeWindow = WindowType::Hann;

    std::atomic<int> pendingFftOrder { kDefaultFftOrder };
    std::atomic<int> pendingWindow { (int) WindowType::Hann };
    std::atomic<float> smoothing { 0.45f };
    std::atomic<float> reactivity { 0.6f };
    std::atomic<float> channelGainLin { 1.0f };
    std::atomic<float> preampLin { 1.0f };
    std::atomic<bool> frozen { false };
    std::atomic<bool> clearHoldRequest { false };

    std::unique_ptr<juce::dsp::FFT> fft;
    juce::AudioBuffer<float> windowBuffer;
    std::array<juce::AudioBuffer<float>, kMaxChannels> fifo;
    std::array<int, kMaxChannels> fifoIndex {};
    juce::AudioBuffer<float> fftScratch;
    float windowSum = (float) kMaxFftSize * 0.5f;
    std::array<std::array<float, kDisplayBins>, kMaxChannels> display {};
    std::array<std::array<float, kDisplayBins>, kMaxChannels> phosphor {};
    std::array<std::array<float, kDisplayBins>, kMaxChannels> hold {};
    std::array<std::atomic<bool>, kMaxChannels> enabled {};
    std::array<std::atomic<bool>, kMaxChannels> holdEnabled {};
    std::array<std::atomic<float>, kMaxChannels> channelPeakDb {};
};
} // namespace Sa16
