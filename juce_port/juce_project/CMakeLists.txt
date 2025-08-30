cmake_minimum_required(VERSION 3.15)
project(CreativeMIDIGenerator VERSION 1.0.0)

# Add JUCE as a subdirectory
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Disable building JUCE extras")
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "Disable building JUCE examples")
add_subdirectory(../../JUCE JUCE)

# Force legacy parameter IDs to resolve VST2/VST3 automation conflict
add_compile_definitions(JUCE_FORCE_USE_LEGACY_PARAM_IDS=1)

# --- Source File Groups ---
set(CORE_SOURCE_FILES
    Source/PluginProcessor.cpp
    Source/Generators/RandomGenerator.cpp
    Source/Generators/EuclideanGenerator.cpp
    Source/Generators/DualEuclideanGenerator.cpp
    Source/Generators/RandomGeneratorV2.cpp
    Source/Generators/Duration.cpp
    Source/Theory/Scales.cpp
    Source/Theory/Duration.cpp
    Source/Looper/Looper.cpp
)

set(UI_SOURCE_FILES
    Source/PluginEditor.cpp
    Source/UI/ToolbarComponent.cpp
    Source/UI/GeneratorSectionComponent.cpp
    Source/UI/LooperSectionComponent.cpp
    Source/UI/RandomGeneratorPanel.cpp
    Source/UI/EuclideanGeneratorPanel.cpp
    Source/UI/DualEuclideanGeneratorPanel.cpp
    Source/UI/RandomGeneratorV2Panel.cpp
    Source/UI/TimelineComponent.cpp
)

set(TEST_RUNNER_SOURCE_FILES
    Source/Tests/HeadlessTestRunner.cpp
    Source/Tests/HeadlessTester.cpp
)

# --- Core Logic Library (Static) ---
add_library(CoreLogic STATIC ${CORE_SOURCE_FILES})

target_include_directories(CoreLogic PUBLIC
    Source
    JuceLibraryCode
)

target_link_libraries(CoreLogic PUBLIC
    juce::juce_audio_processors
    juce::juce_events
    juce::juce_data_structures
    juce::juce_core
    juce::juce_audio_basics
)

target_compile_definitions(CoreLogic PRIVATE
    JUCE_GUI_BASICS_INCLUDE_X11=0
    JUCE_WEB_BROWSER=0
)

# --- Plugin Target ---
juce_add_plugin(CreativeMIDIGenerator
    COMPANY_NAME "CreativeMIDI"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    PLUGIN_MANUFACTURER_CODE "CrMG"
    PLUGIN_CODE "CrMG"
    FORMATS VST3 Standalone
)

target_sources(CreativeMIDIGenerator PRIVATE ${UI_SOURCE_FILES})

target_include_directories(CreativeMIDIGenerator PRIVATE Source)

target_link_libraries(CreativeMIDIGenerator PRIVATE
    CoreLogic
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# --- Headless Test Runner ---
add_executable(HeadlessTestRunner ${TEST_RUNNER_SOURCE_FILES})

target_include_directories(HeadlessTestRunner PRIVATE Source)

target_link_libraries(HeadlessTestRunner PRIVATE
    CoreLogic
)

target_compile_definitions(HeadlessTestRunner PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    IS_HEADLESS_BUILD=1
)
