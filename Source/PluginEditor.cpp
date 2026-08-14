#include "PluginEditor.h"
#include "UI/NeonButtonLnf.h"

namespace
{
void paintDbAxis (juce::Graphics& g, juce::Rectangle<int> area, float viewTop, float viewBottom)
{
    g.setColour (Sa16::Colours::surface);
    g.fillRect (area);
    g.setColour (Sa16::Colours::borderSoft);
    g.drawVerticalLine (area.getRight() - 1, (float) area.getY(), (float) area.getBottom());
    g.setFont (juce::Font (juce::FontOptions (Sa16::kFontAxis)));

    const float y0 = (float) area.getY();
    const float h = (float) area.getHeight();
    const float span = juce::jmax (1.0f, viewTop - viewBottom);
    const float step = Sa16::dbMajorStepForSpan (span);
    const float majorStart = std::ceil (viewBottom / step) * step;

    for (float db = majorStart; db <= viewTop + 0.01f; db += step)
    {
        const float y = y0 + Sa16::dbToY01 (db, viewTop, viewBottom) * h;
        if (y < y0 - 1.0f || y > y0 + h + 1.0f)
            continue;

        g.setColour (Sa16::Colours::textDim);
        g.drawLine ((float) area.getRight() - 5.0f, y, (float) area.getRight() - 1.0f, y, 1.0f);
        g.setColour (Sa16::Colours::textMuted);
        g.drawText (juce::String ((int) std::round (db)),
                    juce::Rectangle<float> ((float) area.getX(), y - 6.0f, (float) area.getWidth() - 7.0f, 13.0f),
                    juce::Justification::centredRight);
    }
}

void paintFreqAxisVertical (juce::Graphics& g, juce::Rectangle<int> area, bool linear, float fMax)
{
    g.setColour (Sa16::Colours::surface);
    g.fillRect (area);
    g.setColour (Sa16::Colours::borderSoft);
    g.drawVerticalLine (area.getRight() - 1, (float) area.getY(), (float) area.getBottom());
    g.setFont (juce::Font (juce::FontOptions (Sa16::kFontAxis)));

    const float y0 = (float) area.getY();
    const float h = (float) area.getHeight();

    auto drawTick = [&] (float hz)
    {
        if (! Sa16::isFreqTickVisible (hz, fMax))
            return;
        const float y = y0 + (1.0f - Sa16::freqToX01 (hz, linear, fMax)) * h;
        if (y < y0 - 1.0f || y > y0 + h + 1.0f)
            return;

        g.setColour (Sa16::Colours::textDim);
        g.drawLine ((float) area.getRight() - 5.0f, y, (float) area.getRight() - 1.0f, y, 1.0f);
        g.setColour (Sa16::Colours::textMuted);
        g.drawText (Sa16::formatFreqLabel (hz),
                    juce::Rectangle<float> ((float) area.getX(), y - 6.0f, (float) area.getWidth() - 7.0f, 13.0f),
                    juce::Justification::centredRight);
    };

    if (linear)
    {
        static const float ticks[] = { 20.0f, 5000.0f, 10000.0f, 15000.0f, 20000.0f };
        for (float hz : ticks)
            drawTick (hz);
    }
    else
    {
        for (int i = 0; i < Sa16::kNumFreqMajorTicks; ++i)
            drawTick (Sa16::kFreqMajorTicksHz[i]);
    }
}

void paintTimeAxisHorizontal (juce::Graphics& g, juce::Rectangle<int> strip, juce::Rectangle<int> plot, float durationSec)
{
    g.setColour (juce::Colour (0xff07080c));
    g.fillRect (strip);
    if (plot.getWidth() < 8)
        return;

    g.setFont (juce::Font (juce::FontOptions (Sa16::kFontAxis)));

    const float x0 = (float) plot.getX();
    const float w = (float) plot.getWidth();
    const float tickTop = (float) strip.getY();
    const float tickBot = tickTop + 5.0f;
    const float duration = juce::jmax (0.25f, durationSec);

    auto drawTick = [&] (float sec, const juce::String& text)
    {
        const float x = x0 + (1.0f - sec / duration) * w;
        g.setColour (Sa16::Colours::textDim);
        g.drawLine (x, tickTop, x, tickBot, 1.0f);
        g.setColour (Sa16::Colours::textMuted);
        g.drawText (text,
                    juce::Rectangle<float> (x - 22.0f, tickTop, 44.0f, (float) strip.getHeight()),
                    juce::Justification::centred);
    };

    drawTick (0.0f, "NOW");
    const float ticks[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f };
    for (float sec : ticks)
    {
        if (sec >= duration - 0.08f)
            continue;
        drawTick (sec, juce::String (sec, sec < 1.0f ? 1 : 0) + "s");
    }
    drawTick (duration, juce::String (duration, 1) + "s");
}

void paintFreqAxis (juce::Graphics& g, juce::Rectangle<int> strip, juce::Rectangle<int> plot, bool linear, float fMax)
{
    g.setColour (juce::Colour (0xff07080c));
    g.fillRect (strip);
    if (plot.getWidth() < 8)
        return;

    g.setFont (juce::Font (juce::FontOptions (Sa16::kFontAxis)));

    const float x0 = (float) plot.getX();
    const float w = (float) plot.getWidth();
    const float tickTop = (float) strip.getY();
    const float tickBot = tickTop + 5.0f;

    auto drawTick = [&] (float hz)
    {
        if (! Sa16::isFreqTickVisible (hz, fMax))
            return;
        const float x = x0 + Sa16::freqToX01 (hz, linear, fMax) * w;
        g.setColour (Sa16::Colours::textDim);
        g.drawLine (x, tickTop, x, tickBot, 1.0f);
        g.setColour (Sa16::Colours::textMuted);
        g.drawText (Sa16::formatFreqLabel (hz),
                    juce::Rectangle<float> (x - 18.0f, tickTop, 36.0f, (float) strip.getHeight()),
                    juce::Justification::centred);
    };

    if (linear)
    {
        static const float ticks[] = { 20.0f, 5000.0f, 10000.0f, 15000.0f, 20000.0f };
        for (float hz : ticks)
            drawTick (hz);
    }
    else
    {
        for (int i = 0; i < Sa16::kNumFreqMajorTicks; ++i)
            drawTick (Sa16::kFreqMajorTicksHz[i]);
    }
}

std::unique_ptr<Sa16::NeonKnob> makeKnob (const juce::String& label, juce::Colour c)
{
    auto k = std::make_unique<Sa16::NeonKnob>();
    k->setLabel (label);
    k->setColour (c);
    return k;
}
}

