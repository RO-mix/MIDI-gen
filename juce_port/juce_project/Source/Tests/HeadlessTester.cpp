#include "HeadlessTester.h"
#include "../PluginProcessor.h"
#include "../Generators/BaseGenerator.h"
#include "../Generators/RandomGenerator.h"
#include "../Generators/EuclideanGenerator.h"
#include "../Generators/DualEuclideanGenerator.h"
#include "../Theory/Scales.h"
#include "../Looper/Looper.h"
#include <climits>

HeadlessTester::HeadlessTester()
{
    processor = std::make_unique<CreativeMidiGeneratorAudioProcessor>();
}

juce::StringArray HeadlessTester::runAllTests()
{
    results.clear();

    // Generator Tests
    results.add(testRandomGeneratorBasic());
    results.add(testRandomGeneratorParameters());
    results.add(testEuclideanGeneratorBasic());
    results.add(testEuclideanGeneratorPatterns());
    results.add(testDualEuclideanGeneratorBasic());
    results.add(testDualEuclideanGeneratorPatterns());

    // Scales and Looper tests remain, as they don't depend on the changed generator API
    results.add(testScalesBasic());
    results.add(testScalesIntervals());
    results.add(testLooperBasic());
    results.add(testLooperRecording());

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

juce::String HeadlessTester::formatTestResult(const juce::String& testName, bool passed, const juce::String& details)
{
    juce::String result = testName + ": " + (passed ? "PASSED" : "FAILED");
    if (!details.isEmpty())
        result += " (" + details + ")";
    testLog.add(result);
    return result;
}

// === REFACTORED GENERATOR TESTS ===

juce::String HeadlessTester::testRandomGeneratorBasic()
{
    try
    {
        auto generator = RandomGenerator();
        juce::MidiBuffer buffer;
        *processor->apvts.getRawParameterValue("RANDOM_NOTE_PROBABILITY") = 1.0f;
        generator.process(buffer, processor->apvts, 44100.0, 1.0);
        return formatTestResult("RandomGenerator Basic", !buffer.isEmpty(), "Generated MIDI events");
    }
    catch (const std::exception& e) { return formatTestResult("RandomGenerator Basic", false, e.what()); }
}

juce::String HeadlessTester::testRandomGeneratorParameters()
{
    try
    {
        auto generator = RandomGenerator();
        *processor->apvts.getRawParameterValue("RANDOM_MIN_NOTE") = 48.0f;
        *processor->apvts.getRawParameterValue("RANDOM_MAX_NOTE") = 72.0f;
        *processor->apvts.getRawParameterValue("RANDOM_NOTE_PROBABILITY") = 1.0f;
        juce::MidiBuffer buffer;
        for (int i = 0; i < 50; ++i)
            generator.process(buffer, processor->apvts, 44100.0, static_cast<double>(i));

        bool allInRange = true;
        for (const auto m : buffer) {
            if (!m.getMessage().isNoteOn()) continue;
            if (m.getMessage().getNoteNumber() < 48 || m.getMessage().getNoteNumber() > 72) {
                allInRange = false; break;
            }
        }
        return formatTestResult("RandomGenerator Parameters", allInRange, "Notes in range");
    }
    catch (const std::exception& e) { return formatTestResult("RandomGenerator Parameters", false, e.what()); }
}

juce::String HeadlessTester::testEuclideanGeneratorBasic()
{
    try
    {
        auto generator = EuclideanGenerator();
        juce::MidiBuffer buffer;
        *processor->apvts.getRawParameterValue("EUCLIDEAN_NOTE_PROBABILITY") = 1.0f;
        generator.process(buffer, processor->apvts, 44100.0, 1.0);
        return formatTestResult("EuclideanGenerator Basic", !buffer.isEmpty(), "Generated MIDI events");
    }
    catch (const std::exception& e) { return formatTestResult("EuclideanGenerator Basic", false, e.what()); }
}

juce::String HeadlessTester::testEuclideanGeneratorPatterns()
{
    try
    {
        auto generator = std::make_unique<EuclideanGenerator>();

        // Тестируем различные евклидовы паттерны
        struct TestPattern {
            int steps;
            int pulses;
            int expectedNotes;
        };

        std::vector<TestPattern> patterns = {
            {8, 3, 3},   // 3 notes in 8 steps
            {16, 4, 4},  // 4 notes in 16 steps
            {12, 5, 5},  // 5 notes in 12 steps
        };

        for (const auto& pattern : patterns)
        {
            generator->setParameter("steps", static_cast<float>(pattern.steps));
            generator->setParameter("pulses", static_cast<float>(pattern.pulses));
            generator->setParameter("noteProbability", 1.0f); // 100% вероятность

            int generatedNotes = 0;

            // Генерируем полный цикл
            for (int step = 0; step < pattern.steps; ++step)
            {
                auto [events, duration] = generator->generate(static_cast<double>(step));
                for (const auto& [msg, dur] : events)
                {
                    if (msg.isNoteOn())
                    {
                        generatedNotes++;
                    }
                }
            }

            if (generatedNotes != pattern.expectedNotes)
            {
                return formatTestResult("EuclideanGenerator Patterns", false,
                    "Pattern " + juce::String(pattern.steps) + "," + juce::String(pattern.pulses) +
                    " generated " + juce::String(generatedNotes) + " notes, expected " +
                    juce::String(pattern.expectedNotes));
            }
        }

        return formatTestResult("EuclideanGenerator Patterns", true, "All patterns generated correctly");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("EuclideanGenerator Patterns", false, e.what());
    }
}

// === ТЕСТЫ DUAL EUCLIDEAN GENERATOR ===

juce::String HeadlessTester::testDualEuclideanGeneratorBasic()
{
    try
    {
        auto generator = std::make_unique<DualEuclideanGenerator>();

        // Тестируем базовую генерацию
        auto [events, duration] = generator->generate(0.0);

        if (events.empty())
        {
            return formatTestResult("DualEuclideanGenerator Basic", false, "No events generated");
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

        return formatTestResult("DualEuclideanGenerator Basic", hasNoteOn, "Generated MIDI events");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("DualEuclideanGenerator Basic", false, e.what());
    }
}

juce::String HeadlessTester::testDualEuclideanGeneratorPatterns()
{
    try
    {
        auto generator = std::make_unique<DualEuclideanGenerator>();

        // Тестируем различные комбинации паттернов для двух машин
        generator->setParameter("stepsA", 16.0f);
        generator->setParameter("pulsesA", 4.0f);
        generator->setParameter("stepsB", 15.0f);
        generator->setParameter("pulsesB", 4.0f);
        generator->setParameter("noteProbability", 1.0f); // 100% вероятность

        // Генерируем несколько циклов
        int totalNotesGenerated = 0;
        int cycles = 3;

        for (int cycle = 0; cycle < cycles; ++cycle)
        {
            // Используем максимальное количество шагов из двух машин
            for (int step = 0; step < 16; ++step)
            {
                auto [events, duration] = generator->generate(static_cast<double>(step));
                for (const auto& [msg, dur] : events)
                {
                    if (msg.isNoteOn())
                    {
                        totalNotesGenerated++;
                    }
                }
            }
        }

        // Проверяем, что были сгенерированы ноты (общее количество может варьироваться)
        bool notesGenerated = totalNotesGenerated > 0;

        return formatTestResult("DualEuclideanGenerator Patterns", notesGenerated,
            "Generated " + juce::String(totalNotesGenerated) + " notes across " +
            juce::String(cycles) + " cycles");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("DualEuclideanGenerator Patterns", false, e.what());
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

// === ТЕСТЫ LOOPER ===

juce::String HeadlessTester::testLooperBasic()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Проверяем начальное состояние
        if (looper->isRecordingActive() || looper->isPlaybackActive())
        {
            return formatTestResult("Looper Basic", false, "Initial state should be inactive");
        }

        if (looper->getRecordedNotesCount() != 0)
        {
            return formatTestResult("Looper Basic", false, "Should start with no recorded notes");
        }

        return formatTestResult("Looper Basic", true, "Basic initialization works");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Basic", false, e.what());
    }
}

juce::String HeadlessTester::testLooperRecording()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Начинаем запись
        looper->startRecording();

        if (!looper->isRecordingActive())
        {
            return formatTestResult("Looper Recording", false, "Recording should be active");
        }

        // Записываем несколько нот
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, 60);

        looper->recordNote(noteOn, 0.0);
        looper->recordNote(noteOff, 0.5);

        if (looper->getRecordedNotesCount() != 2)
        {
            return formatTestResult("Looper Recording", false,
                "Should have 2 recorded notes, got " + juce::String(looper->getRecordedNotesCount()));
        }

        // Останавливаем запись
        looper->stopRecording();

        if (looper->isRecordingActive())
        {
            return formatTestResult("Looper Recording", false, "Recording should be stopped");
        }

        return formatTestResult("Looper Recording", true, "Recording functionality works");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Recording", false, e.what());
    }
}

