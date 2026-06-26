#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState(); sceneB = SceneState(); lastChordPitches = { 60, 64, 67 }; 
    for (int i = 0; i < 4; ++i) { sceneAPresets[i] = SceneState(); sceneBPresets[i] = SceneState(); }
}

PluginProcessor::~PluginProcessor() {}

const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }
bool PluginProcessor::acceptsMidi() const { return true; }
bool PluginProcessor::producesMidi() const { return true; }
bool PluginProcessor::isMidiEffect() const { return false; } 
double PluginProcessor::getTailLengthSeconds() const { return 0.0; }
int PluginProcessor::getNumPrograms() { return 1; }
int PluginProcessor::getCurrentProgram() { return 0; }
void PluginProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String PluginProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void PluginProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate; mLastStep = -1; mLastNotePlayed = -1; mNoteOffTime = 0; mTimeInSamples = 0;
    activeHeldNotes.clear(); latchedNotes.clear(); scheduledNoteOffs.clear();
    std::fill (std::begin (lfoPhases), std::end (lfoPhases), 0.0);
    isFirstNoteOfNewChord = true; juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::releaseResources() {}
bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

std::vector<int> PluginProcessor::generateEuclideanPattern (int steps, int pulses)
{
    std::vector<int> pattern(steps, 0);
    if (pulses <= 0) return pattern;
    if (pulses >= steps) { std::fill(pattern.begin(), pattern.end(), 1); return pattern; }
    int bucket = 0;
    for (int i = 0; i < steps; ++i) { bucket += pulses; if (bucket >= steps) { bucket -= steps; pattern[i] = 1; } }
    return pattern;
}

