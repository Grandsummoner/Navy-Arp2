#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "AppTheme.h"

class OledDisplay : public juce::Component, public juce::Timer
{
public:
    OledDisplay (PluginProcessor& p);
    ~OledDisplay() override;

    void timerCallback() override;
    void paint (juce::Graphics& g) override;

private:
    PluginProcessor& processor;
    int lastLfoRates[8] = { 0 };
    float lastLfoDepths[8] = { 0.0f };
    int lfoOverlayTimer = 0;
    int lfoActiveParamIdx = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OledDisplay)
};