juce::String HeadlessTester::testLooperPlayback()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Записываем паттерн
        looper->startRecording();
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
        looper->recordNote(noteOn, 0.0);
        looper->stopRecording();

        // Начинаем воспроизведение
        looper->startPlayback();

        if (!looper->isPlaybackActive())
        {
            return formatTestResult("Looper Playback", false, "Playback should be active");
        }

        // Получаем буфер воспроизведения
        juce::MidiBuffer buffer = looper->getPlaybackBuffer(44100, 0.0, 1.0);

        if (buffer.isEmpty())
        {
            return formatTestResult("Looper Playback", false, "Playback buffer should not be empty");
        }

        // Останавливаем воспроизведение
        looper->stopPlayback();

        if (looper->isPlaybackActive())
        {
            return formatTestResult("Looper Playback", false, "Playback should be stopped");
        }

        return formatTestResult("Looper Playback", true, "Playback functionality works");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Playback", false, e.what());
    }
}

juce::String HeadlessTester::testLooperLoopPoints()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Устанавливаем точки лупа
        looper->setLoopPoints(0.0, 4.0);

        if (looper->getLoopStart() != 0.0 || looper->getLoopEnd() != 4.0)
        {
            return formatTestResult("Looper Loop Points", false, "Loop points not set correctly");
        }

        // Записываем ноту в начале лупа
        looper->startRecording();
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
        looper->recordNote(noteOn, 0.0);
        looper->stopRecording();

        // Начинаем воспроизведение
        looper->startPlayback();

        // Проверяем воспроизведение в начале лупа
        juce::MidiBuffer buffer = looper->getPlaybackBuffer(44100, 0.0, 0.1);

        if (buffer.isEmpty())
        {
            return formatTestResult("Looper Loop Points", false, "Should play note at loop start");
        }

        // Проверяем воспроизведение в конце лупа (должно быть пустым если нота только в начале)
        juce::MidiBuffer buffer2 = looper->getPlaybackBuffer(44100, 3.9, 4.1);

        // Очищаем лупер
        looper->clear();

        if (looper->getRecordedNotesCount() != 0)
        {
            return formatTestResult("Looper Loop Points", false, "Clear should remove all notes");
        }

        return formatTestResult("Looper Loop Points", true, "Loop points functionality works");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Loop Points", false, e.what());
    }
}

