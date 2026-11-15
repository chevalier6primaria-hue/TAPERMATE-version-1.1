cmake_minimum_required(VERSION 3.22)
project(TAPERMATE VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Mismo codigo de plugin que la v1.0: FL Studio lo reconoce como el mismo plugin (actualizacion)
juce_add_plugin(TAPERMATE
    COMPANY_NAME "TapeLab"
    PLUGIN_MANUFACTURER_CODE Tlab
    PLUGIN_CODE Tpmt
    FORMATS VST3 Standalone
    PRODUCT_NAME "TAPERMATE"
    VERSION 1.1.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(TAPERMATE)

target_sources(TAPERMATE PRIVATE
    Source/Profile.cpp
    Source/TapeMachine.cpp
    Source/IntroOverlay.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(TAPERMATE PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_compile_options(TAPERMATE PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(TAPERMATE
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
