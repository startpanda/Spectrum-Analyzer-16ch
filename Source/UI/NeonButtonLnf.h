#pragma once

#include "Constants.h"

namespace Sa16
{
/** Flat neon chip matching Figma Make `Btn`: 1px border, tint fill, no JUCE chrome. */
struct NeonButtonLnf final : juce::LookAndFeel_V4
{
    float fontSize = kFontUiSmall;
    float corner = 3.0f;
    bool tabStyle = false; // mode tabs: underline instead of full neon border

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (juce::FontOptions (fontSize));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool shouldDrawButtonAsHighlighted,
                               bool /*shouldDrawButtonAsDown*/) override
    {
        auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        const auto accent = button.findColour (juce::TextButton::textColourOnId);

        // Non-toggle action chips (CLEAR / UNSOLO)
        if (! button.getClickingTogglesState())
        {
            const auto fill = button.findColour (juce::TextButton::buttonColourId);
            const auto outline = button.findColour (juce::ComboBox::outlineColourId);
            g.setColour (fill.isTransparent() ? juce::Colour (0x08ffffff) : fill);
            g.fillRoundedRectangle (r, corner);
            g.setColour (outline.isTransparent() ? juce::Colour (0xff1a2030) : outline);
            g.drawRoundedRectangle (r, corner, 1.0f);
            return;
        }

        if (tabStyle)
        {
            if (on)
            {
                g.setColour (juce::Colour (0xff1a2035));
                g.fillRoundedRectangle (r, corner);
                g.setColour (juce::Colour (0xff2a3550));
                g.drawRoundedRectangle (r, corner, 1.0f);
                g.setColour (Colours::accent);
                g.fillRect (r.getX() + 2.0f, r.getBottom() - 2.0f, r.getWidth() - 4.0f, 2.0f);
            }
            else if (shouldDrawButtonAsHighlighted)
            {
                g.setColour (juce::Colour (0x10ffffff));
                g.fillRoundedRectangle (r, corner);
            }
            return;
        }

        if (on)
        {
            g.setColour (accent.withAlpha (0.125f));
            g.fillRoundedRectangle (r, corner);
            g.setColour (accent.withAlpha (0.38f));
            g.drawRoundedRectangle (r, corner, 1.0f);
            g.setColour (accent.withAlpha (0.12f));
            g.drawRoundedRectangle (r.expanded (0.5f), corner + 0.5f, 1.5f);
        }
        else
        {
            g.setColour (juce::Colour (0x08ffffff));
            g.fillRoundedRectangle (r, corner);
            g.setColour (shouldDrawButtonAsHighlighted ? juce::Colour (0xff323848)
                                                       : juce::Colour (0xff252830));
            g.drawRoundedRectangle (r, corner, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool /*shouldDrawButtonAsHighlighted*/,
                         bool /*shouldDrawButtonAsDown*/) override
    {
        const bool on = button.getToggleState();
        const bool bold = on || ! button.getClickingTogglesState();
        g.setFont (juce::Font (juce::FontOptions (fontSize).withStyle (bold ? "Bold" : "Regular")));

        if (! button.getClickingTogglesState())
            g.setColour (button.findColour (juce::TextButton::textColourOffId));
        else if (tabStyle)
            g.setColour (on ? juce::Colour (0xffc8d8e8) : Colours::textDim);
        else
            g.setColour (on ? button.findColour (juce::TextButton::textColourOnId)
                            : button.findColour (juce::TextButton::textColourOffId));

        g.drawText (button.getButtonText(), button.getLocalBounds(),
                    juce::Justification::centred, false);
    }
};

inline NeonButtonLnf& neonChipLnf()
{
    static NeonButtonLnf lnf;
    static bool once = (lnf.fontSize = kFontUiSmall, lnf.corner = 3.0f, lnf.tabStyle = false, true);
    juce::ignoreUnused (once);
    return lnf;
}

inline NeonButtonLnf& neonMiniLnf()
{
    static NeonButtonLnf lnf;
    static bool once = (lnf.fontSize = kFontUiMini, lnf.corner = 2.0f, lnf.tabStyle = false, true);
    juce::ignoreUnused (once);
    return lnf;
}

inline NeonButtonLnf& neonTabLnf()
{
    static NeonButtonLnf lnf;
    static bool once = (lnf.fontSize = kFontUiSmall, lnf.corner = 3.0f, lnf.tabStyle = true, true);
    juce::ignoreUnused (once);
    return lnf;
}

inline NeonButtonLnf& neonFftLnf()
{
    static NeonButtonLnf lnf;
    static bool once = (lnf.fontSize = kFontUi, lnf.corner = 3.0f, lnf.tabStyle = false, true);
    juce::ignoreUnused (once);
    return lnf;
}

inline void styleNeonChip (juce::TextButton& b, juce::Colour accent, NeonButtonLnf& lnf)
{
    b.setLookAndFeel (&lnf);
    b.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff8a9aaa));
    b.setColour (juce::TextButton::textColourOnId, accent);
    b.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    b.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    b.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
}

