#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState(); sceneB = SceneState(); lastChordPitches = { 60, 64, 67 }; 
    for (int i = 0; i < 8; ++i) { sceneAPresets[i] = SceneState(); sceneBPresets[i] = SceneState(); }
}

PluginProcessor::~PluginProcessor() {}
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor (*this); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate; mLastStep = -1; mLastNotePlayed = -1; mNoteOffTime = 0; mTimeInSamples = 0;
    activeHeldNotes.clear(); latchedNotes.clear(); scheduledNoteOffs.clear();
    std::fill (std::begin (lfoPhases), std::end (lfoPhases), 0.0);
    isFirstNoteOfNewChord = true; lastSceneBActiveState = isSceneBActiveAnchor.load();
    std::fill (std::begin (currentSlewValue), std::end (currentSlewValue), 0.5f);
    std::fill (std::begin (currentSlewTarget), std::end (currentSlewTarget), 0.5f);
    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    if (delaySamples <= 0) midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);
    else scheduledNoteOffs.push_back ({ pitch, delaySamples });
}

void PluginProcessor::setActiveAnchor (bool useSceneB)
{
    if (isSceneBActiveAnchor.load() == useSceneB) return;
    captureScene (isSceneBActiveAnchor.load() ? 1 : 0);
    isSceneBActiveAnchor.store (useSceneB); lastSceneBActiveState = useSceneB;
    
    SceneState& targetScene = useSceneB ? sceneB : sceneA;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (targetScene.rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (targetScene.rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (targetScene.legato);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((targetScene.entropy + 1.0f) * 0.5f);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (targetScene.harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (targetScene.chaos);
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (targetScene.rate / 3.0f);
    apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((targetScene.octaves + 3.0f) / 6.0f);
    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetScene.faders[i]);
}

void PluginProcessor::captureActiveParametersToActiveScene()
{
    SceneState& activeScene = isSceneBActiveAnchor.load() ? sceneB : sceneA;
    for (int i = 0; i < 8; ++i)
        activeScene.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    activeScene.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    activeScene.rest        = *apvts.getRawParameterValue (IDs::rest.getParamID());
    activeScene.legato      = *apvts.getRawParameterValue (IDs::legato.getParamID());
    activeScene.entropy     = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    activeScene.harmony     = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    activeScene.chaos       = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    activeScene.rate        = *apvts.getRawParameterValue (IDs::rate.getParamID());
    activeScene.octaves     = static_cast<float> (*apvts.getRawParameterValue (IDs::octaves.getParamID()));
}

void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    captureActiveParametersToActiveScene();
    bool isSceneBActive = isSceneBActiveAnchor.load();
    if (isSceneBActive != lastSceneBActiveState)
    {
        lastSceneBActiveState = isSceneBActive;
        SceneState& targetScene = isSceneBActive ? sceneB : sceneA;
        apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (targetScene.rhythmMorph);
        apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (targetScene.rest);
        apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (targetScene.legato);
        apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((targetScene.entropy + 1.0f) * 0.5f);
        apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (targetScene.harmony);
        apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (targetScene.chaos);
        apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (targetScene.rate / 3.0f);
        apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((targetScene.octaves + 3.0f) / 6.0f);
        for (int i = 0; i < 8; ++i)
            apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetScene.faders[i]);
    }

    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0)), sampleDelta = numSamples;
    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (IDs::cycleLength.getParamID())));
    currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % ((cycleIndex == 0) ? 1 : (cycleIndex == 1) ? 2 : (cycleIndex == 2) ? 4 : 8)) + 1;

    float morphVal = *apvts.getRawParameterValue (IDs::morph.getParamID());
    auto morphValue = [&](float valA, float valB) -> float { return (valA * (1.0f - morphVal)) + (valB * morphVal); };

    float baseMorph = morphValue (sceneA.rhythmMorph, sceneB.rhythmMorph), baseRest = morphValue (sceneA.rest, sceneB.rest), baseLegato = morphValue (sceneA.legato, sceneB.legato), baseRate = morphValue (sceneA.rate, sceneB.rate);
    float baseEntropy = morphValue (sceneA.entropy, sceneB.entropy), baseHarmony = morphValue (sceneA.harmony, sceneB.harmony), baseChaos = morphValue (sceneA.chaos, sceneB.chaos), baseOctaves = morphValue (sceneA.octaves, sceneB.octaves);

    auto applyLfo = [&](int index, float baseVal, juce::ParameterID rateId, juce::ParameterID depthId, float minVal, float maxVal) -> float {
        int rateChoice = static_cast<int> (*apvts.getRawParameterValue (rateId.getParamID())); float depth = *apvts.getRawParameterValue (depthId.getParamID());
        if (rateChoice == 0) return baseVal;
        double divPPQ = (rateChoice == 1) ? 1.0 : (rateChoice == 2) ? 0.5 : (rateChoice == 3) ? 0.25 : 0.125, periodSamples = samplesPerBeat * divPPQ;
        lfoPhases[index] += (sampleDelta / periodSamples); if (lfoPhases[index] >= 1.0) lfoPhases[index] -= 1.0;
        return juce::jlimit (minVal, maxVal, baseVal + (static_cast<float> (std::sin (lfoPhases[index] * juce::MathConstants<double>::twoPi)) * depth * ((maxVal - minVal) * 0.5f)));
    };

    activeMorph = applyLfo (0, baseMorph, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, 0.0f, 1.0f);
    activeRest = applyLfo (1, baseRest, IDs::restLfoRate, IDs::restLfoDepth, 0.0f, 1.0f);
    activeLegato = applyLfo (2, baseLegato, IDs::legatoLfoRate, IDs::legatoLfoDepth, 0.1f, 1.0f);
    modLegato = activeLegato; modRest = (activeLegato >= 0.8f) ? (activeRest * juce::jlimit (0.0f, 1.0f, (1.0f - activeLegato) / 0.2f)) : activeRest;

    float rawRate = applyLfo (3, baseRate, IDs::rateLfoRate, IDs::rateLfoDepth, 0.0f, 3.0f);
    activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate)));
    activeEntropy = applyLfo (4, baseEntropy, IDs::entropyLfoRate, IDs::entropyLfoDepth, -1.0f, 1.0f); modEntropy = activeEntropy;
    activeHarmony = applyLfo (5, baseHarmony, IDs::harmonyLfoRate, IDs::harmonyLfoDepth, 0.0f, 1.0f); modHarmony = activeHarmony;
    activeChaos = applyLfo (6, baseChaos, IDs::chaosLfoRate, IDs::chaosLfoDepth, 0.0f, 1.0f); modChaos = activeChaos;

    float rawOctaves = applyLfo (7, baseOctaves, IDs::octavesLfoRate, IDs::octavesLfoDepth, -3.0f, 3.0f);
    activeOctavesVal = juce::jlimit (-3, 3, static_cast<int> (std::round (rawOctaves)));
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear(); bool isPlaying = false; double bpm = 120.0; mSongPositionPPQ = 0.0;
#if JUCE_MAJOR_VERSION >= 7
    if (auto* playhead = getPlayHead()) { if (auto pos = playhead->getPosition()) { isPlaying = pos->getIsPlaying(); auto bpmOpt = pos->getBpm(); if (bpmOpt.hasValue()) bpm = *bpmOpt; auto ppqOpt = pos->getPpqPosition(); if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt; } }
