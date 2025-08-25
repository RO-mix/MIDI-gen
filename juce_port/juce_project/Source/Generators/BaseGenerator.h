#pragma once
#include <JuceHeader.h>

class BaseGenerator
{
public:
    virtual ~BaseGenerator() = default;

    /**
     * @brief Processes a block of audio and generates MIDI messages.
     *
     * This pure virtual function must be implemented by all derived generator classes.
     * It is responsible for generating MIDI events based on the generator's internal logic
     * and the parameters provided in the AudioProcessorValueTreeState.
     *
     * @param midiMessages The MIDI buffer to which generated messages will be added.
     * @param apvts A reference to the AudioProcessorValueTreeState containing all plugin parameters.
     * @param sampleRate The current sample rate.
     * @param currentBeat The current beat position in the host's timeline.
     */
    virtual void process(juce::MidiBuffer& midiMessages,
                         juce::AudioProcessorValueTreeState& apvts,
                         double sampleRate,
                         double currentBeat) = 0;

    /**
     * @brief Sets the musical scale for the generator.
     *
     * This function can be overridden by generators that use musical scales.
     *
     * @param rootNote The MIDI note number of the root of the scale.
     * @param scaleNotes A vector of integers representing the intervals of the scale.
     */
    virtual void setScale(int rootNote, const std::vector<int>& scaleNotes)
    {
        juce::ignoreUnused(rootNote, scaleNotes);
        // Default implementation does nothing.
        // Derived classes can override this if they use scales.
    }
};