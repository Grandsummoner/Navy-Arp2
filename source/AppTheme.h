#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct AppTheme
{
    juce::Colour background;
    juce::Colour border;
    juce::Colour leftAccent;
    juce::Colour rightAccent;
    juce::Colour textDim;
    juce::Colour trackBg;
    juce::Colour slotOutline;
    juce::Colour faderCap;
    juce::Colour unlitLed;

    static AppTheme get (int index)
    {
        AppTheme t;
        if (index == 1) // Skyline Eurorack (Light Panel)
        {
            t.background    = juce::Colour (0xFFE2E0D8);
            t.border        = juce::Colour (0xFFB8B5AB);
            t.leftAccent    = juce::Colour (0xFFFF3B30);
            t.rightAccent   = juce::Colour (0xFF3A3A38);
            t.textDim       = juce::Colour (0xFF1A1A18);
            t.trackBg       = juce::Colour (0xFFD4D1C9);
            t.slotOutline   = juce::Colour (0xFFA8A59C);
            t.faderCap      = juce::Colour (0xFF1E1E1E);
            t.unlitLed      = juce::Colour (0xFFB8B5AB);
        }
        else if (index == 2) // Monochrome Minimal
        {
            t.background    = juce::Colour (0xFF101010);
            t.border        = juce::Colour (0xFF242424);
            t.leftAccent    = juce::Colour (0xFFFFFFFF);
            t.rightAccent   = juce::Colour (0xFF88888A);
            t.textDim       = juce::Colour (0xFF66666A);
            t.trackBg       = juce::Colour (0xFF080808);
            t.slotOutline   = juce::Colour (0xFF2D2D32);
            t.faderCap      = juce::Colour (0xFF222222);
            t.unlitLed      = juce::Colour (0xFF1C1C1F);
        }
        else if (index == 3) // Matrix Terminal
        {
            t.background    = juce::Colour (0xFF030A05);
            t.border        = juce::Colour (0xFF112A18);
            t.leftAccent    = juce::Colour (0xFF33FF33);
            t.rightAccent   = juce::Colour (0xFF22AA22); 
            t.textDim       = juce::Colour (0xFF1E5F2E);
            t.trackBg       = juce::Colour (0xFF020603);
            t.slotOutline   = juce::Colour (0xFF0E451E);
            t.faderCap      = juce::Colour (0xFF112A18);
            t.unlitLed      = juce::Colour (0xFF0E1A11);
        }
        else // Default Navy Cyber
        {
            t.background    = juce::Colour (0xFF16181F);
            t.border        = juce::Colour (0xFF2A2E3D);
            t.leftAccent    = juce::Colour (0xFF00D2FF);
            t.rightAccent   = juce::Colour (0xFFFFB300);
            t.textDim       = juce::Colour (0xFF55555C);
            t.trackBg       = juce::Colour (0xFF181C24);
            t.slotOutline   = juce::Colour (0xFF242835);
            t.faderCap      = juce::Colour (0xFF1E212A);
            t.unlitLed      = juce::Colour (0xFF1C1E24);
        }
        return t;
    }
};
