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
        for (const auto m : buffer) if (m.getMessage().isNoteOn()) noteCount++;
        bool passed = noteCount > 0 && noteCount <= 3;
        return formatTestResult("EuclideanGenerator Patterns", passed, "Generated " + juce::String(noteCount) + " notes for E(3,8)");
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
        generator.process(buffer, processor->apvts, 44100.0, 1.0);
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
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_STEPS_B") = 15.0f;
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_PULSES_B") = 4.0f;
        *processor->apvts.getRawParameterValue("DUAL_EUCLIDEAN_NOTE_PROBABILITY") = 1.0f;
        juce::MidiBuffer buffer;
        for (double beat = 0.0; beat < 4.0; beat += 0.1)
             generator.process(buffer, processor->apvts, 44100.0, beat);
        int noteCount = 0;
        for (const auto m : buffer) if (m.getMessage().isNoteOn()) noteCount++;
        return formatTestResult("DualEuclideanGenerator Patterns", noteCount > 0, "Generated " + juce::String(noteCount) + " notes");
    }
    catch (const std::exception& e) { return formatTestResult("DualEuclideanGenerator Patterns", false, e.what()); }
}


// === ТЕСТЫ SCALES (Largely Unchanged) ===

juce::String HeadlessTester::testScalesBasic()
{
    try
    {
        Scales scales;
        juce::StringArray testScales = {"Major", "Minor", "Dorian", "Mixolydian"};
        for (const auto& scaleName : testScales)
        {
            auto scaleNotes = scales.getScaleNotes(60, scaleName);
            if (scaleNotes.empty())
                return formatTestResult("Scales Basic", false, "Empty scale: " + scaleName);
            for (int note : scaleNotes)
            {
                if (!scales.isNoteInScale(note, scaleName))
                    return formatTestResult("Scales Basic", false, "Note " + juce::String(note) + " not in scale " + scaleName);
            }
        }
        return formatTestResult("Scales Basic", true, "All basic scales validated");
    }
    catch (const std::exception& e) { return formatTestResult("Scales Basic", false, e.what()); }
}

juce::String HeadlessTester::testScalesIntervals()
{
    try
    {
        Scales scales;
        auto majorNotes = scales.getScaleNotes(60, "Major");
        std::vector<int> expected = {60, 62, 64, 65, 67, 69, 71};
        if (majorNotes.size() < expected.size())
            return formatTestResult("Scales Intervals", false, "Not enough notes in Major scale");
        bool intervalsCorrect = true;
        for (size_t i = 0; i < expected.size(); ++i)
        {
            if (majorNotes[i] != expected[i]) {
                intervalsCorrect = false; break;
            }
        }
        return formatTestResult("Scales Intervals", intervalsCorrect, "Major scale intervals correct");
    }
    catch (const std::exception& e) { return formatTestResult("Scales Intervals", false, e.what()); }
}

// === ТЕСТЫ LOOPER (Largely Unchanged) ===

juce::String HeadlessTester::testLooperBasic()
{
    try
    {
        auto looper = std::make_unique<Looper>();
        if (looper->isRecordingActive() || looper->isPlaybackActive())
            return formatTestResult("Looper Basic", false, "Initial state should be inactive");
        if (looper->getRecordedNotesCount() != 0)
            return formatTestResult("Looper Basic", false, "Should start with no recorded notes");
        return formatTestResult("Looper Basic", true, "Basic initialization works");
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
            return formatTestResult("Looper Recording", false, "Recording should be active");
        
        looper->recordNote(juce::MidiMessage::noteOn(1, 60, 0.8f), 0.0);
        looper->recordNote(juce::MidiMessage::noteOff(1, 60), 0.5);
        if (looper->getRecordedNotesCount() != 1) // Should be 1 note after note-off
            return formatTestResult("Looper Recording", false, "Should have 1 recorded note after note-off");

        looper->stopRecording();
        if (looper->isRecordingActive())
            return formatTestResult("Looper Recording", false, "Recording should be stopped");

        return formatTestResult("Looper Recording", true, "Recording functionality works");
    }
    catch (const std::exception& e) { return formatTestResult("Looper Recording", false, e.what()); }
}