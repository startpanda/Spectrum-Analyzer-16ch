#include "UI/NeonKnob.h"

namespace Sa16
{
NeonKnob::NeonKnob()
{
    setRepaintsOnMouseActivity (true);
}

NeonKnob::~NeonKnob() = default;

void NeonKnob::setColour (juce::Colour c) { accent = c; repaint(); }
void NeonKnob::setLabel (const juce::String& text) { label = text; repaint(); }

void NeonKnob::setSubLabel (const juce::String& text)
{
    subLabel = text;
    repaint();
}

void NeonKnob::setValue (float v01)
{
    value = juce::jlimit (0.0f, 1.0f, v01);
    repaint();
}

void NeonKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    if (bounds.getWidth() < 8.0f || bounds.getHeight() < 8.0f)
        return;

    const float textH = subLabel.isNotEmpty() ? kBottomTextH : 16.0f;
    auto knobArea = bounds.removeFromTop (bounds.getHeight() - textH);
    const float size = juce::jmin (knobArea.getWidth(), knobArea.getHeight());
    if (size < 12.0f)
        return;

    const float cx = knobArea.getCentreX();
    const float cy = knobArea.getCentreY();
    const float r = size * 0.5f - 3.0f;
    if (r < 4.0f)
        return;

    g.setColour (juce::Colour (0xff0d0e12));
    g.fillEllipse (cx - r - 3.0f, cy - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f);
    g.setColour (juce::Colour (0xff1e2028));
    g.drawEllipse (cx - r - 3.0f, cy - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f, 1.0f);

    juce::ColourGradient body (juce::Colour (0xff2a2d38), cx - r * 0.4f, cy - r * 0.5f,
                               juce::Colour (0xff0f1015), cx, cy + r, true);
    g.setGradientFill (body);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colour (0xff2a2d38));
    g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

    const float startDeg = -135.0f;
    const float sweep = 270.0f;
    const float trackR = r - 3.0f;

    auto arc = [&] (float a0, float a1, juce::Colour colour, float thickness)
    {
        juce::Path p;
        p.addCentredArc (cx, cy, trackR, trackR, 0.0f,
                         juce::degreesToRadians (a0 - 90.0f),
                         juce::degreesToRadians (a1 - 90.0f), true);
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    };

    arc (startDeg, startDeg + sweep, juce::Colour (0xff1a1d26), 3.0f);
    if (value > 0.001f)
        arc (startDeg, startDeg + sweep * value, accent, 3.0f);

    const float angle = juce::degreesToRadians (startDeg + sweep * value);
    const float ix = cx + (trackR - 4.0f) * std::sin (angle);
    const float iy = cy - (trackR - 4.0f) * std::cos (angle);
    g.setColour (accent);
    g.drawLine (cx, cy, ix, iy, 2.0f);
    g.setColour (juce::Colour (0xff1a1d24));
    g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
    g.setColour (juce::Colour (0xff333333));
    g.drawEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f, 1.0f);

    g.setFont (juce::Font (juce::FontOptions (kFontKnobLabel)));
    g.setColour (Colours::textMid);
    g.drawText (label, bounds.removeFromTop (kLabelH), juce::Justification::centred);

    if (subLabel.isNotEmpty())
    {
        g.setFont (juce::Font (juce::FontOptions (kFontKnobSub)));
        g.setColour (Colours::textDim);
        g.drawText (subLabel, bounds, juce::Justification::centred);
    }
}

void NeonKnob::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    dragStartY = e.y;
    dragStartValue = value;
}

void NeonKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    const float next = juce::jlimit (0.0f, 1.0f, dragStartValue + (float) (dragStartY - e.y) / 140.0f);
    if (! juce::approximatelyEqual (next, value))
    {
        value = next;
        if (onValueChanged)
            onValueChanged (value);
        repaint();
    }
}

void NeonKnob::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

void NeonKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const float next = juce::jlimit (0.0f, 1.0f, value + wheel.deltaY * 0.05f);
    value = next;
    if (onValueChanged)
        onValueChanged (value);
    repaint();
}
} // namespace Sa16
