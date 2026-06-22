#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p)
{
    addAndMakeVisible (oledDisplay);

    // Bottom faders (Scale degrees)
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };

    for (int i = 0; i < 8; ++i)
    {
        faders[i]->setSliderStyle (juce::Slider::LinearVertical);
        faders[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        faders[i]->setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFFFFF)); // Monochrome paper-white handles
        faders[i]->setColour (juce::Slider::trackColourId, juce::Colour (0xFF1a1a1c));
        addAndMakeVisible (faders[i]);

        faderLabels[i]->setFont (juce::FontOptions (13.0f).withStyle ("bold"));
        faderLabels[i]->setJustificationType (juce::Justification::centred);
        faderLabels[i]->setColour (juce::Label::textColourId, juce::Colour (0xFFaaaaaa));
        addAndMakeVisible (faderLabels[i]);
    }

    // Left sidebar knobs (Rhythm Section)
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle };
    juce::String leftNames[] = { "MORPH", "REST", "LEGATO" };

    for (int i = 0; i < 3; ++i)
    {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 15);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF)); // Cyan LFO rings
        leftKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        leftKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (leftKnobs[i]);

        leftTitles[i]->setText (leftNames[i], juce::dontSendNotification);
        leftTitles[i]->setFont (juce::FontOptions (11.0f).withStyle ("bold"));
        leftTitles[i]->setJustificationType (juce::Justification::centred);
        leftTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFFcccccc)); // Screen-printed white typography
        addAndMakeVisible (leftTitles[i]);
    }

    // Right sidebar knobs (Harmony & Chaos Section)
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle };
    juce::String rightNames[] = { "ENTROPY", "HARMONY", "CHAOS" };

    for (int i = 0; i < 3; ++i)
    {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 15);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300)); // Amber LFO rings
        rightKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        rightKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (rightKnobs[i]);

        rightTitles[i]->setText (rightNames[i], juce::dontSendNotification);
        rightTitles[i]->setFont (juce::FontOptions (11.0f).withStyle ("bold"));
        rightTitles[i]->setJustificationType (juce::Justification::centred);
        rightTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFFcccccc));
        addAndMakeVisible (rightTitles[i]);
    }

    // Octatrack Crossfader
    morphCrossfader.setSliderStyle (juce::Slider::LinearHorizontal);
    morphCrossfader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphCrossfader.setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFFFFF));
    morphCrossfader.setColour (juce::Slider::trackColourId, juce::Colour (0xFF1a1a1c));
    addAndMakeVisible (morphCrossfader);

    // Mechanical Toggle Keys
    addAndMakeVisible (latchButton);
    latchButton.setButtonText ("LATCH");
    latchButton.setClickingTogglesState (true);
    latchButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF141416));
    latchButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF666666));
    latchButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF00D2FF));
    latchButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));

    addAndMakeVisible (chordModeButton);
    chordModeButton.setButtonText ("CHORDS");
    chordModeButton.setClickingTogglesState (true);
    chordModeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF141416));
    chordModeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF666666));
    chordModeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFFFFB300));
    chordModeButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));

    // DICE Buttons with Secondary Text actions
    addAndMakeVisible (diceMelodyButton);
    diceMelodyButton.setButtonText ("DICE M");
    diceMelodyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF220505));
    diceMelodyButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFFF3333)); // Neon Red
    diceMelodyButton.onClick = [this] { 
        if (juce::ModifierKeys::currentModifiers.isShiftDown()) processor.resetAccumulator();
        else processor.diceMelody(); 
    };

    addAndMakeVisible (diceRhythmButton);
    diceRhythmButton.setButtonText ("DICE R");
    diceRhythmButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF052205));
    diceRhythmButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF33FF33)); // Neon Green
    diceRhythmButton.onClick = [this] { 
        if (juce::ModifierKeys::currentModifiers.isShiftDown()) processor.resetRhythm();
        else processor.diceRhythm(); 
    };

    // Scene Capture Toggles
    addAndMakeVisible (sceneAButton);
    sceneAButton.setButtonText ("A");
    sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneAButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));
    sceneAButton.onClick = [this] {
        if (processor.hasSceneA) { processor.clearSceneA(); sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111)); }
        else { processor.captureSceneA(); sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF00D2FF)); }
    };

    addAndMakeVisible (sceneBButton);
    sceneBButton.setButtonText ("B");
    sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneBButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));
    sceneBButton.onClick = [this] {
        if (processor.hasSceneB) { processor.clearSceneB(); sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111)); }
        else { processor.captureSceneB(); sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFFFB300)); }
    };

    // 8 Dual-Mode Pads (Presets 1-8 OR Smart Diatonic Chords I-VIII)
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].setButtonText (juce::String (i + 1));
        presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF0a0a0c));
        presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF555555));

        presetButtons[i].onClick = [this, i] {
            if (chordModeButton.getToggleState()) processor.triggerDiatonicChordPad(i); // Trigger Trance Chord
            else processor.loadPreset(i); // Recall Preset
        };
        presetButtons[i].addMouseListener (this, false);
    }

    // Dropdowns
    addAndMakeVisible (rootKeyBox);
    rootKeyBox.addItemList (juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 1);
    rootKeyBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF0a0a0c));
    rootKeyBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF232326));
    rootKeyBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xFFFFFFFF));

    addAndMakeVisible (scaleTypeBox);
    scaleTypeBox.addItemList (juce::StringArray { "Major", "Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1);
    scaleTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF0a0a0c));
    scaleTypeBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF232326));
    scaleTypeBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xFF00D2FF));

    addAndMakeVisible (cycleLengthBox);
    cycleLengthBox.addItemList (juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 1);
    cycleLengthBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF0a0a0c));
    cycleLengthBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF232326));
    cycleLengthBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xFFFFB300));

    // Bindings
    fader1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader1.getParamID(), fader1);
    fader2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader2.getParamID(), fader2);
    fader3Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader3.getParamID(), fader3);
    fader4Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader4.getParamID(), fader4);
    fader5Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader5.getParamID(), fader5);
    fader6Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader6.getParamID(), fader6);
    fader7Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader7.getParamID(), fader7);
    fader8Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader8.getParamID(), fader8);

    rhythmMorphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rhythmMorph.getParamID(), rhythmMorphKnob);
    restAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rest.getParamID(), restKnob);
    legatoAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::legato.getParamID(), legatoKnob);

    entropyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::entropy.getParamID(), entropyKnob);
    harmonyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::harmony.getParamID(), harmonyKnob);
    chaosAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::chaos.getParamID(), chaosKnob);

    morphAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::morph.getParamID(), morphCrossfader);
    latchAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::latch.getParamID(), latchButton);
    chordModeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::chordMode.getParamID(), chordModeButton);

    rootKeyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::rootKey.getParamID(), rootKeyBox);
    scaleTypeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::scaleType.getParamID(), scaleTypeBox);
    cycleLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::cycleLength.getParamID(), cycleLengthBox);

    setSize (780, 520);
    startTimerHz (30);
}