inline void styleNeonChip (juce::TextButton& b, juce::Colour accent)
{
    styleNeonChip (b, accent, neonChipLnf());
}

inline void styleNeonAction (juce::TextButton& b, juce::Colour accent, bool tinted)
{
    b.setLookAndFeel (&neonChipLnf());
    b.setClickingTogglesState (false);
    b.setColour (juce::TextButton::textColourOffId, accent);
    b.setColour (juce::TextButton::textColourOnId, accent);
    b.setColour (juce::TextButton::buttonColourId,
                 tinted ? accent.withAlpha (0.07f) : juce::Colours::transparentBlack);
    b.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.12f));
    b.setColour (juce::ComboBox::outlineColourId,
                 tinted ? accent.withAlpha (0.2f) : juce::Colour (0xff1a2030));
}

/** Header Bypass: LED + label matching Figma Make. */
struct BypassButtonLnf final : juce::LookAndFeel_V4
{
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&, bool, bool) override
    {
        auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();

        g.setColour (on ? juce::Colour (0x15ff2244) : juce::Colour (0xff111520));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (on ? juce::Colour (0x44ff2244) : juce::Colour (0xff252836));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        const float led = 7.0f;
        const float lx = r.getX() + 8.0f;
        const float ly = r.getCentreY() - led * 0.5f;
        if (on)
        {
            g.setColour (Colours::bypassRed.withAlpha (0.35f));
            g.fillEllipse (lx - 2.0f, ly - 2.0f, led + 4.0f, led + 4.0f);
        }
        g.setColour (on ? Colours::bypassRed : juce::Colour (0xff1e2530));
        g.fillEllipse (lx, ly, led, led);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        const bool on = button.getToggleState();
        g.setFont (juce::Font (juce::FontOptions (kFontUiSmall)));
        g.setColour (on ? Colours::bypassRed : juce::Colour (0xff445566));
        auto text = button.getLocalBounds().withTrimmedLeft (20).withTrimmedRight (4);
        g.drawText ("BYPASS", text, juce::Justification::centredLeft, false);
    }
};

inline BypassButtonLnf& bypassLnf()
{
    static BypassButtonLnf lnf;
    return lnf;
}

inline void styleBypassButton (juce::TextButton& b)
{
    b.setLookAndFeel (&bypassLnf());
    b.setClickingTogglesState (true);
    b.setButtonText ("BYPASS");
    b.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    b.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    b.setColour (juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    b.setColour (juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
}

/** Header preset ComboBox matching Figma Make select. */
struct NeonComboLnf final : juce::LookAndFeel_V4
{
    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (kFontUiSmall));
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font (juce::FontOptions (kFontUiSmall));
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        g.setColour (juce::Colour (0xff111520));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (box.hasKeyboardFocus (true) ? juce::Colour (0xff3a4558) : juce::Colour (0xff252836));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        // Chevron
        const float cx = (float) width - 10.0f;
        const float cy = (float) height * 0.5f;
        juce::Path chevron;
        chevron.startNewSubPath (cx - 3.5f, cy - 1.5f);
        chevron.lineTo (cx, cy + 2.0f);
        chevron.lineTo (cx + 3.5f, cy - 1.5f);
        g.setColour (juce::Colour (0xff556677));
        g.strokePath (chevron, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (6, 1, box.getWidth() - 22, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, juce::Colour (0xffa8b8c8));
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (juce::Colour (0xff0e1118));
        g.setColour (juce::Colour (0xff252836));
        g.drawRect (0, 0, width, height, 1);
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                            bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        if (isSeparator)
        {
            g.setColour (juce::Colour (0xff1a2030));
            g.fillRect (area.reduced (6, 0).withHeight (1).withY (area.getCentreY()));
            return;
        }

        if (isHighlighted && isActive)
        {
            g.setColour (juce::Colour (0xff1a2035));
            g.fillRect (area);
        }

        g.setFont (getPopupMenuFont());
        g.setColour (isTicked ? Colours::accent
                              : (isActive ? juce::Colour (0xff889aaa) : juce::Colour (0xff445566)));
        g.drawText (text, area.reduced (10, 0), juce::Justification::centredLeft, true);
        juce::ignoreUnused (hasSubMenu, shortcutKeyText, icon, textColour);
    }
};

inline NeonComboLnf& neonComboLnf()
{
    static NeonComboLnf lnf;
    return lnf;
}

inline void styleNeonCombo (juce::ComboBox& box)
{
    box.setLookAndFeel (&neonComboLnf());
    box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111520));
    box.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    box.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff8a9aaa));
    box.setColour (juce::ComboBox::textColourId, juce::Colour (0xffa8b8c8));
    box.setColour (juce::ComboBox::focusedOutlineColourId, juce::Colours::transparentBlack);
    box.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0e1118));
    box.setColour (juce::PopupMenu::textColourId, juce::Colour (0xffa8b8c8));
    box.setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff1a2035));
    box.setColour (juce::PopupMenu::highlightedTextColourId, Colours::accent);
}
} // namespace Sa16