void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
    double sampleDelta = numSamples;

    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (IDs::cycleLength.getParamID())));
    int cycleBars = (cycleIndex == 0) ? 1 : (cycleIndex == 1) ? 2 : (cycleIndex == 2) ? 4 : 8;
    currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % cycleBars) + 1;

    // Bipolar Entropy Accumulator
    float baseEntropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    int stepInterval = (std::abs(baseEntropy) <= 0.33f) ? 1 : (std::abs(baseEntropy) <= 0.66f) ? 2 : 4;
    if (currentStep == 0 && mLastStep == 7) {
        if (baseEntropy > 0.05f) accumulatedPitchOffset += stepInterval;
        else if (baseEntropy < -0.05f) accumulatedPitchOffset -= stepInterval;
        if (currentBarInCycle == 1) accumulatedPitchOffset = 0.0f;
    }

    // Capture user manual edits and write them directly to the focused background target [NEW]
    int focusedA = activeSceneAIndex.load(), focusedB = activeSceneBIndex.load(), focusSide = editFocusSide.load();
    auto updateFocusValue = [&](juce::ParameterID baseId, int lfoIndex, juce::ParameterID rateId, juce::ParameterID depthId, int index) {
        float val = *apvts.getRawParameterValue (baseId.getParamID());
        int rateVal = static_cast<int> (*apvts.getRawParameterValue (rateId.getParamID()));
        float depthVal = *apvts.getRawParameterValue (depthId.getParamID());
        SceneState& s = (focusSide == 0) ? sceneAPresets[focusedA] : sceneBPresets[focusedB];
        if (index < 8) s.faders[index] = val;
        else if (index == 8) s.rhythmMorph = val; else if (index == 9) s.rest = val; else if (index == 10) s.legato = val;
        else if (index == 11) s.rate = val; else if (index == 12) s.entropy = val; else if (index == 13) s.harmony = val;
        else if (index == 14) s.chaos = val; else if (index == 15) s.octaves = val;
        s.lfoRates[lfoIndex] = rateVal; s.lfoDepths[lfoIndex] = depthVal;
    };

    updateFocusValue (IDs::rhythmMorph, 0, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, 8);
    updateFocusValue (IDs::rest,        1, IDs::restLfoRate,        IDs::restLfoDepth,        9);
    updateFocusValue (IDs::legato,      2, IDs::legatoLfoRate,      IDs::legatoLfoDepth,      10);
    updateFocusValue (IDs::rate,        3, IDs::rateLfoRate,        IDs::rateLfoDepth,        11);
    updateFocusValue (IDs::entropy,     4, IDs::entropyLfoRate,     IDs::entropyLfoDepth,     12);
    updateFocusValue (IDs::harmony,     5, IDs::harmonyLfoRate,     IDs::harmonyLfoDepth,     13);
    updateFocusValue (IDs::chaos,       6, IDs::chaosLfoRate,       IDs::chaosLfoDepth,       14);
    updateFocusValue (IDs::octaves,     7, IDs::octavesLfoRate,     IDs::octavesLfoDepth,     15);
    for (int i = 0; i < 8; ++i) updateFocusValue (juce::ParameterID ("fader" + juce::String (i + 1), 1), i, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, i);

    // 8-Channel LFO Modulation Matrix Lambda
    auto applyLfo = [&](int index, juce::ParameterID baseId, juce::ParameterID rateId, juce::ParameterID depthId, float minVal, float maxVal) -> float {
        float baseVal = *apvts.getRawParameterValue (baseId.getParamID());
        int rateChoice = static_cast<int> (*apvts.getRawParameterValue (rateId.getParamID()));
        float depth = *apvts.getRawParameterValue (depthId.getParamID());
        if (rateChoice == 0) return baseVal;
        double divPPQ = (rateChoice == 1) ? 1.0 : (rateChoice == 2) ? 0.5 : (rateChoice == 3) ? 0.25 : 0.125;
        double periodSamples = samplesPerBeat * divPPQ;
        lfoPhases[index] += (sampleDelta / periodSamples);
        if (lfoPhases[index] >= 1.0) lfoPhases[index] -= 1.0;
        float sineVal = static_cast<float> (std::sin (lfoPhases[index] * juce::MathConstants<double>::twoPi));
        return juce::jlimit (minVal, maxVal, baseVal + (sineVal * depth * ((maxVal - minVal) * 0.5f)));
    };

    activeMorph   = applyLfo (0, IDs::rhythmMorph, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, 0.0f, 1.0f);
    activeRest    = applyLfo (1, IDs::rest,        IDs::restLfoRate,        IDs::restLfoDepth,        0.0f, 1.0f);
    activeLegato  = applyLfo (2, IDs::legato,      IDs::legatoLfoRate,      IDs::legatoLfoDepth,      0.1f, 1.0f);
    modLegato = activeLegato; modRest = activeRest;

    float rawRate = applyLfo (3, IDs::rate, IDs::rateLfoRate, IDs::rateLfoDepth, 0.0f, 3.0f);
    activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate)));
    activeEntropy = applyLfo (4, IDs::entropy,     IDs::entropyLfoRate,     IDs::entropyLfoDepth,     -1.0f, 1.0f); modEntropy = activeEntropy;
    activeHarmony = applyLfo (5, IDs::harmony,     IDs::harmonyLfoRate,     IDs::harmonyLfoDepth,     0.0f, 1.0f); modHarmony = activeHarmony;
    activeChaos   = applyLfo (6, IDs::chaos,       IDs::chaosLfoRate,       IDs::chaosLfoDepth,       0.0f, 1.0f); modChaos = activeChaos;
    float rawOctaves = applyLfo (7, IDs::octaves, IDs::octavesLfoRate, IDs::octavesLfoDepth, 1.0f, 4.0f);
    activeOctavesVal = juce::jlimit (1, 4, static_cast<int> (std::round (rawOctaves)));
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    if (delaySamples <= 0) midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);
    else scheduledNoteOffs.push_back ({ pitch, delaySamples });
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    bool isPlaying = false; double bpm = 120.0; mSongPositionPPQ = 0.0;
#if JUCE_MAJOR_VERSION >= 7
    if (auto* playhead = getPlayHead()) {
        if (auto pos = playhead->getPosition()) {
            isPlaying = pos->getIsPlaying();
            auto bpmOpt = pos->getBpm(); if (bpmOpt.hasValue()) bpm = *bpmOpt;
            auto ppqOpt = pos->getPpqPosition(); if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt;
        }
    }
#else
    if (auto* playhead = getPlayHead()) {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (playhead->getCurrentPositionInfo (info)) { isPlaying = info.isPlaying; bpm = info.bpm; mSongPositionPPQ = info.ppqPosition; }
    }
