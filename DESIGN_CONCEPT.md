# Creative MIDI Looper: Design & Concept

## 1. High-Level Vision

The core vision is to create a smart, creative MIDI looping tool that goes beyond simple recording and playback. It's designed for musicians and producers to generate musical ideas, evolve them in real-time, and break free from static, repetitive loops. The workflow should feel fluid, musical, and intuitive, minimizing the need for menu diving and maximizing creative flow.

## 2. Core Looping Modes

The looper will operate in two primary modes, selectable by the user, to handle overdubbing of different phrase lengths.

### 2.1. Fixed Mode (Classic Looper)

- **Behavior:** The initial loop's length is sacred.
- **Longer Overdub:** If a user records a phrase longer than the loop, the recording is cut off at the loop's end point. The loop length does not change.
- **Shorter Overdub:** If a user records a shorter phrase, the new notes are simply added to the existing loop. The loop length does not change.
- **Use Case:** Building dense, rhythmic parts where a consistent groove is essential.

### 2.2. Extend Mode (Dynamic Looper)

- **Behavior:** The loop's length can grow dynamically based on the user's performance.
- **Primary Use Case (User Priority):** A short loop (e.g., a 1-bar pattern from Capture) is playing. The user starts recording and plays a longer phrase (e.g., 4 bars). The looper will automatically **extend** the loop's duration to match the new 4-bar length. The original short pattern will be repeated to fill the new space.
- **Goal:** To allow a small idea to seamlessly become the foundation for a longer, more complex phrase. This promotes a fluid workflow where the user can expand a part without stopping the music.

## 3. Melodic Evolution: "Auto-Ornament" Feature

This feature addresses the desire to make loops sound less robotic and more "human."

- **Concept:** When enabled, the feature will automatically add subtle, non-destructive melodic and rhythmic variations to the loop on each repetition.
- **Implementation Ideas:**
    - **Velocity Variation:** Randomizing note velocities slightly on each pass.
    - **Grace Notes:** With a given probability, adding short, quiet "grace notes" before main notes, using the selected musical scale.
    - **Note Substitution:** Occasionally swapping a note for a neighbor within the scale.
    - **Rhythmic Embellishments:** Simple subdivisions, like turning a quarter note into two eighths.
- **UI Control:**
    - An "Enable Auto-Ornament" toggle button.
    - An "Amount" slider to control the intensity and frequency of the variations.

## 4. Future Concepts & Roadmap

### 4.1. Multitrack Functionality

The separation of the "Capture" and "Record" buffers opens the door to a 2-track looping system. A user could have a pattern from the generator playing from the Capture buffer while independently recording and overdubbing a separate part into the Record buffer. This is a potential major evolution for a future version.

### 4.2. Implementation Philosophy

The initial development of these new features will focus on manual control (e.g., explicit buttons for each action). As the features mature, the focus will shift to automating the workflow to create a more seamless and intuitive user experience with minimal clicks required.
