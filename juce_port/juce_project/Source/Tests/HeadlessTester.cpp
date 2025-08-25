#include "HeadlessTester.h"

HeadlessTester::HeadlessTester()
{
    // We need an instance of the processor to access the APVTS
    processor = std::make_unique<CreativeMidiGeneratorAudioProcessor>();
}

juce::StringArray HeadlessTester::runAllTests()
{
    results.clear();
    testLog.clear();
    
    // Run all defined tests
    results.add(testRandomGeneratorBasic());
    results.add(testRandomGeneratorParameters());
    results.add(testEuclideanGeneratorBasic());
    results.add(testEuclideanGeneratorPatterns());
    results.add(testDualEuclideanGeneratorBasic());
    results.add(testDualEuclideanGeneratorPatterns());
    results.add(testScalesBasic());
    results.add(testScalesIntervals());
    results.add(testLooperBasic());
    results.add(testLooperRecording());

    return results;
}

void HeadlessTester::printTestResults(const juce::StringArray& testResults)
{
    // Use std::cout for headless environments
    std::cout << "\n=== HEADLESS TEST RESULTS ===\n";
    int passed = 0;
    for (const auto& result : testResults)
    {
        std::cout << result << "\n";
        if (result.contains("PASSED"))
            passed++;
    }
    std::cout << "\nSUMMARY: " << passed << "/" << testResults.size() << " tests passed\n";
    if (passed == testResults.size())
        std::cout << "🎉 All tests passed!\n";
    else
        std::cout << "⚠️  Some tests failed - check logs for details\n";

    // Also log to a file for CI/CD systems
    juce::File logFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("headless_test_results.txt");
    logFile.replaceWithText(testLog.joinIntoString("\n"));
    std::cout << "Full log written to: " << logFile.getFullPathName() << "\n";
}

juce::String HeadlessTester::formatTestResult(const juce::String& testName, bool passed, const juce::String& details)
{
    juce::String result = testName + ": " + (passed ? "PASSED" : "FAILED");
    if (details.isNotEmpty())
        result += " (" + details + ")";
    testLog.add(result);
    return result;
}

// === GENERATOR TESTS (Refactored for new API) ===

juce::String HeadlessTester::testRandomGeneratorBasic()
{
    try
    {
        auto generator = RandomGenerator();
        juce::MidiBuffer buffer;
        // Set a parameter to ensure generation
        *processor->apvts.getRawParameterValue("RANDOM_NOTE_PROBABILITY") = 1.0f;
        generator.process(buffer, processor->apvts, 44100.0, 0.0);
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
            generator.process(buffer, processor->apvts, 44100.0, static_cast<double>(i) * 0.25);
        
        bool allInRange = true;
        for (const auto msg : buffer)
        {
            if (!msg.getMessage().isNoteOn()) continue;
            if (msg.getMessage().getNoteNumber() < 48 || msg.getMessage().getNoteNumber() > 72)
            {
                allInRange = false;
                break;
            }
        }
        return formatTestResult("RandomGenerator Parameters", allInRange, "Notes in range [48, 72]");
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
        generator.process(buffer, processor->apvts, 44100.0, 0.0);
        return formatTestResult("EuclideanGenerator Basic", !buffer.isEmpty(), "Generated MIDI events");
    }
    catch (const std::exception& e) { return formatTestResult("EuclideanGenerator Basic", false, e.what()); }
}

juce::String HeadlessTester::testEuclideanGeneratorPatterns()
{
    try
    {
        auto generator = EuclideanGenerator();
        *processor->apvts.getRawParameterValue("EUCLIDEAN_STEPS") = 8.0f;
        *processor->apvts.getRawParameterValue("EUCLIDEAN_PULSES") = 3.0f;
        *processor->apvts.getRawParameterValue("EUCLIDEAN_NOTE_PROBABILITY") = 1.0f;
        *processor->apvts.getRawParameterValue("EUCLIDEAN_RATE") = 6; // 1/16

        juce::MidiBuffer buffer;
        // Process for a full cycle (8 steps at 1/16 rate = 2 beats)
        for (double beat = 0.0; beat < 2.0; beat += 0.1)
             generator.process(buffer, processor->apvts, 44100.0, beat);

        int noteCount = 0;
        for (const auto msg : buffer) if (msg.getMessage().isNoteOn()) noteCount++;

        return formatTestResult("EuclideanGenerator Patterns", noteCount == 3, "Generated " + juce::String(noteCount) + " notes for E(3,8)");
    }
    catch (const std::exception& e) { return formatTestResult("EuclideanGenerator Patterns", false, e.what()); }
}