#endif
    int numSamples = buffer.getNumSamples(); updateLfoModulations (numSamples, bpm);
    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;
    float activeFaderProb[8]; for (int i = 0; i < 8; ++i) activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();) {
        it->second -= numSamples;
        if (it->second <= 0) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), 0); it = scheduledNoteOffs.erase(it); }
        else ++it;
    }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            int note = msg.getNoteNumber();
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end()) { activeHeldNotes.push_back (note); std::sort (activeHeldNotes.begin(), activeHeldNotes.end()); }
            if (isLatchActive) {
                if (isFirstNoteOfNewChord) { for (int n : latchedNotes) scheduleNoteOff (processedMidi, n, 0); latchedNotes.clear(); isFirstNoteOfNewChord = false; }
                if (std::find (latchedNotes.begin(), latchedNotes.end(), note) == latchedNotes.end()) { latchedNotes.push_back (note); std::sort (latchedNotes.begin(), latchedNotes.end()); }
            }
        } else if (msg.isNoteOff()) {
            int note = msg.getNoteNumber(); activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
            if (activeHeldNotes.empty()) isFirstNoteOfNewChord = true;
        }
    }
    midiMessages.clear();
    const auto& notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    isCurrentlyPlayingUI.store (!notesToPlay.empty());

    if (! notesToPlay.empty()) {
        bool stepTriggered = false; double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        double stepLengthPPQ = (activeRateIdx == 0) ? 1.0 : (activeRateIdx == 1) ? 0.5 : (activeRateIdx == 2) ? 0.25 : 0.125;
        double stepSamples = samplesPerBeat * stepLengthPPQ;

        if (isPlaying) {
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
            if (stepIndex != mLastStep) { mLastStep = stepIndex; currentStep = stepIndex; stepTriggered = true; }
        } else {
            mTimeInSamples += numSamples; if (mTimeInSamples >= stepSamples) { mTimeInSamples = 0; currentStep = (currentStep + 1) % 8; mLastStep = currentStep; stepTriggered = true; }
        }

        if (stepTriggered) {
            float faderProb = activeFaderProb[currentStep];
            if (juce::Random::getSystemRandom().nextFloat() <= faderProb && !(juce::Random::getSystemRandom().nextFloat() <= modRest)) {
                if (mLastNotePlayed != -1) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0); mLastNotePlayed = -1; }
                int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
                int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));
                std::vector<int> scaleOffsets = { 0, 2, 4, 5, 7, 9, 11, 12 }; 
                if (scaleIdx == 1) scaleOffsets = { 0, 2, 3, 5, 7, 8, 10, 12 }; 
                else if (scaleIdx == 2) scaleOffsets = { 0, 3, 5, 7, 10, 12, 15, 17 }; 
                else if (scaleIdx == 3) scaleOffsets = { 0, 2, 4, 7, 9, 12, 14, 16 };  
                else if (scaleIdx == 4) scaleOffsets = { 0, 2, 3, 5, 7, 9, 10, 12 };  
                else if (scaleIdx == 5) scaleOffsets = { 0, 1, 3, 5, 7, 8, 10, 12 };  
                else if (scaleIdx == 6) scaleOffsets = { 0, 2, 4, 6, 7, 9, 11, 12 };  
                else if (scaleIdx == 7) scaleOffsets = { 0, 2, 4, 5, 7, 9, 10, 12 };  
                else if (scaleIdx == 8) scaleOffsets = { 0, 2, 3, 5, 7, 8, 11, 12 };  
                else if (scaleIdx == 9) scaleOffsets = { 0, 2, 3, 5, 7, 9, 11, 12 };  

                int rawPitch = notesToPlay[currentStep % notesToPlay.size()];
                int octave = (rawPitch / 12) * 12;
                int targetPitch = octave + rootKeyIdx + scaleOffsets[currentStep] + static_cast<int>(accumulatedPitchOffset) + (((currentStep / 2) % activeOctavesVal) * 12);
                if (modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos) targetPitch += (juce::Random::getSystemRandom().nextBool() ? 12 : -12);
                targetPitch = juce::jlimit(0, 127, targetPitch);
                int durationSamples = static_cast<int>(stepSamples * modLegato);

                processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
                mLastNotePlayed = targetPitch; mNoteOffTime = durationSamples; scheduleNoteOff (processedMidi, targetPitch, durationSamples);
            }
        }
    } else { if (mLastStep != -1) { if (mLastNotePlayed != -1) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0); mLastNotePlayed = -1; } mLastStep = -1; } currentStep = 0; }
    midiMessages.swapWith (processedMidi);
}

