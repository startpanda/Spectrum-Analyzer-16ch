#include "UI/SpectrumDisplay.h"

namespace Sa16
{

SpectrumDisplay::SpectrumDisplay()
{
    setOpaque (true);
    for (int i = 0; i < 6; ++i)
        channels[(size_t) i].on = true;
}

void SpectrumDisplay::setSnapshot (const AnalyzerSnapshot& snap)
{
    snapshot = snap;

    if (viewMode == ViewMode::Spectrogram && ! frozen)
        pushSpectrogramRow (computeSpectrogramRow (snap));

    repaint();
}

void SpectrumDisplay::setChannelStates (const std::array<ChannelState, kMaxChannels>& states)
{
    channels = states;
    repaint();
}

void SpectrumDisplay::setViewMode (ViewMode mode)
{
    if (viewMode == mode)
        return;

    viewMode = mode;
    modeLabel = juce::String (viewModeName (mode)).toUpperCase();
    clearSpectrogramHistory();

    if (mode == ViewMode::Spectrogram)
        pushSpectrogramRow (computeSpectrogramRow (snapshot));

    repaint();
}

void SpectrumDisplay::clearSpectrogramHistory()
{
    for (auto& row : spectrogramHistory)
        row.fill (kDbFloor);
    spectrogramRow = 0;
}

void SpectrumDisplay::pushSpectrogramRow (const std::array<float, kDisplayBins>& row)
{
    spectrogramHistory[(size_t) spectrogramRow] = row;
    spectrogramRow = (spectrogramRow + 1) % kSpectrogramHistoryRows;
}

std::array<float, kDisplayBins> SpectrumDisplay::computeSpectrogramRow (const AnalyzerSnapshot& snap) const
{
    for (int i = 0; i < kMaxChannels; ++i)
    {
        if (channels[(size_t) i].solo && channels[(size_t) i].on)
            return snap.magnitudes[(size_t) i];
    }

    std::array<float, kDisplayBins> out {};
    out.fill (kDbFloor);
    bool hasData = false;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        if (! channels[(size_t) ch].on || ! snap.channelEnabled[(size_t) ch])
            continue;

        hasData = true;
        for (int b = 0; b < kDisplayBins; ++b)
            out[(size_t) b] = juce::jmax (out[(size_t) b], snap.magnitudes[(size_t) ch][(size_t) b]);
    }

    if (hasData)
        return out;

    return snap.magnitudes[0];
}

bool SpectrumDisplay::isChannelVisibleInMode (int ch) const noexcept
{
    if (! channels[(size_t) ch].on)
        return false;

    switch (viewMode)
    {
        case ViewMode::Stereo:
            return ch < 2;
        case ViewMode::Surround:
            return ch < 6;
        case ViewMode::Mastering:
        case ViewMode::Spectrogram:
            return false;
        case ViewMode::Analyzer:
        default:
            return true;
    }
}

std::array<float, kDisplayBins> SpectrumDisplay::computeMasteringSum() const
{
    std::array<float, kDisplayBins> sumLin {};
    sumLin.fill (0.0f);
    int count = 0;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        if (! channels[(size_t) ch].on)
            continue;

        ++count;
        for (int i = 0; i < kDisplayBins; ++i)
            sumLin[(size_t) i] += juce::Decibels::decibelsToGain (snapshot.magnitudes[(size_t) ch][(size_t) i]);
    }

    std::array<float, kDisplayBins> out {};
    if (count == 0)
    {
        out.fill (kDbFloor);
        return out;
    }

    for (int i = 0; i < kDisplayBins; ++i)
        out[(size_t) i] = juce::Decibels::gainToDecibels (sumLin[(size_t) i] / (float) count, kDbFloor);

    return out;
}

std::array<float, kDisplayBins> SpectrumDisplay::computeMasteringPhosphor() const
{
    std::array<float, kDisplayBins> maxLin {};
    maxLin.fill (0.0f);
    bool hasData = false;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        if (! channels[(size_t) ch].on)
            continue;

        hasData = true;
        for (int i = 0; i < kDisplayBins; ++i)
            maxLin[(size_t) i] = juce::jmax (maxLin[(size_t) i],
                                             juce::Decibels::decibelsToGain (snapshot.phosphor[(size_t) ch][(size_t) i]));
    }

    std::array<float, kDisplayBins> out {};
    if (! hasData)
    {
        out.fill (kDbFloor);
        return out;
    }

    for (int i = 0; i < kDisplayBins; ++i)
        out[(size_t) i] = juce::Decibels::gainToDecibels (maxLin[(size_t) i], kDbFloor);

    return out;
}