Sa16SpectrumEditor::Sa16SpectrumEditor (Sa16SpectrumProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setOpaque (true);
    setSize (Sa16::kEditorWidth, Sa16::kEditorHeight);
    setResizable (false, false);
}

Sa16SpectrumEditor::~Sa16SpectrumEditor()
{
    stopTimer();
    clearButtonLookAndFeels();
}

void Sa16SpectrumEditor::clearButtonLookAndFeels()
{
    auto clear = [] (juce::Button* b)
    {
        if (b != nullptr)
            b->setLookAndFeel (nullptr);
    };

    for (auto& b : modeButtons) clear (b.get());
    for (auto& b : fftButtons) clear (b.get());
    for (auto& b : windowButtons) clear (b.get());
    clear (freezeBtn.get());
    clear (linBtn.get());
    clear (inspectBtn.get());
    clear (measureBtn.get());
    clear (bypassBtn.get());
    clear (aboutBtn.get());
}

void Sa16SpectrumEditor::visibilityChanged()
{
    if (isShowing())
        finishUiSetup();
    else
        stopTimer();
}

void Sa16SpectrumEditor::parentHierarchyChanged()
{
    if ((isOnDesktop() || getParentComponent() != nullptr) && ! uiReady)
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Sa16SpectrumEditor> (this)]
        {
            if (safe != nullptr)
                safe->finishUiSetup();
        });
    }
}

