#pragma once
#include <JuceHeader.h>
#include "PendingNoteOff.h"
#include <memory> // For std::unique_ptr

// Forward-declare the structs to avoid including their headers here.
// This reduces dependencies and compile times.
struct LiveNote;
namespace std { template <typename T> class vector; }


class BaseGenerator
{
public:
    BaseGenerator();
    virtual ~BaseGenerator(); // Destructor must be defined in .cpp for pimpl

    const std::vector<LiveNote>& getRecentNotes() const;

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
    virtual juce::Array<PendingNoteOff> process(juce::MidiBuffer& midiMessages,
                                                juce::AudioProcessorValueTreeState& apvts,
                                                double sampleRate,
                                                double blockStartTime,
                                                double blockEndTime,
                                                int numSamples) = 0;

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

    /**
     * @brief Generates a pattern of a specific duration.
     *
     * This function can be overridden by generators to produce a complete
     * musical pattern, which can be used for features like 'Capture'.
     *
     * @param durationInBeats The desired duration of the pattern in beats.
     * @param apvts A reference to the AudioProcessorValueTreeState.
     * @param sampleRate The current sample rate.
     * @return A MidiBuffer containing the generated pattern.
     */
    virtual juce::MidiBuffer getPattern(double durationInBeats, juce::AudioProcessorValueTreeState& apvts, double sampleRate)
    {
        // Default implementation returns an empty buffer.
        juce::ignoreUnused(durationInBeats, apvts, sampleRate);
        return juce::MidiBuffer();
    }

    virtual void reset()
    {
        // Default implementation does nothing.
    }

protected:
    void clearRecentNotes();
    void addRecentNote(const LiveNote& note);

    // Using the Pimpl idiom to hide implementation details (like std::vector)
    // from this header file, reducing compile times and dependencies.
    struct GeneratorState;
    std::unique_ptr<GeneratorState> pimpl;
};