void SpectrumDisplay::setLinearFreq (bool linear) { linearFreq = linear; repaint(); }
void SpectrumDisplay::setInspect (bool enabled, float x01)
{
    inspect = enabled;
    inspectX = juce::jlimit (0.0f, 1.0f, x01);
    inspectY = 0.5f;
    repaint();
}
void SpectrumDisplay::setMeasurement (bool enabled) { measurement = enabled; repaint(); }
void SpectrumDisplay::setFrozen (bool f) { frozen = f; repaint(); }
void SpectrumDisplay::setBypassed (bool b) { bypassed = b; repaint(); }
void SpectrumDisplay::setModeLabel (const juce::String& label) { modeLabel = label; repaint(); }

void SpectrumDisplay::applyDbViewTop (float top, bool notify)
{
    const float next = clampDbViewTop (top);
    if (std::abs (next - dbViewTop) < 0.01f)
        return;

    dbViewTop = next;
    repaint();
    if (notify && onDbViewChanged)
        onDbViewChanged();
}

void SpectrumDisplay::setDbViewTop (float viewTopDb)
{
    applyDbViewTop (viewTopDb, false);
}

float SpectrumDisplay::freqMax() const noexcept
{
    return clampDisplayFreqMax (snapshot.sampleRate > 0.0 ? snapshot.sampleRate : 48000.0);
}

float SpectrumDisplay::dbToY (float db, float height) const noexcept
{
    return dbToY01 (db, dbViewTop, kDbFloor) * height;
}

float SpectrumDisplay::xOf (int bin, float width) const noexcept
{
    const float fMax = freqMax();
    const float hz = displayBinFreq (bin, fMax);
    return freqToX01 (hz, linearFreq, fMax) * width;
}

std::array<float, kDisplayBins> SpectrumDisplay::smoothQuietSpectrum (const std::array<float, kDisplayBins>& data) const
{
    std::array<float, kDisplayBins> out = data;

    for (int d = 0; d < kDisplayBins; ++d)
    {
        const int i0 = juce::jmax (0, d - 2);
        const int i1 = juce::jmin (kDisplayBins - 1, d + 2);

        float localMax = data[(size_t) d];
        float localMin = data[(size_t) d];
        for (int i = i0; i <= i1; ++i)
        {
            localMax = juce::jmax (localMax, data[(size_t) i]);
            localMin = juce::jmin (localMin, data[(size_t) i]);
        }

        const float span = localMax - localMin;
        float quietness = 0.0f;

        if (localMax < dbViewTop - 30.0f)
            quietness = juce::jlimit (0.0f, 1.0f, (dbViewTop - 30.0f - localMax) / 35.0f);

        if (span < 14.0f)
            quietness = juce::jmax (quietness, juce::jlimit (0.0f, 1.0f, (14.0f - span) / 14.0f));

        if (quietness <= 0.001f)
            continue;

        float smoothed = data[(size_t) d];
        if (d >= 2 && d < kDisplayBins - 2)
        {
            smoothed = (data[(size_t) (d - 2)] + data[(size_t) (d - 1)] * 2.0f + data[(size_t) d] * 4.0f
                        + data[(size_t) (d + 1)] * 2.0f + data[(size_t) (d + 2)])
                       * 0.1f;
        }
        else if (d >= 1 && d < kDisplayBins - 1)
        {
            smoothed = (data[(size_t) (d - 1)] + data[(size_t) d] * 2.0f + data[(size_t) (d + 1)]) * 0.25f;
        }

        out[(size_t) d] = data[(size_t) d] * (1.0f - quietness) + smoothed * quietness;
    }

    return out;
}

