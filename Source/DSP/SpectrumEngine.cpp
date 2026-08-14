#include "DSP/SpectrumEngine.h"
#include <cmath>
#include <cstring>

namespace Sa16
{
namespace
{
float magToDb (float mag) noexcept
{
    return juce::Decibels::gainToDecibels (juce::jmax (mag, 1.0e-12f), kDbFloor);
}
}

SpectrumEngine::SpectrumEngine()
{
    for (auto& e : enabled)
        e.store (false, std::memory_order_relaxed);
    for (auto& h : holdEnabled)
        h.store (false, std::memory_order_relaxed);
    for (auto& p : channelPeakDb)
        p.store (kDbFloor, std::memory_order_relaxed);

    for (int i = 0; i < 6; ++i)
        enabled[(size_t) i].store (true, std::memory_order_relaxed);

    for (auto& row : display)
        row.fill (kDbFloor);
    for (auto& row : phosphor)
        row.fill (kDbFloor);
    for (auto& row : hold)
        row.fill (kDbFloor);
}

void SpectrumEngine::prepare (double sampleRate, int /*samplesPerBlock*/, int channels)
{
    sampleRateHz = sampleRate;
    numChannels = juce::jlimit (1, kMaxChannels, channels);

    windowBuffer.setSize (1, kMaxFftSize, false, true, true);
    fftScratch.setSize (1, kMaxFftSize * 2, false, true, true);

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        fifo[(size_t) ch].setSize (1, kMaxFftSize, false, true, true);
        fifoIndex[(size_t) ch] = 0;
    }

    applyFftOrder (pendingFftOrder.load (std::memory_order_relaxed));
    prepared = true;
}

void SpectrumEngine::reset()
{
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        fifoIndex[(size_t) ch] = 0;
        fifo[(size_t) ch].clear();
        display[(size_t) ch].fill (kDbFloor);
        phosphor[(size_t) ch].fill (kDbFloor);
        hold[(size_t) ch].fill (kDbFloor);
        channelPeakDb[(size_t) ch].store (kDbFloor, std::memory_order_relaxed);
    }
}

void SpectrumEngine::setFftSize (int size)
{
    const int order = fftOrderFromSize (size);
    if (order < kMinFftOrder || order > kMaxFftOrder)
        return;

    pendingFftOrder.store (order, std::memory_order_relaxed);
}

void SpectrumEngine::applyFftOrder (int order)
{
    order = juce::jlimit (kMinFftOrder, kMaxFftOrder, order);
    if (order == fftOrder && fft != nullptr)
        return;

    fftOrder = order;
    fftSize = 1 << fftOrder;
    hopSize = fftSize / 4;
    fft = std::make_unique<juce::dsp::FFT> (fftOrder);

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        fifoIndex[(size_t) ch] = 0;
        fifo[(size_t) ch].clear();
        display[(size_t) ch].fill (kDbFloor);
        phosphor[(size_t) ch].fill (kDbFloor);
    }

    rebuildWindow();
}

void SpectrumEngine::setWindowType (WindowType type)
{
    pendingWindow.store ((int) type, std::memory_order_relaxed);
}

void SpectrumEngine::setSmoothing (float v)
{
    smoothing.store (juce::jlimit (0.0f, 1.0f, v), std::memory_order_relaxed);
}

void SpectrumEngine::setReactivity (float v)
{
    reactivity.store (juce::jlimit (0.0f, 1.0f, v), std::memory_order_relaxed);
}

void SpectrumEngine::setChannelGainDb (float db)
{
    channelGainLin.store (juce::Decibels::decibelsToGain (db), std::memory_order_relaxed);
}

void SpectrumEngine::setPreampDb (float db)
{
    preampLin.store (juce::Decibels::decibelsToGain (db), std::memory_order_relaxed);
}

