#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState();
    sceneB = SceneState();
    lastChordPitches = { 60, 64, 67 }; 
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
    mSampleRate = sampleRate;
    mLastStep = -1;
    mLastNotePlayed = -1;
    mNoteOffTime = 0;
    mTimeInSamples = 0;
    activeHeldNotes.clear();
    latchedNotes.clear();
    scheduledNoteOffs.clear();
    std::fill (std::begin (lfoPhases), std::end (lfoPhases), 0.0);
    isFirstNoteOfNewChord = true;
    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

std::vector<int> PluginProcessor::generateEuclideanPattern (int steps, int pulses)
{
    std::vector<int> pattern(steps, 0);
    if (pulses <= 0) return pattern;
    if (pulses >= steps) { std::fill(pattern.begin(), pattern.end(), 1); return pattern; }

    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps) { bucket -= steps; pattern[i] = 1; }
    }
    return pattern;
}

void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
    double sampleDelta = numSamples;

    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (IDs::cycleLength.getParamID())));
    int cycleBars = 4;
    if (cycleIndex == 0)      cycleBars = 1;
    else if (cycleIndex == 1) cycleBars = 2;
    else if (cycleIndex == 2) cycleBars = 4;
    else if (cycleIndex == 3) cycleBars = 8;

    if (cycleBars > 0)
        currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % cycleBars) + 1;
    else
        currentBarInCycle = 1;

    // Bipolar Entropy Accumulator
    float baseEntropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());

    float absEntropy = std::abs(baseEntropy);
    int stepInterval = 1; 
    if (absEntropy > 0.33f && absEntropy <= 0.66f) stepInterval = 2; 
    else if (absEntropy > 0.66f) stepInterval = 4; 

    if (currentStep == 0 && mLastStep == 7) 
    {
        if (baseEntropy > 0.05f) accumulatedPitchOffset += stepInterval;
        else if (baseEntropy < -0.05f) accumulatedPitchOffset -= stepInterval;
        
        if (currentBarInCycle == 1) accumulatedPitchOffset = 0.0f;
    }

    // 8-Channel LFO Modulation Matrix
    auto applyLfo = [&](int index, juce::ParameterID baseId, juce::ParameterID rateId, juce::ParameterID depthId, float minVal, float maxVal) -> float {
        float baseVal = *apvts.getRawParameterValue (baseId.getParamID());
        int rateChoice = static_cast<int> (*apvts.getRawParameterValue (rateId.getParamID()));
        float depth = *apvts.getRawParameterValue (depthId.getParamID());
        
        if (rateChoice == 0) 
            return baseVal;
            
        double divPPQ = 0.25; 
        if (rateChoice == 1)      divPPQ = 1.0;   
        else if (rateChoice == 2) divPPQ = 0.5;   
        else if (rateChoice == 3) divPPQ = 0.25;  
        else if (rateChoice == 4) divPPQ = 0.125; 
        
        double periodSamples = samplesPerBeat * divPPQ;
        lfoPhases[index] += (sampleDelta / periodSamples);
        if (lfoPhases[index] >= 1.0) lfoPhases[index] -= 1.0;
        
        float sineVal = static_cast<float> (std::sin (lfoPhases[index] * juce::MathConstants<double>::twoPi));
        float range = maxVal - minVal;
        float mod = (sineVal * depth * (range * 0.5f));
        return juce::jlimit (minVal, maxVal, baseVal + mod);
    };

    activeMorph   = applyLfo (0, IDs::rhythmMorph, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, 0.0f, 1.0f);
    activeRest    = applyLfo (1, IDs::rest,        IDs::restLfoRate,        IDs::restLfoDepth,        0.0f, 1.0f);
    
    activeLegato  = applyLfo (2, IDs::legato,      IDs::legatoLfoRate,      IDs::legatoLfoDepth,      0.1f, 1.0f);
    modLegato = activeLegato;
    modRest = activeRest;

    float rawRate = applyLfo (3, IDs::rate, IDs::rateLfoRate, IDs::rateLfoDepth, 0.0f, 3.0f);
    activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate)));

    activeEntropy = applyLfo (4, IDs::entropy,     IDs::entropyLfoRate,     IDs::entropyLfoDepth,     -1.0f, 1.0f);
    modEntropy = activeEntropy;

    activeHarmony = applyLfo (5, IDs::harmony,     IDs::harmonyLfoRate,     IDs::harmonyLfoDepth,     0.0f, 1.0f);
    modHarmony = activeHarmony;

    activeChaos   = applyLfo (6, IDs::chaos,       IDs::chaosLfoRate,       IDs::chaosLfoDepth,       0.0f, 1.0f);
    modChaos = activeChaos;

    float rawOctaves = applyLfo (7, IDs::octaves, IDs::octavesLfoRate, IDs::octavesLfoDepth, 1.0f, 4.0f);
    activeOctavesVal = juce::jlimit (1, 4, static_cast<int> (std::round (rawOctaves)));
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    if (delaySamples <= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);
    }
    else
    {
        scheduledNoteOffs.push_back ({ pitch, delaySamples });
    }
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    bool isPlaying = false;
    double bpm = 120.0;
    mSongPositionPPQ = 0.0;