#else
    if (auto* playhead = getPlayHead()) { juce::AudioPlayHead::CurrentPositionInfo info; if (playhead->getCurrentPositionInfo (info)) { isPlaying = info.isPlaying; bpm = info.bpm; mSongPositionPPQ = info.ppqPosition; } }
#endif
    int numSamples = buffer.getNumSamples(); updateLfoModulations (numSamples, bpm);
    float slewFactor = static_cast<float>(1.0 - std::exp (-1.0 / (0.15 * mSampleRate)));
    for (int i = 0; i < 24; ++i) currentSlewValue[i] += (currentSlewTarget[i] - currentSlewValue[i]) * slewFactor;

    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f, isArpActive = *apvts.getRawParameterValue (IDs::arpSeq.getParamID()) > 0.5f;
    bool isPolyActive = *apvts.getRawParameterValue (IDs::poly.getParamID()) > 0.5f, isFreezeActive = *apvts.getRawParameterValue (IDs::freeze.getParamID()) > 0.5f;

    if (!isLatchActive) { latchedNotes.clear(); isFirstNoteOfNewChord = true; }
    float activeFaderProb[8]; for (int i = 0; i < 8; ++i) activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();) {
        it->second -= numSamples;
        if (it->second <= 0) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), juce::jlimit (0, numSamples - 1, it->second + numSamples)); it = scheduledNoteOffs.erase(it); }
        else ++it;
    }

    if (isLatchActive && latchedNotes.empty() && !activeHeldNotes.empty()) { latchedNotes = activeHeldNotes; isFirstNoteOfNewChord = false; }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn() && !isFreezeActive) { 
            int note = msg.getNoteNumber();
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end()) { activeHeldNotes.push_back (note); std::sort (activeHeldNotes.begin(), activeHeldNotes.end()); }
            if (isLatchActive) {
                if (isFirstNoteOfNewChord) { for (int n : latchedNotes) scheduleNoteOff (processedMidi, n, 0); latchedNotes.clear(); isFirstNoteOfNewChord = false; }
                if (std::find (latchedNotes.begin(), latchedNotes.end(), note) == latchedNotes.end()) { latchedNotes.push_back (note); std::sort (latchedNotes.begin(), latchedNotes.end()); }
            }
        } else if (msg.isNoteOff() && !isFreezeActive) {
            int note = msg.getNoteNumber(); activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
            if (activeHeldNotes.empty()) isFirstNoteOfNewChord = true;
        }
    }
    midiMessages.clear();
    
    std::vector<int> notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    isCurrentlyPlayingUI.store (!notesToPlay.empty() || isFreezeActive);

    if (!notesToPlay.empty() || isFreezeActive) {
        bool stepTriggered = false; double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        double stepLengthPPQ = (activeRateIdx == 0) ? 1.0 : (activeRateIdx == 1) ? 0.5 : (activeRateIdx == 2) ? 0.25 : 0.125, stepSamples = samplesPerBeat * stepLengthPPQ;
        double swingOffset = 0.08 * activeMorph * stepLengthPPQ, triggerThreshold = stepLengthPPQ;
        if (currentStep % 2 == 1) triggerThreshold += swingOffset;

        if (isPlaying) {
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
            if (stepIndex != mLastStep) { mLastStep = stepIndex; currentStep = stepIndex; stepTriggered = true; }
        } else {
            double activeStepSamples = stepSamples;
            if (currentStep % 2 == 0) activeStepSamples *= (1.0 + (swingOffset * 4.0)); else activeStepSamples *= (1.0 - (swingOffset * 4.0));
            mTimeInSamples += numSamples; if (mTimeInSamples >= activeStepSamples) { mTimeInSamples = 0; stepTriggered = true; }
        }

        if (stepTriggered) {
            int playDirection = 0; 
            if (activeEntropy >= -0.1f && activeEntropy <= 0.1f) playDirection = 0;
            else if (activeEntropy > 0.1f && activeEntropy <= 0.5f) playDirection = 1;
            else if (activeEntropy > 0.5f) playDirection = 2;
            else if (activeEntropy < -0.1f && activeEntropy >= -0.5f) playDirection = 3;
            else if (activeEntropy < -0.5f) playDirection = 4;

            static bool goingForward = true;
            if (playDirection == 1) { 
                if (goingForward) { currentStep++; if (currentStep >= 7) { currentStep = 7; goingForward = false; } }
                else { currentStep--; if (currentStep <= 0) { currentStep = 0; goingForward = true; } }
            } else if (playDirection == 2) { 
                float rVal = juce::Random::getSystemRandom().nextFloat();
                if (rVal < 0.7f) currentStep = (currentStep + 1) % 8; else if (rVal < 0.9f) currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 3) { 
                currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 4) { 
                currentStep = (juce::Random::getSystemRandom().nextFloat() < 0.2f) ? (currentStep + 2) % 8 : (currentStep + 1) % 8;
            } else { 
                currentStep = isPlaying ? (static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8) : ((currentStep + 1) % 8);
            }
            mLastStep = currentStep;

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

                int rawPitch = 60, octaveBase = 60;
                if (isArpActive && !notesToPlay.empty()) { rawPitch = notesToPlay[currentStep % notesToPlay.size()]; octaveBase = ((rawPitch - rootKeyIdx) / 12) * 12 + rootKeyIdx; }
                else { rawPitch = 48 + rootKeyIdx + scaleOffsets[currentStep % scaleOffsets.size()]; octaveBase = ((rawPitch - rootKeyIdx) / 12) * 12 + rootKeyIdx; }

                std::vector<int> pitchList; pitchList.push_back (rawPitch);
                if (isPolyActive) {
                    int maxAllowedNotes = (modHarmony > 0.25f && modHarmony < 0.5f) ? 2 : (modHarmony >= 0.5f && modHarmony < 0.75f) ? 3 : (modHarmony >= 0.75f) ? 4 : 1;
                    if (maxAllowedNotes > 1) { for (int n = 1; n < maxAllowedNotes; ++n) pitchList.push_back (octaveBase + scaleOffsets[(currentStep + n * 2) % scaleOffsets.size()]); }
                }

                for (auto& pitch : pitchList) {
                    int noteOffset = (pitch - rootKeyIdx) % 12, octBase = ((pitch - rootKeyIdx) / 12) * 12 + rootKeyIdx, nearestVal = scaleOffsets[0], minDiff = 12;
                    for (int offset : scaleOffsets) { int diff = std::abs (offset - noteOffset); if (diff < minDiff) { minDiff = diff; nearestVal = offset; } }
                    pitch = octBase + nearestVal;
                }

                int rangeShift = activeOctavesVal, octaveShiftCount = (rangeShift > 0) ? ((currentStep / 2) % (rangeShift + 1)) : ((rangeShift < 0) ? -((currentStep / 2) % (std::abs(rangeShift) + 1)) : 0);
                for (auto pitch : pitchList) {
                    int targetPitch = juce::jlimit(0, 127, pitch + (octaveShiftCount * 12) + ((modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos) ? (juce::Random::getSystemRandom().nextBool() ? 12 : -12) : 0));
                    processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
                    mLastNotePlayed = targetPitch; mNoteOffTime = static_cast<int>(stepSamples * modLegato); scheduleNoteOff (processedMidi, targetPitch, mNoteOffTime);
                }
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
    int baseRoot = 48 + rootIdx, r = getScalePitch (padIndex), t = getScalePitch (padIndex + 2), f = getScalePitch (padIndex + 4);

    std::vector<int> newChord;
    if (activeHarmony >= 0.34f && activeHarmony < 0.67f) { t = getScalePitch (padIndex + 3); newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }
    else if (activeHarmony >= 0.67f) { int s = getScalePitch (padIndex + 6); newChord = { baseRoot + r, baseRoot + t, baseRoot + f, baseRoot + s }; }
    else { newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }

    if (!lastChordPitches.empty() && newChord.size() == lastChordPitches.size()) {
        int pitchDiff = newChord[2] - lastChordPitches[2];
        if (pitchDiff > 5) newChord[2] -= 12; else if (pitchDiff < -5) newChord[0] += 12; 
    }
    std::sort(newChord.begin(), newChord.end()); lastChordPitches = newChord;
    latchedNotes = newChord; apvts.getParameter(IDs::latch.getParamID())->setValueNotifyingHost(1.0f); 
}

void PluginProcessor::savePreset (int slotIndex) 
{ 
    if (slotIndex >= 0 && slotIndex < 8) 
    { 
        sceneAPresets[slotIndex] = sceneA; sceneBPresets[slotIndex] = sceneB;
        sceneASlotsSaved[slotIndex] = hasSceneA; sceneBSlotsSaved[slotIndex] = hasSceneB;

        for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); 
        presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); 
        presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); 
        presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); 
        presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); 
        presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); 
        presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); 
        presets[slotIndex].rate = *apvts.getRawParameterValue (IDs::rate.getParamID());
        presets[slotIndex].octaves = static_cast<float> (*apvts.getRawParameterValue (IDs::octaves.getParamID()));
        
        juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
        juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::ent