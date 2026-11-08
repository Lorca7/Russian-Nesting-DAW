cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple silicon + Intel), runs on macOS 11 and later including Sonoma 14.6.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

project(MatryoshkaFugue VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MF_BUILD_TESTS "Build the engine unit tests" ON)

# ---- JUCE (downloaded automatically; point FETCHCONTENT_SOURCE_DIR_JUCE at a local checkout to build offline)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(MF_FORMATS Standalone AU VST3)
else()
    set(MF_FORMATS Standalone VST3)
endif()

juce_add_plugin(MatryoshkaFugue
    PRODUCT_NAME "Matryoshka Fugue"
    COMPANY_NAME "Russian Nesting DAW"
    BUNDLE_ID com.russiannestingdaw.matryoshkafugue
    PLUGIN_MANUFACTURER_CODE RnDw
    PLUGIN_CODE MtFg
    FORMATS ${MF_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MICROPHONE_PERMISSION_ENABLED FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth Generator
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png")

set(MF_ENGINE_SOURCES
    Source/engine/Scales.cpp
    Source/engine/FractalFugue.cpp
    Source/engine/Presets.cpp
    Source/engine/Project.cpp)

set(MF_APP_SOURCES
    ${MF_ENGINE_SOURCES}
    Source/dsp/BuiltInSamples.cpp
    Source/dsp/Voices.cpp
    Source/dsp/FugueEngine.cpp
    Source/io/ProjectIO.cpp
    Source/io/Exporters.cpp
    Source/ui/LookAndFeel.cpp
    Source/ui/DollShelf.cpp
    Source/ui/NestingRoll.cpp
    Source/ui/SubjectEditor.cpp
    Source/ui/SampleBench.cpp
    Source/ui/Panels.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(MatryoshkaFugue PRIVATE ${MF_APP_SOURCES})

target_compile_definitions(MatryoshkaFugue PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MatryoshkaFugue
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MF_BUILD_TESTS)
    enable_testing()
    add_executable(EngineTests tests/EngineTests.cpp ${MF_ENGINE_SOURCES})
    add_test(NAME EngineTests COMMAND EngineTests)

    # Renders audio and MIDI through the real DSP engine and exporters, without a window.
    juce_add_console_app(RenderTests PRODUCT_NAME "RenderTests")
    target_sources(RenderTests PRIVATE
        tests/RenderTests.cpp
        ${MF_ENGINE_SOURCES}
        Source/dsp/BuiltInSamples.cpp
        Source/dsp/Voices.cpp
        Source/dsp/FugueEngine.cpp
        Source/io/ProjectIO.cpp
        Source/io/Exporters.cpp)
    target_compile_definitions(RenderTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(RenderTests PRIVATE juce::juce_audio_formats juce::juce_audio_basics juce::juce_core
                                      PUBLIC juce::juce_recommended_config_flags)
    add_test(NAME RenderTests COMMAND RenderTests)
endif()

option(MF_BUILD_SNAPSHOT "Build the offscreen UI snapshot tool (CI)" OFF)
if(MF_BUILD_SNAPSHOT)
    juce_add_console_app(UISnapshot PRODUCT_NAME "UISnapshot")
    target_sources(UISnapshot PRIVATE tests/UISnapshot.cpp ${MF_APP_SOURCES})
    target_compile_definitions(UISnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(UISnapshot PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
                                     PUBLIC juce::juce_recommended_config_flags)
endif()