juce::String HeadlessTester::testLooperModes()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Проверяем переключение режимов
        looper->setMode(LooperMode::MidiLooper);
        if (looper->getMode() != LooperMode::MidiLooper)
        {
            return formatTestResult("Looper Modes", false, "Failed to set MIDI Looper mode");
        }

        looper->setMode(LooperMode::GenerationLooper);
        if (looper->getMode() != LooperMode::GenerationLooper)
        {
            return formatTestResult("Looper Modes", false, "Failed to set Generation Looper mode");
        }

        // Проверяем что данные очищаются при смене режима
        looper->setMode(LooperMode::MidiLooper);
        looper->startRecording();
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
        looper->recordNote(noteOn, 0.0);
        looper->stopRecording();

        if (looper->getRecordedNotesCount() == 0)
        {
            return formatTestResult("Looper Modes", false, "No notes recorded in MIDI mode");
        }

        // Меняем режим и проверяем что данные очистились
        looper->setMode(LooperMode::GenerationLooper);
        if (looper->getRecordedNotesCount() != 0)
        {
            return formatTestResult("Looper Modes", false, "Data not cleared when switching modes");
        }

        return formatTestResult("Looper Modes", true, "Mode switching works correctly");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Modes", false, e.what());
    }
}

juce::String HeadlessTester::testLooperEffects()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Устанавливаем параметры эффектов
        looper->setPitchShift(2); // +2 полутонов
        looper->setPlaybackSpeed(1.5f); // Ускорение
        looper->setReverse(true); // Реверс

        // Проверяем геттеры
        if (looper->getPitchShift() != 2)
        {
            return formatTestResult("Looper Effects", false, "Pitch shift not set correctly");
        }

        if (looper->getPlaybackSpeed() != 1.5f)
        {
            return formatTestResult("Looper Effects", false, "Playback speed not set correctly");
        }

        if (!looper->getReverse())
        {
            return formatTestResult("Looper Effects", false, "Reverse not set correctly");
        }

        // Тестируем с генерацией буфера
        juce::MidiBuffer testBuffer;
        testBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        testBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), 22050); // 0.5 сек при 44100 Hz

        looper->setMode(LooperMode::GenerationLooper);
        looper->startRecording();
        looper->recordMidiBuffer(testBuffer, 0.0);
        looper->stopRecording();

        // Проверяем воспроизведение с эффектами
        looper->startPlayback();
        juce::MidiBuffer playbackBuffer = looper->getPlaybackBuffer(44100, 0.0, 1.0);

        if (playbackBuffer.isEmpty())
        {
            return formatTestResult("Looper Effects", false, "Playback buffer with effects is empty");
        }

        return formatTestResult("Looper Effects", true, "Effects functionality works");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Effects", false, e.what());
    }
}

