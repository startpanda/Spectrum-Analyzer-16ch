#include "UI/ChannelStrip.h"
#include "UI/NeonButtonLnf.h"

namespace Sa16
{
ChannelStrip::ChannelStrip()
{
    for (auto* b : { &onBtn, &soloBtn, &holdBtn, &msBtn })
    {
        addAndMakeVisible (*b);
        b->setClickingTogglesState (true);
    }

    onBtn.onClick = [this]
    {
        state.on = onBtn.getToggleState();
        notify();
        repaint();
    };
    soloBtn.onClick = [this]
    {
        state.solo = soloBtn.getToggleState();
        notify();
        repaint();
    };
    holdBtn.onClick = [this]
    {
        state.hold = holdBtn.getToggleState();
        notify();
        repaint();
    };
    msBtn.onClick = [this]
    {
        state.mids = msBtn.getToggleState();
        notify();
        repaint();
    };
}

ChannelStrip::~ChannelStrip()
{
    for (auto* b : { &onBtn, &soloBtn, &holdBtn, &msBtn })
        b->setLookAndFeel (nullptr);
}

void ChannelStrip::setChannelIndex (int index)
{
    channelIndex = index;
    msBtn.setButtonText (channelIndex % 2 == 0 ? CharPointer_UTF8 ("L\xe2\x86\x92M")
                                               : CharPointer_UTF8 ("R\xe2\x86\x92S"));
    styleMini (onBtn, chColour (channelIndex));
    styleMini (soloBtn, juce::Colour (0xffffe600));
    styleMini (holdBtn, Colours::orange);
    styleMini (msBtn, juce::Colour (0xffaa44ff));
    repaint();
}

void ChannelStrip::setState (const ChannelState& s)
{
    state = s;
    onBtn.setToggleState (state.on, juce::dontSendNotification);
    soloBtn.setToggleState (state.solo, juce::dontSendNotification);
    holdBtn.setToggleState (state.hold, juce::dontSendNotification);
    msBtn.setToggleState (state.mids, juce::dontSendNotification);
    setAlpha (state.on ? 1.0f : 0.4f);
    repaint();
}

void ChannelStrip::styleMini (juce::TextButton& b, juce::Colour activeColour)
{
    styleNeonChip (b, activeColour, neonMiniLnf());
    b.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff6a7585));
}

void ChannelStrip::notify()
{
    setAlpha (state.on ? 1.0f : 0.4f);
    if (onChanged)
        onChanged (channelIndex, state);
}

void ChannelStrip::resized()
{
    auto r = getLocalBounds().reduced (3, 3);
    r.removeFromTop (16);
    auto row = r.removeFromTop (18);
    const int w = row.getWidth() / 3;
    onBtn.setBounds (row.removeFromLeft (w).reduced (1, 0));
    soloBtn.setBounds (row.removeFromLeft (w).reduced (1, 0));
    holdBtn.setBounds (row.reduced (1, 0));
    r.removeFromTop (3);
    msBtn.setBounds (r.removeFromTop (16));
}

void ChannelStrip::paint (juce::Graphics& g)
{
    auto colour = chColour (channelIndex);
    g.setColour (state.on ? juce::Colour (0xff0d0f16) : juce::Colour (0xff080910));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    g.setColour (state.on ? colour.withAlpha (0.16f) : juce::Colour (0xff111420));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);

    auto header = getLocalBounds().reduced (6, 5).removeFromTop (12);
    g.setColour (state.on ? colour : juce::Colour (0xff1e2230));
    g.fillEllipse ((float) header.getX(), (float) header.getY() + 2.0f, 8.0f, 8.0f);
    if (state.on)
    {
        g.setColour (colour.withAlpha (0.35f));
        g.drawEllipse ((float) header.getX() - 1.0f, (float) header.getY() + 1.0f, 10.0f, 10.0f, 1.5f);
    }

    g.setFont (juce::Font (juce::FontOptions (kFontUiMini).withStyle ("Bold")));
    g.setColour (state.on ? colour : Colours::textMuted);
    g.drawText (juce::String (channelIndex + 1).paddedLeft ('0', 2),
                header.withTrimmedLeft (12), juce::Justification::centredLeft);
}
} // namespace Sa16
