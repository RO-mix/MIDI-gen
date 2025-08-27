#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class TestLooperFeatures : public juce::UnitTest
{
public:
    TestLooperFeatures() : juce::UnitTest("Looper Features Test") {}

    void runTest() override
    {
        beginTest("Capture Test");
        {
            CreativeMidiGeneratorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 512);

            // Before capture, looper should be empty
            expect(processor.getLooperNotes().empty(), "Looper should be empty initially");

            // Perform capture
            processor.captureFromGenerator();

            // After capture, looper should have notes
            expect(!processor.getLooperNotes().empty(), "Looper should have notes after capture");
            expect(processor.getLooperDurationInBeats() > 0, "Looper should have a duration after capture");
        }

        beginTest("Auto-Recapture Test");
        {
            CreativeMidiGeneratorAudioProcessor processor;
            processor.prepareToPlay(44100.0, 512);

            // 1. Set recapture period to "Every 2 loops" (index 1 of 6 items -> normalized value 0.2)
            auto* recapParam = processor.apvts.getParameter("LOOPER_RECAPTURE_PERIOD");
            recapParam->setValueNotifyingHost(1.0f / 5.0f);

            // 2. Perform initial capture and start playing
            processor.captureFromGenerator();
            processor.toggleLooperPlay(); // Start playback

            const auto initialNotes = processor.getLooperNotes();
            expect(!initialNotes.empty(), "Initial capture should not be empty");
            const double loopDuration = processor.getLooperDurationInBeats();
            expect(loopDuration > 0, "Initial loop duration should be positive");

            // 3. Simulate time passing for more than 2 loop cycles
            const double beatsToSimulate = loopDuration * 2.5;
            const double samplesPerBeat = 44100.0 * 60.0 / 120.0;
            const int totalSamplesToSimulate = static_cast<int>(beatsToSimulate * samplesPerBeat);
            const int blockSize = 512;

            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::MidiBuffer midi;

            for (int i = 0; i < totalSamplesToSimulate; i += blockSize)
            {
                processor.processBlock(buffer, midi);
            }

            // 4. Check if the loop has been recaptured
            const auto finalNotes = processor.getLooperNotes();
            expect(!finalNotes.empty(), "Final notes should not be empty");

            bool contentIsDifferent = initialNotes.size() != finalNotes.size();
            if (!contentIsDifferent && !initialNotes.empty() && !finalNotes.empty())
            {
                contentIsDifferent = initialNotes[0].message.getNoteNumber() != finalNotes[0].message.getNoteNumber();
            }

            expect(contentIsDifferent, "Loop content should have changed after auto-recapture");
        }
    }
};

// This static instance will be automatically registered with the UnitTestRunner
static TestLooperFeatures testLooperFeatures;