juce::String HeadlessTester::testLooperEdgeCases()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Тест 1: Пустой буфер
        auto emptyBuffer = looper->getPlaybackBuffer(44100, 0.0, 1.0);
        if (!emptyBuffer.isEmpty())
        {
            return formatTestResult("Looper Edge Cases", false, "Empty buffer should return empty playback");
        }

        // Тест 2: Нулевая длительность
        looper->setLoopPoints(0.0, 0.0);
        auto zeroDurationBuffer = looper->getPlaybackBuffer(44100, 0.0, 1.0);
        if (!zeroDurationBuffer.isEmpty())
        {
            return formatTestResult("Looper Edge Cases", false, "Zero duration should return empty buffer");
        }

        // Тест 3: Экстремальные значения эффектов
        looper->setPitchShift(12); // Максимум
        looper->setPlaybackSpeed(0.25f); // Минимум

        // Тест 4: Многократное переключение режимов
        for (int i = 0; i < 10; ++i)
        {
            looper->setMode(LooperMode::MidiLooper);
            looper->setMode(LooperMode::GenerationLooper);
        }

        // Тест 5: Запись без запуска воспроизведения
        looper->startRecording();
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
        looper->recordNote(noteOn, 0.0);
        looper->stopRecording();

        if (looper->isPlaybackActive())
        {
            return formatTestResult("Looper Edge Cases", false, "Recording should not auto-start playback");
        }

        return formatTestResult("Looper Edge Cases", true, "All edge cases handled correctly");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Edge Cases", false, e.what());
    }
}

juce::String HeadlessTester::testLooperComplexPatterns()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Тест 1: Сложный полиритмический паттерн
        looper->startRecording();

        // Создаем сложный паттерн с разными длительностями и каналами
        struct NoteEvent {
            double time;
            int note;
            int channel;
            bool isNoteOn;
        };

        std::vector<NoteEvent> complexPattern = {
            {0.0, 60, 1, true},   // C4
            {0.25, 64, 2, true},  // E4
            {0.5, 67, 1, true},   // G4
            {0.75, 72, 2, true},  // C5
            {1.0, 60, 1, false},  // Note off
            {1.25, 64, 2, false},
            {1.5, 67, 1, false},
            {1.75, 72, 2, false},
            {2.0, 60, 1, true},   // Повтор
            {2.5, 64, 2, true},
            {3.0, 67, 1, true},
            {3.5, 72, 2, true}
        };

        for (const auto& event : complexPattern)
        {
            juce::MidiMessage msg = event.isNoteOn ?
                juce::MidiMessage::noteOn(event.channel, event.note, 0.8f) :
                juce::MidiMessage::noteOff(event.channel, event.note);
            looper->recordNote(msg, event.time);
        }

        looper->stopRecording();

        if (looper->getRecordedNotesCount() != complexPattern.size())
        {
            return formatTestResult("Looper Complex Patterns", false,
                "Complex pattern not recorded correctly. Expected: " +
                juce::String(complexPattern.size()) + ", Got: " +
                juce::String(looper->getRecordedNotesCount()));
        }

        // Тест 2: Воспроизведение с эффектами
        looper->setPitchShift(7); // Пентатоника вверх
        looper->setPlaybackSpeed(2.0f); // Двойная скорость
        looper->startPlayback();

        auto playbackBuffer = looper->getPlaybackBuffer(44100, 0.0, 2.0);
        if (playbackBuffer.isEmpty())
        {
            return formatTestResult("Looper Complex Patterns", false, "Complex pattern playback failed");
        }

        return formatTestResult("Looper Complex Patterns", true, "Complex patterns handled correctly");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Complex Patterns", false, e.what());
    }
}

