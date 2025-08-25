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

    // 2. Draw Grid
    const int numBars = 4; // Example: 4 bars
    const int beatsPerBar = 4;
    const float barWidth = getWidth() / (float)numBars;
    const float beatWidth = barWidth / (float)beatsPerBar;

    g.setColour(juce::Colours::dimgrey);
    for (int i = 1; i < numBars; ++i)
    {
        g.drawVerticalLine(i * barWidth, 0.0f, getHeight());
    }

    g.setColour(juce::Colours::darkslategrey);
    for (int i = 1; i < numBars * beatsPerBar; ++i)
    {
        if (i % beatsPerBar != 0)
        {
            g.drawVerticalLine(i * beatWidth, 0.0f, getHeight());
        }
    }

    // 3. Draw Notes
    auto& notes = audioProcessor.getLooperNotes();
    double loopDuration = 4.0; // Assume 4 beats for now
    if (!notes.empty())
    {
        // Simple auto-zoom
        int minNote = 127, maxNote = 0;
        for (const auto& note : notes)
        {
            if (note.message.getNoteNumber() < minNote) minNote = note.message.getNoteNumber();
            if (note.message.getNoteNumber() > maxNote) maxNote = note.message.getNoteNumber();
        }
        int noteRange = juce::jmax(12, maxNote - minNote);

        for (const auto& note : notes)
        {
            float x = (float)(note.beatTime / loopDuration) * getWidth();
            float w = (float)(0.25 / loopDuration) * getWidth(); // Assume 1/16th note duration for now
            float y = 1.0f - (float)(note.message.getNoteNumber() - minNote) / noteRange;
            y *= getHeight();

            g.setColour(juce::Colour::fromHSV(note.message.getVelocity() / 127.0f, 0.8f, 0.9f, 1.0f));
            g.fillRect(x, y, w, 10.0f);
        }
    }


    // 4. Draw Playhead
    float progress = (float)audioProcessor.getLooperPlaybackProgress();
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawVerticalLine(progress * getWidth(), 0.0f, (float)getHeight());
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
