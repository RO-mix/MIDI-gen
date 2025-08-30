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
    // This method is required by the PluginProcessor but the looper
    // currently does not need to do any special preparation.
    // We can add logic here later if needed.
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void Looper::recordNote(const juce::MidiMessage& message, double beatTime)
{
    if (!isRecording) return;

    // juce::Logger::writeToLog("Looper::recordNote: Received " + message.getDescription() + " at beat " + juce::String(beatTime));

    // The processor is now responsible for stopping the recording at the correct time.
    // if (beatTime - recordingStartTime_ >= maxRecordLengthBeats_)
    // {
    //     stopRecording();
    //     return;
    // }

    if (message.isNoteOn())
    {
        // Add to pending notes, waiting for a note off
        pendingNotes.push_back({message, beatTime, 0.0});
    }
    else if (message.isNoteOff())
    {
        // Find the matching note on
        auto it = std::find_if(pendingNotes.begin(), pendingNotes.end(), [&](const RecordedNote& n) {
            return n.message.getNoteNumber() == message.getNoteNumber() &&
                   n.message.getChannel() == message.getChannel();
        });

        if (it != pendingNotes.end())
        {
            // Calculate duration and add to the main list
            it->durationInBeats = beatTime - it->beatTime;
            // Ensure minimum duration for visibility
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
        // Копируем буфер для Generation Looper
        generationBuffer = buffer;

        // Также сохраняем как отдельные ноты для совместимости
        for (const auto& event : buffer)
        {
            auto message = event.getMessage();
            double timeInBeats = startTime + (event.samplePosition / 44100.0) * (120.0 / 60.0); // Предполагаем 120 BPM
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
    currentlyPlayingNotes.clear(); // Clear the set of playing notes
    // The processor will be responsible for sending a general All-Notes-Off
    // to prevent hanging notes more robustly.
    return juce::MidiBuffer();
}

void Looper::clear()
{
    // This method should only clear data, not state.
    // The processor is responsible for starting/stopping playback and recording.
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
    // Очищаем данные при смене режима
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

        // Determine the number of loops we need to check (usually just one or two)
        double firstLoopIndex = floor((startTime - loopStart - noteEnd) / loopDuration);
        double lastLoopIndex = floor((endTime - loopStart - noteStart) / loopDuration);

        for (double loopIndex = firstLoopIndex; loopIndex <= lastLoopIndex; ++loopIndex)
        {
            double loopOffset = loopIndex * loopDuration;
            double absoluteNoteStart = loopStart + noteStart + loopOffset;
            double absoluteNoteEnd = loopStart + noteEnd + loopOffset;

            // Schedule Note On
            if (absoluteNoteStart >= startTime && absoluteNoteStart < endTime)
            {
                int samplePosition = static_cast<int>(((absoluteNoteStart - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    buffer.addEvent(applyEffects(note.message, 0), samplePosition);
                    currentlyPlayingNotes.insert(note.message.getNoteNumber());
                }
            }

            // Schedule Note Off
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

    // New PAD Mode Logic:
    // Instead of holding note-off messages, this mode now simulates a sustain pedal (CC 64)
    // being held for the duration of the loop. This allows all notes within the loop to sustain
    // naturally and release together when the loop cycles.
    if (isPadMode)
    {
        // We need to check if the start or end of a loop cycle falls within the current buffer.
        double loopCycleStart = floor((startTime - loopStart) / loopDuration) * loopDuration + loopStart;

        while (loopCycleStart < endTime)
        {
            double absoluteLoopStart = loopCycleStart;
            double absoluteLoopEnd = loopCycleStart + loopDuration;

            // If the loop start is in this block, send Sustain ON.
            if (absoluteLoopStart >= startTime && absoluteLoopStart < endTime)
            {
                int samplePosition = static_cast<int>(((absoluteLoopStart - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    // Using CC 64 for Sustain Pedal, which is the MIDI standard.
                    // The user mentioned CC 67, but that is for "Soft Pedal".
                    buffer.addEvent(juce::MidiMessage::controllerEvent(channel, 64, 127), samplePosition);
                }
            }

            // If the loop end is in this block, send Sustain OFF.
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

    double loopDuration = bufferSize; // Используем размер буфера как длительность лупа
    if (loopDuration <= 0.0)
        return buffer;

    // Вычисляем текущую позицию в лупе
    double currentTime = startTime;
    double loopPosition = std::fmod(currentTime, loopDuration);

    // Для Generation Looper нужно обрабатывать весь буфер с учетом эффектов
    for (const auto& event : generationBuffer)
    {
        auto originalMessage = event.getMessage();
        double originalTime = (event.samplePosition / 44100.0) * (120.0 / 60.0); // Предполагаем 120 BPM

        // Применяем эффекты скорости воспроизведения
        double adjustedTime = originalTime;
        if (playbackSpeed != 1.0f)
        {
            adjustedTime = originalTime * playbackSpeed;
        }

        // Проверяем попадание в текущее окно
        if (adjustedTime >= loopPosition && adjustedTime < loopPosition + (endTime - startTime))
        {
            // Вычисляем позицию в сэмплах
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

        // Применяем питч-шифт
        if (pitchShift != 0)
        {
            noteNumber += pitchShift;
            noteNumber = juce::jlimit(0, 127, noteNumber);
        }

        // Применяем реверс (для Generation Looper)
        if (reverse && currentMode == LooperMode::GenerationLooper)
        {
            // Для реверса меняем направление времени
            // Это упрощенная реализация
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
    // We should be able to record while playing (for overdubbing),
    // so we don't automatically stop playback here anymore.
    // The processor logic will handle what gets recorded.
    if (recording)
    {
        // isPlaying = false; // This was preventing overdubbing on a playing loop.
    }
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

    // Finalize any notes that are still "on" (lacking a note-off)
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
                pendingNote.durationInBeats = 0.25; // Give it a default short duration
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

    // Normalize beat times and set loop duration
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
    loopEnd = std::ceil(maxBeat * 4.0) / 4.0; // Quantize to nearest 1/16th note up

    pristine_loop_ = recordedNotes;
}

void Looper::loadFromMidiBuffer(const juce::MidiBuffer& buffer, double sampleRate, double bpm, bool isOverdub, double requestedDuration)
{
    if (!isOverdub)
    {
        clear();
    }

    std::map<int, RecordedNote> pendingNotes;
    const double beatsPerSample = bpm / 60.0 / sampleRate;

    for (const auto metadata : buffer)
    {
        const auto message = metadata.getMessage();
        const double beatTime = metadata.samplePosition * beatsPerSample;

        if (message.isNoteOn())
        {
            // If there's a hanging note of the same pitch, finalize it.
            if (pendingNotes.count(message.getNoteNumber()))
            {
                pendingNotes[message.getNoteNumber()].durationInBeats = beatTime - pendingNotes[message.getNoteNumber()].beatTime;
                if (pendingNotes[message.getNoteNumber()].durationInBeats <= 0)
                    pendingNotes[message.getNoteNumber()].durationInBeats = 0.125;
                recordedNotes.push_back(pendingNotes[message.getNoteNumber()]);
                pendingNotes.erase(message.getNoteNumber());
            }

            RecordedNote newNote;
            newNote.message = message;
            newNote.beatTime = beatTime;
            pendingNotes[message.getNoteNumber()] = newNote;
        }
        else if (message.isNoteOff())
        {
            if (pendingNotes.count(message.getNoteNumber()))
            {
                pendingNotes[message.getNoteNumber()].durationInBeats = beatTime - pendingNotes[message.getNoteNumber()].beatTime;
                 if (pendingNotes[message.getNoteNumber()].durationInBeats <= 0)
                    pendingNotes[message.getNoteNumber()].durationInBeats = 0.125;
                if (pendingNotes[message.getNoteNumber()].beatTime < requestedDuration)
                    recordedNotes.push_back(pendingNotes[message.getNoteNumber()]);
                pendingNotes.erase(message.getNoteNumber());
            }
        }
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

    recordedNotes = pristine_loop_; // Start from the pristine version
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
    return juce::MidiBuffer(); // No notes are cut off when doubling
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
    else // keep second half
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
                // Note started in the first half but carries over, should be cut off
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
        recordedNotes.push_back(note); // Add the original note

        int noteNumber = note.message.getNoteNumber();

        // Bass variations (add octave down)
        if (noteNumber < 48 && random.nextFloat() < bassIntensity)
        {
            auto bassNote = note;
            bassNote.message = juce::MidiMessage::noteOn(note.message.getChannel(), noteNumber - 12, note.message.getVelocity());
            recordedNotes.push_back(bassNote);
        }

        // Mid variations (add thirds/fifths)
        if (noteNumber >= 48 && noteNumber < 72 && random.nextFloat() < midIntensity)
        {
            // Find note in scale
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
                int harmonyIndex = (noteIndex + (random.nextBool() ? 2 : 4)) % scaleNotes.size(); // Third or Fifth
                int octave = noteNumber / 12;
                int harmonyNoteNumber = octave * 12 + rootNote + scaleNotes[harmonyIndex];
                if (harmonyNoteNumber >= 128) harmonyNoteNumber -= 12;

                auto harmonyNote = note;
                harmonyNote.message = juce::MidiMessage::noteOn(note.message.getChannel(), harmonyNoteNumber, (juce::uint8)(note.message.getVelocity() * 0.8f));
                recordedNotes.push_back(harmonyNote);
            }
        }

        // High variations (add grace notes)
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

    pristine_loop_ = recordedNotes; // The variation becomes the new pristine state
}