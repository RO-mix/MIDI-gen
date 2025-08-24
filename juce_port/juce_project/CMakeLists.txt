cmake_minimum_required(VERSION 3.16)

project(CreativeMIDIGenerator VERSION 1.0.0)

# Установка JUCE
set(JUCE_PATH "D:/PROG/JUCE" CACHE PATH "Path to JUCE")
set(CMAKE_PREFIX_PATH "${JUCE_PATH}/cmake")

# Находим JUCE
find_package(JUCE REQUIRED)

# Настройки JUCE
set(JUCE_MODULES
    juce_audio_basics
    juce_audio_devices
    juce_audio_formats
    juce_audio_plugin_client
    juce_audio_processors
    juce_audio_utils
    juce_core
    juce_data_structures
    juce_events
    juce_graphics
    juce_gui_basics
    juce_gui_extra
)

# Создаем плагин
juce_add_plugin(CreativeMIDIGenerator
    PRODUCT_NAME "Creative MIDI Generator"
    DESCRIPTION "MIDI Pattern Generator with Euclidean rhythms"
    PLUGIN_MANUFACTURER_CODE "CrMG"
    PLUGIN_CODE "CrMG"
    FORMATS VST3
    VST3_CATEGORIES "Instrument"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Добавляем исходные файлы
target_sources(CreativeMIDIGenerator PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Generators/BaseGenerator.cpp
    Source/Generators/RandomGenerator.cpp
    Source/Theory/Scales.cpp
    Source/Sequencer/Sequencer.cpp
    Source/Looper/Looper.cpp
)

# Добавляем заголовочные файлы
target_include_directories(CreativeMIDIGenerator PRIVATE
    Source
    Source/Generators
    Source/Theory
    Source/Sequencer
    Source/Looper
)

# Подключаем модули JUCE
target_link_libraries(CreativeMIDIGenerator PRIVATE ${JUCE_MODULES})

# Настройки компиляции
target_compile_definitions(CreativeMIDIGenerator PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Настройки для Visual Studio
if(MSVC)
    target_compile_options(CreativeMIDIGenerator PRIVATE /W4)
endif()

# Установка
set_target_properties(CreativeMIDIGenerator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)