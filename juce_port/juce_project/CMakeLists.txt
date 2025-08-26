cmake_minimum_required(VERSION 3.15)
project(CreativeMIDIGenerator VERSION 1.0.0)

# Add JUCE as a subdirectory
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Disable building JUCE extras")
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "Disable building JUCE examples")
add_subdirectory(../../JUCE JUCE)

# Force legacy parameter IDs to resolve VST2/VST3 automation conflict
add_compile_definitions(JUCE_FORCE_USE_LEGACY_PARAM_IDS=1)

# --- Core Logic Source Files ---
set(CORE_SOURCE_FILES
    Source/Generators/RandomGenerator.cpp
    Source/Generators/EuclideanGenerator.cpp
    Source/Generators/DualEuclideanGenerator.cpp
    Source/Generators/RandomGeneratorV2.cpp
    Source/Generators/Duration.cpp
    Source/Theory/Scales.cpp
    Source/Looper/Looper.cpp
)

# --- UI Component Source Files ---
set(UI_SOURCE_FILES
    Source/UI/ToolbarComponent.cpp
    Source/UI/GeneratorSectionComponent.cpp
    Source/UI/LooperSectionComponent.cpp
    Source/UI/RandomGeneratorPanel.cpp
    Source/UI/EuclideanGeneratorPanel.cpp
    Source/UI/DualEuclideanGeneratorPanel.cpp
    Source/UI/RandomGeneratorV2Panel.cpp
    Source/UI/TimelineComponent.cpp
)

# --- Plugin Target ---
set(PLUGIN_SOURCE_FILES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${CORE_SOURCE_FILES}
    ${UI_SOURCE_FILES}
)

# Add the plugin target
juce_add_plugin(CreativeMIDIGenerator
    COMPANY_NAME "CreativeMIDI"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    PLUGIN_MANUFACTURER_CODE "CrMG"
    PLUGIN_CODE "CrMG"
    FORMATS VST3 Standalone
)

target_sources(CreativeMIDIGenerator PRIVATE ${PLUGIN_SOURCE_FILES})

# Add include directories for the plugin
target_include_directories(CreativeMIDIGenerator PRIVATE
    Source
    Source/Generators
    Source/Theory
    Source/Looper
    Source/UI
    Source/Sequencer
    JuceLibraryCode
)

# juce_set_post_build_step(CreativeMIDIGenerator)

# Link the plugin to required JUCE modules
target_link_libraries(CreativeMIDIGenerator PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_events
    juce::juce_data_structures
    juce::juce_core
)

# --- Headless Test Runner ---
# Source files for the test runner
set(TEST_RUNNER_SOURCE_FILES
    Source/Tests/HeadlessTestRunner.cpp
    Source/Tests/HeadlessTester.cpp
    ${CORE_SOURCE_FILES}
)

# Add the executable for the test runner
add_executable(HeadlessTestRunner ${TEST_RUNNER_SOURCE_FILES})

# Link the test runner to required JUCE modules
target_link_libraries(HeadlessTestRunner PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_audio_basics
)

# Add include directories for the test runner
target_include_directories(HeadlessTestRunner PRIVATE
    Source
    Source/Generators
    Source/Theory
    Source/Tests
    JuceLibraryCode
)

# Set preprocessor definitions for the test runner
target_compile_definitions(HeadlessTestRunner PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
)
