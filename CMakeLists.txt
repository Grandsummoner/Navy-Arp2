cmake_minimum_required(VERSION 3.15)

project(NavyArp VERSION 1.0.0 LANGUAGES C CXX)

# 1. Include your local JUCE directory
add_subdirectory(JUCE)

# 2. Add and define the Navy Arp plugin targets
juce_add_plugin(NavyArp
    COMPANY_NAME "Navy Audio"
    BUNDLE_ID "com.navyaudio.navyarp"
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "Dgav"
    FORMATS VST3 Standalone CLAP
    PRODUCT_NAME "Navy Arp"
    PLUGIN_NAME "Navy Arp"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE                  # Corrected argument name to enable MIDI In
    NEEDS_MIDI_OUTPUT TRUE                 # Corrected argument name to enable MIDI Out
    COPY_PLUGIN_AFTER_BUILD FALSE          # Disable default multi-folder copy step
)

# 3. Mandatory generator macro call to create JuceHeader.h for your target
juce_generate_juce_header(NavyArp)

# 4. Custom post-build step to copy ONLY the single flat VST3 file to your custom folder
add_custom_command(TARGET NavyArp_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "$<TARGET_FILE:NavyArp_VST3>"
    "D:/Music - Software/VST/Navy Arp.vst3"
)

# 5. Source Files mapping
target_sources(NavyArp PRIVATE
    source/AppTheme.h
    source/ChromaCapsLookAndFeel.cpp
    source/ChromaCapsLookAndFeel.h
    source/OledDisplay.cpp
    source/OledDisplay.h
    source/PluginEditor.cpp
    source/PluginEditor.h
    source/PluginProcessor.cpp
    source/PluginProcessor.h
)

# 6. Global compiler definitions
target_compile_definitions(NavyArp
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# 7. Library dependencies
target_link_libraries(NavyArp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_events
        juce::juce_data_structures
        juce::juce_core
)