void SpectrumEngine::setFrozen (bool f) { frozen.store (f, std::memory_order_relaxed); }
void SpectrumEngine::clearHolds() { clearHoldRequest.store (true, std::memory_order_relaxed); }

void SpectrumEngine::setChannelEnabled (int ch, bool en)
{
    if (juce::isPositiveAndBelow (ch, kMaxChannels))
        enabled[(size_t) ch].store (en, std::memory_order_relaxed);
}

void SpectrumEngine::setChannelHold (int ch, bool h)
{
    if (juce::isPositiveAndBelow (ch, kMaxChannels))
        holdEnabled[(size_t) ch].store (h, std::memory_order_relaxed);
}

void SpectrumEngine::rebuildWindow()
{
    if (windowBuffer.getNumSamples() < fftSize)
        return;

    auto* wOut = windowBuffer.getWritePointer (0);
    float sum = 0.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        const float n = (float) i / (float) juce::jmax (1, fftSize - 1);
        float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * n);

        switch (activeWindow)
        {
            case WindowType::Hamming:
                w = 0.54f - 0.46f * std::cos (juce::MathConstants<float>::twoPi * n);
                break;
            case WindowType::Blackman:
                w = 0.42f
                    - 0.5f * std::cos (juce::MathConstants<float>::twoPi * n)
                    + 0.08f * std::cos (2.0f * juce::MathConstants<float>::twoPi * n);
                break;
            case WindowType::FlatTop:
                w = 1.0f
                    - 1.93f * std::cos (juce::MathConstants<float>::twoPi * n)
                    + 1.29f * std::cos (2.0f * juce::MathConstants<float>::twoPi * n)
                    - 0.388f * std::cos (3.0f * juce::MathConstants<float>::twoPi * n)
                    + 0.032f * std::cos (4.0f * juce::MathConstants<float>::twoPi * n);
                break;
            case WindowType::Kaiser:
            {
                const float x = 2.0f * n - 1.0f;
                w = std::exp (-8.0f * x * x);
                break;
            }
            case WindowType::Hann:
            default:
                break;
        }

        wOut[i] = w;
        sum += w;
    }

    windowSum = juce::jmax (1.0e-6f, sum);
}

void SpectrumEngine::process (const juce::AudioBuffer<float>& buffer)
{
    if (! prepared || fft == nullptr)
        return;

    const int wantOrder = pendingFftOrder.load (std::memory_order_relaxed);
    if (wantOrder != fftOrder)
        applyFftOrder (wantOrder);

    if (frozen.load (std::memory_order_relaxed))
        return;

    const auto wantWindow = static_cast<WindowType> (pendingWindow.load (std::memory_order_relaxed));
    if (wantWindow != activeWindow)
    {
        activeWindow = wantWindow;
        rebuildWindow();
    }

    if (clearHoldRequest.exchange (false, std::memory_order_relaxed))
        for (auto& row : hold)
            row.fill (kDbFloor);

    const int nCh = juce::jmin (numChannels, buffer.getNumChannels(), kMaxChannels);
    const int n = buffer.getNumSamples();
    const float gain = channelGainLin.load (std::memory_order_relaxed)
                     * preampLin.load (std::memory_order_relaxed);

    for (int ch = 0; ch < nCh; ++ch)
    {
        if (! enabled[(size_t) ch].load (std::memory_order_relaxed))
            continue;

        const float* src = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            pushSample (ch, src[i] * gain);
    }
}

void SpectrumEngine::pushSample (int ch, float sample)
{
    if (fifo[(size_t) ch].getNumSamples() < fftSize)
        return;

    auto* buf = fifo[(size_t) ch].getWritePointer (0);
    auto& idx = fifoIndex[(size_t) ch];

    if (! juce::isPositiveAndBelow (idx, fftSize))
    {
        idx = 0;
        return;
    }

    buf[idx++] = sample;

    if (idx >= fftSize)
    {
        runFft (ch);

        const int keep = fftSize - hopSize;
        std::memmove (buf, buf + hopSize, (size_t) keep * sizeof (float));
        idx = keep;
    }
}

