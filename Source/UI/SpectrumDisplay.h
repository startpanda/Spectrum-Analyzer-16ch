#pragma once

#include "Constants.h"

namespace Sa16
{
class SpectrumDisplay final : public juce::Component
{
public:
    SpectrumDisplay();

    void setSnapshot (const AnalyzerSnapshot& snap);
    void setChannelStates (const std::array<ChannelState, kMaxChannels>& states);
    void setViewMode (ViewMode mode);
    ViewMode getViewMode() const noexcept { return viewMode; }
    void setLinearFreq (bool linear);
    void setInspect (bool enabled, float x01);
    void setMeasurement (bool enabled);
    void setFrozen (bool frozen);
    void setBypassed (bool bypassed);
    void setModeLabel (const juce::String& label);

    /** Visible dB window: sliding top, fixed floor at kDbFloor (-110). */
    void setDbViewTop (float viewTopDb);
    float getDbViewTop() const noexcept { return dbViewTop; }
    float getDbViewBottom() const noexcept { return kDbFloor; }
    float getDbViewSpan() const noexcept { return dbViewTop - kDbFloor; }
    float getSpectrogramSpanSeconds() const noexcept { return spectrogramSpanSeconds(); }

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    std::function<void (float)> onInspectMoved;
    std::function<void()> onDbViewChanged;

private:
    float dbToY (float db, float height) const noexcept;
    float xOf (int bin, float width) const noexcept;
    float freqMax() const noexcept;
    bool isChannelVisibleInMode (int ch) const noexcept;
    std::array<float, kDisplayBins> computeSpectrogramRow (const AnalyzerSnapshot& snap) const;
    void clearSpectrogramHistory();
    void pushSpectrogramRow (const std::array<float, kDisplayBins>& row);
    std::array<float, kDisplayBins> computeMasteringSum() const;
    std::array<float, kDisplayBins> computeMasteringPhosphor() const;
    void paintGrid (juce::Graphics& g, float w, float h) const;
    void paintChannelTraces (juce::Graphics& g, float w, float h, int soloId, float alphaMul) const;
    void paintMasteringTrace (juce::Graphics& g, float w, float h) const;
    void paintSpectrogram (juce::Graphics& g, float w, float h) const;
    void paintSpectrogramOverlay (juce::Graphics& g, float w, float h) const;
    juce::Colour colourForSpectrogramDb (float db) const noexcept;
    void paintScopeTrace (juce::Graphics& g, float w, float h,
                          const std::array<float, kDisplayBins>& instant,
                          const std::array<float, kDisplayBins>& afterglow,
                          juce::Colour colour, float alpha) const;
    void applyDbViewTop (float top, bool notify);
    void buildSpectrumPaths (const std::array<float, kDisplayBins>& data,
                             float w, float h,
                             juce::Path& fill, juce::Path& line) const;
    void buildSpectrumLinePath (const std::array<float, kDisplayBins>& data,
                                float w, float h, juce::Path& line) const;
    void buildScopeAfterglowBand (const std::array<float, kDisplayBins>& instant,
                                  const std::array<float, kDisplayBins>& afterglow,
                                  float w, float h, juce::Path& band) const;
    std::array<float, kDisplayBins> smoothQuietSpectrum (const std::array<float, kDisplayBins>& data) const;
    std::array<float, kDisplayBins> smoothLowFreqSpectrum (const std::array<float, kDisplayBins>& data) const;
    float binIndexForPlotX (float x, float plotWidth) const noexcept;
    float dbAtPlotX (float x, float plotWidth, const std::array<float, kDisplayBins>& data) const noexcept;
    float dbAtPlotXSmooth (float x, float plotWidth, const std::array<float, kDisplayBins>& data) const noexcept;
    void appendDenseSmoothTrace (juce::Path& path, const std::array<float, kDisplayBins>& data,
                                 float w, float h, float x0, float x1, bool moveToFirst) const;

    AnalyzerSnapshot snapshot;
    std::array<ChannelState, kMaxChannels> channels {};
    ViewMode viewMode = ViewMode::Analyzer;
    std::array<std::array<float, kDisplayBins>, kSpectrogramHistoryRows> spectrogramHistory {};
    int spectrogramRow = 0;
    bool linearFreq = false;
    bool inspect = false;
    bool measurement = false;
    bool frozen = false;
    bool bypassed = false;
    float inspectX = 0.5f;
    float inspectY = 0.5f;
    juce::String modeLabel { "ANALYZER" };

    float dbViewTop = kDbViewTopDefault;
    bool panningDb = false;
    float panStartY = 0.0f;
    float panStartTop = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};
} // namespace Sa16
