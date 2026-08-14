#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace Sa16
{
inline constexpr int kMaxChannels = 16;
inline constexpr int kDisplayBins = 1024;
inline constexpr float kScopeTraceStepPx = 0.5f;
inline constexpr float kScopeTraceStepLoPx = 0.5f;
/** Left portion of plot gets denser, smoother trace (low frequencies). */
inline constexpr float kScopeLowFreqWidth01 = 0.58f;
/** Bins below this frequency get extra spatial smoothing. */
inline constexpr float kScopeSmoothFreqMaxHz = 1200.0f;
inline constexpr float kScopeSmoothFreqBlend01 = 0.14f;
inline constexpr int kSpectrogramHistoryRows = 256;
inline constexpr float kSpectrogramRefreshHz = 60.0f;

inline float spectrogramSpanSeconds() noexcept
{
    return (float) kSpectrogramHistoryRows / kSpectrogramRefreshHz;
}

/** Inferno-style LUT used by pro spectrograms (RX / Insight). t = 0..1 magnitude. */
inline juce::Colour spectrogramColour (float t01) noexcept
{
    const float t = juce::jlimit (0.0f, 1.0f, t01);
    static constexpr float stops[][4] = {
        { 0.00f, 0.001f, 0.000f, 0.014f },
        { 0.13f, 0.087f, 0.032f, 0.153f },
        { 0.25f, 0.258f, 0.050f, 0.338f },
        { 0.38f, 0.416f, 0.089f, 0.434f },
        { 0.50f, 0.647f, 0.158f, 0.208f },
        { 0.62f, 0.859f, 0.302f, 0.047f },
        { 0.75f, 0.988f, 0.518f, 0.075f },
        { 0.88f, 0.987f, 0.773f, 0.208f },
        { 1.00f, 0.988f, 1.000f, 0.643f }
    };

    constexpr int n = (int) (sizeof (stops) / sizeof (stops[0]));
    int i = 0;
    while (i < n - 2 && t > stops[i + 1][0])
        ++i;

    const float u = (t - stops[i][0]) / juce::jmax (1.0e-6f, stops[i + 1][0] - stops[i][0]);
    return juce::Colour::fromFloatRGBA (stops[i][1] + (stops[i + 1][1] - stops[i][1]) * u,
                                        stops[i][2] + (stops[i + 1][2] - stops[i][2]) * u,
                                        stops[i][3] + (stops[i + 1][3] - stops[i][3]) * u,
                                        1.0f);
}
inline constexpr int kMaxFftOrder = 15; // 32768
inline constexpr int kMinFftOrder = 8;  // 256
inline constexpr int kDefaultFftOrder = 12; // 4096
inline constexpr float kDbFloor = -110.0f;
inline constexpr float kDbCeiling = 12.0f;   // absolute top headroom
inline constexpr float kDbMajorStep = 10.0f;
inline constexpr float kDbViewTopMin = -70.0f; // when slid down (still ≥40 dB of range)
inline constexpr float kDbViewTopDefault = 0.0f;

/** Map dB → normalised Y inside a visible window [viewBottom, viewTop]. */
inline float dbToY01 (float db, float viewTop, float viewBottom) noexcept
{
    const float span = juce::jmax (1.0f, viewTop - viewBottom);
    return 1.0f - juce::jlimit (0.0f, 1.0f, (db - viewBottom) / span);
}

inline float dbToY01 (float db) noexcept
{
    return dbToY01 (db, kDbViewTopDefault, kDbFloor);
}

inline float clampDbViewTop (float top) noexcept
{
    return juce::jlimit (kDbViewTopMin, kDbCeiling, top);
}

/** Zoom knob 0..1 → view top (+12 … -70). Higher zoom = higher ceiling. */
inline float zoom01ToDbViewTop (float zoom01) noexcept
{
    const float t = juce::jlimit (0.0f, 1.0f, zoom01);
    return kDbViewTopMin + t * (kDbCeiling - kDbViewTopMin);
}

inline float dbViewTopToZoom01 (float top) noexcept
{
    return juce::jlimit (0.0f, 1.0f,
                         (top - kDbViewTopMin) / (kDbCeiling - kDbViewTopMin));
}

inline float dbMajorStepForSpan (float span) noexcept
{
    if (span <= 50.0f)  return 5.0f;
    if (span <= 80.0f)  return 6.0f;
    return 10.0f;
}

/** Audible / analyzer display band (IEC-style audio spectrum). */
inline constexpr float kFreqMinHz = 20.0f;
inline constexpr float kFreqMaxHz = 20000.0f;

/** Plot X maps 20 Hz → 0, fMax → 1 across the full plot width. */
inline constexpr float kFreqPlotMargin01 = 0.0f;

/** Major log-frequency ticks used by pro audio analyzers. */
inline constexpr float kFreqMajorTicksHz[] = {
    20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
    1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
};
inline constexpr int kNumFreqMajorTicks = 10;

/** Decade multipliers for minor log ticks (2–9 × 10^n). */
inline constexpr float kFreqMinorMults[] = { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
inline constexpr int kNumFreqMinorMults = 8;

inline float clampDisplayFreqMax (double sampleRate) noexcept
{
    const float nyquist = (float) (sampleRate * 0.5);
    return juce::jmin (kFreqMaxHz, juce::jmax (kFreqMinHz * 2.0f, nyquist * 0.999f));
}

/** Map Hz → normalised plot X (0…1). 20 Hz / fMax align to plot edges. */
inline float freqToX01 (float hz, bool linear, float fMax = kFreqMaxHz) noexcept
{
    const float m = kFreqPlotMargin01;
    const float inner = juce::jmax (1.0e-6f, 1.0f - 2.0f * m);
    const float f = juce::jmax (1.0e-6f, hz);

    float t;
    if (linear)
        t = (f - kFreqMinHz) / juce::jmax (1.0e-6f, fMax - kFreqMinHz);
    else
        t = std::log (f / kFreqMinHz) / std::log (fMax / kFreqMinHz);

    return m + t * inner;
}

inline float x01ToFreq (float x01, bool linear, float fMax = kFreqMaxHz) noexcept
{
    const float m = kFreqPlotMargin01;
    const float inner = juce::jmax (1.0e-6f, 1.0f - 2.0f * m);
    const float t = (x01 - m) / inner;

    if (linear)
        return kFreqMinHz + t * (fMax - kFreqMinHz);
    return kFreqMinHz * std::pow (fMax / kFreqMinHz, t);
}

/** True when a frequency tick/label should be drawn (inside labelled band only). */
inline bool isFreqTickVisible (float hz, float fMax) noexcept
{
    return hz >= kFreqMinHz - 0.5f && hz <= fMax + 0.5f;
}

/** Display bins are always log-spaced across [kFreqMinHz, fMax]. */
inline float displayBinFreq (int bin, float fMax = kFreqMaxHz) noexcept
{
    const float t = (float) juce::jlimit (0, kDisplayBins - 1, bin) / (float) (kDisplayBins - 1);
    return kFreqMinHz * std::pow (fMax / kFreqMinHz, t);
}

inline juce::String formatFreqLabel (float hz, bool withUnit = false) noexcept
{
    if (hz >= 1000.0f)
    {
        const float k = hz / 1000.0f;
        const juce::String s = (std::abs (k - std::round (k)) < 0.05f)
                                   ? juce::String ((int) std::round (k))
                                   : juce::String (k, 1);
        return withUnit ? s + " kHz" : s + "k";
    }

    const int whole = (int) std::round (hz);
    return withUnit ? juce::String (whole) + " Hz" : juce::String (whole);
}

inline constexpr int kEditorWidth = 1180;
inline constexpr int kEditorHeight = 644;
inline constexpr int kHeaderH = 34;
inline constexpr int kPlotH = 360;
inline constexpr int kFreqAxisH = 22;
inline constexpr int kToggleH = 36;
inline constexpr int kStripH = 72;
inline constexpr int kKnobH = 80;
inline constexpr int kDbAxisW = 40;
inline constexpr int kRightPanelW = 112;
inline constexpr int kRightPanelPadX = 6;
inline constexpr int kRightPanelPadTop = 8;
inline constexpr int kFftDividerGap = 4;
inline constexpr int kFftLabelH = 14;
inline constexpr int kFftLabelGap = 6;
inline constexpr int kFftMinButtonH = 20;
inline constexpr int kFftButtonGap = 1;
inline constexpr int kBypassButtonH = 28;
inline constexpr int kBypassGapAbove = 8;

inline int rightPanelFftDividerY (int rightBoundsY) noexcept
{
    return rightBoundsY + kRightPanelPadTop + kFftDividerGap;
}

inline int rightPanelFftLabelY (int rightBoundsY) noexcept
{
    return rightPanelFftDividerY (rightBoundsY) + 1 + kFftDividerGap;
}

inline int rightPanelFftHeaderH() noexcept
{
    return kFftDividerGap + 1 + kFftDividerGap + kFftLabelH + kFftLabelGap;
}

inline constexpr float kFontUiMini = 9.0f;
inline constexpr float kFontUiSmall = 10.0f;
inline constexpr float kFontUi = 11.0f;
inline constexpr float kFontUiLarge = 12.5f;
inline constexpr float kFontUiTitle = 14.0f;
inline constexpr float kFontAxis = 11.0f;
inline constexpr float kFontKnobLabel = 12.0f;
inline constexpr float kFontKnobSub = 10.5f;
/** Author credit shown in header (UTF-8). */
inline constexpr const char* kAuthorNameUtf8 = "AI\xe5\x8f\x82\xe6\x95\xb0\xe4\xb9\x8b\xe9\x97\xb4";
inline constexpr const char* kAuthorEmail = "zpan477@gmail.com";
inline constexpr const char* kAboutLabelUtf8 = "\xe5\x85\xb3\xe4\xba\x8e";

inline juce::String authorName() { return juce::String (juce::CharPointer_UTF8 (kAuthorNameUtf8)); }
inline juce::String aboutLabel() { return juce::String (juce::CharPointer_UTF8 (kAboutLabelUtf8)); }

inline constexpr uint32_t kChannelColors[kMaxChannels] = {
    0xff00e5ff, 0xff00ff88, 0xffffe600, 0xffff9500,
    0xffff3366, 0xffcc44ff, 0xff44aaff, 0xff00ffcc,
    0xffffcc00, 0xffff6622, 0xffff2266, 0xff9933ff,
    0xff33ccff, 0xff66ff44, 0xffffaa00, 0xffff44aa
};

inline juce::Colour chColour (int index) noexcept
{
    return juce::Colour (kChannelColors[(size_t) juce::jlimit (0, kMaxChannels - 1, index)]);
}

namespace Colours
{
    inline const juce::Colour bg          { 0xff07090d };
    inline const juce::Colour shell       { 0xff0a0c12 };
    inline const juce::Colour surface     { 0xff08090e };
    inline const juce::Colour headerTop   { 0xff111522 };
    inline const juce::Colour headerBot   { 0xff0c0f18 };
    inline const juce::Colour border      { 0xff181c28 };
    inline const juce::Colour borderSoft  { 0xff141720 };
    inline const juce::Colour panelLine   { 0xff1e2230 };
    inline const juce::Colour textBright  { 0xffe8f2ff };
    inline const juce::Colour textMid     { 0xffa8b8c8 };
    inline const juce::Colour textDim     { 0xff7a8a9a };
    inline const juce::Colour textMuted   { 0xff8a9aaa };
    inline const juce::Colour accent      { 0xff00d4ff };
    inline const juce::Colour bypassRed   { 0xffff2244 };
    inline const juce::Colour activeGreen { 0xff00ff88 };
    inline const juce::Colour orange      { 0xffff9500 };
    inline const juce::Colour traceFill   { 0xff3a404a };
    inline const juce::Colour traceLine   { 0xfff2f6fa };
    inline const juce::Colour scopePhosphor { 0xff5cff9a };
}

enum class ViewMode { Analyzer = 0, Stereo, Mastering, Spectrogram, Surround, NumModes };
enum class WindowType { Hann = 0, Hamming, Blackman, FlatTop, Kaiser, NumWindows };

inline const char* viewModeName (ViewMode m) noexcept
{
    switch (m)
    {
        case ViewMode::Analyzer:     return "Analyzer";
        case ViewMode::Stereo:       return "Stereo";
        case ViewMode::Mastering:    return "Mastering";
        case ViewMode::Spectrogram:  return "Spectrogram";
        case ViewMode::Surround:     return "Surround";
        default:                     return "Analyzer";
    }
}

inline const char* windowName (WindowType w) noexcept
{
    switch (w)
    {
        case WindowType::Hann:     return "Hann";
        case WindowType::Hamming:  return "Hamming";
        case WindowType::Blackman: return "Blackman";
        case WindowType::FlatTop:  return "Flat-Top";
        case WindowType::Kaiser:   return "Kaiser";
        default:                   return "Hann";
    }
}

inline constexpr int kFftSizes[] = { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };
inline constexpr int kNumFftSizes = 8;
inline constexpr int kDefaultFftSizeIndex = 4; // 4096

inline juce::StringArray fftSizeChoiceLabels()
{
    juce::StringArray labels;
    for (int i = 0; i < kNumFftSizes; ++i)
        labels.add (juce::String (kFftSizes[(size_t) i]));
    return labels;
}

inline int fftOrderFromSize (int size) noexcept
{
    for (int o = kMinFftOrder; o <= kMaxFftOrder; ++o)
        if ((1 << o) == size)
            return o;
    return kDefaultFftOrder;
}

struct ChannelState
{
    bool on = false;
    bool solo = false;
    bool hold = false;
    bool mids = false;
};

struct AnalyzerSnapshot
{
    std::array<std::array<float, kDisplayBins>, kMaxChannels> magnitudes {};
    std::array<std::array<float, kDisplayBins>, kMaxChannels> phosphor {};
    std::array<std::array<float, kDisplayBins>, kMaxChannels> holdPeaks {};
    std::array<bool, kMaxChannels> channelEnabled {};
    int activeChannels = 0;
    float peakDb = kDbFloor;
    float inspectDb = kDbFloor;
    double sampleRate = 48000.0;
};
} // namespace Sa16