juce::String HeadlessTester::testLooperIntegration()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Тест 1: MIDI Looper режим - интеграция с генераторами
        looper->setMode(LooperMode::MidiLooper);
        looper->startRecording();

        // Симулируем генерацию нот как от реального генератора
        for (int i = 0; i < 16; ++i) // 16 шагов
        {
            double beatTime = static_cast<double>(i) * 0.25; // 16th notes
            int note = 60 + (i % 7); // Простая мелодия

            juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, note, 0.8f);
            juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, note);

            looper->recordNote(noteOn, beatTime);
            looper->recordNote(noteOff, beatTime + 0.2); // Короткие ноты
        }

        looper->stopRecording();

        // Тест 2: Generation Looper режим - работа с буфером
        looper->setMode(LooperMode::GenerationLooper);

        juce::MidiBuffer genBuffer;
        genBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        genBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), 22050);

        looper->startRecording();
        looper->recordMidiBuffer(genBuffer, 0.0);
        looper->stopRecording();

        if (looper->getRecordedNotesCount() == 0)
        {
            return formatTestResult("Looper Integration", false, "Generation mode recording failed");
        }

        // Тест 3: Переключение между режимами во время работы
        looper->startPlayback();
        bool wasPlaying = looper->isPlaybackActive();

        looper->setMode(LooperMode::MidiLooper);
        bool stillPlaying = looper->isPlaybackActive();

        if (wasPlaying != stillPlaying)
        {
            return formatTestResult("Looper Integration", false, "Mode switch should preserve playback state");
        }

        return formatTestResult("Looper Integration", true, "Integration scenarios work correctly");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Integration", false, e.what());
    }
}

juce::String HeadlessTester::testLooperPerformance()
{
    try
    {
        auto looper = std::make_unique<Looper>();

        // Тест 1: Производительность с большим количеством нот
        looper->startRecording();
        const int noteCount = 1000;

        for (int i = 0; i < noteCount; ++i)
        {
            double time = static_cast<double>(i) * 0.01; // 10ms intervals
            int note = 60 + (i % 24); // 2 октавы
            juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, note, 0.8f);
            looper->recordNote(noteOn, time);
        }

        looper->stopRecording();

        if (looper->getRecordedNotesCount() != noteCount)
        {
            return formatTestResult("Looper Performance", false,
                "Performance test failed. Expected: " + juce::String(noteCount) +
                ", Got: " + juce::String(looper->getRecordedNotesCount()));
        }

        // Тест 2: Быстрое воспроизведение буфера
        looper->startPlayback();
        auto startTime = juce::Time::getMillisecondCounter();

        for (int i = 0; i < 100; ++i)
        {
            auto buffer = looper->getPlaybackBuffer(44100, 0.0, 1.0);
            if (buffer.isEmpty())
            {
                return formatTestResult("Looper Performance", false, "Performance buffer was empty");
            }
        }

        auto endTime = juce::Time::getMillisecondCounter();
        auto duration = endTime - startTime;

        // Должен выполняться менее чем за 100мс для 100 итераций
        if (duration > 100)
        {
            return formatTestResult("Looper Performance", false,
                "Performance too slow: " + juce::String(duration) + "ms");
        }

        // Тест 3: Память - проверка отсутствия утечек
        looper->clear();
        if (looper->getRecordedNotesCount() != 0)
        {
            return formatTestResult("Looper Performance", false, "Memory cleanup failed");
        }

        return formatTestResult("Looper Performance", true,
            "Performance test passed. Processed " + juce::String(noteCount) +
            " notes in " + juce::String(duration) + "ms");
    }
    catch (const std::exception& e)
    {
        return formatTestResult("Looper Performance", false, e.what());
    }
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

    // Сохраняем результат в лог
    testLog.add(result);

    return result;
}