#include "Looper.h"

Looper::Looper()
{
    // Инициализация с настройками по умолчанию
    currentMode = LooperMode::MidiLooper;
    bufferSize = 4.0; // 4 beats
    playbackSpeed = 1.0f;
    pitchShift = 0;
    reverse = false;
}

void Looper::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void Looper::recordNote(const juce::MidiMessage& message, double beatTime)
{
    if (!isRecording) return;

    if (message.isNoteOn())
    {
        pendingNotes.push_back({message, beatTime, 0.0});
    }
    else if (message.isNoteOff())
    {
        auto it = std::find_if(pendingNotes.begin(), pendingNotes.end(), [&](const RecordedNote& n) {
            return n.message.getNoteNumber() == message.getNoteNumber() &&
                   n.message.getChannel() == message.getChannel();
        });

        if (it != pendingNotes.end())
        {
            it->durationInBeats = beatTime - it->beatTime;
            if (it->durationInBeats <= 0) it->durationInBeats = 0.1;
            recordedNotes.push_back(*it);
            pendingNotes.erase(it);
        }
    }
}

void Looper::recordMidiBuffer(const juce::MidiBuffer& buffer, double startTime)
{
    if (isRecording && currentMode == LooperMode::GenerationLooper)
    {
        generationBuffer = buffer;
        for (const auto& event : buffer)
        {
            auto message = event.getMessage();
            double timeInBeats = startTime + (event.samplePosition / 44100.0) * (120.0 / 60.0);
            recordedNotes.push_back({message, timeInBeats});
        }
    }
}

void Looper::startPlayback()
{
    isPlaying = true;
    isRecording = false;
}

juce::MidiBuffer Looper::stopPlayback()
{
    isPlaying = false;
    currentlyPlayingNotes.clear();
    return juce::MidiBuffer();
}

void Looper::clear()
{
    recordedNotes.clear();
    pristine_loop_.clear();
    pendingNotes.clear();
    generationBuffer.clear();
    loopStart = 0.0;
    loopEnd = 0.0;
}

juce::MidiBuffer Looper::getPlaybackBuffer(int numSamples, double startTime, double endTime, bool isPadMode, int channel)
{
    if (!isPlaying)
        return juce::MidiBuffer();

    switch (currentMode)
    {
        case LooperMode::MidiLooper:
            return processMidiLooperBuffer(numSamples, startTime, endTime, isPadMode, channel);
        case LooperMode::GenerationLooper:
            return processGenerationLooperBuffer(numSamples, startTime, endTime);
        default:
            return juce::MidiBuffer();
    }
}

void Looper::setMode(LooperMode mode)
{
    currentMode = mode;
    clear();
}