void SpectrumEngine::runFft (int ch)
{
    if (fft == nullptr
        || fftScratch.getNumSamples() < fftSize * 2
        || fifo[(size_t) ch].getNumSamples() < fftSize
        || windowBuffer.getNumSamples() < fftSize)
        return;

    auto* data = fftScratch.getWritePointer (0);
    const float* time = fifo[(size_t) ch].getReadPointer (0);
    const float* win = windowBuffer.getReadPointer (0);

    juce::FloatVectorOperations::clear (data, fftSize * 2);

    for (int i = 0; i < fftSize; ++i)
        data[i] = time[i] * win[i];

    fft->performFrequencyOnlyForwardTransform (data, true);

    const int half = fftSize / 2;
    const float norm = 2.0f / windowSum;
    const float react = reactivity.load (std::memory_order_relaxed);
    const float smooth = smoothing.load (std::memory_order_relaxed);
    const float attack = juce::jmap (react, 0.55f, 0.96f);
    const float release = juce::jmap (react, 0.35f, 0.85f);
    const float phosphorFall = juce::jmap (smooth, 0.25f, 2.4f);
    const bool doHold = holdEnabled[(size_t) ch].load (std::memory_order_relaxed);

    float peak = kDbFloor;
    auto& row = display[(size_t) ch];
    auto& glow = phosphor[(size_t) ch];
    const float fMax = clampDisplayFreqMax (sampleRateHz);
    const float binHz = (float) (sampleRateHz / (double) fftSize);

    for (int d = 0; d < kDisplayBins; ++d)
    {
        const float fCenter = displayBinFreq (d, fMax);
        const float fPrev = d > 0 ? displayBinFreq (d - 1, fMax) : kFreqMinHz;
        const float fNext = (d + 1 < kDisplayBins) ? displayBinFreq (d + 1, fMax) : fMax;
        const float fLo = std::sqrt (fPrev * fCenter);
        const float fHi = std::sqrt (fCenter * fNext);

        const int b0 = juce::jlimit (1, half, (int) std::floor (fLo / binHz));
        const int b1 = juce::jlimit (1, half, (int) std::ceil  (fHi / binHz));

        float mag = 0.0f;
        for (int b = b0; b <= b1; ++b)
            mag = juce::jmax (mag, data[b]);

        const float db = magToDb (mag * norm);
        peak = juce::jmax (peak, db);

        const float prev = row[(size_t) d];
        const float coeff = db > prev ? attack : release;
        row[(size_t) d] = prev + (db - prev) * coeff;
        glow[(size_t) d] = juce::jmax (row[(size_t) d], glow[(size_t) d] - phosphorFall);
    }

    for (int d = 0; d < kDisplayBins; ++d)
    {
        if (doHold)
            hold[(size_t) ch][(size_t) d] = juce::jmax (hold[(size_t) ch][(size_t) d], row[(size_t) d]);
    }

    channelPeakDb[(size_t) ch].store (peak, std::memory_order_relaxed);
}

AnalyzerSnapshot SpectrumEngine::getSnapshot() const
{
    AnalyzerSnapshot snap;
    snap.magnitudes = display;
    snap.phosphor = phosphor;
    snap.holdPeaks = hold;
    snap.sampleRate = sampleRateHz;
    snap.activeChannels = 0;
    snap.peakDb = kDbFloor;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        const bool on = enabled[(size_t) ch].load (std::memory_order_relaxed);
        snap.channelEnabled[(size_t) ch] = on;
        if (on && ch < numChannels)
        {
            ++snap.activeChannels;
            snap.peakDb = juce::jmax (snap.peakDb, channelPeakDb[(size_t) ch].load (std::memory_order_relaxed));
        }
    }

    return snap;
}
} // namespace Sa16
