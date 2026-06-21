#pragma once

#include <JuceHeader.h>

// ==============================================================================
// Parameter IDs representing all physical controls on the UI
// ==============================================================================
namespace IDs 
{
    #define DECLARE_ID(name) const juce::ParameterID name (#name, 1)
    
    // The 8 Scale-Degree Faders (Bottom row)
    DECLARE_ID(fader1);
    DECLARE_ID(fader2);
    DECLARE_ID(fader3);
    DECLARE_ID(fader4);
    DECLARE_ID(fader5);
    DECLARE_ID(fader6);
    DECLARE_ID(fader7);
    DECLARE_ID(fader8);

    // The Left Sidebar Knobs (Rhythm)
    DECLARE_ID(rhythmMorph);
    DECLARE_ID(rest);
    DECLARE_ID(legato);

    // The Right Sidebar Knobs (Harmony/Chaos)
    DECLARE_ID(entropy);
    DECLARE_ID(harmony);
    DECLARE_ID(chaos);

    // The Octatrack Crossfader
    DECLARE_ID(morph);

    // Toggle Buttons
    DECLARE_ID(latch);
    
    #undef DECLARE_ID
}

// ==============================================================================
class PluginProcessor : public juce::AudioProcessor
{
public:
    // ==============================================================================
    PluginProcessor();
    ~PluginProcessor() override;

    // ==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // ==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // ==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // ==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    // ==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ==============================================================================
    // The APVTS manages your parameter states, DAW automation, and saving/recalling.
    // It is public so that your PluginEditor can easily bind sliders to it.
    // ==============================================================================
    juce::AudioProcessorValueTreeState apvts;

private:
    // Helper function to create and define your parameter list
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};