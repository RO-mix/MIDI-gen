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

    // --- Unified Looper/Live View ---
    double loopDuration = audioProcessor.getLooperDurationInBeats();
    if (loopDuration <= 0) loopDuration = 16.0; // Default to 4 bars if no loop

    // 2. Draw Grid
    const double visualGridResolution = 0.25; // 16th notes
    const int numGridLines = static_cast<int>(loopDuration / visualGridResolution);
    for (int i = 1; i <= numGridLines; ++i)
    {
        const double beat = i * visualGridResolution;
        if (beat >= loopDuration) continue;

        const float x = (float)(beat / loopDuration) * getWidth();

        const bool isBarLine = (fmod(beat, 4.0) < 0.001);
        const bool isBeatLine = (fmod(beat, 1.0) < 0.001);

        if (isBarLine) g.setColour(juce::Colours::grey);
        else if (isBeatLine) g.setColour(juce::Colours::dimgrey);
        else g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));

        g.drawVerticalLine(juce::roundToInt(x), 0.0f, (float)getHeight());
    }

    // 3. Get all notes to be drawn (looper + live)
    auto looperNotes = audioProcessor.getLooperNotes();
    auto liveNotes = audioProcessor.getLiveNotes();

    int minNote = 127, maxNote = 0;
    for (const auto& note : looperNotes) {
        minNote = juce::jmin(minNote, note.message.getNoteNumber());
        maxNote = juce::jmax(maxNote, note.message.getNoteNumber());
    }
    double currentBeat = audioProcessor.getCurrentBeat();
    for (const auto& note : liveNotes) {
        // Only consider live notes that are relevant to the current view
        if (note.startTime >= currentBeat - loopDuration && note.startTime < currentBeat + loopDuration)
        {
            minNote = juce::jmin(minNote, note.noteNumber);
            maxNote = juce::jmax(maxNote, note.noteNumber);
        }
    }
    int noteRange = juce::jmax(24, maxNote - minNote); // Ensure a minimum visible range

    // 4. Draw Looper Notes
    for (const auto& note : looperNotes)
    {
        float x = (float)(fmod(note.beatTime, loopDuration) / loopDuration) * getWidth();
        float w = (float)(note.durationInBeats / loopDuration) * getWidth();
        float noteHeight = (float)getHeight() / (noteRange + 1);
        float y = (1.0f - (float)(note.message.getNoteNumber() - minNote) / noteRange) * getHeight() - noteHeight;

        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect(x + 2, y + 2, w, noteHeight);

        auto brightness = juce::jmap((float)note.message.getVelocity(), 1.0f, 127.0f, 0.6f, 1.0f);
        g.setColour(juce::Colour::fromHSV(0.58f, 0.8f, brightness, 1.0f)); // Blue-ish for looper
        g.fillRect(x, y, w, noteHeight);
    }

    // 5. Draw Live Notes (from generator or MIDI input)
    for (const auto& note : liveNotes)
    {
        double timeInLoop = fmod(note.startTime, loopDuration);
        float x = (float)(timeInLoop / loopDuration) * getWidth();
        float w = (float)(note.duration / loopDuration) * getWidth();

        if (x + w < 0 || x > getWidth()) continue;

        float noteHeight = (float)getHeight() / (noteRange + 1);
        float y = (1.0f - (float)(note.noteNumber - minNote) / noteRange) * getHeight() - noteHeight;

        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect(x + 2, y + 2, w, noteHeight);

        auto brightness = juce::jmap((float)note.velocity, 1.0f, 127.0f, 0.6f, 1.0f);
        g.setColour(juce::Colour::fromHSV(0.08f, 0.9f, brightness, 1.0f)); // Orange-ish for live
        if (audioProcessor.isLooperRecording())
            g.setColour(juce::Colour::fromHSV(0.0f, 0.9f, brightness, 1.0f)); // Red-ish for recording

        g.fillRect(x, y, w, noteHeight);
    }

    // 6. Draw Playhead
    if (audioProcessor.isLooperPlaying() || audioProcessor.isLooperRecording())
    {
        float progress = (float)audioProcessor.getLooperPlaybackProgress();
        g.setColour(audioProcessor.isLooperRecording() ? juce::Colours::red : juce::Colours::white);
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
    // Only repaint if something is happening, to save CPU.
    if (audioProcessor.isPlaying() || audioProcessor.isLooperPlaying() || audioProcessor.isLooperRecording())
    {
        repaint();
    }
}
