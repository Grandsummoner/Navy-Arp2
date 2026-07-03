#pragma once

#include <JuceHeader.h>

namespace AppTheme
{
    struct Theme
    {
        juce::Colour background;
        juce::Colour border;
        juce::Colour textDim;
        juce::Colour slotOutline;
        juce::Colour knobFillLeft;
        juce::Colour knobFillRight;
        juce::Colour faderCap;
        juce::Colour crossfaderTrackA; // Accent color for Scene A (Left of crossfader) [43]
        juce::Colour crossfaderTrackB; // Accent color for Scene B (Right of crossfader) [43]
    };

    inline Theme get (int index)
    {
        switch (index)
        {
            case 0: // Navy Cyber
                return {
                    juce::Colour::fromString ("#FF0A0B10"), // background (obsidian deep navy)
                    juce::Colour::fromString ("#FF5D8AA8"), // border (softer slate blue-grey)
                    juce::Colour::fromString ("#FF888888"), // textDim
                    juce::Colour::fromString ("#FF181C24"), // slotOutline
                    juce::Colour::fromString ("#FFFFB300"), // knobFillLeft (Amber/Gold)
                    juce::Colour::fromString ("#FFFFB300"), // knobFillRight (Amber/Gold)
                    juce::Colour::fromString ("#FF0A1D33"), // faderCap (Deep Navy Blue)
                    juce::Colour::fromString ("#FFFFB300"), // crossfaderTrackA (Amber/Gold)
                    juce::Colour::fromString ("#FF00D2FF")  // crossfaderTrackB (Neon Ice Blue)
                };
            case 1: // Skyline Eurorack (Beige Faceplate)
                return {
                    juce::Colour::fromString ("#FFE8E4DB"), // background (classic warm eurorack beige)
                    juce::Colour::fromString ("#FF55555C"), // border (aluminum anthracite grey)
                    juce::Colour::fromString ("#FF4A4B50"), // textDim (high contrast dark grey for beige)
                    juce::Colour::fromString ("#FFC8C3BC"), // slotOutline
                    juce::Colour::fromString ("#FFFF5533"), // knobFillLeft (Accented Red-Orange)
                    juce::Colour::fromString ("#FFFF5533"), // knobFillRight (Accented Red-Orange)
                    juce::Colour::fromString ("#FF1B2838"), // faderCap (Clean Navy Blue)
                    juce::Colour::fromString ("#FFFF5533"), // crossfaderTrackA (Accented Red-Orange)
                    juce::Colour::fromString ("#FF48929B")  // crossfaderTrackB (Muted Teal Blue)
                };
            case 2: // Monochrome Minimal
                return {
                    juce::Colour::fromString ("#FF1E2127"), // background
                    juce::Colour::fromString ("#FFFFB300"), // border
                    juce::Colour::fromString ("#FF9E9E9E"), // textDim
                    juce::Colour::fromString ("#FF303030"), // slotOutline
                    juce::Colour::fromString ("#FFFFB300"), // knobFillLeft (Amber/Gold)
                    juce::Colour::fromString ("#FFFFB300"), // knobFillRight (Amber/Gold)
                    juce::Colour::fromString ("#FF0A1D33"), // faderCap (Deep Navy Blue)
                    juce::Colour::fromString ("#FFFFB300"), // crossfaderTrackA (Amber/Gold)
                    juce::Colour::fromString ("#FFFFB300")  // crossfaderTrackB (Amber/Gold)
                };
            case 3: // Matrix Terminal
                return {
                    juce::Colour::fromString ("#FF030803"), // background
                    juce::Colour::fromString ("#FF33FF33"), // border
                    juce::Colour::fromString ("#FF558855"), // textDim
                    juce::Colour::fromString ("#FF112211"), // slotOutline
                    juce::Colour::fromString ("#FF33FF33"), // knobFillLeft (Matrix Green)
                    juce::Colour::fromString ("#FF33FF33"), // knobFillRight (Matrix Green)
                    juce::Colour::fromString ("#FF051505"), // faderCap (Very Dark Greenish Navy)
                    juce::Colour::fromString ("#FF33FF33"), // crossfaderTrackA (Matrix Green)
                    juce::Colour::fromString ("#FF33FF33")  // crossfaderTrackB (Matrix Green)
                };
            default:
                return {
                    juce::Colour::fromString ("#FFE5E0D8"), // background
                    juce::Colour::fromString ("#FF55555C"), // border
                    juce::Colour::fromString ("#FF3A3A3D"), // textDim
                    juce::Colour::fromString ("#FFC4BCB4"), // slotOutline
                    juce::Colour::fromString ("#FFFF5533"), // knobFillLeft (Red-Orange)
                    juce::Colour::fromString ("#FFFF5533"), // knobFillRight (Red-Orange)
                    juce::Colour::fromString ("#FF1B2838"), // faderCap (Clean Navy Blue)
                    juce::Colour::fromString ("#FFFF5533"), // crossfaderTrackA (Red-Orange)
                    juce::Colour::fromString ("#FF48929B")  // crossfaderTrackB (Muted Teal Blue)
                };
        }
    }
}