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

        // Draw Grid
        const double gridResolution = 0.25; // 16th notes
        const int numGridLines = static_cast<int>(viewWidthBeats / gridResolution);
        for (int i = 1; i <= numGridLines; ++i)
        {
            const double beat = i * gridResolution;
            const float x = (float)(beat / viewWidthBeats) * getWidth();

            const bool isBarLine = (fmod(beat, 4.0) < 0.001);
            const bool isBeatLine = (fmod(beat, 1.0) < 0.001);

            if (isBarLine)
            {
                g.setColour(juce::Colours::grey);
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
            }
            else if (isBeatLine)
            {
                g.setColour(juce::Colours::dimgrey);
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
            }
            else
            {
                g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
            }
        }

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
        auto* gridParam = audioProcessor.apvts.getRawParameterValue("LOOPER_QUANTIZE_GRID");
        int gridChoice = gridParam ? static_cast<int>(gridParam->load()) : 0;
        double gridResolution = 0.0;
        switch (gridChoice) {
            // "Off", "1/4", "1/8", "1/16", "1/32", "1/64"
            case 1: gridResolution = 1.0; break;
            case 2: gridResolution = 0.5; break;
            case 3: gridResolution = 0.25; break;
            case 4: gridResolution = 0.125; break;
            case 5: gridResolution = 0.0625; break;
            default: break; // Case 0 is "Off"
        }
        double visualGridResolution = (gridResolution > 0) ? gridResolution : 0.5; // Use 8th notes if off

        const int numGridLines = static_cast<int>(loopDuration / visualGridResolution);
        for (int i = 1; i <= numGridLines; ++i)
        {
            const double beat = i * visualGridResolution;
            if (beat >= loopDuration) continue;

            const float x = (float)(beat / loopDuration) * getWidth();

            const bool isBarLine = (fmod(beat, 4.0) < 0.001);
            const bool isBeatLine = (fmod(beat, 1.0) < 0.001);

            if (isBarLine)
            {
                g.setColour(juce::Colours::grey);
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
            }
            else if (isBeatLine)
            {
                g.setColour(juce::Colours::dimgrey);
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
            }
            else
            {
                g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
            }
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