juce::String HeadlessTester::testDualEuclideanGeneratorBasic()
{
    try
    {
        auto generator = DualEuclideanGenerator();
        juce::MidiBuffer buffer;
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_PROBABILITY") = 1.0f;
        generator.process(buffer, processor->apvts, 44100.0, 0.0);
        return formatTestResult("DualEuclideanGenerator Basic", !buffer.isEmpty(), "Generated MIDI events");
    }
    catch (const std::exception& e) { return formatTestResult("DualEuclideanGenerator Basic", false, e.what()); }
}

juce::String HeadlessTester::testDualEuclideanGeneratorPatterns()
{
    try
    {
        auto generator = DualEuclideanGenerator();
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_A") = 16.0f;
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_A") = 4.0f;
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_B") = 16.0f;
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_B") = 5.0f;
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_PROBABILITY") = 1.0f;

        juce::MidiBuffer buffer;
        for (double beat = 0.0; beat < 4.0; beat += 0.1)
             generator.process(buffer, processor->apvts, 44100.0, beat);

        int noteCount = 0;
        for (const auto msg : buffer) if (msg.getMessage().isNoteOn()) noteCount++;

        return formatTestResult("DualEuclideanGenerator Patterns", noteCount == 9, "Generated " + juce::String(noteCount) + " notes for E(4,16)+E(5,16)");
    }
    catch (const std::exception& e) { return formatTestResult("DualEuclideanGenerator Patterns", false, e.what()); }
}

// === SCALES TESTS ===

juce::String HeadlessTester::testScalesBasic()
{
    try
    {
        juce::StringArray testScaleNames = Scales::getAvailableScaleNames();
        for (const auto& scaleName : testScaleNames)
        {
            auto scaleNotes = Scales::getScaleNotes(60, scaleName);
            if (scaleNotes.empty())
                return formatTestResult("Scales Basic", false, "Empty scale: " + scaleName);
        }
        return formatTestResult("Scales Basic", true, "All " + juce::String(testScaleNames.size()) + " scales validated");
    }
    catch (const std::exception& e) { return formatTestResult("Scales Basic", false, e.what()); }
}

juce::String HeadlessTester::testScalesIntervals()
{
    try
    {
        auto majorNotes = Scales::getScaleNotes(60, "Major");
        std::vector<int> expected = {60, 62, 64, 65, 67, 69, 71};

        bool intervalsCorrect = majorNotes.size() >= expected.size();
        if (intervalsCorrect) {
            for (size_t i = 0; i < expected.size(); ++i) {
                if (majorNotes[i] != expected[i]) {
                    intervalsCorrect = false;
                    break;
                }
            }
        }
        return formatTestResult("Scales Intervals", intervalsCorrect, "Major scale intervals correct");
    }
    catch (const std::exception& e) { return formatTestResult("Scales Intervals", false, e.what()); }
}

// === LOOPER TESTS ===

juce::String HeadlessTester::testLooperBasic()
{
    try
    {
        auto looper = std::make_unique<Looper>();
        bool initialStateCorrect = !looper->isRecordingActive() && !looper->isPlaybackActive() && looper->getRecordedNotesCount() == 0;
        return formatTestResult("Looper Basic", initialStateCorrect, "Initial state is correct");
    }
    catch (const std::exception& e) { return formatTestResult("Looper Basic", false, e.what()); }
}

juce::String HeadlessTester::testLooperRecording()
{
    try
    {
        auto looper = std::make_unique<Looper>();
        looper->startRecording();
        if (!looper->isRecordingActive())
            return formatTestResult("Looper Recording", false, "Recording should be active after start");
        
        looper->recordNote(juce::MidiMessage::noteOn(1, 60, 0.8f), 0.0);
        looper->recordNote(juce::MidiMessage::noteOff(1, 60), 0.5);
        if (looper->getRecordedNotesCount() != 1)
            return formatTestResult("Looper Recording", false, "Should have 1 note after note-off");

        looper->stopRecording();
        if (looper->isRecordingActive())
            return formatTestResult("Looper Recording", false, "Recording should be stopped");

        return formatTestResult("Looper Recording", true, "Recording functionality works");
    }
    catch (const std::exception& e) { return formatTestResult("Looper Recording", false, e.what()); }
}