std::array<float, kDisplayBins> SpectrumDisplay::smoothLowFreqSpectrum (const std::array<float, kDisplayBins>& data) const
{
    std::array<float, kDisplayBins> out = data;
    const float fMax = freqMax();

    auto smooth7 = [] (const std::array<float, kDisplayBins>& src, int d) -> float
    {
        if (d >= 3 && d < kDisplayBins - 3)
        {
            return (src[(size_t) (d - 3)] + src[(size_t) (d - 2)] * 2.0f + src[(size_t) (d - 1)] * 3.0f
                    + src[(size_t) d] * 4.0f + src[(size_t) (d + 1)] * 3.0f + src[(size_t) (d + 2)] * 2.0f
                    + src[(size_t) (d + 3)])
                   * (1.0f / 16.0f);
        }

        if (d >= 2 && d < kDisplayBins - 2)
        {
            return (src[(size_t) (d - 2)] + src[(size_t) (d - 1)] * 2.0f + src[(size_t) d] * 4.0f
                    + src[(size_t) (d + 1)] * 2.0f + src[(size_t) (d + 2)])
                   * 0.1f;
        }

        if (d >= 1 && d < kDisplayBins - 1)
            return (src[(size_t) (d - 1)] + src[(size_t) d] * 2.0f + src[(size_t) (d + 1)]) * 0.25f;

        return src[(size_t) d];
    };

    for (int d = 0; d < kDisplayBins; ++d)
    {
        const float hz = displayBinFreq (d, fMax);
        if (hz > kScopeSmoothFreqMaxHz)
            continue;

        const float fade = 1.0f - juce::jlimit (0.0f, 1.0f,
                                                std::log (hz / kFreqMinHz)
                                                    / std::log (kScopeSmoothFreqMaxHz / kFreqMinHz));
        if (fade <= 0.001f)
            continue;

        const float smoothed = smooth7 (data, d);
        out[(size_t) d] = data[(size_t) d] * (1.0f - fade) + smoothed * fade;
    }

    for (int d = 0; d < kDisplayBins; ++d)
    {
        const float hz = displayBinFreq (d, fMax);
        if (hz > 350.0f)
            continue;

        const float fade = 1.0f - juce::jlimit (0.0f, 1.0f,
                                                std::log (hz / kFreqMinHz) / std::log (350.0f / kFreqMinHz));
        if (fade <= 0.001f)
            continue;

        const float smoothed = smooth7 (out, d);
        out[(size_t) d] = out[(size_t) d] * (1.0f - fade) + smoothed * fade;
    }

    return out;
}

float SpectrumDisplay::binIndexForPlotX (float x, float plotWidth) const noexcept
{
    const float fMax = freqMax();
    const float x01 = juce::jlimit (0.0f, 1.0f, x / juce::jmax (1.0f, plotWidth));
    const float hz = juce::jmax (kFreqMinHz, x01ToFreq (x01, linearFreq, fMax));

    return juce::jlimit (0.0f, (float) (kDisplayBins - 1),
                         std::log (hz / kFreqMinHz) / std::log (fMax / kFreqMinHz)
                             * (float) (kDisplayBins - 1));
}

float SpectrumDisplay::dbAtPlotX (float x, float plotWidth, const std::array<float, kDisplayBins>& data) const noexcept
{
    const float t = binIndexForPlotX (x, plotWidth);
    const int i0 = (int) t;
    const int i1 = juce::jmin (i0 + 1, kDisplayBins - 1);
    const float frac = t - (float) i0;
    return data[(size_t) i0] * (1.0f - frac) + data[(size_t) i1] * frac;
}

