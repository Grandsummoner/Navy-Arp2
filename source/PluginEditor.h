#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// ==============================================================================
// Custom component representing our premium High-Quality OLED screen display
// ==============================================================================
class OledDisplay : public juce::Component
{
public:
    OledDisplay() {}
    ~OledDisplay() override {}

    void paint (juce::Graphics& g) override
    {
        // 1. OLED Pitch Black Background
        g.fillAll (juce::Colour (0xFF000000));

        // 2. OLED Outer Border (Thin, sharp vector line with neon-glow feel)
        g.setColour (juce::Colour (0xFF112233));
        g.drawRect (getLocalBounds().toFloat(), 1.5f);

        // 3. Render Placeholder OLED Vector Graphics
        // (This simulates our Sharp Retro-Glow visualizer and active range piano keys)
        g.setColour (juce::Colour (0xFF00D2FF)); // Neon Aqua Blue
        g.setFont (juce::Font ("Consolas", 14.0f, juce::Font::bold));
        g.drawFittedText ("--- NAVY-ARP OLED ACTIVE ---", getLocalBounds().removeFromTop (30), juce::Justification::centred, 1);

        // Draw simulated "Sharp Retro-Glow" VU-meters
        auto area = getLocalBounds().reduced (15);
        area.removeFromTop (30);
        int barWidth = area.getWidth() / 16;
        int spacing = 2;

        g.setColour (juce::Colour (0xFFFFB300)); // Warm Amber Glow
        for (int i = 0; i < 16; ++i)
        {
            // Simulate bouncing vertical steps
            float heightPercent = 0.2f + 0.6f * std::sin (i * 0.5f);
            int barHeight = static_cast<int>(area.getHeight() * heightPercent);
            
            juce::Rectangle<int> bar (area.getX() + (i * barWidth) + spacing, 
                                      area.getBottom() - barHeight - 15, 
                                      barWidth - (spacing * 2), 
                                      barHeight);

            // Alternate colors for musical intervals (roots/fifths/passing)
            if (i % 3 == 0)
                g.setColour (juce::Colour (0xFF00D2FF)); // Aqua
            else if (i % 4 == 0)
                g.setColour (juce::Colour (0xFFB080FF)); // Muted Lavender
            else
                g.setColour (juce::Colour (0xFFFFB300)); // Amber

            g.fillRect (bar);
        }

        // Draw simple simulated piano keys at the bottom of the screen
        g.setColour (juce::Colour (0xFF333333));
        g.drawHorizontalLine (area.getBottom() - 10, area.getX(), area.getRight());
    }
};

// ==============================================================================
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    // ==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Reference to the main audio processor
    PluginProcessor& processor;

    // 1. Central OLED Display Component
    OledDisplay oledDisplay;

    // 2. The 8 Scale-Degree Faders (Bottom shelf)
    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;

    // 3. The Left Sidebar Knobs (Rhythm)
    juce::Slider rhythmMorphKnob, restKnob, legatoKnob;
    juce::Label rhythmMorphLabel, restLabel, legatoLabel;

    // 4. The Right Sidebar Knobs (Harmony & Chaos)
    juce::Slider entropyKnob, harmonyKnob, chaosKnob;
    juce::Label entropyLabel, harmonyLabel, chaosLabel;

    // 5. The Octatrack-style Scene Morph Crossfader
    juce::Slider morphCrossfader;

    // 6. Tactile Toggle & Momentary Buttons
    juce::TextButton latchButton;
    juce::TextButton diceMelodyButton;
    juce::TextButton diceRhythmButton;
    juce::TextButton sceneAButton;
    juce::TextButton sceneBButton;

    // 7. The 8 OLED-Style Preset Buttons
    juce::TextButton presetButtons[8];

    // ==============================================================================
    // APVTS Attachments to link our UI sliders/buttons directly to DAW parameters
    // ==============================================================================
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader3Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader4Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader5Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader6Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader7Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader8Attachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rhythmMorphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> restAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> legatoAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};