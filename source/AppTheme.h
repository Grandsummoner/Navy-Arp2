#pragma once

#include <JuceHeader.h>

namespace AppTheme
{
    struct Theme
    {
        juce::Colour backgroundStart; 
        juce::Colour backgroundEnd;   
        juce::Colour border;
        juce::Colour textDim;
        juce::Colour slotOutline;
        juce::Colour knobFillLeft;     
        juce::Colour knobFillRight;    
        juce::Colour faderCap;         
        juce::Colour crossfaderTrackA; 
        juce::Colour crossfaderTrackB; 
    };

    inline Theme get (int index)
    {
        switch (index)
        {
            case 0: // Navy Cyber
                return {
                    juce::Colour::fromString ("#FF0D1E36"), 
                    juce::Colour::fromString ("#FF030508"), 
                    juce::Colour::fromString ("#FF5D8AA8"), 
                    juce::Colour::fromString ("#FF888888"), 
                    juce::Colour::fromString ("#FF181C24"), 
                    juce::Colour::fromString ("#FFFFB300"), 
                    juce::Colour::fromString ("#FFFFB300"), 
                    juce::Colour::fromString ("#FF0A1D33"), 
                    juce::Colour::fromString ("#FFFFB300"), 
                    juce::Colour::fromString ("#FF00D2FF")  
                };
            case 1: // Skyline Eurorack
                return {
                    juce::Colour::fromString ("#FFE8E4DB"), 
                    juce::Colour::fromString ("#FFE8E4DB"), 
                    juce::Colour::fromString ("#FF55555C"), 
                    juce::Colour::fromString ("#FF4A4B50"), 
                    juce::Colour::fromString ("#FFC8C3BC"), 
                    juce::Colour::fromString ("#FFFF5533"), 
                    juce::Colour::fromString ("#FFFF5533"), 
                    juce::Colour::fromString ("#FF1B2838"), 
                    juce::Colour::fromString ("#FFFF5533"), 
                    juce::Colour::fromString ("#FF48929B")  
                };
            case 2: // Monochrome Minimal
                return {
                    juce::Colour::fromString ("#FF23272F"), 
                    juce::Colour::fromString ("#FF121417"), 
                    juce::Colour::fromString ("#FFFFB300"), 
                    juce::Colour::fromString ("#FF9E9E9E"), 
                    juce::Colour::fromString ("#FF303030"), 
                    juce::Colour::fromString ("#FFFFB300"), 
                    juce::Colour::fromString ("#FFFFB300"), 
                    juce::Colour::fromString ("#FF0A1D33"), 
                    juce::Colour::fromString ("#FFFFB300"), 
                    juce::Colour::fromString ("#FFFFB300")  
                };
            case 3: // Matrix Terminal
                return {
                    juce::Colour::fromString ("#FF030B03"), 
                    juce::Colour::fromString ("#FF000000"), 
                    juce::Colour::fromString ("#FF33FF33"), 
                    juce::Colour::fromString ("#FF558855"), 
                    juce::Colour::fromString ("#FF112211"), 
                    juce::Colour::fromString ("#FF33FF33"), 
                    juce::Colour::fromString ("#FF33FF33"), 
                    juce::Colour::fromString ("#FF051505"), 
                    juce::Colour::fromString ("#FF33FF33"), 
                    juce::Colour::fromString ("#FF33FF33")  
                };
            default:
                return {
                    juce::Colour::fromString ("#FFE5E0D8"), 
                    juce::Colour::fromString ("#FFE5E0D8"), 
                    juce::Colour::fromString ("#FF55555C"), 
                    juce::Colour::fromString ("#FF3A3A3D"), 
                    juce::Colour::fromString ("#FFC4BCB4"), 
                    juce::Colour::fromString ("#FFFF5533"), 
                    juce::Colour::fromString ("#FFFF5533"), 
                    juce::Colour::fromString ("#FF1B2838"), 
                    juce::Colour::fromString ("#FFFF5533"), 
                    juce::Colour::fromString ("#FF48929B")  
                };
        }
    }
}