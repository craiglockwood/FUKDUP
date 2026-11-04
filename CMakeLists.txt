cmake_minimum_required(VERSION 3.22)
project(FUKDUP VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(FUKDUP_FORMATS VST3 AU Standalone)
    set(FUKDUP_COPY TRUE)
else()
    set(FUKDUP_FORMATS VST3 Standalone)
    set(FUKDUP_COPY FALSE)
endif()

juce_add_plugin(FUKDUP
    COMPANY_NAME "FUKDUP"
    BUNDLE_ID com.fukdup.fukdup
    PLUGIN_MANUFACTURER_CODE Fkdp
    PLUGIN_CODE Fkd1
    FORMATS ${FUKDUP_FORMATS}
    PRODUCT_NAME "FUKDUP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${FUKDUP_COPY}
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(FUKDUP PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(FUKDUP PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(FUKDUP
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
