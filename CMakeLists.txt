cmake_minimum_required(VERSION 3.22)
project(Nineight VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(NINEIGHT_NO_WEBVIEW "Build with a generic fallback editor instead of the WebView UI" OFF)

# JUCE needs a patch for hosts that bundle their own glib/gtk (Ardour):
# the stock parent-process webkit availability probe dlopens against the
# host's libs and fails. Apply once; no-op when already applied.
set(JUCE_WEBVIEW_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/patches/0001-juce-webview-skip-host-availability-check.patch)
execute_process(
    COMMAND git apply --reverse --check ${JUCE_WEBVIEW_PATCH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE
    RESULT_VARIABLE JUCE_PATCH_ALREADY_APPLIED
    ERROR_QUIET OUTPUT_QUIET)
if(NOT JUCE_PATCH_ALREADY_APPLIED EQUAL 0)
    execute_process(
        COMMAND git apply ${JUCE_WEBVIEW_PATCH}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE
        RESULT_VARIABLE JUCE_PATCH_RESULT)
    if(NOT JUCE_PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to apply JUCE webview patch (${JUCE_WEBVIEW_PATCH})")
    endif()
    message(STATUS "Applied JUCE webview host-check patch")
endif()

add_subdirectory(third_party/JUCE)

# ymfm (BSD-3, Aaron Giles) — same sources the web app compiles to WASM
add_library(ymfm STATIC
    third_party/ymfm/src/ymfm_opn.cpp
    third_party/ymfm/src/ymfm_adpcm.cpp
    third_party/ymfm/src/ymfm_ssg.cpp)
target_include_directories(ymfm PUBLIC third_party/ymfm/src)

set(NINEIGHT_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND NINEIGHT_FORMATS AU)
endif()

# WebView2 backend: required on Windows for WebBrowserComponent resource provider.
set(NINEIGHT_WEBVIEW2_ARG "")
if(WIN32)
    set(NINEIGHT_WEBVIEW2_ARG NEEDS_WEBVIEW2 TRUE)
endif()

juce_add_plugin(Nineight
    COMPANY_NAME "NINEIGHT"
    PLUGIN_MANUFACTURER_CODE Nin8
    PLUGIN_CODE Opna
    FORMATS ${NINEIGHT_FORMATS}
    ${NINEIGHT_WEBVIEW2_ARG}
    PRODUCT_NAME "NINEIGHT"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Instrument Synth)

juce_add_binary_data(NineightAssets SOURCES
    assets/ym2608_rhythm_rom.bin
    ui/webui.zip)

target_sources(Nineight PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/bridge/WebViewBridge.cpp)

target_include_directories(Nineight PRIVATE src)

target_compile_definitions(Nineight PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    $<$<BOOL:${NINEIGHT_NO_WEBVIEW}>:NINEIGHT_NO_WEBVIEW=1>)

target_link_libraries(Nineight
    PRIVATE
        NineightAssets
        ymfm
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
