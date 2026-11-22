cmake_minimum_required(VERSION 3.22)
project(RoboTune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RoboTune
    COMPANY_NAME "RoboTune"
    PLUGIN_MANUFACTURER_CODE Rbtn
    PLUGIN_CODE Rtun
    FORMATS VST3 Standalone
    PRODUCT_NAME "RoboTune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx "Pitch Shift"
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(RoboTune PRIVATE
    Source/AutoTuneEngine.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(RoboTune PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(RoboTune
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lib_flags
            juce::juce_recommended_warning_flags)

juce_generate_juce_header(RoboTune)