juce::MidiBuffer Looper::processMidiLooperBuffer(int numSamples, double startTime, double endTime, bool isPadMode, int channel)
{
    playbackHead_ = startTime;
    juce::MidiBuffer buffer;

    if (recordedNotes.empty())
        return buffer;

    double loopDuration = loopEnd - loopStart;
    if (loopDuration <= 0.0)
        return buffer;

    double blockDuration = endTime - startTime;

    for (const auto& note : recordedNotes)
    {
        double noteStart = note.beatTime;
        double noteEnd = note.beatTime + note.durationInBeats;

        double firstLoopIndex = floor((startTime - loopStart - noteEnd) / loopDuration);
        double lastLoopIndex = floor((endTime - loopStart - noteStart) / loopDuration);

        for (double loopIndex = firstLoopIndex; loopIndex <= lastLoopIndex; ++loopIndex)
        {
            double loopOffset = loopIndex * loopDuration;
            double absoluteNoteStart = loopStart + noteStart + loopOffset;
            double absoluteNoteEnd = loopStart + noteEnd + loopOffset;

            if (absoluteNoteStart >= startTime && absoluteNoteStart < endTime)
            {
                int samplePosition = static_cast<int>(((absoluteNoteStart - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    buffer.addEvent(applyEffects(note.message, 0), samplePosition);
                    currentlyPlayingNotes.insert(note.message.getNoteNumber());
                }
            }

            if (absoluteNoteEnd >= startTime && absoluteNoteEnd < endTime)
            {
                int samplePosition = static_cast<int>(((absoluteNoteEnd - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    auto noteOffMessage = juce::MidiMessage::noteOff(note.message.getChannel(), note.message.getNoteNumber());
                    buffer.addEvent(applyEffects(noteOffMessage, 0), samplePosition);
                    currentlyPlayingNotes.erase(note.message.getNoteNumber());
                }
            }
        }
    }

    if (isPadMode)
    {
        double loopCycleStart = floor((startTime - loopStart) / loopDuration) * loopDuration + loopStart;
        while (loopCycleStart < endTime)
        {
            double absoluteLoopStart = loopCycleStart;
            double absoluteLoopEnd = loopCycleStart + loopDuration;
            if (absoluteLoopStart >= startTime && absoluteLoopStart < endTime)
            {
                int samplePosition = static_cast<int>(((absoluteLoopStart - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    buffer.addEvent(juce::MidiMessage::controllerEvent(channel, 64, 127), samplePosition);
                }
            }
            if (absoluteLoopEnd >= startTime && absoluteLoopEnd < endTime)
            {
                int samplePosition = static_cast<int>(((absoluteLoopEnd - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    buffer.addEvent(juce::MidiMessage::controllerEvent(channel, 64, 0), samplePosition);
                }
            }
            loopCycleStart += loopDuration;
        }
    }

    return buffer;
}

juce::MidiBuffer Looper::processGenerationLooperBuffer(int numSamples, double startTime, double endTime)
{
    playbackHead_ = startTime;
    juce::MidiBuffer buffer;

    if (generationBuffer.isEmpty())
        return buffer;

    double loopDuration = bufferSize;
    if (loopDuration <= 0.0)
        return buffer;

    double currentTime = startTime;
    double loopPosition = std::fmod(currentTime, loopDuration);

    for (const auto& event : generationBuffer)
    {
        auto originalMessage = event.getMessage();
        double originalTime = (event.samplePosition / 44100.0) * (120.0 / 60.0);
        double adjustedTime = originalTime;
        if (playbackSpeed != 1.0f)
        {
            adjustedTime = originalTime * playbackSpeed;
        }

        if (adjustedTime >= loopPosition && adjustedTime < loopPosition + (endTime - startTime))
        {
            double timeInWindow = adjustedTime - loopPosition;
            double timeInSamples = timeInWindow * (numSamples / (endTime - startTime));
            int samplePosition = static_cast<int>(timeInSamples);

            if (samplePosition >= 0 && samplePosition < numSamples)
            {
                juce::MidiMessage processedMessage = applyEffects(originalMessage, timeInWindow);
                buffer.addEvent(processedMessage, samplePosition);
            }
        }
    }

    return buffer;
}

double Looper::getPlaybackProgress() const
{
    if (!isPlaying || (loopEnd - loopStart) <= 0.0)
    {
        return 0.0;
    }
    double progress = (playbackHead_ - loopStart) / (loopEnd - loopStart);
    return std::fmod(progress, 1.0);
}

juce::MidiMessage Looper::applyEffects(const juce::MidiMessage& message, [[maybe_unused]] double timeOffset)
{
    juce::MidiMessage result = message;
    if (message.isNoteOn() || message.isNoteOff())
    {
        int noteNumber = message.getNoteNumber();
        if (pitchShift != 0)
        {
            noteNumber += pitchShift;
            noteNumber = juce::jlimit(0, 127, noteNumber);
        }
        if (message.isNoteOn())
        {
            result = juce::MidiMessage::noteOn(message.getChannel(), noteNumber, message.getVelocity());
        }
        else if (message.isNoteOff())
        {
            result = juce::MidiMessage::noteOff(message.getChannel(), noteNumber);
        }
    }
    return result;
}

void Looper::setRecording(bool recording)
{
    isRecording = recording;
}

void Looper::setLoopPoints(double start, double end)
{
    loopStart = start;
    loopEnd = end;
}

void Looper::startRecording(double maxDuration, bool isOverdub, double currentBeat)
{
    maxRecordLengthBeats_ = maxDuration;
    recordingStartTime_ = currentBeat;
    setRecording(true);
    if (!isOverdub)
    {
        clear();
    }
}

void Looper::stopRecording()
{
    if (!isRecording) return;
    isRecording = false;

    if (!pendingNotes.empty())
    {
        double endOfLoop = 0.0;
        if (!recordedNotes.empty())
        {
            endOfLoop = std::max_element(recordedNotes.begin(), recordedNotes.end(),
                [](const RecordedNote& a, const RecordedNote& b) {
                    return (a.beatTime + a.durationInBeats) < (b.beatTime + b.durationInBeats);
                })->beatTime;
        }
        for (auto& pendingNote : pendingNotes)
        {
            pendingNote.durationInBeats = endOfLoop - pendingNote.beatTime;
            if (pendingNote.durationInBeats <= 0)
            {
                pendingNote.durationInBeats = 0.25;
            }
            recordedNotes.push_back(pendingNote);
        }
        pendingNotes.clear();
    }

    if (recordedNotes.empty())
    {
        loopStart = 0.0;
        loopEnd = 0.0;
        return;
    }

    double minBeat = recordedNotes.front().beatTime;
    double maxBeat = 0.0;
    for (auto& note : recordedNotes)
    {
        note.beatTime -= minBeat;
        if (note.beatTime + note.durationInBeats > maxBeat)
        {
            maxBeat = note.beatTime + note.durationInBeats;
        }
    }
    loopStart = 0.0;
    loopEnd = std::ceil(maxBeat * 4.0) / 4.0;
    pristine_loop_ = recordedNotes;
}

void Looper::loadFromMidiBuffer(const juce::MidiBuffer& buffer, double sampleRate, double bpm, bool isOverdub, double requestedDuration)
{
    if (!isOverdub)
    {
        clear();
    }

    std::map<int, RecordedNote> pendingNotesMap;
    const double beatsPerSample = bpm / 60.0 / sampleRate;

    for (const auto metadata : buffer)
    {
        const auto message = metadata.getMessage();
        const double beatTime = metadata.samplePosition * beatsPerSample;

        if (message.isNoteOn())
        {
            if (pendingNotesMap.count(message.getNoteNumber()))
            {
                pendingNotesMap[message.getNoteNumber()].durationInBeats = beatTime - pendingNotesMap[message.getNoteNumber()].beatTime;
                if (pendingNotesMap[message.getNoteNumber()].durationInBeats <= 0)
                    pendingNotesMap[message.getNoteNumber()].durationInBeats = 0.125;
                recordedNotes.push_back(pendingNotesMap[message.getNoteNumber()]);
                pendingNotesMap.erase(message.getNoteNumber());
            }
            
            RecordedNote newNote;
            newNote.message = message;
            newNote.beatTime = beatTime;
            pendingNotesMap[message.getNoteNumber()] = newNote;
        }
        else if (message.isNoteOff())
        {
            if (pendingNotesMap.count(message.getNoteNumber()))
            {
                pendingNotesMap[message.getNoteNumber()].durationInBeats = beatTime - pendingNotesMap[message.getNoteNumber()].beatTime;
                 if (pendingNotesMap[message.getNoteNumber()].durationInBeats <= 0)
                    pendingNotesMap[message.getNoteNumber()].durationInBeats = 0.125;
                if (pendingNotesMap[message.getNoteNumber()].beatTime < requestedDuration)
                    recordedNotes.push_back(pendingNotesMap[message.getNoteNumber()]);
                pendingNotesMap.erase(message.getNoteNumber());
            }
        }
    }

    // Finalize any remaining hanging notes from the buffer
    for (auto const& [noteNum, pendingNote] : pendingNotesMap)
    {
        auto noteToPush = pendingNote;
        noteToPush.durationInBeats = requestedDuration - noteToPush.beatTime;
        if (noteToPush.durationInBeats <= 0)
            noteToPush.durationInBeats = 0.125;
        
        if (noteToPush.beatTime < requestedDuration)
            recordedNotes.push_back(noteToPush);
    }

    pristine_loop_ = recordedNotes;

    if (!isOverdub)
    {
        loopStart = 0.0;
        loopEnd = requestedDuration;
    }
}

void Looper::unquantize()
{
    recordedNotes = pristine_loop_;
}

void Looper::quantize(double gridInBeats)
{
    if (gridInBeats <= 0)
    {
        unquantize();
        return;
    }

    recordedNotes = pristine_loop_;
    for (auto& note : recordedNotes)
    {
        note.beatTime = std::round(note.beatTime / gridInBeats) * gridInBeats;
    }
}

juce::MidiBuffer Looper::doubleLoop()
{
    if (recordedNotes.empty())
    {
        loopEnd *= 2.0;
        return juce::MidiBuffer();
    }

    const double currentDuration = loopEnd - loopStart;
    std::vector<RecordedNote> notesToAppend;
    for (const auto& note : recordedNotes)
    {
        RecordedNote newNote = note;
        newNote.beatTime += currentDuration;
        notesToAppend.push_back(newNote);
    }

    recordedNotes.insert(recordedNotes.end(), notesToAppend.begin(), notesToAppend.end());
    loopEnd *= 2.0;
    pristine_loop_ = recordedNotes;
    return juce::MidiBuffer();
}

juce::MidiBuffer Looper::splitLoop(bool keepFirstHalf)
{
    juce::MidiBuffer noteOffsToSend;
    const double currentDuration = loopEnd - loopStart;
    const double midpoint = loopStart + currentDuration / 2.0;

    std::vector<RecordedNote> notesToKeep;

    if (keepFirstHalf)
    {
        for (const auto& note : recordedNotes)
        {
            if (note.beatTime < midpoint)
            {
                notesToKeep.push_back(note);
                if (note.beatTime + note.durationInBeats > midpoint)
                {
                    noteOffsToSend.addEvent(juce::MidiMessage::noteOff(note.message.getChannel(), note.message.getNoteNumber()), 0);
                }
            }
        }
        loopEnd = midpoint;
    }
    else
    {
        for (const auto& note : recordedNotes)
        {
            if (note.beatTime >= midpoint)
            {
                RecordedNote newNote = note;
                newNote.beatTime -= midpoint;
                notesToKeep.push_back(newNote);
            }
            else if (note.beatTime < midpoint && note.beatTime + note.durationInBeats > midpoint)
            {
                noteOffsToSend.addEvent(juce::MidiMessage::noteOff(note.message.getChannel(), note.message.getNoteNumber()), 0);
            }
        }
        loopEnd = midpoint;
    }

    recordedNotes = notesToKeep;
    pristine_loop_ = recordedNotes;
    return noteOffsToSend;
}

bool Looper::isRecordingTimeExceeded(double currentBeat) const
{
    if (!isRecording)
        return false;
    return currentBeat - recordingStartTime_ >= maxRecordLengthBeats_;
}

void Looper::generateVariations(float bassIntensity, float midIntensity, float highIntensity, int rootNote, const std::vector<int>& scaleNotes)
{
    if (pristine_loop_.empty() || scaleNotes.empty()) return;

    recordedNotes.clear();
    juce::Random random;

    for (const auto& note : pristine_loop_)
    {
        recordedNotes.push_back(note);

        int noteNumber = note.message.getNoteNumber();

        if (noteNumber < 48 && random.nextFloat() < bassIntensity)
        {
            auto bassNote = note;
            bassNote.message = juce::MidiMessage::noteOn(note.message.getChannel(), noteNumber - 12, note.message.getVelocity());
            recordedNotes.push_back(bassNote);
        }

        if (noteNumber >= 48 && noteNumber < 72 && random.nextFloat() < midIntensity)
        {
            int noteIndex = -1;
            for(int i=0; i< (int)scaleNotes.size(); ++i)
            {
                if((noteNumber % 12) == (rootNote % 12 + scaleNotes[i]) % 12)
                {
                    noteIndex = i;
                    break;
                }
            }

            if(noteIndex != -1)
            {
                int harmonyIndex = (noteIndex + (random.nextBool() ? 2 : 4)) % scaleNotes.size();
                int octave = noteNumber / 12;
                int harmonyNoteNumber = octave * 12 + rootNote + scaleNotes[harmonyIndex];
                if (harmonyNoteNumber >= 128) harmonyNoteNumber -= 12;

                auto harmonyNote = note;
                harmonyNote.message = juce::MidiMessage::noteOn(note.message.getChannel(), harmonyNoteNumber, (juce::uint8)(note.message.getVelocity() * 0.8f));
                recordedNotes.push_back(harmonyNote);
            }
        }

        if (noteNumber >= 72 && random.nextFloat() < highIntensity)
        {
            int graceNoteNumber = noteNumber + (random.nextBool() ? 1 : -1);
            if (graceNoteNumber < 0 || graceNoteNumber > 127) graceNoteNumber = noteNumber;

            auto graceNote = note;
            graceNote.beatTime -= 0.05;
            graceNote.durationInBeats = 0.05;
            graceNote.message = juce::MidiMessage::noteOn(note.message.getChannel(), graceNoteNumber, (juce::uint8)(note.message.getVelocity() * 0.7f));
            recordedNotes.push_back(graceNote);
        }
    }

    pristine_loop_ = recordedNotes;
}