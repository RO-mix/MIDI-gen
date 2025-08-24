#include "HeadlessTester.h"

HeadlessTester::HeadlessTester()
{
    // Инициализация логгера
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("CreativeMIDIGenerator")
                        .getChildFile("headless_test.log");
    logger = std::make_unique<juce::FileLogger>(logFile, "Headless Test Log");
}

juce::StringArray HeadlessTester::runAllTests()
{
    juce::StringArray results;

    results.add(testRandomGeneratorBasic());
    results.add(testRandomGeneratorScales());
    results.add(testRandomGeneratorParameters());
    results.add(testScalesBasic());
    results.add(testScalesIntervals());

    // Дополнительные тесты
    if (testRandomDistribution())
        results.add("Random Distribution Test: PASSED");
    else
        results.add("Random Distribution Test: FAILED");

    if (testScaleFiltering())
        results.add("Scale Filtering Test: PASSED");
    else
        results.add("Scale Filtering Test: FAILED");

    if (testParameterRanges())
        results.add("Parameter Ranges Test: PASSED");
    else
        results.add("Parameter Ranges Test: FAILED");

    return results;
}

void HeadlessTester::printTestResults(const juce::StringArray& results)
{
    std::cout << "\n=== HEADLESS TEST RESULTS ===" << std::endl;

    int passed = 0;
    int total = results.size();

    for (const auto& result : results)
    {
        std::cout << result << std::endl;
        if (result.contains("PASSED"))
            passed++;
    }

    std::cout << "\nSUMMARY: " << passed << "/" << total << " tests passed" << std::endl;

    if (passed == total)
        std::cout << "🎉 All tests passed!" << std::endl;
    else
        std::cout << "⚠️  Some tests failed - check logs for details" << std::endl;
}

// === ТЕСТЫ RANDOM GENERATOR ===

juce::String HeadlessTester::testRandomGeneratorBasic()
{
    try
    {
        auto generator = std::make_unique<RandomGenerator>();

        // Тестируем базовую генерацию
        auto [events, duration] = generator->generate(0.0);

        if (events.empty())
        {
            return formatTestResult("RandomGenerator Basic", false, "No events generated");
        }

        // Проверяем, что сгенерированы MIDI события
        bool hasNoteOn = false;
        for (const auto& [msg, dur] : events)
        {
            if (msg.isNoteOn())
            {
                hasNoteOn = true;
                break;
            }
        }

        return formatTestResult("RandomGenerator Basic", hasNoteOn, "Generated MIDI events");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("RandomGenerator Basic", false, e.what());
    }
}

juce::String HeadlessTester::testRandomGeneratorScales()
{
    try
    {
        auto generator = std::make_unique<RandomGenerator>();
        Scales scales;

        // Тестируем с различными ладами
        juce::StringArray scaleNames = scales.getAvailableScaleNames();

        for (const auto& scaleName : scaleNames)
        {
            std::vector<int> scaleNotes = scales.getScaleNotes(60, scaleName); // C4
            generator->setScaleNotes(scaleNotes);

            auto [events, duration] = generator->generate(0.0);

            // Проверяем, что все ноты в ладу
            for (const auto& [msg, dur] : events)
            {
                if (msg.isNoteOn())
                {
                    int note = msg.getNoteNumber();
                    if (!scales.isNoteInScale(note, scaleName))
                    {
                        return formatTestResult("RandomGenerator Scales", false,
                            "Note " + juce::String(note) + " not in scale " + scaleName);
                    }
                }
            }
        }

        return formatTestResult("RandomGenerator Scales", true, "All scales tested");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("RandomGenerator Scales", false, e.what());
    }
}

juce::String HeadlessTester::testRandomGeneratorParameters()
{
    try
    {
        auto generator = std::make_unique<RandomGenerator>();

        // Тестируем параметры
        generator->setParameter("minNote", 48.0f);  // C3
        generator->setParameter("maxNote", 72.0f);  // C5
        generator->setParameter("noteProbability", 1.0f); // Всегда генерировать

        // Генерируем несколько нот
        std::vector<int> generatedNotes;
        for (int i = 0; i < 50; ++i)
        {
            auto [events, duration] = generator->generate(static_cast<double>(i));
            for (const auto& [msg, dur] : events)
            {
                if (msg.isNoteOn())
                {
                    generatedNotes.push_back(msg.getNoteNumber());
                }
            }
        }

        // Проверяем диапазон
        bool allInRange = true;
        for (int note : generatedNotes)
        {
            if (note < 48 || note > 72)
            {
                allInRange = false;
                break;
            }
        }

        return formatTestResult("RandomGenerator Parameters", allInRange,
            "Generated " + juce::String(generatedNotes.size()) + " notes in range");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("RandomGenerator Parameters", false, e.what());
    }
}

// === ТЕСТЫ SCALES ===

