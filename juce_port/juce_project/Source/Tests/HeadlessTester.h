#pragma once
#include <JuceHeader.h>
#include "../Generators/BaseGenerator.h"
#include "../Generators/RandomGenerator.h"
#include "../Theory/Scales.h"
#include "../PluginProcessor.h"
#include <memory>

/**
 * Headless тестовая система для генераторов MIDI паттернов
 * Позволяет тестировать логику без GUI для полной автоматизации
 */
class HeadlessTester
{
public:
    HeadlessTester();
    ~HeadlessTester() = default;

    // Утилиты для тестирования
    juce::StringArray runAllTests();
    void printTestResults(const juce::StringArray& results);

private:
    std::unique_ptr<CreativeMidiGeneratorAudioProcessor> processor;

    // Тестовые утилиты
    juce::String testRandomGeneratorBasic();
    juce::String testRandomGeneratorParameters();
    juce::String testScalesBasic();
    juce::String testScalesIntervals();
    juce::String testEuclideanGeneratorBasic();
    juce::String testEuclideanGeneratorPatterns();
    juce::String testDualEuclideanGeneratorBasic();
    juce::String testDualEuclideanGeneratorPatterns();
    juce::String testLooperBasic();
    juce::String testLooperRecording();
    juce::String testLooperCapture();
    juce::String testLooperAutoRecapture();

    // Вспомогательные методы
    juce::String formatTestResult(const juce::String& testName, bool passed, const juce::String& details = "");

    // Логирование
    juce::StringArray testLog;
    juce::StringArray results;
};