float SpectrumDisplay::dbAtPlotXSmooth (float x, float plotWidth, const std::array<float, kDisplayBins>& data) const noexcept
{
    const float x01 = juce::jlimit (0.0f, 1.0f, x / juce::jmax (1.0f, plotWidth));
    const float blendEnd = kScopeLowFreqWidth01 + kScopeSmoothFreqBlend01;

    const float t = binIndexForPlotX (x, plotWidth);
    const int i1 = juce::jlimit (1, kDisplayBins - 2, (int) t);
    const float frac = t - (float) i1;

    const float y0 = data[(size_t) (i1 - 1)];
    const float y1 = data[(size_t) i1];
    const float y2 = data[(size_t) (i1 + 1)];
    const float y3 = data[(size_t) (i1 + 2)];

    const float frac2 = frac * frac;
    const float frac3 = frac2 * frac;
    const float cubic = 0.5f * ((2.0f * y1)
                                  + (-y0 + y2) * frac
                                  + (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * frac2
                                  + (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * frac3);
    const float linear = y1 * (1.0f - frac) + y2 * frac;

    if (x01 <= kScopeLowFreqWidth01)
        return cubic;

    if (x01 >= blendEnd)
        return linear;

    const float blend = (x01 - kScopeLowFreqWidth01) / juce::jmax (0.001f, kScopeSmoothFreqBlend01);
    return cubic * (1.0f - blend) + linear * blend;
}

void SpectrumDisplay::appendDenseSmoothTrace (juce::Path& path,
                                              const std::array<float, kDisplayBins>& data,
                                              float w, float h,
                                              float x0, float x1, bool moveToFirst) const
{
    if (x1 <= x0)
        return;

    const float step = juce::jmax (0.5f, kScopeTraceStepPx);
    const int segments = juce::jmax (1, (int) std::ceil ((x1 - x0) / step));

    for (int s = 0; s <= segments; ++s)
    {
        const float x = x0 + (x1 - x0) * (float) s / (float) segments;
        const float y = dbToY (dbAtPlotXSmooth (x, w, data), h);

        if (moveToFirst && s == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }
}

void SpectrumDisplay::buildSpectrumPaths (const std::array<float, kDisplayBins>& data,
                                          float w, float h,
                                          juce::Path& fill, juce::Path& line) const
{
    fill.clear();
    line.clear();

    if (kDisplayBins < 2 || w < 2.0f)
        return;

    appendDenseSmoothTrace (line, data, w, h, 0.0f, w, true);
    appendDenseSmoothTrace (fill, data, w, h, 0.0f, w, true);
    fill.lineTo (w, h);
    fill.lineTo (0.0f, h);
    fill.closeSubPath();
}

void SpectrumDisplay::buildSpectrumLinePath (const std::array<float, kDisplayBins>& data,
                                             float w, float h,
                                             juce::Path& line) const
{
    line.clear();
    if (kDisplayBins < 2 || w < 2.0f)
        return;

    appendDenseSmoothTrace (line, data, w, h, 0.0f, w, true);
}

void SpectrumDisplay::buildScopeAfterglowBand (const std::array<float, kDisplayBins>& instant,
                                               const std::array<float, kDisplayBins>& afterglow,
                                               float w, float h,
                                               juce::Path& band) const
{
    band.clear();
    if (kDisplayBins < 2 || w < 2.0f)
        return;

    appendDenseSmoothTrace (band, instant, w, h, 0.0f, w, true);

    const float step = juce::jmax (0.5f, kScopeTraceStepPx);
    const int segments = juce::jmax (1, (int) std::ceil (w / step));

    for (int s = segments - 1; s >= 0; --s)
    {
        const float x = w * (float) s / (float) segments;
        band.lineTo (x, dbToY (dbAtPlotXSmooth (x, w, afterglow), h));
    }

    band.closeSubPath();
}

void SpectrumDisplay::paintScopeTrace (juce::Graphics& g, float w, float h,
                                       const std::array<float, kDisplayBins>& instant,
                                       const std::array<float, kDisplayBins>& afterglow,
                                       juce::Colour colour, float alpha) const
{
    const auto smoothInstant = smoothLowFreqSpectrum (smoothQuietSpectrum (instant));
    const auto smoothGlow = smoothLowFreqSpectrum (smoothQuietSpectrum (afterglow));

    juce::Path instantLine, glowLine, afterglowBand;
    buildSpectrumLinePath (smoothInstant, w, h, instantLine);
    buildSpectrumLinePath (smoothGlow, w, h, glowLine);
    buildScopeAfterglowBand (smoothInstant, smoothGlow, w, h, afterglowBand);

    const auto scopeTint = colour.interpolatedWith (Colours::scopePhosphor, 0.42f);

    g.setColour (scopeTint.withAlpha (alpha * 0.16f));
    g.fillPath (afterglowBand);

    g.setColour (scopeTint.withAlpha (alpha * 0.34f));
    g.strokePath (glowLine, juce::PathStrokeType (2.1f));

    g.setColour (scopeTint.withAlpha (alpha * 0.58f));
    g.strokePath (instantLine, juce::PathStrokeType (1.25f));

    g.setColour (Colours::traceLine.withAlpha (alpha * 0.96f));
    g.strokePath (instantLine, juce::PathStrokeType (0.85f));
}

void SpectrumDisplay::mouseMove (const juce::MouseEvent& e)
{
    if (! inspect)
        return;
    inspectX = juce::jlimit (0.0f, 1.0f, (float) e.x / (float) juce::jmax (1, getWidth()));
    inspectY = juce::jlimit (0.0f, 1.0f, (float) e.y / (float) juce::jmax (1, getHeight()));
    if (onInspectMoved)
        onInspectMoved (inspectX);
    repaint();
}

void SpectrumDisplay::mouseDown (const juce::MouseEvent& e)
{
    panningDb = ! inspect;
    panStartY = (float) e.y;
    panStartTop = dbViewTop;

    if (inspect)
        mouseMove (e);
}

void SpectrumDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (inspect)
    {
        mouseMove (e);
        return;
    }

    if (! panningDb)
        return;

    const float h = (float) juce::jmax (1, getHeight());
    const float span = juce::jmax (1.0f, getDbViewSpan());
    // Drag down → raise top (louder ceiling); drag up → lower top.
    applyDbViewTop (panStartTop + ((float) e.y - panStartY) / h * span, true);
}

void SpectrumDisplay::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const float span = juce::jmax (1.0f, getDbViewSpan());
    applyDbViewTop (dbViewTop + wheel.deltaY * span * 0.12f, true);
}

juce::Colour SpectrumDisplay::colourForSpectrogramDb (float db) const noexcept
{
    const float span = juce::jmax (1.0f, dbViewTop - kDbFloor);
    return spectrogramColour ((db - kDbFloor) / span);
}

void SpectrumDisplay::paintGrid (juce::Graphics& g, float w, float h) const
{
    const float fMax = freqMax();
    const float viewBottom = kDbFloor;
    const float span = getDbViewSpan();
    const float step = dbMajorStepForSpan (span);
    const float minor = step * 0.5f;

    g.setColour (juce::Colour::fromFloatRGBA (40.0f / 255.0f, 45.0f / 255.0f, 58.0f / 255.0f, 0.28f));
    for (float decade = 10.0f; decade <= fMax; decade *= 10.0f)
    {
        for (int i = 0; i < kNumFreqMinorMults; ++i)
        {
            const float hz = decade * kFreqMinorMults[i];
            if (! isFreqTickVisible (hz, fMax))
                continue;
            const float x = freqToX01 (hz, linearFreq, fMax) * w;
            g.drawLine (x, 0.0f, x, h, 0.4f);
        }
    }

    const float minorStart = std::ceil (viewBottom / minor) * minor;
    for (float db = minorStart; db <= dbViewTop + 0.01f; db += minor)
    {
        if (std::fmod (std::abs (db), step) < 0.1f)
            continue;
        const float y = dbToY (db, h);
        if (y < -1.0f || y > h + 1.0f)
            continue;
        g.drawLine (0.0f, y, w, y, 0.4f);
    }

    g.setColour (juce::Colour::fromFloatRGBA (50.0f / 255.0f, 55.0f / 255.0f, 70.0f / 255.0f, 0.55f));
    for (int i = 0; i < kNumFreqMajorTicks; ++i)
    {
        const float hz = kFreqMajorTicksHz[i];
        if (! isFreqTickVisible (hz, fMax))
            break;
        const float x = freqToX01 (hz, linearFreq, fMax) * w;
        g.drawLine (x, 0.0f, x, h, 0.6f);
    }

    const float majorStart = std::ceil (viewBottom / step) * step;
    for (float db = majorStart; db <= dbViewTop + 0.01f; db += step)
    {
        const float y = dbToY (db, h);
        if (y < -1.0f || y > h + 1.0f)
            continue;
        g.drawLine (0.0f, y, w, y, 0.6f);
    }

    if (0.0f <= dbViewTop && 0.0f >= viewBottom)
    {
        const float y0 = dbToY (0.0f, h);
        g.setColour (juce::Colour::fromFloatRGBA (80.0f / 255.0f, 90.0f / 255.0f, 110.0f / 255.0f, 0.55f));
        for (float x = 0.0f; x < w; x += 8.0f)
            g.drawLine (x, y0, juce::jmin (x + 4.0f, w), y0, 1.0f);
    }
}

void SpectrumDisplay::paintChannelTraces (juce::Graphics& g, float w, float h, int soloId, float alphaMul) const
{
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        if (! isChannelVisibleInMode (ch))
            continue;

        const float alpha = (soloId >= 0 ? (ch == soloId ? 1.0f : 0.18f) : 1.0f) * alphaMul;
        const auto colour = chColour (ch);

        paintScopeTrace (g, w, h,
                         snapshot.magnitudes[(size_t) ch],
                         snapshot.phosphor[(size_t) ch],
                         colour, alpha);

        if (channels[(size_t) ch].hold)
        {
            juce::Path holdLine;
            buildSpectrumLinePath (smoothQuietSpectrum (snapshot.holdPeaks[(size_t) ch]), w, h, holdLine);
            g.setColour (colour.withAlpha (alpha * 0.38f));
            g.strokePath (holdLine, juce::PathStrokeType (0.75f));
        }
    }
}