void PluginProcessor::triggerDiatonicChordPad (int padIndex)
{
    int rootIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
    int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));
    std::vector<int> scaleOffsets = { 0, 2, 4, 5, 7, 9, 11 };
    if (scaleIdx == 1) scaleOffsets = { 0, 2, 3, 5, 7, 8, 10 };
    else if (scaleIdx == 2) scaleOffsets = { 0, 3, 5, 7, 10, 12, 14 };
    else if (scaleIdx == 3) scaleOffsets = { 0, 2, 4, 7, 9, 12, 14 };
    else if (scaleIdx == 4) scaleOffsets = { 0, 2, 3, 5, 7, 9, 10 };
    else if (scaleIdx == 5) scaleOffsets = { 0, 1, 3, 5, 7, 8, 10 };
    else if (scaleIdx == 6) scaleOffsets = { 0, 2, 4, 6, 7, 9, 11 };
    else if (scaleIdx == 7) scaleOffsets = { 0, 2, 4, 5, 7, 9, 10 };
    else if (scaleIdx == 8) scaleOffsets = { 0, 2, 3, 5, 7, 8, 11 };
    else if (scaleIdx == 9) scaleOffsets = { 0, 2, 3, 5, 7, 9, 11 };

    auto getScalePitch = [&](int degree) -> int { return scaleOffsets[degree % 7] + ((degree / 7) * 12); };
    int baseRoot = 48 + rootIdx;
    int r = getScalePitch (padIndex), t = getScalePitch (padIndex + 2), f = getScalePitch (padIndex + 4);

    std::vector<int> newChord;
    if (activeHarmony >= 0.34f && activeHarmony < 0.67f) { t = getScalePitch (padIndex + 3); newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }
    else if (activeHarmony >= 0.67f) { int s = getScalePitch (padIndex + 6); newChord = { baseRoot + r, baseRoot + t, baseRoot + f, baseRoot + s }; }
    else { newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }

    if (! lastChordPitches.empty() && newChord.size() == lastChordPitches.size()) {
        int pitchDiff = newChord[2] - lastChordPitches[2];
        if (pitchDiff > 5) newChord[2] -= 12; else if (pitchDiff < -5) newChord[0] += 12; 
    }
    std::sort(newChord.begin(), newChord.end()); lastChordPitches = newChord;
    latchedNotes = newChord; apvts.getParameter(IDs::latch.getParamID())->setValueNotifyingHost(1.0f); 
}

// Compact unified Scene save/load functions [NEW]
void PluginProcessor::savePreset (int slotIndex) { if (slotIndex >= 0 && slotIndex < 8) { for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); presetSlotsSaved[slotIndex] = true; } }
void PluginProcessor::loadPreset (int slotIndex) { if (slotIndex >= 0 && slotIndex < 8 && presetSlotsSaved[slotIndex]) { apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph); apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest); apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato); apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (presets[slotIndex].entropy); apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony); apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos); for (int i = 0; i < 8; ++i) apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]); } }
void PluginProcessor::captureSceneA() { for (int i = 0; i < 8; ++i) sceneA.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); sceneA.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); sceneA.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); sceneA.legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); sceneA.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); sceneA.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); sceneA.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); hasSceneA = true; }
void PluginProcessor::captureSceneB() { for (int i = 0; i < 8; ++i) sceneB.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); sceneB.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); sceneB.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); sceneB.legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); sceneB.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); sceneB.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); sceneB.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); hasSceneB = true; }
void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }
void PluginProcessor::resetRhythm() { apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); }

// Compressed unified 4x4 scene load and save implementations [5] [NEW]
void PluginProcessor::saveScene (int slotIndex, int side) {
    SceneState& s = (side == 0) ? sceneAPresets[slotIndex] : sceneBPresets[slotIndex];
    for (int i = 0; i < 8; ++i) s.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    s.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); s.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); s.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    s.rate = *apvts.getRawParameterValue (IDs::rate.getParamID()); s.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); s.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    s.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); s.octaves = *apvts.getRawParameterValue (IDs::octaves.getParamID());
    juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
    juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
    for (int i = 0; i < 8; ++i) { s.lfoRates[i] = static_cast<int> (*apvts.getRawParameterValue (rates[i].getParamID())); s.lfoDepths[i] = *apvts.getRawParameterValue (depths[i].getParamID()); }
    if (side == 0) sceneASlotsSaved[slotIndex] = true; else sceneBSlotsSaved[slotIndex] = true;
}

void PluginProcessor::loadScene (int slotIndex, int side) {
    if ((side == 0 && !sceneASlotsSaved[slotIndex]) || (side == 1 && !sceneBSlotsSaved[slotIndex])) return;
    SceneState& s = (side == 0) ? sceneAPresets[slotIndex] : sceneBPresets[slotIndex];
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (s.rhythmMorph); apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (s.rest); apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (s.legato);
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (s.rate); apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (s.entropy); apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (s.harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (s.chaos); apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost (s.octaves);
    juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
    juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
    for (int i = 0; i < 8; ++i) { apvts.getParameter (rates[i].getParamID())->setValueNotifyingHost (static_cast<float>(s.lfoRates[i]) / 4.0f); apvts.getParameter (depths[i].getParamID())->setValueNotifyingHost (s.lfoDepths[i]); }
    for (int i = 0; i < 8; ++i) apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (s.faders[i]);
}