void Sa16SpectrumEditor::finishUiSetup()
{
    if (uiReady)
        return;
    uiReady = true;

    display = std::make_unique<Sa16::SpectrumDisplay>();
    addAndMakeVisible (*display);
    display->onInspectMoved = [this] (float x)
    {
        inspectX = x;
        freqReadout = formatFreq (x);
        repaint (toggleBounds);
    };
    display->onDbViewChanged = [this]
    {
        if (display == nullptr)
            return;
        const float top = display->getDbViewTop();
        const float z = Sa16::dbViewTopToZoom01 (top);
        const auto t = juce::String ((int) std::round (top)) + " dB";
        if (sideZoom != nullptr)
        {
            sideZoom->setValue (z);
            sideZoom->setSubLabel (t);
        }
        repaint (dbAxisBounds);
    };
    display->setDbViewTop (Sa16::kDbViewTopDefault);

    for (int i = 0; i < (int) Sa16::ViewMode::NumModes; ++i)
    {
        modeButtons[(size_t) i] = std::make_unique<juce::TextButton> (Sa16::viewModeName ((Sa16::ViewMode) i));
        auto* b = modeButtons[(size_t) i].get();
        addAndMakeVisible (b);
        b->setClickingTogglesState (true);
        b->setRadioGroupId (1001);
        Sa16::styleNeonChip (*b, Sa16::Colours::accent, Sa16::neonTabLnf());
        b->onClick = [this, i]
        {
            syncViewModeToUi ((Sa16::ViewMode) i);
        };
    }
    syncViewModeToUi (processor.getViewMode());

    for (int i = 0; i < Sa16::kNumFftSizes; ++i)
    {
        fftButtons[(size_t) i] = std::make_unique<juce::TextButton> (juce::String (Sa16::kFftSizes[i]));
        auto* b = fftButtons[(size_t) i].get();
        addAndMakeVisible (b);
        b->setClickingTogglesState (true);
        b->setRadioGroupId (1002);
        b->onClick = [this, i]
        {
            setParamChoice ("fft_size", i);
            for (auto& fb : fftButtons)
                if (fb != nullptr) fb->repaint();
        };
        Sa16::styleNeonChip (*b, Sa16::Colours::orange, Sa16::neonFftLnf());
    }
    {
        const int fftIdx = juce::jlimit (0, Sa16::kNumFftSizes - 1,
                                         (int) getParamFloat ("fft_size", (float) Sa16::kDefaultFftSizeIndex));
        if (fftButtons[(size_t) fftIdx] != nullptr)
            fftButtons[(size_t) fftIdx]->setToggleState (true, juce::dontSendNotification);
    }

    for (int i = 0; i < (int) Sa16::WindowType::NumWindows; ++i)
    {
        windowButtons[(size_t) i] = std::make_unique<juce::TextButton> (Sa16::windowName ((Sa16::WindowType) i));
        auto* b = windowButtons[(size_t) i].get();
        addAndMakeVisible (b);
        b->setClickingTogglesState (true);
        b->setRadioGroupId (1003);
        b->onClick = [this, i]
        {
            setParamChoice ("window", i);
            for (auto& wb : windowButtons)
                if (wb != nullptr) wb->repaint();
        };
        Sa16::styleNeonChip (*b, juce::Colour (0xff00ff88));
    }
    windowButtons[0]->setToggleState (true, juce::dontSendNotification);

    freezeBtn = std::make_unique<juce::TextButton> (CharPointer_UTF8 ("\xe2\x9d\x84 FREEZE"));
    linBtn = std::make_unique<juce::TextButton> ("LIN");
    inspectBtn = std::make_unique<juce::TextButton> ("INSPECT");
    measureBtn = std::make_unique<juce::TextButton> ("MEASURE");
    bypassBtn = std::make_unique<juce::TextButton> ("BYPASS");
    aboutBtn = std::make_unique<juce::TextButton> (Sa16::aboutLabel());

    for (auto* b : { freezeBtn.get(), linBtn.get(), inspectBtn.get(), measureBtn.get(),
                     bypassBtn.get(), aboutBtn.get() })
        addAndMakeVisible (b);

    Sa16::styleNeonChip (*freezeBtn, juce::Colour (0xff4488ff));
    Sa16::styleNeonChip (*linBtn, Sa16::Colours::accent);
    Sa16::styleNeonChip (*inspectBtn, juce::Colour (0xffffe600));
    Sa16::styleNeonChip (*measureBtn, juce::Colour (0xffcc44ff));
    Sa16::styleBypassButton (*bypassBtn);
    Sa16::styleNeonAction (*aboutBtn, Sa16::Colours::accent, false);

    freezeBtn->setClickingTogglesState (true);
    linBtn->setClickingTogglesState (true);
    inspectBtn->setClickingTogglesState (true);
    measureBtn->setClickingTogglesState (true);
    bypassBtn->setClickingTogglesState (true);

    freezeBtn->onClick = [this]
    {
        setParamBool ("freeze", freezeBtn->getToggleState());
        if (display != nullptr)
            display->setFrozen (freezeBtn->getToggleState());
    };
    linBtn->onClick = [this]
    {
        setParamBool ("linear", linBtn->getToggleState());
        if (display != nullptr)
            display->setLinearFreq (linBtn->getToggleState());
        repaint (freqAxisBounds);
        repaint (dbAxisBounds);
    };
    inspectBtn->onClick = [this]
    {
        inspect = inspectBtn->getToggleState();
        if (display != nullptr)
            display->setInspect (inspect, inspectX);
    };
    measureBtn->onClick = [this]
    {
        measurement = measureBtn->getToggleState();
        if (display != nullptr)
            display->setMeasurement (measurement);
    };
    bypassBtn->onClick = [this]
    {
        processor.setBypass (bypassBtn->getToggleState());
        if (display != nullptr)
            display->setBypassed (bypassBtn->getToggleState());
        repaint (headerBounds);
        repaint();
    };
    aboutBtn->onClick = [this] { showAboutDialog(); };

    sideChannel = makeKnob ("Channel", Sa16::Colours::orange);
    sideReactivity = makeKnob ("Reactivity", Sa16::Colours::accent);
    sideZoom = makeKnob ("dB Top", juce::Colour (0xffcc44ff));
    knSmoothing = makeKnob ("Smoothing", juce::Colour (0xff44aaff));
    knPreamp = makeKnob ("Preamp", juce::Colour (0xffff6622));

    for (auto* k : { sideChannel.get(), sideReactivity.get(), sideZoom.get(),
                     knSmoothing.get(), knPreamp.get() })
        addAndMakeVisible (k);

    sideChannel->onValueChanged = [this] (float v)
    {
        const float db = v * 200.0f - 100.0f;
        setParamFloat ("channel_gain", db);
        sideChannel->setSubLabel ((db >= 0 ? "+" : "") + juce::String ((int) std::round (db)) + " dB");
    };
    sideReactivity->onValueChanged = [this] (float v)
    {
        setParamFloat ("reactivity", v);
        const auto t = juce::String ((int) std::round (v * 100.0f)) + "%";
        sideReactivity->setSubLabel (t);
    };
    sideZoom->onValueChanged = [this] (float v)
    {
        setParamFloat ("zoom", v);
        const float top = Sa16::zoom01ToDbViewTop (v);
        const auto t = juce::String ((int) std::round (top)) + " dB";
        sideZoom->setSubLabel (t);
        if (display != nullptr)
        {
            display->setDbViewTop (top);
            repaint (dbAxisBounds);
        }
    };
    knPreamp->onValueChanged = [this] (float v)
    {
        const float db = v * 40.0f - 20.0f;
        setParamFloat ("preamp", db);
        knPreamp->setSubLabel ((db >= 0 ? "+" : "") + juce::String ((int) std::round (db)) + " dB");
    };
    knSmoothing->onValueChanged = [this] (float v)
    {
        setParamFloat ("smoothing", v);
        knSmoothing->setSubLabel (juce::String ((int) std::round (v * 100.0f)) + "%");
    };

    sideChannel->setValue (0.5f);
    sideChannel->setSubLabel ("+0 dB");
    sideReactivity->setValue (0.6f);
    sideReactivity->setSubLabel ("60%");
    const float zoom01 = Sa16::dbViewTopToZoom01 (Sa16::kDbViewTopDefault);
    sideZoom->setValue (zoom01);
    sideZoom->setSubLabel ("0 dB");
    knSmoothing->setValue (0.45f);
    knSmoothing->setSubLabel ("45%");
    knPreamp->setValue (0.5f);
    knPreamp->setSubLabel ("+0 dB");

    for (int i = 0; i < Sa16::kMaxChannels; ++i)
    {
        strips[(size_t) i] = std::make_unique<Sa16::ChannelStrip>();
        addAndMakeVisible (*strips[(size_t) i]);
        strips[(size_t) i]->setChannelIndex (i);
        strips[(size_t) i]->setState (processor.getChannelStates()[(size_t) i]);
        strips[(size_t) i]->onChanged = [this] (int idx, Sa16::ChannelState s)
        {
            applyChannelState (idx, s);
        };
    }

    resized();
    startTimerHz (60);
}

