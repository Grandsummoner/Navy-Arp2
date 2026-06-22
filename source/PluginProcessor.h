#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <array>
#include <cmath>

namespace IDs 
{
    #define DECLARE_ID(name) const juce::ParameterID name (#name, 1)
    DECLARE_ID(fader1); DECLARE_ID(fader2); DECLARE_ID(fader3); DECLARE_ID(fader4);
    DECLARE_ID(fader5); DECLARE_ID(fader6); DECLARE_ID(fader7); DECLARE_ID(fader8);
    DECLARE_ID(rhythmMorph); DECLARE_ID(rest); DECLARE_ID(legato);
    DECLARE_ID(entropy); DECLARE_ID(harmony); DECLARE_ID(chaos);
    DECLARE_ID(morph); DECLARE_ID(latch); DECLARE_ID(chordMode);
    DECLARE_ID(rootKey); DECLARE_ID(scaleType); DECLARE_ID(cycleLength);
    #undef DECLARE_ID
}

struct SceneState {
    float faders[8] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    float rhythmMorph = 0.0f;
    float rest = 0.1f;
    float legato = 0.5f;
    float entropy = 0.0f;
    float harmony = 0.0f;
    float chaos = 0.0f;
};

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Presets & Scenes
    void savePreset (int slotIndex);
    void loadPreset (int slotIndex);
    bool isPresetSaved (int slotIndex) const { return presetSlotsSaved[slotIndex]; }

    void captureSceneA();
    void captureSceneB();
    void clearSceneA() { hasSceneA = false; }
    void clearSceneB() { hasSceneB = false; }

    // Generative Triggers
    void diceMelody();
    void diceRhythm();
    void resetAccumulator();
    void resetRhythm();
    void triggerDiatonicChordPad (int padIndex);

    SceneState sceneA;
    SceneState sceneB;
    bool hasSceneA = false;
    bool hasSceneB = false;

    // Real-time tracking for UI
    int currentStep = 0;
    int currentBarInCycle = 1;
    juce::String activeChordExtensionText = "TRIAD";
    std::vector<int> activeHeldNotes;
    std::vector<int> latchedNotes;
    bool isFirstNoteOfNewChord = true;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateLfoModulations (int numSamples, double bpm);
    std::vector<int> generateEuclideanPattern (int steps, int pulses);
    void scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples);

    double mSampleRate = 44100.0;
    int mLastStep = -1;
    int mTimeInSamples = 0;
    double mSongPositionPPQ = 0.0;
    
    // Decrementing note-off event queue (Pitch, RemainingSamples)
    std::vector<std::pair<int, int>> scheduledNoteOffs;

    // LFO internal phases
    double lfoPhaseEntropy = 0.0;
    double lfoPhaseChaos = 0.0;
    double lfoPhaseMorph = 0.0;
    double lfoPhaseLegato = 0.0;

    // Modulated effective parameter values
    float modRest = 0.1f;
    float modLegato = 0.5f;
    float modEntropy = 0.0f;
    float modHarmony = 0.0f;
    float modChaos = 0.0f;
    float accumulatedPitchOffset = 0.0f;

    // Smart Diatonic Chord voice leading cache
    std::vector<int> lastChordPitches;

    SceneState presets[8];
    bool presetSlotsSaved[8] = { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};