void SpectrumDisplay::paintMasteringTrace (juce::Graphics& g, float w, float h) const
{
    paintScopeTrace (g, w, h,
                     computeMasteringSum(),
                     computeMasteringPhosphor(),
                     Colours::accent, 1.0f);
}

void SpectrumDisplay::paintSpectrogram (juce::Graphics& g, float w, float h) const
{
    const int iw = kSpectrogramHistoryRows;
    const int ih = juce::jmax (2, (int) std::round (h));
    juce::Image img (juce::Image::RGB, iw, ih, false);
    juce::Image::BitmapData pixels (img, juce::Image::BitmapData::writeOnly);

    for (int x = 0; x < iw; ++x)
    {
        const int age = iw - 1 - x;
        const int idx = (spectrogramRow - 1 - age + kSpectrogramHistoryRows) % kSpectrogramHistoryRows;
        const auto& row = spectrogramHistory[(size_t) idx];

        for (int y = 0; y < ih; ++y)
        {
            const float x01 = 1.0f - ((float) y + 0.5f) / (float) ih;
            const float db = dbAtPlotX (x01 * w, w, row);
            pixels.setPixelColour (x, y, colourForSpectrogramDb (db));
        }
    }

    g.drawImage (img, juce::Rectangle<float> (0.0f, 0.0f, w, h));
}