void Sa16SpectrumEditor::syncViewModeToUi (Sa16::ViewMode mode)
{
    processor.setViewMode (mode);

    if (display != nullptr)
        display->setViewMode (mode);

    for (int i = 0; i < (int) Sa16::ViewMode::NumModes; ++i)
        if (modeButtons[(size_t) i] != nullptr)
            modeButtons[(size_t) i]->setToggleState (i == (int) mode, juce::dontSendNotification);

    for (auto& mb : modeButtons)
        if (mb != nullptr)
            mb->repaint();

    repaint (dbAxisBounds);
    repaint (freqAxisBounds);
}

void Sa16SpectrumEditor::applyChannelState (int index, Sa16::ChannelState state)
{
    processor.getChannelStates()[(size_t) index] = state;
    if (strips[(size_t) index] != nullptr)
        strips[(size_t) index]->setState (state);
    if (display != nullptr)
        display->setChannelStates (processor.getChannelStates());
    processor.getEngine().setChannelEnabled (index, state.on);
    processor.getEngine().setChannelHold (index, state.hold);
}

void Sa16SpectrumEditor::setParamFloat (const juce::String& id, float value)
{
    if (auto* p = processor.getAPVTS().getParameter (id))
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            p->setValueNotifyingHost (ranged->convertTo0to1 (value));
}

