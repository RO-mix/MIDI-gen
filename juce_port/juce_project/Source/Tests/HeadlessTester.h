#pragma once
#include <JuceHeader.h>
#include "../Generators/BaseGenerator.h"
#include "../Generators/RandomGenerator.h"
#include "../Theory/Scales.h"

/**
 * Headless тестовая система для генераторов MIDI паттернов
 * Позволяет тестировать логику без GUI для полной автоматизации
 */
class HeadlessTester
{
public:
    HeadlessTester();
    ~HeadlessTester() = default;

    // Методы тестирования генераторов
    bool testRandomGenerator();
    bool testScales();
    bool testMidiOutput();

    // Утилиты для тестирования
    juce::StringArray runAllTests();
    void printTestResults(const juce::StringArray& results);

    // Статистические тесты
    bool testRandomDistribution(int testSize = 1000);
    bool testScaleFiltering();
    bool testParameterRanges();

private:
    // Тестовые утилиты
    juce::String testRandomGeneratorBasic();
    juce::String testRandomGeneratorScales();
    juce::String testRandomGeneratorParameters();
    juce::String testScalesBasic();
    juce::String testScalesIntervals();
    juce::String testEuclideanGeneratorBasic();
    juce::String testEuclideanGeneratorPatterns();
    juce::String testDualEuclideanGeneratorBasic();
    juce::String testDualEuclideanGeneratorPatterns();
    juce::String testLooperBasic();
    juce::String testLooperRecording();
    juce::String testLooperPlayback();
    juce::String testLooperLoopPoints();
    juce::String testLooperModes();
    juce::String testLooperEffects();
    juce::String testLooperEdgeCases();
    juce::String testLooperComplexPatterns();
    juce::String testLooperIntegration();
    juce::String testLooperPerformance();

    // Вспомогательные методы
    std::vector<int> collectGeneratedNotes(std::unique_ptr<BaseGenerator> generator, int numBeats);
    bool isNoteInScale(int note, const juce::String& scaleName);
    juce::String formatTestResult(const juce::String& testName, bool passed, const juce::String& details = "");

    // Логирование
    juce::StringArray testLog;
    juce::StringArray results;
};