void SpectrumDisplay::paintSpectrogramOverlay (juce::Graphics& g, float w, float h) const
{
    const float fMax = freqMax();

    g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.045f));
    for (float decade = 10.0f; decade <= fMax; decade *= 10.0f)
    {
        for (int i = 0; i < kNumFreqMinorMults; ++i)
        {
            const float hz = decade * kFreqMinorMults[i];
            if (! isFreqTickVisible (hz, fMax))
                continue;
            const float y = (1.0f - freqToX01 (hz, linearFreq, fMax)) * h;
            g.drawLine (0.0f, y, w, y, 0.4f);
        }
    }

    g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.10f));
    for (int i = 0; i < kNumFreqMajorTicks; ++i)
    {
        const float hz = kFreqMajorTicksHz[i];
        if (! isFreqTickVisible (hz, fMax))
            break;
        const float y = (1.0f - freqToX01 (hz, linearFreq, fMax)) * h;
        g.drawLine (0.0f, y, w, y, 0.6f);
    }

    const float duration = spectrogramSpanSeconds();
    const float timeSteps[] = { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f };
    g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.06f));
    for (float sec : timeSteps)
    {
        if (sec >= duration - 0.05f)
            continue;
        const float x = (1.0f - sec / duration) * w;
        g.drawLine (x, 0.0f, x, h, 0.5f);
    }

    const float barW = 8.0f;
    const float barX = 8.0f;
    const float barY = 28.0f;
    const float barH = juce::jmax (48.0f, h - 48.0f);
    juce::ColourGradient ramp (spectrogramColour (1.0f), barX, barY,
                               spectrogramColour (0.0f), barX, barY + barH, false);
    for (int i = 1; i < 8; ++i)
    {
        const float t = 1.0f - (float) i / 8.0f;
        ramp.addColour ((float) i / 8.0f, spectrogramColour (t));
    }
    g.setGradientFill (ramp);
    g.fillRect (barX, barY, barW, barH);
    g.setColour (juce::Colour (0x66ffffff));
    g.drawRect (barX, barY, barW, barH, 1.0f);

    g.setFont (juce::Font (juce::FontOptions (kFontUiMini)));
    g.setColour (Colours::textMid);
    const auto topDb = juce::String ((int) std::round (dbViewTop));
    const auto botDb = juce::String ((int) std::round (kDbFloor));
    g.drawText (topDb, juce::Rectangle<float> (barX + 10.0f, barY - 1.0f, 34.0f, 12.0f),
                juce::Justification::centredLeft);
    g.drawText (botDb, juce::Rectangle<float> (barX + 10.0f, barY + barH - 11.0f, 34.0f, 12.0f),
                juce::Justification::centredLeft);
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    if (w < 2.0f || h < 2.0f)
        return;

    g.setColour (Colours::surface);
    g.fillRect (bounds);

    int soloId = -1;
    for (int i = 0; i < kMaxChannels; ++i)
    {
        if (channels[(size_t) i].solo && isChannelVisibleInMode (i))
        {
            soloId = i;
            break;
        }
    }

    const float alphaMul = bypassed ? 0.25f : 1.0f;

    switch (viewMode)
    {
        case ViewMode::Mastering:
            paintGrid (g, w, h);
            paintMasteringTrace (g, w, h);
            break;
        case ViewMode::Spectrogram:
            paintSpectrogram (g, w, h);
            paintSpectrogramOverlay (g, w, h);
            break;
        default:
            paintGrid (g, w, h);
            paintChannelTraces (g, w, h, soloId, alphaMul);
            break;
    }

    if (measurement && viewMode != ViewMode::Spectrogram)
    {
        const float step = dbMajorStepForSpan (getDbViewSpan());
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiMini)));
        const float majorStart = std::ceil ((kDbFloor + 0.1f) / step) * step;
        for (float db = majorStart; db < dbViewTop - 0.1f; db += step)
        {
            if (std::abs (db) < 0.1f)
                continue;
            const float y = dbToY (db, h);
            g.setColour (juce::Colour (0x18ffffff));
            for (float x = 0.0f; x < w; x += 7.0f)
                g.drawLine (x, y, juce::jmin (x + 3.0f, w), y, 1.0f);
            g.setColour (Colours::textMid);
            g.drawText (juce::String ((int) std::round (db)),
                        juce::Rectangle<float> (w - 32.0f, y - 8.0f, 28.0f, 12.0f),
                        juce::Justification::centredRight);
        }
    }

    if (inspect)
    {
        const float x = inspectX * w;
        const float y = inspectY * h;

        if (viewMode == ViewMode::Spectrogram)
        {
            g.setColour (juce::Colour (0x1fffffff));
            g.drawLine (x, 0.0f, x, h, 6.0f);
            g.drawLine (0.0f, y, w, y, 6.0f);
            g.setColour (juce::Colour (0x99ffffff));
            for (float yy = 0.0f; yy < h; yy += 9.0f)
                g.drawLine (x, yy, x, juce::jmin (yy + 5.0f, h), 1.0f);
            for (float xx = 0.0f; xx < w; xx += 9.0f)
                g.drawLine (xx, y, juce::jmin (xx + 5.0f, w), y, 1.0f);

            const float hz = juce::jlimit (kFreqMinHz, juce::jmin (kFreqMaxHz, freqMax()),
                                           x01ToFreq (1.0f - inspectY, linearFreq, freqMax()));
            const float ageSec = (1.0f - inspectX) * spectrogramSpanSeconds();
            const int age = juce::jlimit (0, kSpectrogramHistoryRows - 1,
                                          (int) std::round ((1.0f - inspectX) * (float) (kSpectrogramHistoryRows - 1)));
            const int idx = (spectrogramRow - 1 - age + kSpectrogramHistoryRows) % kSpectrogramHistoryRows;
            const float db = dbAtPlotX ((1.0f - inspectY) * w, w, spectrogramHistory[(size_t) idx]);
            const juce::String tip = formatFreqLabel (hz, true) + "  "
                                   + juce::String (ageSec, 2) + " s  "
                                   + juce::String (db, 1) + " dB";
            const float tipW = 168.0f;
            float tipX = x + 8.0f;
            if (inspectX > 0.55f)
                tipX = x - tipW - 8.0f;
            const float tipY = juce::jlimit (8.0f, h - 28.0f, y + 8.0f);
            auto tipR = juce::Rectangle<float> (tipX, tipY, tipW, 16.0f);
            g.setColour (juce::Colour (0xcc0a0c14));
            g.fillRoundedRectangle (tipR, 3.0f);
            g.setColour (juce::Colour (0xff2a3040));
            g.drawRoundedRectangle (tipR, 3.0f, 1.0f);
            g.setColour (juce::Colour (0xffaabbcc));
            g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiSmall)));
            g.drawText (tip, tipR, juce::Justification::centred);
        }
        else
        {
            g.setColour (juce::Colour (0x1fffffff));
            g.drawLine (x, 0.0f, x, h, 6.0f);
            g.setColour (juce::Colour (0x99ffffff));
            for (float yy = 0.0f; yy < h; yy += 9.0f)
                g.drawLine (x, yy, x, juce::jmin (yy + 5.0f, h), 1.0f);

            const float hz = juce::jlimit (kFreqMinHz, juce::jmin (kFreqMaxHz, freqMax()),
                                           x01ToFreq (inspectX, linearFreq, freqMax()));
            const juce::String tip = formatFreqLabel (hz, true);
            const float tipW = 72.0f;
            float tipX = x + 8.0f;
            if (inspectX > 0.7f)
                tipX = x - tipW - 8.0f;
            auto tipR = juce::Rectangle<float> (tipX, h - 24.0f, tipW, 16.0f);
            g.setColour (juce::Colour (0xcc0a0c14));
            g.fillRoundedRectangle (tipR, 3.0f);
            g.setColour (juce::Colour (0xff2a3040));
            g.drawRoundedRectangle (tipR, 3.0f, 1.0f);
            g.setColour (juce::Colour (0xffaabbcc));
            g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiSmall)));
            g.drawText (tip, tipR, juce::Justification::centred);
        }
    }

    {
        auto badge = juce::Rectangle<float> (8.0f, 8.0f, 72.0f, 16.0f);
        g.setColour (Colours::accent.withAlpha (0.07f));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (Colours::accent.withAlpha (0.16f));
        g.drawRoundedRectangle (badge, 3.0f, 1.0f);
        g.setColour (Colours::accent.withAlpha (0.4f));
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUi)));
        g.drawText (modeLabel, badge, juce::Justification::centred);
    }

    if (viewMode != ViewMode::Spectrogram)
    {
        const auto label = juce::String ((int) std::round (dbViewTop)) + " / -110 dB";
        auto badge = juce::Rectangle<float> (88.0f, 8.0f, 86.0f, 16.0f);
        g.setColour (juce::Colour (0x660a0c14));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (Colours::textMid);
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiSmall)));
        g.drawText (label, badge, juce::Justification::centred);
    }

    if (frozen)
    {
        auto badge = juce::Rectangle<float> (w - 62.0f, 8.0f, 54.0f, 16.0f);
        g.setColour (juce::Colour (0x221144ff));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (juce::Colour (0x551144ff));
        g.drawRoundedRectangle (badge, 3.0f, 1.0f);
        g.setColour (juce::Colour (0xff4488ff));
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUi)));
        g.drawText ("FROZEN", badge, juce::Justification::centred);
    }

    if (bypassed)
    {
        g.setColour (Colours::bypassRed.withAlpha (0.67f));
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiTitle)));
        g.drawText ("BYPASSED", bounds, juce::Justification::centred);
    }
}
} // namespace Sa16
