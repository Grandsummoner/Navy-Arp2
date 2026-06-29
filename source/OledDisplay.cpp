#include "OledDisplay.h"
#include "PluginProcessor.h"

OledDisplay::OledDisplay (PluginProcessor& p)
    : processor (p)
{
}

OledDisplay::~OledDisplay()
{
}

void OledDisplay::showParameterOverlay (const juce::String& paramName, float baseValue, const juce::String& lfoVibeText)
{
    activeParamName = paramName;
    activeParamValue = baseValue;
    activeLfoVibe = lfoVibeText;
    isOverlayActive = true;
    
    repaint();
    startTimer (1500); // 1.5 second display timeout
}

void OledDisplay::setFreezeActive (bool shouldBeActive)
{
    if (isFreezeActive != shouldBeActive)
    {
        isFreezeActive = shouldBeActive;
        repaint();
    }
}

void OledDisplay::timerCallback()
{
    isOverlayActive = false;
    stopTimer();
    repaint();
}

void OledDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Fill Screen Background (Obsidian #0F1116)
    g.setColour (juce::Colour::fromString ("#FF0F1116"));
    g.fillRoundedRectangle (bounds, 4.0f);

    // 2. Draw Bezel with Dual Offset Lines (Inset Glass Look)
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawHorizontalLine (0, 0.0f, bounds.getWidth());
    g.drawVerticalLine (0, 0.0f, bounds.getHeight());

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (static_cast<int> (bounds.getHeight() - 1.0f), 0.0f, bounds.getWidth());
    g.drawVerticalLine (static_cast<int> (bounds.getWidth() - 1.0f), 0.0f, bounds.getHeight());

    auto displayArea = bounds.reduced (12.0f);

    if (isOverlayActive)
    {
        // =====================================================================
        // RENDER: MOMENTARY PARAMETER OVERLAY SCREEN (ACTIVE ONLY ON INTERACTION)
        // =====================================================================
        
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colours::white);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (activeParamName.toUpperCase(), displayArea.removeFromTop (18.0f), juce::Justification::left, true);

        displayArea.removeFromTop (4.0f);

        auto valueBarArea = displayArea.removeFromTop (12.0f);
        g.setColour (juce::Colours::darkgrey.withAlpha (0.3f));
        g.fillRoundedRectangle (valueBarArea, 2.0f);
        
        float fillWidth = valueBarArea.getWidth() * activeParamValue;
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour (0xFF00D2FF));
        g.fillRoundedRectangle (valueBarArea.withWidth (fillWidth), 2.0f);

        displayArea.removeFromTop (6.0f);

        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (9.5f, juce::Font::plain));
        
        juce::String valuePercentStr = "VALUE: " + juce::String (static_cast<int> (activeParamValue * 100.0f)) + "%";
        g.drawText (valuePercentStr, displayArea.removeFromTop (12.0f), juce::Justification::left, true);
        g.drawText ("LFO: " + activeLfoVibe, displayArea.removeFromTop (12.0f), juce::Justification::left, true);
    }
    else
    {
        // =====================================================================
        // RENDER: SEQUENCER PROGRESS GRID & RUNNING STEP PLAYHEAD
        // =====================================================================
        
        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        
        auto stepArea = displayArea.removeFromTop (24.0f);
        const float totalSpacing = 4.0f;
        const float stepWidth = (stepArea.getWidth() - (7.0f * totalSpacing)) / 8.0f;
        const int activeStep = processor.currentStep; // Fetches the actual running playhead step
        const bool isPlaying = processor.isCurrentlyPlayingUI.load();

        // Draw 8 horizontal sequencer step blocks
        for (int i = 0; i < 8; ++i)
        {
            auto cell = stepArea.removeFromLeft (stepWidth);
            
            // Draw step container background
            g.setColour (juce::Colour (0xFF181C24).withAlpha (0.6f));
            g.fillRoundedRectangle (cell, 2.0f);

            // Highlight the cell that matches the active playhead step in real-time
            if (isPlaying && i == activeStep)
            {
                g.setColour (isFreezeActive ? juce::Colour (0xFF80D8FF) : juce::Colour (0xFF00D2FF));
                g.fillRoundedRectangle (cell, 2.0f);
                g.setColour (juce::Colours::black);
            }
            else
            {
                g.setColour (juce::Colours::grey);
            }

            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText (juce::String (i + 1), cell, juce::Justification::centred, true);
            stepArea.removeFromLeft (totalSpacing); // Advance column spacing
        }

        displayArea.removeFromTop (14.0f);

        // =====================================================================
        // RENDER: PLAYBACK METADATA & CYCLES (VU METERS OMITTED)
        // =====================================================================
        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (10.0f, juce::Font::plain));

        juce::String statusText = isPlaying ? "STATE: ACTIVE" : "STATE: STOPPED";
        if (isFreezeActive) statusText = "STATE: FROZEN";

        juce::String barCycleText = "BAR: " + juce::String (processor.currentBarInCycle);

        g.drawText (statusText, displayArea.withWidth (150.0f), juce::Justification::left, true);
        g.drawText (barCycleText, displayArea, juce::Justification::right, true);
    }
}

void OledDisplay::resized()
{
}