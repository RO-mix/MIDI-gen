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

    if (beatTime - recordingStartTime_ >= maxRecordLengthBeats_)
    {
        stopRecording();
        startPlayback();
        return;
    }

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

void Looper::stopPlayback()
{
    isPlaying = false;
}

void Looper::clear()
{
    recordedNotes.clear();
    generationBuffer.clear();
    isPlaying = false;
    isRecording = false;
}

juce::MidiBuffer Looper::getPlaybackBuffer(int numSamples, double startTime, double endTime, bool isPadMode)
{
    if (!isPlaying)
        return juce::MidiBuffer();

    switch (currentMode)
    {
        case LooperMode::MidiLooper:
            return processMidiLooperBuffer(numSamples, startTime, endTime, isPadMode);
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

juce::MidiBuffer Looper::processMidiLooperBuffer(int numSamples, double startTime, double endTime)
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
        // Check two full loops to handle events that wrap around the start/end of the block
        for (int i = -1; i <= 1; ++i)
        {
            double loopOffset = i * loopDuration;
            double noteOnBeat = note.beatTime + loopOffset;
            double noteOffBeat = note.beatTime + note.durationInBeats + loopOffset;

            // Schedule Note On
            if (noteOnBeat >= startTime && noteOnBeat < endTime)
            {
                int samplePosition = static_cast<int>(((noteOnBeat - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    buffer.addEvent(applyEffects(note.message, 0), samplePosition);
                }
            }

            // Schedule Note Off
            if (!isPadMode && noteOffBeat >= startTime && noteOffBeat < endTime)
            {
                int samplePosition = static_cast<int>(((noteOffBeat - startTime) / blockDuration) * numSamples);
                if (samplePosition >= 0 && samplePosition < numSamples)
                {
                    auto noteOff = juce::MidiMessage::noteOff(note.message.getChannel(), note.message.getNoteNumber());
                    buffer.addEvent(applyEffects(noteOff, 0), samplePosition);
                }
            }
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
    if (recording)
    {
        isPlaying = false; // Останавливаем воспроизведение при начале записи
    }
}

void Looper::setLoopPoints(double start, double end)
{
    loopStart = start;
    loopEnd = end;
}

void Looper::startRecording(double maxDuration, bool isOverdub)
{
    maxRecordLengthBeats_ = maxDuration;
    recordingStartTime_ = playbackHead_; // Assume playbackHead is current time
    setRecording(true);
    if (!isOverdub)
    {
        clear(); // Очищаем предыдущие записи
    }
}

void Looper::stopRecording()
{
    setRecording(false);
}

void Looper::loadFromMidiBuffer(const juce::MidiBuffer& buffer, double sampleRate, double bpm, bool isOverdub)
{
    if (!isOverdub)
    {
        clear();
    }

    std::map<int, std::pair<double, int>> noteOnEvents; // note -> { beatTime, velocity }

    const double beatsPerSample = bpm / 60.0 / sampleRate;

    for (const auto metadata : buffer)
    {
        const auto message = metadata.getMessage();
        const double beatTime = metadata.samplePosition * beatsPerSample;

        if (message.isNoteOn())
        {
            noteOnEvents[message.getNoteNumber()] = { beatTime, message.getVelocity() };
        }
        else if (message.isNoteOff())
        {
            auto it = noteOnEvents.find(message.getNoteNumber());
            if (it != noteOnEvents.end())
            {
                RecordedNote newNote;
                newNote.message = juce::MidiMessage::noteOn(message.getChannel(),
                                                            message.getNoteNumber(),
                                                            (juce::uint8)it->second.second);
                newNote.beatTime = it->second.first;
                newNote.durationInBeats = beatTime - it->second.first;

                // Ensure duration is not negative or zero
                if (newNote.durationInBeats <= 0)
                    newNote.durationInBeats = 0.125; // Default to a 32nd note

                recordedNotes.push_back(newNote);
                noteOnEvents.erase(it);
            }
        }
    }

    pristine_loop_ = recordedNotes; // Save a pristine copy

    // Update loop points based on content
    if (!recordedNotes.empty())
    {
        double maxBeat = 0.0;
        for (const auto& note : recordedNotes)
        {
            maxBeat = std::max(maxBeat, note.beatTime + note.durationInBeats);
        }
        loopStart = 0.0;
        // Round up to the nearest bar
        loopEnd = std::ceil(maxBeat / 4.0) * 4.0;
        if (loopEnd == 0) loopEnd = 4.0; // Ensure at least one bar
    }
    else
    {
        loopStart = 0.0;
        loopEnd = 4.0; // Default to 4 beats if empty
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

void Looper::doubleLoop()
{
    if (recordedNotes.empty())
    {
        loopEnd *= 2.0;
        return;
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
    pristine_loop_ = recordedNotes; // This becomes the new pristine state
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

void Looper::splitLoop(bool keepFirstHalf)
{
    const double currentDuration = loopEnd - loopStart;
    const double midpoint = loopStart + currentDuration / 2.0;

    if (keepFirstHalf)
    {
        recordedNotes.erase(
            std::remove_if(recordedNotes.begin(), recordedNotes.end(),
                [&](const RecordedNote& note) {
                    return note.beatTime >= midpoint;
                }),
            recordedNotes.end());
        loopEnd = midpoint;
    }
    else // keep second half
    {
        recordedNotes.erase(
            std::remove_if(recordedNotes.begin(), recordedNotes.end(),
                [&](const RecordedNote& note) {
                    return note.beatTime < midpoint;
                }),
            recordedNotes.end());

        // Shift remaining notes to the start of the loop
        for (auto& note : recordedNotes)
        {
            note.beatTime -= midpoint;
        }
        loopEnd = midpoint;
    }
    pristine_loop_ = recordedNotes; // This becomes the new pristine state
}