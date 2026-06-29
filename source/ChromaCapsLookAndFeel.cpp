#include "ChromaCapsLookAndFeel.h"
#include "PluginProcessor.h"

ChromaCapsLookAndFeel::ChromaCapsLookAndFeel (PluginProcessor& p, juce::AudioProcessorEditor* editor)
    : processor (p), parentEditor (editor)
{
}

ChromaCapsLookAndFeel::~ChromaCapsLookAndFeel()
{
}

void ChromaCapsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    auto& lf = button.getLookAndFeel();
    g.setFont (lf.getTextButtonFont (button, button.getHeight()));
    
    const bool isButtonA = (button.getButtonText() == "A");
    const bool isButtonB = (button.getButtonText() == "B");
    
    if (isButtonA || isButtonB)
    {
        const bool isSceneB = processor.isSceneBActiveAnchor.load();
        const bool isActiveAnchor = (isButtonA && !isSceneB) || (isButtonB && isSceneB);
        
        if (isActiveAnchor)
            g.setColour (juce::Colours::black); 
        else
            g.setColour (juce::Colours::white.withAlpha (0.7f));
    }
    else if (button.getClickingTogglesState() && button.getToggleState())
    {
        // High contrast bold black text on active glowing toggle buttons
        g.setColour (juce::Colours::black);
    }
    else
    {
        g.setColour (button.findColour (juce::TextButton::textColourOffId).withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    }
    
    const int indent = 2;
    g.drawFittedText (button.getButtonText(), 
                      indent, indent, 
                      button.getWidth() - 2 * indent, 
                      button.getHeight() - 2 * indent, 
                      juce::Justification::centred, 2);
}

void ChromaCapsLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const float cornerSize = 4.0f;
    auto baseColour = backgroundColour;

    const bool isButtonA = (button.getButtonText() == "A");
    const bool isButtonB = (button.getButtonText() == "B");

    if (isButtonA || isButtonB)
    {
        const bool isSceneB = processor.isSceneBActiveAnchor.load();
        const bool isActiveAnchor = (isButtonA && !isSceneB) || (isButtonB && isSceneB);

        if (isActiveAnchor)
            baseColour = juce::Colour (0xFF00D2FF); // Active anchor highlighted in Cyan
        else
            baseColour = juce::Colour (0xFF2A2D36); // Inactive dark background
    }
    else if (button.getClickingTogglesState())
    {
        // Handles Latch, Seq, Poly, and Freeze toggle states
        if (button.getToggleState())
            baseColour = juce::Colour (0xFF00D2FF); // High contrast Cyan on active toggle state
        else
            baseColour = juce::Colour (0xFF2D313A); // Dark charcoal on inactive state
    }

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter (0.1f);

    g.setColour (baseColour);
    g.fillRoundedRectangle (bounds, cornerSize);

    // Draw thin outline
    g.setColour (button.findColour (juce::ComboBox::outlineColourId, true).withAlpha (0.15f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);
}

void ChromaCapsLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
    {
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRect (x, y, width, height);
        return;
    }

    const bool isVertical = (style == juce::Slider::LinearVertical);
    
    if (isVertical)
    {
        const float trackWidth = 6.0f;
        const float trackX = static_cast<float>(x) + (static_cast<float>(width) - trackWidth) * 0.5f;
        
        // Base track background slot
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (trackX, static_cast<float>(y), trackWidth, static_cast<float>(height), 3.0f);

        // Real-Time per-fader level meters rendering
        if (slider.getComponentID().startsWith ("fader"))
        {
            // Parse out fader index from ComponentID (e.g. fader1 -> 0, fader2 -> 1)
            int faderIndex = slider.getComponentID().getLastCharacters (1).getIntValue() - 1;
            if (faderIndex >= 0 && faderIndex < 8)
            {
                float level = processor.currentSlewValue[faderIndex];
                if (level > 0.01f)
                {
                    // Draw a glowing level meter indicator rising from the bottom of the slot track
                    float levelHeight = static_cast<float> (height) * level;
                    float levelY = static_cast<float> (y) + static_cast<float> (height) - levelHeight;
                    
                    g.setColour (juce::Colour (0xFF00D2FF).withAlpha (level)); // Cyan fader meter glow
                    g.fillRoundedRectangle (trackX, levelY, trackWidth, levelHeight, 3.0f);
                }
            }
        }
        
        // Inner 3D bevel highlighting
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawVerticalLine (static_cast<int>(trackX), static_cast<float>(y), static_cast<float>(y + height));
        
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawVerticalLine (static_cast<int>(trackX + trackWidth), static_cast<float>(y), static_cast<float>(y + height));

        // Draw fader cap handle thumb
        const float thumbHeight = 16.0f;
        const float thumbWidth = 24.0f;
        const float thumbX = static_cast<float>(x) + (static_cast<float>(width) - thumbWidth) * 0.5f;
        const float thumbY = sliderPos - (thumbHeight * 0.5f);

        const juce::Colour capColour = slider.findColour (juce::Slider::thumbColourId);
        g.setColour (capColour);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        g.setColour (juce::Colours::white);
        g.fillRect (thumbX + 2.0f, thumbY + (thumbHeight * 0.5f) - 1.0f, thumbWidth - 4.0f, 2.0f);
    }
    else
    {
        const float trackHeight = 6.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;
        
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (static_cast<float>(x), trackY, static_cast<float>(width), trackHeight, 3.0f);
        
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawHorizontalLine (static_cast<int>(trackY), static_cast<float>(x), static_cast<float>(x + width));
        
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawHorizontalLine (static_cast<int>(trackY + trackHeight), static_cast<float>(x), static_cast<float>(x + width));

        // Horizontal thumb
        const float thumbWidth = 16.0f;
        const float thumbHeight = 24.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        const juce::Colour capColour = slider.findColour (juce::Slider::thumbColourId);
        g.setColour (capColour);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        g.setColour (juce::Colours::white);
        g.fillRect (thumbX + (thumbWidth * 0.5f) - 1.0f, thumbY + 2.0f, 2.0f, thumbHeight - 4.0f);
    }
}