#if JUCE_MAJOR_VERSION >= 7
    if (auto* playhead = getPlayHead())
    {
        if (auto pos = playhead->getPosition())
        {
            isPlaying = pos->getIsPlaying();
            auto bpmOpt = pos->getBpm();
            if (bpmOpt.hasValue()) bpm = *bpmOpt;
            auto ppqOpt = pos->getPpqPosition();
            if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt;
        }
    }
#else
    if (auto* playhead = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (playhead->getCurrentPositionInfo (info))
        {
            isPlaying = info.isPlaying;
            bpm = info.bpm;
            mSongPositionPPQ = info.ppqPosition;
        }
    }
#endif

    int numSamples = buffer.getNumSamples();
    updateLfoModulations (numSamples, bpm);

    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;

    float activeFaderProb[8];
    for (int i = 0; i < 8; ++i)
        activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    // 1. Process Decrementing Note-Off Queue
    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();)
    {
        it->second -= numSamples;
        if (it->second <= 0)
        {
            processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), 0);
            it = scheduledNoteOffs.erase(it);
        }
        else ++it;
    }

    // 2. Monitor physical keyboard pressed MIDI keys
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end())
            {
                activeHeldNotes.push_back (note);
                std::sort (activeHeldNotes.begin(), activeHeldNotes.end());
            }

            if (isLatchActive)
            {
                if (isFirstNoteOfNewChord)
                {
                    for (int n : latchedNotes) scheduleNoteOff (processedMidi, n, 0);
                    latchedNotes.clear();
                    isFirstNoteOfNewChord = false;
                }
                if (std::find (latchedNotes.begin(), latchedNotes.end(), note) == latchedNotes.end())
                {
                    latchedNotes.push_back (note);
                    std::sort (latchedNotes.begin(), latchedNotes.end());
                }
            }
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
            if (activeHeldNotes.empty()) isFirstNoteOfNewChord = true;
        }
    }
    midiMessages.clear();

    const auto& notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    
    isCurrentlyPlayingUI.store (!notesToPlay.empty());

    // 3. Dual-Clock Step Generation
    if (! notesToPlay.empty())
    {
        bool stepTriggered = false;
        double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        
        double stepLengthPPQ = 0.25; 
        if (activeRateIdx == 0)      stepLengthPPQ = 1.0;   
        else if (activeRateIdx == 1) stepLengthPPQ = 0.5;   
        else if (activeRateIdx == 2) stepLengthPPQ = 0.25;  
        else if (activeRateIdx == 3) stepLengthPPQ = 0.125; 

        double stepSamples = samplesPerBeat * stepLengthPPQ;

        if (isPlaying)
        {
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
            if (stepIndex != mLastStep)
            {
                mLastStep = stepIndex;
                currentStep = stepIndex;
                stepTriggered = true;
            }
        }
        else
        {
            mTimeInSamples += numSamples;
            if (mTimeInSamples >= stepSamples)
            {
                mTimeInSamples = 0;
                currentStep = (currentStep + 1) % 8;
                mLastStep = currentStep;
                stepTriggered = true;
            }
        }

        // 4. Arpeggiator Step & Euclidean Execution
        if (stepTriggered)
        {
            float faderProb = activeFaderProb[currentStep];
            
            int ratchetPulses = static_cast<int>(std::round(activeMorph * 8.0f));
            std::vector<int> euclidRatchets = generateEuclideanPattern (8, ratchetPulses);
            bool isRatchetStep = euclidRatchets[currentStep] == 1;
            juce::ignoreUnused(isRatchetStep);

            bool shouldPlay = (juce::Random::getSystemRandom().nextFloat() <= faderProb);
            bool isRest = (juce::Random::getSystemRandom().nextFloat() <= modRest);

            if (shouldPlay && ! isRest)
            {
                if (mLastNotePlayed != -1)
                {
                    processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
                    mLastNotePlayed = -1;
                }

                int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
                int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

                std::vector<int> scaleOffsets = { 0, 2, 4, 5, 7, 9, 11, 12 }; 
                if (scaleIdx == 1)      scaleOffsets = { 0, 2, 3, 5, 7, 8, 10, 12 }; 
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

                int octaveShiftCount = (currentStep / 2) % activeOctavesVal;

                int targetPitch = octave + rootKeyIdx + scaleOffsets[currentStep] + static_cast<int>(accumulatedPitchOffset) + (octaveShiftCount * 12);

                if (modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos)
                    targetPitch += (juce::Random::getSystemRandom().nextBool() ? 12 : -12);

                targetPitch = juce::jlimit(0, 127, targetPitch);
                int durationSamples = static_cast<int>(stepSamples * modLegato);

                processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
                mLastNotePlayed = targetPitch;
                mNoteOffTime = durationSamples;
                
                scheduleNoteOff (processedMidi, targetPitch, durationSamples);
            }
        }
    }
    else 
    { 
        if (mLastStep != -1)
        {
            if (mLastNotePlayed != -1)
            {
                processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
                mLastNotePlayed = -1;
            }
            mLastStep = -1; 
        }
        currentStep = 0; 
    }

    midiMessages.swapWith (processedMidi);
}

