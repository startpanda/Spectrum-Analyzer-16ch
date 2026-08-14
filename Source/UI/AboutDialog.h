#pragma once

#include "Constants.h"
#include "UI/NeonButtonLnf.h"

namespace Sa16
{
/** Dark neon About dialog matching the SA16 editor chrome. */
class AboutDialog final : public juce::Component
{
public:
    AboutDialog()
    {
        setOpaque (false);

        closeBtn.setButtonText ("OK");
        styleNeonChip (closeBtn, Colours::accent);
        closeBtn.setClickingTogglesState (false);
        closeBtn.onClick = [this]
        {
            if (onClose)
                onClose();
        };
        addAndMakeVisible (closeBtn);

        emailBtn.setButtonText (kAuthorEmail);
        styleNeonAction (emailBtn, Colours::accent, false);
        emailBtn.onClick = []
        {
            juce::URL ("mailto:" + juce::String (kAuthorEmail)).launchInDefaultBrowser();
        };
        addAndMakeVisible (emailBtn);
    }

    ~AboutDialog() override
    {
        closeBtn.setLookAndFeel (nullptr);
        emailBtn.setLookAndFeel (nullptr);
    }

    std::function<void()> onClose;

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0x99000000));

        auto card = getCardBounds().toFloat();

        g.setColour (juce::Colour (0xff0a0c12));
        g.fillRoundedRectangle (card, 8.0f);

        juce::ColourGradient top (juce::Colour (0xff141820), card.getX(), card.getY(),
                                  juce::Colour (0xff0a0c12), card.getX(), card.getY() + 56.0f, false);
        g.setGradientFill (top);
        g.fillRoundedRectangle (card.getX(), card.getY(), card.getWidth(), 56.0f, 8.0f);
        g.setColour (juce::Colour (0xff0a0c12));
        g.fillRect (card.getX(), card.getY() + 40.0f, card.getWidth(), 20.0f);

        g.setColour (Colours::accent.withAlpha (0.35f));
        g.drawRoundedRectangle (card.reduced (0.5f), 8.0f, 1.2f);
        g.setColour (juce::Colour (0xff1e2530));
        g.drawRoundedRectangle (card.reduced (2.0f), 7.0f, 1.0f);

        // Neon bars
        const float heights[] = { 0.4f, 0.65f, 0.9f, 0.7f, 0.5f };
        float bx = card.getX() + 22.0f;
        const float baseY = card.getY() + 34.0f;
        for (int i = 0; i < 5; ++i)
        {
            const float bh = heights[i] * 18.0f;
            g.setColour (chColour (i * 3));
            g.fillRoundedRectangle (bx, baseY - bh, 3.5f, bh, 1.5f);
            bx += 6.0f;
        }

        g.setFont (juce::Font (juce::FontOptions (kFontUiTitle)));
        g.setColour (Colours::textBright);
        g.drawText ("SA16", (int) bx + 8, (int) card.getY() + 12, 50, 28, juce::Justification::centredLeft);

        g.setFont (juce::Font (juce::FontOptions (kFontUiSmall)));
        g.setColour (Colours::textDim);
        g.drawText (aboutLabel(), (int) card.getRight() - 72, (int) card.getY() + 14, 52, 22,
                    juce::Justification::centredRight);

        auto body = card.reduced (24.0f, 0.0f).withTrimmedTop (64.0f).withTrimmedBottom (58.0f);

        g.setFont (juce::Font (juce::FontOptions (kFontUi)));
        g.setColour (Colours::textDim);
        g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe4\xbd\x9c\xe8\x80\x85")),
                    body.removeFromTop (18.0f), juce::Justification::centredLeft);

        g.setFont (juce::Font (juce::FontOptions (15.0f)).boldened());
        g.setColour (Colours::accent);
        g.drawText (authorName(), body.removeFromTop (26.0f), juce::Justification::centredLeft);

        body.removeFromTop (10.0f);
        g.setFont (juce::Font (juce::FontOptions (kFontUi)));
        g.setColour (Colours::textDim);
        g.drawText ("Email", body.removeFromTop (18.0f), juce::Justification::centredLeft);

        body.removeFromTop (28.0f); // email button sits here via resized()

        body.removeFromTop (8.0f);
        g.setColour (juce::Colour (0xff161a24));
        g.fillRect (body.getX(), body.getY(), body.getWidth(), 1.0f);
        body.removeFromTop (12.0f);

        g.setFont (juce::Font (juce::FontOptions (kFontUi)));
        g.setColour (Colours::textMid);
        g.drawText ("SA16 Spectrum Analyzer", body.removeFromTop (20.0f), juce::Justification::centredLeft);

        g.setFont (juce::Font (juce::FontOptions (kFontUiSmall)));
        g.setColour (Colours::textDim);
        g.drawText ("v" + juce::String (JucePlugin_VersionString) + "   ·   16-Channel Analyzer",
                    body.removeFromTop (18.0f), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto card = getCardBounds();
        closeBtn.setBounds (card.getCentreX() - 40, card.getBottom() - 42, 80, 28);

        auto emailArea = card.reduced (24, 0).withTrimmedTop (118).withHeight (26);
        emailBtn.setBounds (emailArea);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! getCardBounds().contains (e.getPosition()) && onClose)
            onClose();
    }

private:
    juce::Rectangle<int> getCardBounds() const
    {
        constexpr int w = 340;
        constexpr int h = 280;
        return { (getWidth() - w) / 2, (getHeight() - h) / 2, w, h };
    }

    juce::TextButton closeBtn, emailBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutDialog)
};
} // namespace Sa16
