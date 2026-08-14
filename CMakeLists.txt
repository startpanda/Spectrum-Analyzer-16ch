cmake_minimum_required(VERSION 3.22)

project(SA16SpectrumAnalyzer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../JUCE-8.0.6 ${CMAKE_BINARY_DIR}/JUCE)

juce_add_plugin(SA16Spectrum
    COMPANY_NAME "SA16"
    BUNDLE_ID "com.sa16.SpectrumAnalyzer016"
    PLUGIN_MANUFACTURER_CODE SA16
    PLUGIN_CODE Sp16
    FORMATS VST3 Standalone
    PRODUCT_NAME "SA16 Spectrum Analyzer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "D:/work/audio/vt3/VST3"
    VST3_CATEGORIES "Fx|Analyzer"
    DESCRIPTION "16-Channel Spectrum Analyzer")

juce_generate_juce_header(SA16Spectrum)

target_sources(SA16Spectrum
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/SpectrumEngine.cpp
        Source/UI/NeonKnob.cpp
        Source/UI/ChannelStrip.cpp
        Source/UI/SpectrumDisplay.cpp)

# Custom standalone: show UI before opening audio devices
target_sources(SA16Spectrum_Standalone PRIVATE Source/MainStandalone.cpp)

target_compile_definitions(SA16Spectrum
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_compile_definitions(SA16Spectrum_Standalone
    PRIVATE
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)

target_include_directories(SA16Spectrum
    PRIVATE
        Source)

target_link_libraries(SA16Spectrum
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if (MSVC)
    foreach (_t IN ITEMS SA16Spectrum SA16Spectrum_VST3 SA16Spectrum_Standalone SA16Spectrum_rc_lib)
        if (TARGET ${_t})
            target_compile_options (${_t} PRIVATE /utf-8 /W3)
        endif()
    endforeach()
endif()