void PluginProcessor::triggerDiatonicChordPad (int padIndex)
{
    int rootIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
    int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

    std::vector<int> scaleOffsets;
    switch (scaleIdx)
    {
        case 1:  scaleOffsets = { 0, 2, 3, 5, 7, 8, 10 }; break; 
        case 2:  scaleOffsets = { 0, 3, 5, 7, 10, 12, 14 }; break; 
        case 3:  scaleOffsets = { 0, 2, 4, 7, 9, 12, 14 }; break; 
        case 4:  scaleOffsets = { 0, 2, 3, 5, 7, 9, 10 }; break; 
        case 5:  scaleOffsets = { 0, 1, 3, 5, 7, 8, 10 }; break; 
        case 6:  scaleOffsets = { 0, 2, 4, 6, 7, 9, 11 }; break; 
        case 7:  scaleOffsets = { 0, 2, 4, 5, 7, 9, 10 }; break; 
        case 8:  scaleOffsets = { 0, 2, 3, 5, 7, 8, 11 }; break; 
        case 9:  scaleOffsets = { 0, 2, 3, 5, 7, 9, 11 }; break; 
        default: scaleOffsets = { 0, 2, 4, 5, 7, 9, 11 }; break; 
    }

    auto getScalePitch = [&](int degree) -> int {
        int octaveShift = (degree / 7) * 12;
        return scaleOffsets[degree % 7] + octaveShift;
    };

    int baseRoot = 48 + rootIdx;
    int r = getScalePitch (padIndex);
    int t = getScalePitch (padIndex + 2); 
    int f = getScalePitch (padIndex + 4); 

    std::vector<int> newChord;
    if (activeHarmony >= 0.34f && activeHarmony < 0.67f)
    {
        t = getScalePitch (padIndex + 3); 
        newChord = { baseRoot + r, baseRoot + t, baseRoot + f };
    }
    else if (activeHarmony >= 0.67f)
    {
        int s = getScalePitch (padIndex + 6); 
        newChord = { baseRoot + r, baseRoot + t, baseRoot + f, baseRoot + s };
    }
    else
    {
        newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; 
    }

    if (! lastChordPitches.empty() && newChord.size() == lastChordPitches.size())
    {
        int pitchDiff = newChord[2] - lastChordPitches[2];
        if (pitchDiff > 5) newChord[2] -= 12; 
        else if (pitchDiff < -5) newChord[0] += 12; 
    }
    
    std::sort(newChord.begin(), newChord.end());
    lastChordPitches = newChord;

    latchedNotes = newChord;
    apvts.getParameter(IDs::latch.getParamID())->setValueNotifyingHost(1.0f); 
}