float Sa16SpectrumEditor::getParamFloat (const juce::String& id, float fallback) const
{
    if (auto* p = processor.getAPVTS().getRawParameterValue (id))
        return p->load();
    return fallback;
}

void Sa16SpectrumEditor::setParamChoice (const juce::String& id, int index)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (processor.getAPVTS().getParameter (id)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
        p->endChangeGesture();
    }
}

void Sa16SpectrumEditor::setParamBool (const juce::String& id, bool value)
{
    if (auto* p = processor.getAPVTS().getParameter (id))
        p->setValueNotifyingHost (value ? 1.0f : 0.0f);
}

juce::String Sa16SpectrumEditor::formatFreq (float x01) const
{
    const bool linear = linBtn != nullptr && linBtn->getToggleState();
    const float fMax = Sa16::clampDisplayFreqMax (processor.getSampleRateHz());
    const float x = juce::jlimit (0.0f, 1.0f, x01);
    return Sa16::formatFreqLabel (juce::jlimit (Sa16::kFreqMinHz, juce::jmin (Sa16::kFreqMaxHz, fMax),
                                                Sa16::x01ToFreq (x, linear, fMax)), true);
}

void Sa16SpectrumEditor::showAboutDialog()
{
    if (aboutDialog != nullptr)
    {
        aboutDialog->toFront (true);
        return;
    }

    aboutDialog = std::make_unique<Sa16::AboutDialog>();
    aboutDialog->onClose = [this]
    {
        aboutDialog.reset();
    };
    addAndMakeVisible (*aboutDialog);
    aboutDialog->setBounds (getLocalBounds());
    aboutDialog->toFront (true);
}

void Sa16SpectrumEditor::timerCallback()
{
    if (uiReady)
        syncFromProcessor();
}

void Sa16SpectrumEditor::syncFromProcessor()
{
    if (display == nullptr)
        return;

    auto snap = processor.getEngine().getSnapshot();
    display->setChannelStates (processor.getChannelStates());
    display->setSnapshot (snap);
    display->setBypassed (processor.isBypassed());
    levelReadout = juce::String (snap.peakDb, 1) + " dB";
    if (inspect)
        freqReadout = formatFreq (inspectX);
    repaint (toggleBounds);
}

