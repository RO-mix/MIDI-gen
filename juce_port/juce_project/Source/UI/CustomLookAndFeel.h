#pragma once
#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (button.getButtonText() == "OVR")
        {
            auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

            // Draw square button
            auto baseColour = button.getToggleState() ? juce::Colours::orange.withAlpha(0.9f)
                                                      : juce::Colours::darkgrey.withAlpha(0.5f);

            if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
                baseColour = baseColour.brighter(0.2f);

            g.setColour(baseColour);
            g.fillRect(bounds);

            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(juce::FontOptions(14.0f));
            g.drawText("OVR", bounds, juce::Justification::centred, 1);
        }
        else
        {
            // Default drawing for other toggle buttons
            LookAndFeel_V4::drawToggleButton(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }
};