void PluginProcessor::savePreset (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8) return;
    for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    presetSlotsSaved[slotIndex] = true;
}

void PluginProcessor::loadPreset (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8 || ! presetSlotsSaved[slotIndex]) return;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (presets[slotIndex].entropy);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos);

    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]);
}

void PluginProcessor::captureSceneA()
{
    for (int i = 0; i < 8; ++i) sceneA.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    sceneA.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    sceneA.rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneA.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneA.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneA.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneA.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    hasSceneA = true;
}

void PluginProcessor::captureSceneB()
{
    for (int i = 0; i < 8; ++i) sceneB.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    sceneB.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    sceneB.rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneB.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneB.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneB.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneB.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    hasSceneB = true;
}

void PluginProcessor::diceMelody()
{
    auto* random = &juce::Random::getSystemRandom();
    for (int i = 1; i <= 8; ++i) apvts.getParameter ("fader" + juce::String(i))->setValueNotifyingHost (random->nextFloat());
}

void PluginProcessor::diceRhythm()
{
    auto* random = &juce::Random::getSystemRandom();
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (random->nextFloat());
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (random->nextFloat() * 0.5f);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (0.2f + random->nextFloat() * 0.8f);
}

void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }
void PluginProcessor::resetRhythm() { apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); }

bool PluginProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor (*this); }

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (int i = 1; i <= 8; ++i)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("fader" + juce::String (i), 1), "Fader " + juce::String (i), 0.0f, 1.0f, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rhythmMorph, "Rhythm Morph", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rest, "Rest", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::legato, "Legato", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::entropy, "Entropy", -1.0f, 1.0f, 0.0f)); 
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::harmony, "Harmony", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::chaos, "Chaos", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::morph, "Morph Crossfader", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::latch, "Latch Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::chordMode, "Chord Mode", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rootKey, "Root Key", 
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleType, "Scale", 
        juce::StringArray { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::cycleLength, "Cycle Length", 
        juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2)); 

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rate, "Rate", 
        juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2)); 

    params.push_back (std::make_unique<juce::AudioParameterInt> (IDs::octaves, "Octaves", 1, 4, 1)); 

    // Dynamic Panel Theme selector parameter [1]
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::panelTheme, "Panel Theme", 
        juce::StringArray { "Navy Cyber", "Skyline Eurorack", "Monochrome Minimal", "Matrix Terminal" }, 0));

    // Helper lambda to register LFO parameters
    auto registerLfoParams = [&](juce::ParameterID rateId, juce::ParameterID depthId, juce::String name) {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (rateId, name + " LFO Speed", 
            juce::StringArray { "Off", "1/4", "1/8", "1/16", "1/32" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (depthId, name + " LFO Depth", 0.0f, 1.0f, 0.0f));
    };

    registerLfoParams (IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, "Morph");
    registerLfoParams (IDs::restLfoRate,        IDs::restLfoDepth,        "Rest");
    registerLfoParams (IDs::legatoLfoRate,      IDs::legatoLfoDepth,      "Legato");
    registerLfoParams (IDs::rateLfoRate,        IDs::rateLfoDepth,        "Rate");
    registerLfoParams (IDs::entropyLfoRate,     IDs::entropyLfoDepth,     "Entropy");
    registerLfoParams (IDs::harmonyLfoRate,     IDs::harmonyLfoDepth,     "Harmony");
    registerLfoParams (IDs::chaosLfoRate,       IDs::chaosLfoDepth,       "Chaos");
    registerLfoParams (IDs::octavesLfoRate,     IDs::octavesLfoDepth,     "Octaves");

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }