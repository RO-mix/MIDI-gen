#include <JuceHeader.h>
#include "HeadlessTester.h"

// Консольное приложение для запуска headless тестов
class HeadlessTestRunnerApplication : public juce::JUCEApplication
{
public:
    HeadlessTestRunnerApplication() = default;

    const juce::String getApplicationName() override    { return "HeadlessTestRunner"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise(const juce::String& commandLine) override
    {
        std::cout << "🎵 Creative MIDI Generator - Headless Test Runner" << std::endl;
        std::cout << "==================================================" << std::endl;

        // Создаем тестер
        HeadlessTester tester;

        // Запускаем все тесты
        juce::StringArray results = tester.runAllTests();

        // Выводим результаты
        tester.printTestResults(results);

        // Выходим с кодом возврата
        int passedCount = 0;
        for (const auto& result : results)
        {
            if (result.contains("PASSED"))
                passedCount++;
        }

        int exitCode = (passedCount == results.size()) ? 0 : 1;
        systemRequestedQuit();
        setApplicationReturnValue(exitCode);
    }

    void shutdown() override
    {
        std::cout << "\nShutting down test runner..." << std::endl;
    }

    void systemRequestedQuit() override
    {
        quit();
    }
};

// Точка входа для standalone приложения
START_JUCE_APPLICATION(HeadlessTestRunnerApplication)