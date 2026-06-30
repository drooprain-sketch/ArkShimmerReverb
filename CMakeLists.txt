cmake_minimum_required(VERSION 3.22)

project(ArkShimmerReverb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options:
#   cmake -B build -DJUCE_PATH=/absolute/path/to/JUCE
#   cmake --build build --config Release
#
# If JUCE_PATH is not provided, CMake will try to fetch JUCE from GitHub.
# For an offline build, download JUCE first and pass -DJUCE_PATH.
if(DEFINED JUCE_PATH)
    message(STATUS "Using JUCE from: ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(ArkShimmerReverb
    COMPANY_NAME "Berrios Audio"
    BUNDLE_ID "com.berriosaudio.arkshimmerreverb"
    PLUGIN_MANUFACTURER_CODE BrAu
    PLUGIN_CODE ArSR
    FORMATS VST3 Standalone
    PRODUCT_NAME "Ark Shimmer Reverb"
    DESCRIPTION "Cinematic worship shimmer reverb with pitch-feedback engine"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(ArkShimmerReverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(ArkShimmerReverb
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ArkShimmerReverb
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
