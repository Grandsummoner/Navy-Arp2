#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "AppTheme.h"

class PluginEditor; // Forward declaration to prevent circular include

class ChromaCapsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromaCapsLookAndFeel (PluginProcessor& p, PluginEditor* e = nullptr);

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, 
                           float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, 
                           juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    void drawVectorDice (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour pipColour);

    PluginProcessor& processor;
    PluginEditor* editor;
};