void Sa16SpectrumEditor::paint (juce::Graphics& g)
{
    g.fillAll (Sa16::Colours::shell);

    if (! uiReady)
    {
        g.setColour (Sa16::Colours::textMid);
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiTitle)));
        g.drawText ("Loading SA16...", getLocalBounds(), juce::Justification::centred);
        return;
    }

    // Header gradient
    if (! headerBounds.isEmpty())
    {
        juce::ColourGradient grad (Sa16::Colours::headerTop, 0.0f, (float) headerBounds.getY(),
                                   Sa16::Colours::headerBot, 0.0f, (float) headerBounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (headerBounds);
        g.setColour (Sa16::Colours::border);
        g.drawHorizontalLine (headerBounds.getBottom() - 1, (float) headerBounds.getX(), (float) headerBounds.getRight());

        // Neon logo bars
        const float heights[] = { 0.4f, 0.65f, 0.9f, 0.7f, 0.5f };
        float bx = (float) headerBounds.getX() + 14.0f;
        const float baseY = (float) headerBounds.getCentreY() + 8.0f;
        for (int i = 0; i < 5; ++i)
        {
            const float bh = heights[i] * 16.0f;
            g.setColour (Sa16::chColour (i * 3));
            g.fillRoundedRectangle (bx, baseY - bh, 3.0f, bh, 1.5f);
            bx += 5.0f;
        }

        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiTitle)));
        g.setColour (Sa16::Colours::textBright);
        g.drawText ("SA16", (int) bx + 6, headerBounds.getY(), 42, headerBounds.getHeight(),
                    juce::Justification::centredLeft);

        auto badge = juce::Rectangle<int> ((int) bx + 48, headerBounds.getCentreY() - 8, 140, 16);
        g.setColour (juce::Colour (0xff1e2530));
        g.drawRoundedRectangle (badge.toFloat(), 2.0f, 1.0f);
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiSmall)));
        g.setColour (juce::Colour (0xff667788));
        g.drawText ("SPECTRUM ANALYZER 016", badge, juce::Justification::centred);

        // Author + About + version (right side): [AI参数之间] [关于] [v1.0.0]
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiSmall)));
        g.setColour (Sa16::Colours::textMid);
        g.drawText (Sa16::authorName(),
                    headerBounds.getRight() - 248, headerBounds.getY(), 110, headerBounds.getHeight(),
                    juce::Justification::centredRight);

        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiSmall)));
        g.setColour (Sa16::Colours::textDim);
        g.drawText ("v" + juce::String (JucePlugin_VersionString),
                    headerBounds.getRight() - 44, headerBounds.getY(), 36, headerBounds.getHeight(),
                    juce::Justification::centredRight);
    }

    // Mode tab underline is drawn by NeonButtonLnf (tabStyle)

    if (display != nullptr)
    {
        const auto plot = display->getBounds();
        const bool spectrogram = display->getViewMode() == Sa16::ViewMode::Spectrogram;
        const bool linear = linBtn != nullptr && linBtn->getToggleState();
        const double sr = processor.getSampleRateHz() > 0.0 ? processor.getSampleRateHz() : 48000.0;
        const float fMax = Sa16::clampDisplayFreqMax (sr);

        if (! dbAxisBounds.isEmpty())
        {
            auto axis = juce::Rectangle<int> (dbAxisBounds.getX(), plot.getY(),
                                              dbAxisBounds.getWidth(), plot.getHeight());
            if (spectrogram)
                paintFreqAxisVertical (g, axis, linear, fMax);
            else
                paintDbAxis (g, axis, display->getDbViewTop(), display->getDbViewBottom());
        }
        if (! freqAxisBounds.isEmpty())
        {
            if (spectrogram)
                paintTimeAxisHorizontal (g, freqAxisBounds, plot, display->getSpectrogramSpanSeconds());
            else
                paintFreqAxis (g, freqAxisBounds, plot, linear, fMax);
        }
    }
    else if (! dbAxisBounds.isEmpty())
    {
        paintDbAxis (g, dbAxisBounds, 0.0f, Sa16::kDbFloor);
    }

    g.setColour (Sa16::Colours::surface);
    if (! rightBounds.isEmpty())
    {
        g.fillRect (rightBounds);
        g.setColour (Sa16::Colours::borderSoft);
        g.drawVerticalLine (rightBounds.getX(), (float) rightBounds.getY(), (float) rightBounds.getBottom());

        // Divider above FFT list
        const int divY = Sa16::rightPanelFftDividerY (rightBounds.getY());
        g.setColour (juce::Colour (0xff161a24));
        g.fillRect (rightBounds.getX() + 12, divY, rightBounds.getWidth() - 24, 1);

        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiMini)));
        g.setColour (Sa16::Colours::textDim);
        g.drawText ("FFT POINTS", rightBounds.getX(), Sa16::rightPanelFftLabelY (rightBounds.getY()),
                    rightBounds.getWidth(), Sa16::kFftLabelH, juce::Justification::centred);
    }

    if (! toggleBounds.isEmpty())
    {
        g.setColour (juce::Colour (0xff080a10));
        g.fillRect (toggleBounds);
        g.setColour (juce::Colour (0xff131620));
        g.drawHorizontalLine (toggleBounds.getY(), (float) toggleBounds.getX(), (float) toggleBounds.getRight());

        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontAxis)));
        g.setColour (Sa16::Colours::textDim);
        const int rx = toggleBounds.getRight() - 240;
        g.drawText ("FREQ", rx, toggleBounds.getY(), 40, toggleBounds.getHeight(), juce::Justification::centredLeft);
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiLarge)));
        g.setColour (Sa16::Colours::accent);
        g.drawText (freqReadout, rx + 40, toggleBounds.getY(), 78, toggleBounds.getHeight(), juce::Justification::centredLeft);
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontAxis)));
        g.setColour (Sa16::Colours::textDim);
        g.drawText ("LEVEL", rx + 122, toggleBounds.getY(), 44, toggleBounds.getHeight(), juce::Justification::centredLeft);
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiLarge)));
        g.setColour (Sa16::Colours::activeGreen);
        g.drawText (levelReadout, rx + 166, toggleBounds.getY(), 70, toggleBounds.getHeight(), juce::Justification::centredLeft);

        // Divider between toggles and window chips (after MEASURE)
        if (measureBtn != nullptr)
        {
            const int dx = measureBtn->getRight() + 5;
            g.setColour (juce::Colour (0xff1e2230));
            g.fillRect (dx, toggleBounds.getCentreY() - 9, 1, 18);
        }
    }

    if (! stripBounds.isEmpty())
    {
        g.setColour (juce::Colour (0xff070810));
        g.fillRect (stripBounds);
        g.setColour (juce::Colour (0xff0e1018));
        g.drawHorizontalLine (stripBounds.getY(), (float) stripBounds.getX(), (float) stripBounds.getRight());
    }

    if (! knobBounds.isEmpty())
    {
        juce::ColourGradient grad (juce::Colour (0xff07080d), 0.0f, (float) knobBounds.getY(),
                                   juce::Colour (0xff060710), 0.0f, (float) knobBounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (knobBounds);
        g.setColour (juce::Colour (0xff0e1018));
        g.drawHorizontalLine (knobBounds.getY(), (float) knobBounds.getX(), (float) knobBounds.getRight());

        // Status column (Figma right of bottom knobs)
        auto status = knobBounds.reduced (16, 12).removeFromRight (70);
        g.setColour (Sa16::Colours::activeGreen);
        g.fillEllipse ((float) status.getX(), (float) status.getY() + 4.0f, 5.0f, 5.0f);
        g.setFont (juce::Font (juce::FontOptions (Sa16::kFontUiSmall)));
        g.setColour (Sa16::Colours::textDim);
        g.drawText ("ACTIVE", status.getX() + 10, status.getY(), 50, 14, juce::Justification::centredLeft);

        int onCount = 0;
        for (auto& ch : processor.getChannelStates())
            if (ch.on) ++onCount;

        g.setColour (Sa16::Colours::textMid);
        g.drawText (juce::String (onCount) + "/16 CH", status.getX(), status.getY() + 18, 60, 12,
                    juce::Justification::centredLeft);
        const double sr = processor.getSampleRateHz();
        g.drawText (juce::String (sr / 1000.0, 0) + " kHz", status.getX(), status.getY() + 32, 60, 12,
                    juce::Justification::centredLeft);
        g.drawText ("32-bit", status.getX(), status.getY() + 46, 60, 12, juce::Justification::centredLeft);
    }

    // Outer border
    g.setColour (Sa16::Colours::border);
    g.drawRect (getLocalBounds().toFloat().reduced (0.5f), 1.0f);
}

