# Handoff Document for Creative MIDI Looper Project

## 1. Overall Goal

The main goal of this task is to fix a series of critical bugs in the MIDI looper, overhaul the user interface for better usability and clarity, and implement a new "Extend" feature for more creative looping.

## 2. Analysis of Bugs and Required Fixes

Through investigation and trial-and-error (including multiple environment resets), the following issues and their solutions have been identified.

### 2.1. Recording Logic (Critical)
- **Bug:** Recording from external MIDI sources does not work. Overdubbing creates duplicate notes because the looper re-records its own output.
- **File:** `juce_port/juce_project/Source/PluginProcessor.cpp`
- **Fix:**
    1.  At the beginning of `processBlock`, create a copy of the incoming `midiMessages`: `juce::MidiBuffer externalInput = midiMessages;`
    2.  In the recording section of `processBlock`, remove all existing logic and replace it with code that *only* records from the `externalInput` buffer and the `generatedMidi` buffer. **Do not** record from `looperPlaybackMidi`.

### 2.2. Note Duration Bug (Critical)
- **Bug:** When a recording is stopped, notes that were still playing are saved with an incorrect (longer) duration.
- **Files:** `Looper.h`, `Looper.cpp`, `PluginProcessor.cpp`
- **Fix:**
    1.  The signature of `Looper::stopRecording()` must be changed to `void stopRecording(double stopBeat);` to accept the precise beat time when the recording was stopped.
    2.  The call site in `PluginProcessor::executePendingLooperAction` must be updated to pass `currentBeat_`: `looper_->stopRecording(currentBeat_);`.
    3.  The logic inside `Looper::stopRecording` must be completely replaced. The new logic should first finalize the duration of any "pending" notes using the `stopBeat`, then collect *all* notes from the recording session, and only then normalize all their start times relative to the start of the recording pass. This prevents mixing absolute and relative timestamps, which was the cause of the bug.

### 2.3. Timeline UI Bugs
- **Bug:** The timeline does not show a live, scrolling view of the generator's output when the looper is idle. The note coloring is incorrect, and notes are positioned incorrectly during recording.
- **File:** `juce_port/juce_project/Source/UI/TimelineComponent.cpp`
- **Fix:**
    1.  The `paint()` method must be refactored.
    2.  A separate rendering path for "Live Mode" (when the looper is off) must be implemented to draw the `liveNotes` with a scrolling effect.
    3.  The `isCaptureBuffer` flag (see below) must be used to decide the color for playback (blue for capture, orange for normal).
    4.  Newly recorded notes must be drawn in red, with their position calculated relative to `recordingStartTime`, not with `fmod`.

### 2.4. Headless Test Build Failure
- **Bug:** The `HeadlessTestRunner` target fails to build in the Linux environment due to missing GUI dependencies (e.g., X11, GTK).
- **File:** `juce_port/juce_project/CMakeLists.txt`
- **Fix:** Add the following compile definitions to the `HeadlessTestRunner` target to instruct JUCE to exclude GUI components:
    ```cmake
    target_compile_definitions(HeadlessTestRunner PRIVATE
        # ... other flags
        JUCE_GUI_BASICS_INCLUDE_X11=0
        JUCE_WEB_BROWSER=0
    )
    ```

## 3. UI and Feature Enhancements

### 3.1. `isCaptureBuffer` State
- **Purpose:** To differentiate between a recorded loop and a captured pattern for the new color scheme.
- **Implementation:**
    - Add `bool isCaptureBuffer` and `getIsCaptureBuffer()` to `Looper.h`.
    - Set the flag to `true` in `loadFromMidiBuffer` and `false` in `startRecording` and `clear`.
    - Expose this via a getter in `PluginProcessor`.

### 3.2. Other UI Changes
- **Move Start Button:** Remove from `ToolbarComponent` and add to `GeneratorSectionComponent` (left of the dropdown).
- **Hide BPM Slider:** Use `#if JucePlugin_Build_Standalone` to ensure it only appears in the standalone build.
- **Rename Buttons:** Rename "Overdub" buttons in `LooperSectionComponent` to "OVR".
- **Restyle Headers:** In `GeneratorSectionComponent` and `LooperSectionComponent`, increase the title font size (e.g., to `20.0f`) and remove the `g.drawRect()` call in the `paint` methods.

## 4. New "Extend" Feature
- **Concept:** A user-selectable mode that allows a loop to grow in length.
- **Logic:**
    - Add a `LOOPER_EXTEND_MODE` `AudioParameterBool`.
    - Add a corresponding "Extend" toggle button to the UI.
    - In `PluginProcessor`, when starting a recording, if "Extend" is on and a loop is playing, set the recording length to a very large number.
    - In the new `Looper::stopRecording` logic, if the new recorded material is longer than the original loop, tile the original loop's notes to fill the new duration, then add the new notes on top.

## 5. Recommended Plan for Next Developer

The following plan consolidates all fixes and features into a logical order. It is recommended to perform these steps carefully, verifying each change.

1.  **Fix Core Recording Logic:** Apply the fixes to `PluginProcessor.cpp` as described in section 2.1.
2.  **Implement `isCaptureBuffer` State:** Implement the changes in `Looper.h/cpp` and `PluginProcessor.h/cpp` as described in 3.1.
3.  **Fix Note Duration Bug:** Implement the changes to `Looper.h`, `PluginProcessor.cpp`, and `Looper.cpp` as described in 2.2.
4.  **Overhaul Timeline UI:** Implement the `paint()` method refactor in `TimelineComponent.cpp` as described in 2.3.
5.  **Implement All UI Layout Changes:** Apply all changes described in 3.2.
6.  **Implement "Extend" Looper Mode:** Implement the feature as described in section 4.
7.  **Fix and Run Headless Tests:** Apply the CMake fix from 2.4 and run the tests to ensure everything passes.
8.  **Submit.**

This document should provide a clear path forward.
