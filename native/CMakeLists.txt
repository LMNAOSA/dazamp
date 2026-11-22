cmake_minimum_required(VERSION 3.22)
project(DAZAMP VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use an existing JUCE installation, or download JUCE when configuring on a Mac.
find_package(JUCE CONFIG QUIET)
if (NOT JUCE_FOUND)
  include(FetchContent)
  FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(DAZAMP
  COMPANY_NAME "DAZAMP Audio"
  BUNDLE_ID "audio.dazamp.plugin"
  PLUGIN_MANUFACTURER_CODE DazA
  PLUGIN_CODE Da01
  FORMATS AU VST3 Standalone
  PRODUCT_NAME "DAZAMP"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(DAZAMP)
target_sources(DAZAMP PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h)
target_compile_definitions(DAZAMP PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(DAZAMP PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)
