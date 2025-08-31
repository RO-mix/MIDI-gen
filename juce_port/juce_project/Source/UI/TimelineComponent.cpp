#include "TimelineComponent.h"

TimelineComponent::TimelineComponent(CreativeMidiGeneratorAudioProcessor& p)
    : audioProcessor(p)
{
    startTimerHz(30); // Refresh rate for playhead animation
}

TimelineComponent::~TimelineComponent()
{
    stopTimer();
}

void TimelineComponent::paint(juce::Graphics& g)
{
    // 1. Setup
    g.fillAll(juce::Colours::black.withAlpha(0.5f));
    const int width = getWidth();
    const int height = getHeight();

    const auto orange = juce::Colour::fromHSV(0.083f, 0.9f, 0.9f, 1.0f);
    const auto red = juce::Colours::red;
    const auto blue = juce::Colour::fromHSV(0.58f, 0.8f, 0.95f, 1.0f);

    const bool isRecording = audioProcessor.isLooperRecording();
    const bool isPlaying = audioProcessor.isLooperPlaying();

    // 2. Determine Mode and Render
    if (!isRecording && !isPlaying)
    {
        // --- LIVE MODE ---
        const float viewWidthBeats = 16.0f; // 4 bars
        const float pixelsPerBeat = (float)width / viewWidthBeats;
        const double currentBeat = audioProcessor.getCurrentBeat();
        const float playheadX = width * 0.25f; // Playhead at 1/4th of the view

        // Draw scrolling grid
        double firstBeat = currentBeat - (playheadX / pixelsPerBeat);
        double lastBeat = firstBeat + viewWidthBeats;
        for (double beat = floor(firstBeat * 4.0)/4.0; beat < lastBeat; beat += 0.25)
        {
            float x = playheadX + (float)(beat - currentBeat) * pixelsPerBeat;
            if (x < 0 || x > width) continue;
            bool isBarLine = fmod(beat, 4.0) < 0.001;
            bool isBeatLine = fmod(beat, 1.0) < 0.001;
            if (isBarLine) g.setColour(juce::Colours::grey);
            else if (isBeatLine) g.setColour(juce::Colours::dimgrey);
            else continue; // Only draw beat/bar lines
            g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)height);
        }

        // Draw notes
        auto liveNotes = audioProcessor.getLiveNotes();
        if (!liveNotes.empty())
        {
            int minNote = 127, maxNote = 0;
            for (const auto& note : liveNotes) {
                minNote = juce::jmin(minNote, note.noteNumber);
                maxNote = juce::jmax(maxNote, note.noteNumber);
            }
            int noteRange = juce::jmax(12, maxNote - minNote);

            for (const auto& note : liveNotes)
            {
                float x = playheadX + (float)(note.startTime - currentBeat) * pixelsPerBeat;
                float w = (float)note.duration * pixelsPerBeat;
                if (x + w < 0 || x > width) continue;

                float noteHeight = (float)height / (noteRange + 1);
                float y = (1.0f - (float)(note.noteNumber - minNote) / noteRange) * height - noteHeight;
                g.setColour(orange.withAlpha(juce::jmap((float)note.velocity, 1.0f, 127.0f, 0.6f, 1.0f)));
                g.fillRect(x, y, w, noteHeight);
            }
        }
        // Draw playhead
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawVerticalLine(juce::roundToInt(playheadX), 0.0f, (float)height);
    }
    else
    {
        // --- LOOPER MODES (PLAYBACK / RECORDING) ---
        const double loopDuration = audioProcessor.getLooperDurationInBeats();
        if (loopDuration <= 0) {
             g.setColour(juce::Colours::darkgrey);
             g.drawRect(getLocalBounds(), 1);
             return;
        }

        // --- Draw Grid ---
        double gridStepBeats = 0.0;
        if (auto* gridParam = audioProcessor.apvts.getRawParameterValue("LOOPER_QUANTIZE_GRID"))
        {
            int choice = static_cast<int>(gridParam->load());
            switch (choice) {
                case 1: gridStepBeats = 1.0; break;    // 1/4 is whole beat
                case 2: gridStepBeats = 0.5; break;    // 1/8
                case 3: gridStepBeats = 0.25; break;   // 1/16
                case 4: gridStepBeats = 0.125; break;  // 1/32
                case 5: gridStepBeats = 0.0625; break; // 1/64
                default: gridStepBeats = 0.0; break;   // Off
            }
        }

        // Always draw at least the bar/beat lines to fix bug with short loops
        const double baseStep = (loopDuration < 1.0) ? 0.25 : 1.0;
        for (double beat = 0; beat < loopDuration; beat += baseStep)
        {
            float x = (float)(beat / loopDuration * width);
            bool isBarLine = fmod(beat, 4.0) < 0.001;
            g.setColour(isBarLine ? juce::Colours::grey : juce::Colours::dimgrey);
            g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)height);
        }

        // Then, draw the quantization grid lines if active
        if (gridStepBeats > 0)
        {
            for (double beat = 0; beat < loopDuration; beat += gridStepBeats)
            {
                // Avoid re-drawing lines that are on the beat
                if (fmod(beat, baseStep) < 0.001) continue;

                float x = (float)(beat / loopDuration * width);
                g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)height);
            }
        }

        // Determine note range from all visible notes
        auto looperNotes = audioProcessor.getLooperNotes();
        auto liveNotes = audioProcessor.getLiveNotes();
        int minNote = 127, maxNote = 0;
        if (!looperNotes.empty() || (isRecording && !liveNotes.empty()))
        {
            for (const auto& note : looperNotes) {
                minNote = juce::jmin(minNote, note.message.getNoteNumber());
                maxNote = juce::jmax(maxNote, note.message.getNoteNumber());
            }
            if (isRecording) {
                double recordingStartTime = audioProcessor.getLooperRecordingStartTime();
                for (const auto& note : liveNotes) {
                    if (note.startTime >= recordingStartTime) {
                        minNote = juce::jmin(minNote, note.noteNumber);
                        maxNote = juce::jmax(maxNote, note.noteNumber);
                    }
                }
            }
        }
        int noteRange = juce::jmax(24, maxNote - minNote);

        // Draw playback notes
        juce::Colour playbackColour = audioProcessor.isLooperCaptureBuffer() ? blue : orange;
        for (const auto& note : looperNotes)
        {
            float x = (float)(note.beatTime / loopDuration * width);
            float w = (float)(note.durationInBeats / loopDuration * width);
            float noteHeight = (float)height / (noteRange + 1);
            float y = (1.0f - (float)(note.message.getNoteNumber() - minNote) / noteRange) * height - noteHeight;
            g.setColour(playbackColour.withAlpha(juce::jmap((float)note.message.getVelocity(), 1.0f, 127.0f, 0.6f, 1.0f)));
            g.fillRect(x, y, w, noteHeight);
        }

        // Draw newly recorded notes (if recording)
        if (isRecording)
        {
            double recordingStartTime = audioProcessor.getLooperRecordingStartTime();
            for (const auto& note : liveNotes)
            {
                if (note.startTime < recordingStartTime) continue;
                // Position relative to recording start, NOT using fmod
                double relativeBeat = note.startTime - recordingStartTime;
                if (relativeBeat >= loopDuration && loopDuration > 0) continue;

                float x = (float)(relativeBeat / loopDuration * width);
                float w = (float)(note.duration / loopDuration * width);
                float noteHeight = (float)height / (noteRange + 1);
                float y = (1.0f - (float)(note.noteNumber - minNote) / noteRange) * height - noteHeight;
                g.setColour(red.withAlpha(juce::jmap((float)note.velocity, 1.0f, 127.0f, 0.6f, 1.0f)));
                g.fillRect(x, y, w, noteHeight);
            }
        }

        // Draw playhead
        float progress = isRecording ? (float)audioProcessor.getLooperRecordProgress() : (float)audioProcessor.getLooperPlaybackProgress();
        g.setColour(isRecording ? red : juce::Colours::white.withAlpha(0.7f));
        g.drawVerticalLine(juce::roundToInt(progress * width), 0.0f, (float)height);
    }

    // 3. Draw border
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

void TimelineComponent::resized()
{
    // Nothing to do here for now, as this component doesn't have children.
}

void TimelineComponent::timerCallback()
{
    // This will trigger a repaint to update the playhead animation.
    repaint();
}
