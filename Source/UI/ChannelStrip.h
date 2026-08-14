#pragma once

#include "Constants.h"

namespace Sa16
{
class ChannelStrip final : public juce::Component
{
public:
    ChannelStrip();
    ~ChannelStrip() override;

    void setChannelIndex (int index);
    void setState (const ChannelState& state);
    ChannelState getState() const noexcept { return state; }

    std::function<void (int, ChannelState)> onChanged;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void notify();
    void styleMini (juce::TextButton& b, juce::Colour activeColour);

    int channelIndex = 0;
    ChannelState state;
    juce::TextButton onBtn { "ON" }, soloBtn { "S" }, holdBtn { "H" }, msBtn { "L->M" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelStrip)
};
} // namespace Sa16