juce::String HeadlessTester::testScalesBasic()
{
    try
    {
        Scales scales;

        // Тестируем основные лады
        juce::StringArray testScales = {"Major", "Minor", "Dorian", "Mixolydian"};

        for (const auto& scaleName : testScales)
        {
            auto scaleNotes = scales.getScaleNotes(60, scaleName); // C4

            if (scaleNotes.empty())
            {
                return formatTestResult("Scales Basic", false, "Empty scale: " + scaleName);
            }

            // Проверяем, что все ноты в правильном диапазоне
            for (int note : scaleNotes)
            {
                if (!scales.isNoteInScale(note, scaleName))
                {
                    return formatTestResult("Scales Basic", false,
                        "Note " + juce::String(note) + " not in scale " + scaleName);
                }
            }
        }

        return formatTestResult("Scales Basic", true, "All basic scales validated");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Scales Basic", false, e.what());
    }
}

juce::String HeadlessTester::testScalesIntervals()
{
    try
    {
        Scales scales;

        // Тестируем интервалы для мажорного лада
        auto majorNotes = scales.getScaleNotes(60, "Major");

        // Ожидаемые интервалы для C Major: C(60), D(62), E(64), F(65), G(67), A(69), B(71)
        std::vector<int> expected = {60, 62, 64, 65, 67, 69, 71};

        if (majorNotes.size() < expected.size())
        {
            return formatTestResult("Scales Intervals", false, "Not enough notes in Major scale");
        }

        // Проверяем первые ноты
        bool intervalsCorrect = true;
        for (size_t i = 0; i < expected.size(); ++i)
        {
            if (majorNotes[i] != expected[i])
            {
                intervalsCorrect = false;
                break;
            }
        }

        return formatTestResult("Scales Intervals", intervalsCorrect, "Major scale intervals correct");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Scales Intervals", false, e.what());
    }
}

// === СТАТИСТИЧЕСКИЕ ТЕСТЫ ===

bool HeadlessTester::testRandomDistribution(int testSize)
{
    auto generator = std::make_unique<RandomGenerator>();
    generator->setParameter("noteProbability", 1.0f); // Всегда генерировать

    std::map<int, int> noteCounts;

    // Собираем статистику
    for (int i = 0; i < testSize; ++i)
    {
        auto [events, duration] = generator->generate(static_cast<double>(i));
        for (const auto& [msg, dur] : events)
        {
            if (msg.isNoteOn())
            {
                noteCounts[msg.getNoteNumber()]++;
            }
        }
    }

    // Проверяем распределение (должно быть примерно равномерным)
    if (noteCounts.empty())
        return false;

    int minCount = INT_MAX;
    int maxCount = 0;

    for (const auto& pair : noteCounts)
    {
        minCount = std::min(minCount, pair.second);
        maxCount = std::max(maxCount, pair.second);
    }

    // Разница между min и max не должна превышать 20%
    float ratio = static_cast<float>(maxCount) / minCount;
    return ratio < 1.2f;
}

bool HeadlessTester::testScaleFiltering()
{
    Scales scales;
    auto generator = std::make_unique<RandomGenerator>();

    // Тестируем фильтрацию по ладу
    auto chromaticNotes = scales.getScaleNotes(60, "Chromatic"); // Все ноты
    auto majorNotes = scales.getScaleNotes(60, "Major");         // Только мажор

    generator->setScaleNotes(majorNotes);

    // Генерируем много нот
    for (int i = 0; i < 100; ++i)
    {
        auto [events, duration] = generator->generate(static_cast<double>(i));
        for (const auto& [msg, dur] : events)
        {
            if (msg.isNoteOn())
            {
                int note = msg.getNoteNumber();
                if (!scales.isNoteInScale(note, "Major"))
                {
                    return false; // Нота не в ладу!
                }
            }
        }
    }

    return true;
}

bool HeadlessTester::testParameterRanges()
{
    auto generator = std::make_unique<RandomGenerator>();

    // Тестируем граничные значения
    generator->setParameter("minNote", 0.0f);
    generator->setParameter("maxNote", 127.0f);
    generator->setParameter("noteProbability", 0.0f);

    // При 0% вероятности не должно быть нот
    auto [events, duration] = generator->generate(0.0);
    if (!events.empty())
        return false;

    // Тестируем 100% вероятность
    generator->setParameter("noteProbability", 1.0f);
    auto [events2, duration2] = generator->generate(0.0);
    if (events2.empty())
        return false;

    return true;
}

// === ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ===

std::vector<int> HeadlessTester::collectGeneratedNotes(std::unique_ptr<BaseGenerator> generator, int numBeats)
{
    std::vector<int> notes;

    for (int beat = 0; beat < numBeats; ++beat)
    {
        auto [events, duration] = generator->generate(static_cast<double>(beat));
        for (const auto& [msg, dur] : events)
        {
            if (msg.isNoteOn())
            {
                notes.push_back(msg.getNoteNumber());
            }
        }
    }

    return notes;
}

bool HeadlessTester::isNoteInScale(int note, const juce::String& scaleName)
{
    Scales scales;
    return scales.isNoteInScale(note, scaleName);
}

juce::String HeadlessTester::formatTestResult(const juce::String& testName, bool passed, const juce::String& details)
{
    juce::String result = testName + ": " + (passed ? "PASSED" : "FAILED");
    if (!details.isEmpty())
        result += " (" + details + ")";

    // Логируем результат
    if (logger)
        logger->logMessage(result);

    return result;
}