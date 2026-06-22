#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdint>
#include "PluginProcessor.h"

// ==============================================================================
// Custom component for High-Contrast Brushed Metal / OLED Screen Look
// ==============================================================================
class OledDisplay : public juce::Component, public juce::Timer
{
public:
    OledDisplay (PluginProcessor& p) : processor (p) { startTimerHz (30); }
    ~OledDisplay() override { stopTimer(); }

    void timerCallback() override { repaint(); }

    void paint (juce::Graphics& g) override
    {
        // Absolute Pitch-Black OLED
        g.fillAll (juce::Colour (0xFF000000));
        g.setColour (juce::Colour (0xFF223344));
        g.drawRect (getLocalBounds().toFloat(), 2.0f);

        // Header status readouts
        g.setColour (juce::Colour (0xFFFFFFFF)); // Industrial paper-white
        g.setFont (juce::FontOptions ("Consolas", 13.0f, juce::Font::bold));
        
        juce::String modeText = processor.activeChordExtensionText;
        juce::String status = "NAVY-ARP v2.0 | EXT: [" + modeText + "] | BAR: [" + juce::String(processor.currentBarInCycle) + "]";
        g.drawFittedText (status, getLocalBounds().reduced(10, 5).removeFromTop(20), juce::Justification::centred, 1);

        // Step VU-Meters
        auto area = getLocalBounds().reduced (15);
        area.removeFromTop (25);
        int barWidth = area.getWidth() / 8;
        int spacing = 5;

        for (int i = 0; i < 8; ++i)
        {
            float faderProb = *processor.apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
            int barHeight = static_cast<int>(area.getHeight() * faderProb * 0.75f);
            
            juce::Rectangle<int> bar (area.getX() + (i * barWidth) + spacing, 
                                      area.getBottom() - barHeight - 15, 
                                      barWidth - (spacing * 2), 
                                      barHeight);

            // Active step color-coding
            bool isLatch = *processor.apvts.getRawParameterValue(IDs::latch.getParamID()) > 0.5f;
            bool isPlaying = isLatch ? !processor.latchedNotes.empty() : !processor.activeHeldNotes.empty();

            if (i == processor.currentStep && isPlaying)
            {
                if (i == 0)      g.setColour (juce::Colour (0xFF33FF33)); // Beat 1: Emerald Green
                else if (i == 4) g.setColour (juce::Colour (0xFFFF3333)); // Beat 2: Ruby Red
                else             g.setColour (juce::Colour (0xFF00D2FF)); // Electric Cyan
                g.fillRect (bar.expanded(2, 2));
            }
            else
            {
                g.setColour (juce::Colour (0xFF334455)); // Idle dark LED state
                g.fillRect (bar);
            }
        }
    }

private:
    PluginProcessor& processor;
};

// ==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Right-click Save Preset interception
    void mouseDown (const juce::MouseEvent& event) override
    {
        for (int i = 0; i < 8; ++i)
        {
            if (event.eventComponent == &presetButtons[i])
            {
                if (event.mods.isRightButtonDown())
                {
                    processor.savePreset(i);
                    presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF004455)); // Outer glow
                }
            }
        }
    }

private:
    PluginProcessor& processor;
    OledDisplay oledDisplay;

    // Fluted metal encoder knobs
    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;

    juce::Slider rhythmMorphKnob, restKnob, legatoKnob;
    juce::Label rhythmMorphTitle, restTitle, legatoTitle;

    juce::Slider entropyKnob, harmonyKnob, chaosKnob;
    juce::Label entropyTitle, harmonyTitle, chaosTitle;

    juce::Slider morphCrossfader;

    // Tactile Mechanical Keys
    juce::TextButton latchButton;
    juce::TextButton chordModeButton;
    juce::TextButton diceMelodyButton;
    juce::TextButton diceRhythmButton;
    juce::TextButton sceneAButton;
    juce::TextButton sceneBButton;
    juce::TextButton presetButtons[8];

    // Selectors
    juce::ComboBox rootKeyBox;
    juce::ComboBox scaleTypeBox;
    juce::ComboBox cycleLengthBox;

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
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chordModeAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cycleLengthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};