void Sa16SpectrumEditor::resized()
{
    auto shell = getLocalBounds();
    headerBounds = shell.removeFromTop (Sa16::kHeaderH);
    knobBounds = shell.removeFromBottom (Sa16::kKnobH);
    stripBounds = shell.removeFromBottom (Sa16::kStripH);
    toggleBounds = shell.removeFromBottom (Sa16::kToggleH);
    freqAxisBounds = shell.removeFromBottom (Sa16::kFreqAxisH);
    dbAxisBounds = shell.removeFromLeft (Sa16::kDbAxisW);
    rightBounds = shell.removeFromRight (Sa16::kRightPanelW);
    plotBounds = shell; // designed ~332px

    if (! uiReady)
        return;

    if (display != nullptr)
        display->setBounds (plotBounds);

    if (aboutDialog != nullptr)
        aboutDialog->setBounds (getLocalBounds());

    // Mode tabs; leave room for author + About + version
    auto modes = headerBounds.withTrimmedLeft (200).withTrimmedRight (256).reduced (0, 3);
    const int modeW = juce::jmax (1, modes.getWidth() / (int) Sa16::ViewMode::NumModes);
    for (int i = 0; i < (int) Sa16::ViewMode::NumModes; ++i)
        if (modeButtons[(size_t) i] != nullptr)
            modeButtons[(size_t) i]->setBounds (modes.removeFromLeft (modeW).reduced (1, 0));

    if (aboutBtn != nullptr)
        aboutBtn->setBounds (headerBounds.getRight() - 100, headerBounds.getY() + 5, 48, headerBounds.getHeight() - 10);

    auto right = rightBounds.reduced (Sa16::kRightPanelPadX, Sa16::kRightPanelPadTop);
    right.removeFromTop (Sa16::rightPanelFftHeaderH());

    if (bypassBtn != nullptr)
    {
        auto bypassArea = right.removeFromBottom (Sa16::kBypassButtonH);
        right.removeFromBottom (Sa16::kBypassGapAbove);
        bypassBtn->setBounds (bypassArea);
    }

    const int fftGapTotal = Sa16::kFftButtonGap * (Sa16::kNumFftSizes - 1);
    const int fftH = juce::jmax (Sa16::kFftMinButtonH,
                                 (right.getHeight() - fftGapTotal) / Sa16::kNumFftSizes);
    for (int i = 0; i < Sa16::kNumFftSizes; ++i)
    {
        if (fftButtons[(size_t) i] != nullptr)
        {
            fftButtons[(size_t) i]->setBounds (right.removeFromTop (fftH));
            if (i + 1 < Sa16::kNumFftSizes)
                right.removeFromTop (Sa16::kFftButtonGap);
        }
    }

    auto toggles = toggleBounds.reduced (12, 6);
    if (freezeBtn != nullptr) freezeBtn->setBounds (toggles.removeFromLeft (78).reduced (2, 0));
    if (linBtn != nullptr) linBtn->setBounds (toggles.removeFromLeft (40).reduced (2, 0));
    if (inspectBtn != nullptr) inspectBtn->setBounds (toggles.removeFromLeft (70).reduced (2, 0));
    if (measureBtn != nullptr) measureBtn->setBounds (toggles.removeFromLeft (70).reduced (2, 0));
    toggles.removeFromLeft (10); // gap for divider painted in paint()
    for (int i = 0; i < (int) Sa16::WindowType::NumWindows; ++i)
        if (windowButtons[(size_t) i] != nullptr)
            windowButtons[(size_t) i]->setBounds (toggles.removeFromLeft (62).reduced (2, 0));

    auto stripsArea = stripBounds.reduced (12, 6);
    const int stripW = juce::jmax (1, stripsArea.getWidth() / Sa16::kMaxChannels);
    for (int i = 0; i < Sa16::kMaxChannels; ++i)
        if (strips[(size_t) i] != nullptr)
            strips[(size_t) i]->setBounds (stripsArea.removeFromLeft (stripW).reduced (2, 0));

    auto knobs = knobBounds.reduced (16, 8).withTrimmedRight (80);
    const int kw = juce::jmax (1, knobs.getWidth() / 5);
    Sa16::NeonKnob* bottom[] = {
        sideChannel.get(), sideReactivity.get(), sideZoom.get(),
        knSmoothing.get(), knPreamp.get()
    };
    for (auto* k : bottom)
        if (k != nullptr)
            k->setBounds (knobs.removeFromLeft (kw).reduced (4, 0));
}
