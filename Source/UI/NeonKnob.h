#pragma once

#include "Constants.h"

namespace Sa16
{
class NeonKnob final : public juce::Component
{
public:
    NeonKnob();
    ~NeonKnob() override;

    void setColour (juce::Colour c);
    void setLabel (const juce::String& text);
    void setSubLabel (const juce::String& text);
    juce::String getSubLabel() const noexcept { return subLabel; }
    void setValue (float v01);
    float getValue() const noexcept { return value; }

    std::function<void (float)> onValueChanged;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    static constexpr float kBottomTextH = 26.0f;
    static constexpr float kLabelH = 16.0f;

    juce::Colour accent { Colours::orange };
    juce::String label, subLabel;
    float value = 0.5f;
    float dragStartValue = 0.5f;
    int dragStartY = 0;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonKnob)
};
} // namespace Sa16
