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
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);

    // Determine current mode
    enum class TimelineMode { Live, Playback, Recording };
    TimelineMode mode = TimelineMode::Live;
    if (audioProcessor.isLooperRecording())
        mode = TimelineMode::Recording;
    else if (audioProcessor.isLooperPlaying())
        mode = TimelineMode::Playback;

    // 2. Draw content based on mode
    if (mode == TimelineMode::Live)
    {
        // --- Live Generator View ---
        const float viewWidthBeats = 16.0f; // Show 4 bars
        const auto& notes = audioProcessor.getLiveNotes();

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
                float x = getWidth() * (float)((note.startTime - audioProcessor.getCurrentBeat() + viewWidthBeats) / viewWidthBeats);
                float w = getWidth() * (float)(note.duration / viewWidthBeats);
                if (x + w < 0 || x > getWidth()) continue;

                float noteHeight = (float)getHeight() / (noteRange + 1);
                float y = (1.0f - (float)(note.noteNumber - minNote) / noteRange) * getHeight() - noteHeight;

                // Shadow
                g.setColour(juce::Colours::black.withAlpha(0.4f));
                g.fillRect(x + 2, y + 2, w, noteHeight);

                // Note
                auto brightness = juce::jmap((float)note.velocity, 1.0f, 127.0f, 0.6f, 1.0f);
                g.setColour(juce::Colour::fromHSV(0.08f, 0.9f, brightness, 1.0f));
                g.fillRect(x, y, w, noteHeight);
            }
        }
        // Draw "now" cursor at the end
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawVerticalLine(getWidth() - 2, 0.0f, (float)getHeight());
    }
    else
    {
        // --- Looper Playback/Recording View ---
        double loopDuration = audioProcessor.getLooperDurationInBeats();
        if (loopDuration <= 0) loopDuration = 4.0; // Default if no loop

        // Draw Grid
        const int numBars = static_cast<int>(std::ceil(loopDuration / 4.0));
        const float barWidth = getWidth() / (float)loopDuration * 4.0f;
        g.setColour(juce::Colours::dimgrey);
        for (int i = 1; i < numBars; ++i)
        {
            g.drawVerticalLine(i * barWidth, 0.0f, getHeight());
        }

        // Draw Notes
        auto& notes = audioProcessor.getLooperNotes();
        if (!notes.empty())
        {
            int minNote = 127, maxNote = 0;
            for (const auto& note : notes) {
                minNote = juce::jmin(minNote, note.message.getNoteNumber());
                maxNote = juce::jmax(maxNote, note.message.getNoteNumber());
            }
            int noteRange = juce::jmax(12, maxNote - minNote);

            for (const auto& note : notes)
            {
                float x = (float)(note.beatTime / loopDuration) * getWidth();
                float w = (float)(note.durationInBeats / loopDuration) * getWidth();
                float noteHeight = (float)getHeight() / (noteRange + 1);
                float y = (1.0f - (float)(note.message.getNoteNumber() - minNote) / noteRange) * getHeight() - noteHeight;

                // Shadow
                g.setColour(juce::Colours::black.withAlpha(0.4f));
                g.fillRect(x + 2, y + 2, w, noteHeight);

                // Note
                auto brightness = juce::jmap((float)note.message.getVelocity(), 1.0f, 127.0f, 0.6f, 1.0f);
                g.setColour(juce::Colour::fromHSV(0.08f, 0.9f, brightness, 1.0f));
                g.fillRect(x, y, w, noteHeight);
            }
        }

        // Draw Playhead
        float progress = (float)audioProcessor.getLooperPlaybackProgress();
        g.setColour(mode == TimelineMode::Recording ? juce::Colours::red : juce::Colours::white);
        g.setOpacity(0.7f);
        g.drawVerticalLine(progress * getWidth(), 0.0f, (float)getHeight());
    }
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