PluginEditor::~PluginEditor() { stopTimer(); }

void PluginEditor::timerCallback()
{
    float morphValue = morphCrossfader.getValue();
    
    // Real-time parameter broadcasting across threads
    if (processor.hasSceneA && processor.hasSceneB && morphCrossfader.isMouseButtonDown())
    {
        for (int i = 0; i < 8; ++i)
        {
            float targetValue = (processor.sceneA.faders[i] * (1.0f - morphValue)) + (processor.sceneB.faders[i] * morphValue);
            processor.apvts.getParameter ("fader" + juce::String(i + 1))->setValueNotifyingHost (targetValue);
        }

        processor.apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost((processor.sceneA.rhythmMorph * (1.0f - morphValue)) + (processor.sceneB.rhythmMorph * morphValue));
        processor.apvts.getParameter(IDs::rest.getParamID())->setValueNotifyingHost((processor.sceneA.rest * (1.0f - morphValue)) + (processor.sceneB.rest * morphValue));
        processor.apvts.getParameter(IDs::legato.getParamID())->setValueNotifyingHost((processor.sceneA.legato * (1.0f - morphValue)) + (processor.sceneB.legato * morphValue));
        processor.apvts.getParameter(IDs::entropy.getParamID())->setValueNotifyingHost((processor.sceneA.entropy * (1.0f - morphValue)) + (processor.sceneB.entropy * morphValue));
        processor.apvts.getParameter(IDs::harmony.getParamID())->setValueNotifyingHost((processor.sceneA.harmony * (1.0f - morphValue)) + (processor.sceneB.harmony * morphValue));
        processor.apvts.getParameter(IDs::chaos.getParamID())->setValueNotifyingHost((processor.sceneA.chaos * (1.0f - morphValue)) + (processor.sceneB.chaos * morphValue));
    }

    // Dynamic Note Reader updating bottom fader text labels
    int activeKeyIdx = rootKeyBox.getSelectedItemIndex();
    int activeScaleIdx = scaleTypeBox.getSelectedItemIndex();
    
    std::vector<int> offsets = { 0, 2, 4, 5, 7, 9, 11, 12 }; // Major
    if (activeScaleIdx == 1)      offsets = { 0, 2, 3, 5, 7, 8, 10, 12 }; // Minor
    else if (activeScaleIdx == 2) offsets = { 0, 3, 5, 7, 10, 12, 15, 17 }; // Pent Minor
    else if (activeScaleIdx == 3) offsets = { 0, 2, 4, 7, 9, 12, 14, 16 };  // Pent Major
    else if (activeScaleIdx == 4) offsets = { 0, 2, 3, 5, 7, 9, 10, 12 };  // Dorian
    else if (activeScaleIdx == 5) offsets = { 0, 1, 3, 5, 7, 8, 10, 12 };  // Phrygian
    else if (activeScaleIdx == 6) offsets = { 0, 2, 4, 6, 7, 9, 11, 12 };  // Lydian
    else if (activeScaleIdx == 7) offsets = { 0, 2, 4, 5, 7, 9, 10, 12 };  // Mixo
    else if (activeScaleIdx == 8) offsets = { 0, 2, 3, 5, 7, 8, 11, 12 };  // Harmonic Min
    else if (activeScaleIdx == 9) offsets = { 0, 2, 3, 5, 7, 9, 11, 12 };  // Melodic Min

    juce::String notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };
    juce::Label* labels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };

    for (int i = 0; i < 8; ++i)
    {
        int pitchIdx = (activeKeyIdx + offsets[i]) % 12;
        labels[i]->setText (notes[pitchIdx], juce::dontSendNotification);
    }

    // Keep Preset glow updated
    for (int i = 0; i < 8; ++i)
        if (processor.isPresetSaved (i)) presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF003344));
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Horizontal Anodized Brushed Black Aluminum Texture simulation
    g.fillAll (juce::Colour (0xFF121214));
    g.setColour (juce::Colour (0xFF1c1c1e));
    for (int y = 0; y < getHeight(); y += 4) g.drawHorizontalLine (y, 0.0f, static_cast<float>(getWidth()));

    // Screen-printed Industrial Faceplate Borders
    g.setColour (juce::Colour (0xFF2d2d30));
    g.drawRect (getLocalBounds().toFloat(), 4.0f);

    // Corner Allen Bolts
    g.setColour (juce::Colour (0xFF08080a));
    g.fillEllipse (12, 12, 14, 14); g.fillEllipse (getWidth() - 26, 12, 14, 14);
    g.fillEllipse (12, getHeight() - 26, 14, 14); g.fillEllipse (getWidth() - 26, getHeight() - 26, 14, 14);
    g.setColour (juce::Colour (0xFF333338));
    g.drawEllipse (12, 12, 14, 14, 1.5f); g.drawEllipse (getWidth() - 26, 12, 14, 14, 1.5f);
    g.drawEllipse (12, getHeight() - 26, 14, 14, 1.5f); g.drawEllipse (getWidth() - 26, getHeight() - 26, 14, 14, 1.5f);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (18);

    // 1. Bottom Row: 8 Scale-Degree Faders
    auto bottomArea = area.removeFromBottom (125);
    auto faderLabelArea = bottomArea.removeFromBottom (20);
    int faderWidth = bottomArea.getWidth() / 8;
    
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    
    for (int i = 0; i < 8; ++i)
    {
        faders[i]->setBounds (bottomArea.removeFromLeft (faderWidth).reduced (8, 0));
        faderLabels[i]->setBounds (faderLabelArea.removeFromLeft (faderWidth));
    }

    area.removeFromBottom (10);

    // 2. Scene Morph Strip
    auto morphArea = area.removeFromBottom (36);
    sceneAButton.setBounds (morphArea.removeFromLeft (36).reduced (0, 3));
    sceneBButton.setBounds (morphArea.removeFromRight (36).reduced (0, 3));
    morphCrossfader.setBounds (morphArea.reduced (12, 4));

    area.removeFromBottom (10);

    // 3. Sidebars
    auto leftSidebar = area.removeFromLeft (105);
    auto rightSidebar = area.removeFromRight (105);
    
    int leftRowHeight = leftSidebar.getHeight() / 4;
    rhythmMorphTitle.setBounds (leftSidebar.removeFromTop (12).reduced (2, 0));
    rhythmMorphKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight - 12).reduced (4));
    restTitle.setBounds (leftSidebar.removeFromTop (12).reduced (2, 0));
    restKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight - 12).reduced (4));
    legatoTitle.setBounds (leftSidebar.removeFromTop (12).reduced (2, 0));
    legatoKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight - 12).reduced (4));
    
    auto leftBtnArea = leftSidebar.reduced(2, 6);
    latchButton.setBounds (leftBtnArea.removeFromLeft(leftBtnArea.getWidth() / 2).reduced(1, 0));
    chordModeButton.setBounds (leftBtnArea.reduced(1, 0));

    int rightRowHeight = rightSidebar.getHeight() / 4;
    entropyTitle.setBounds (rightSidebar.removeFromTop (12).reduced (2, 0));
    entropyKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight - 12).reduced (4));
    harmonyTitle.setBounds (rightSidebar.removeFromTop (12).reduced (2, 0));
    harmonyKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight - 12).reduced (4));
    chaosTitle.setBounds (rightSidebar.removeFromTop (12).reduced (2, 0));
    chaosKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight - 12).reduced (4));
    
    auto diceArea = rightSidebar.reduced (2, 6);
    diceMelodyButton.setBounds (diceArea.removeFromLeft (diceArea.getWidth() / 2).reduced (1, 0));
    diceRhythmButton.setBounds (diceArea.reduced (1, 0));

    // 4. Center Section: OLED Display, Dropdowns, and 8 Dual-Mode Pads
    auto presetArea = area.removeFromBottom (34);
    auto oledArea = area.reduced (6, 4);
    
    rootKeyBox.setBounds (oledArea.removeFromLeft (60).removeFromTop (26).translated (4, 4));
    scaleTypeBox.setBounds (oledArea.removeFromLeft (115).removeFromTop (26).translated (8, 4));
    cycleLengthBox.setBounds (oledArea.removeFromRight (85).removeFromTop (26).translated (-4, 4));
    
    oledDisplay.setBounds (area.reduced (5, 3));

    int presetWidth = presetArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i) presetButtons[i].setBounds (presetArea.removeFromLeft (presetWidth).reduced (4, 3));
}