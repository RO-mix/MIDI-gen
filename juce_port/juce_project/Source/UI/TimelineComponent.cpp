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
    // 1. Draw Background
    g.fillAll(juce::Colours::black.withAlpha(0.5f));

    // Define Colors
    const auto orange = juce::Colour::fromHSV(0.083f, 0.9f, 0.9f, 1.0f);
    const auto red = juce::Colours::red;
    const auto blue = juce::Colour::fromHSV(0.58f, 0.8f, 0.95f, 1.0f);

    // Determine current mode and note colors
    enum class TimelineMode { Live, Playback, Recording };
    TimelineMode mode = TimelineMode::Live;
    juce::Colour noteColour = orange;

    if (audioProcessor.isLooperRecording())
    {
        mode = TimelineMode::Recording;
    }
    else if (audioProcessor.isLooperPlaying())
    {
        mode = TimelineMode::Playback;
        if (audioProcessor.isLooperCaptureBuffer())
        {
            noteColour = blue;
        }
    }

    if (mode == TimelineMode::Live)
    {
        // --- Live Generator View ---
        const float viewWidthBeats = 16.0f; // Show 4 bars
        const double gridResolution = 0.25; // 16th notes
        double currentBeat = audioProcessor.getCurrentBeat();
        double startBeat = currentBeat - fmod(currentBeat, gridResolution);

        for (double beat = startBeat; beat < currentBeat + viewWidthBeats + gridResolution; beat += gridResolution)
        {
            float x = getWidth() * (float)((beat - currentBeat) / viewWidthBeats);
            if (x < 0 || x > getWidth()) continue;

            const bool isBarLine = (fmod(beat, 4.0) < 0.001);
            const bool isBeatLine = (fmod(beat, 1.0) < 0.001);

            if (isBarLine) g.setColour(juce::Colours::grey);
            else if (isBeatLine) g.setColour(juce::Colours::dimgrey);
            else g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));

            g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
        }

        // Draw Live Notes
        const auto& notes = audioProcessor.getLiveNotes();
        const float nowX = getWidth() * 0.5f;
        const float pixelsPerBeat = getWidth() / viewWidthBeats;

        if (!notes.empty())
        {
            int minNote = 127, maxNote = 0;
            for (const auto& note : notes)
            {
                minNote = juce::jmin(minNote, note.noteNumber);
                maxNote = juce::jmax(maxNote, note.noteNumber);
            }
            int noteRange = juce::jmax(12, maxNote - minNote);

            for (const auto& note : notes)
            {
                float x = nowX + (float)(note.startTime - audioProcessor.getCurrentBeat()) * pixelsPerBeat;
                float w = (float)note.duration * pixelsPerBeat;
                if (x + w < 0 || x > getWidth()) continue;

                float noteHeight = (float)getHeight() / (noteRange + 1);
                float y = (1.0f - (float)(note.noteNumber - minNote) / noteRange) * getHeight() - noteHeight;

                auto brightness = juce::jmap((float)note.velocity, 1.0f, 127.0f, 0.6f, 1.0f);
                g.setColour(orange.withAlpha(brightness));
                g.fillRect(x, y, w, noteHeight);
                g.setColour(orange.darker(0.2f));
                g.drawRect(x, y, w, noteHeight, 0.5f);
            }
        }

        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawVerticalLine(juce::roundToInt(nowX), 0.0f, (float)getHeight());
        g.drawRect(getLocalBounds(), 1);
        return;
    }

    // --- Draw Grid for Looper Modes ---
    double loopDuration = audioProcessor.getLooperDurationInBeats();
    if (loopDuration <= 0) loopDuration = 4.0; // Default if no loop

    const int numBarLines = static_cast<int>(loopDuration / 4.0);
    for (int i = 1; i <= numBarLines; ++i)
    {
        const float x = (float)(i * 4.0 / loopDuration) * getWidth();
        g.setColour(juce::Colours::grey);
        g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
    }
    const int numBeatLines = static_cast<int>(loopDuration);
     for (int i = 1; i <= numBeatLines; ++i)
    {
        if (fmod((double)i, 4.0) < 0.001) continue; // Skip bar lines
        const float x = (float)(i / loopDuration) * getWidth();
        g.setColour(juce::Colours::dimgrey);
        g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
    }


    // --- Draw Notes ---
    auto& looperNotes = audioProcessor.getLooperNotes();
    if (looperNotes.empty() && mode != TimelineMode::Recording)
    {
        g.drawRect(getLocalBounds(), 1); // Draw border and return
        return;
    }

    int minNote = 127, maxNote = 0;
    for (const auto& note : looperNotes) {
        minNote = juce::jmin(minNote, note.message.getNoteNumber());
        maxNote = juce::jmax(maxNote, note.message.getNoteNumber());
    }
    // Also consider live notes for range if recording
    if (mode == TimelineMode::Recording) {
        for (const auto& note : audioProcessor.getLiveNotes()) {
             if (note.startTime >= audioProcessor.getLooperRecordingStartTime()) {
                minNote = juce::jmin(minNote, note.noteNumber);
                maxNote = juce::jmax(maxNote, note.noteNumber);
             }
        }
    }

    int noteRange = juce::jmax(24, maxNote - minNote);
    if (noteRange == 0) noteRange = 24;

    // Draw existing Looper Notes
    for (const auto& note : looperNotes)
    {
        float x = (float)(note.beatTime / loopDuration) * getWidth();
        float w = (float)(note.durationInBeats / loopDuration) * getWidth();
        float noteHeight = (float)getHeight() / (noteRange + 1);
        float y = (1.0f - (float)(note.message.getNoteNumber() - minNote) / noteRange) * getHeight() - noteHeight;

        auto brightness = juce::jmap((float)note.message.getVelocity(), 1.0f, 127.0f, 0.6f, 1.0f);
        g.setColour(noteColour.withAlpha(brightness));
        g.fillRect(x, y, w, noteHeight);
        g.setColour(noteColour.darker(0.2f));
        g.drawRect(x, y, w, noteHeight, 0.5f);
    }

    // Draw newly recorded notes on top
    if (mode == TimelineMode::Recording)
    {
        auto& liveNotes = audioProcessor.getLiveNotes();
        double recordingStartTime = audioProcessor.getLooperRecordingStartTime();

        for (const auto& note : liveNotes)
        {
            if (note.startTime < recordingStartTime) continue;

            double timeInLoop = note.startTime - recordingStartTime;
            if (timeInLoop >= loopDuration) continue;

            float x = (float)(timeInLoop / loopDuration) * getWidth();
            float w = (float)(note.duration / loopDuration) * getWidth();
            float noteHeight = (float)getHeight() / (noteRange + 1);
            float y = (1.0f - (float)(note.noteNumber - minNote) / noteRange) * getHeight() - noteHeight;

            auto brightness = juce::jmap((float)note.velocity, 1.0f, 127.0f, 0.6f, 1.0f);
            g.setColour(red.withAlpha(brightness));
            g.fillRect(x, y, w, noteHeight);
            g.setColour(red.darker(0.2f));
            g.drawRect(x, y, w, noteHeight, 0.5f);
        }
    }

    // --- Draw Playhead ---
    if (mode == TimelineMode::Recording)
    {
        float progress = (float)audioProcessor.getLooperRecordProgress();
        g.setColour(juce::Colours::red.withAlpha(0.8f));
        g.drawVerticalLine(juce::roundToInt(progress * getWidth()), 0.0f, (float)getHeight());
    }
    else if (mode == TimelineMode::Playback)
    {
        float progress = (float)audioProcessor.getLooperPlaybackProgress();
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawVerticalLine(juce::roundToInt(progress * getWidth()), 0.0f, (float)getHeight());
    }

    // Draw border last to be on top
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
