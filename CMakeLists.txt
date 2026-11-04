cmake_minimum_required(VERSION 3.22)
project(SCATTR VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Statically link the MSVC runtime so users don't need the VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Scattr
    COMPANY_NAME "YourName"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Ysnm
    PLUGIN_CODE Sctr
    FORMATS VST3
    VST3_CATEGORIES Instrument Synth
    PRODUCT_NAME "Nature")

# --- embedded assets -------------------------------------------------------------------------
# Textures/*.wav -> looping layers for the WIND knob. Re-run CMake after adding or replacing a file.
file(GLOB SCATTR_TEXTURES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Textures/*.wav"
     "${CMAKE_CURRENT_SOURCE_DIR}/Textures/*.aif"
     "${CMAKE_CURRENT_SOURCE_DIR}/Textures/*.flac")
if(NOT SCATTR_TEXTURES)
    message(FATAL_ERROR "No wind sample found. Expected Textures/wind.wav in ${CMAKE_CURRENT_SOURCE_DIR}/Textures")
endif()

juce_add_binary_data(ScattrTextures
    HEADER_NAME "TexturesData.h"
    NAMESPACE TexturesData
    SOURCES ${SCATTR_TEXTURES})

target_sources(Scattr PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Scattr PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Scattr
    PRIVATE
        ScattrTextures
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
