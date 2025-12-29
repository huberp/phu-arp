add_library(phu-arp MODULE
    PluginProcessor.cpp
    PluginEditor.cpp
)

target_include_directories(phu-arp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(phu-arp PRIVATE juce::juce_audio_utils juce::juce_events juce::juce_data_structures)

# MIDI-only plugin
set_target_properties(phu-arp PROPERTIES
    SUFFIX ".vst3"
    CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON
)