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
    double loopDuration = 4.0; // TODO: Get this from the looper
    if (!notes.empty())
    {
        int minNote = 127, maxNote = 0;
        for (const auto& note : notes) {
            if (note.message.isNoteOn())
            {
                minNote = juce::jmin(minNote, note.message.getNoteNumber());
                maxNote = juce::jmax(maxNote, note.message.getNoteNumber());
            }
        }
        int noteRange = juce::jmax(12, maxNote - minNote);

        for (const auto& note : notes)
        {
            if (!note.message.isNoteOn()) continue;

            float x = (float)(std::fmod(note.beatTime, loopDuration) / loopDuration) * getWidth();
            float w = (float)(note.durationInBeats / loopDuration) * getWidth();
            float noteHeight = (float)getHeight() / (noteRange + 1);
            float y = (1.0f - (float)(note.message.getNoteNumber() - minNote) / noteRange) * getHeight() - noteHeight;

            g.setColour(juce::Colour::fromHSV(note.message.getVelocity() / 127.0f, 0.9f, 0.9f, 1.0f));
            g.fillRect(x, y, w, noteHeight);
            g.setColour(juce::Colours::black);
            g.drawRect(x, y, w, noteHeight, 0.5f);
        }
    }

    // 4. Draw Playhead
    float progress = (float)audioProcessor.getLooperPlaybackProgress();
    g.setColour(juce::Colours::white.withAlpha(0.7f));
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
