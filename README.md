# Navy Arp 2

Navy Arp 2 is an 8-step MIDI arpeggiator and step-sequencer VST3 plugin built with the JUCE framework in C++17. It features a custom hardware-style dark UI measuring exactly 1000 x 681 pixels (expandable to 1300 x 681 pixels), designed for real-time performance, parameter morphing, and tactile playability.

---

## Key Architectural Features

### 1. Dual-Scene Snapshot Morphing (A/B)
The engine maintains two complete, independent sequencer states (`sceneA` and `sceneB`) comprising faders, rotary dial positions, and LFO settings.
* **The Independent Crossfader:** A central horizontal crossfader ("morph" slider) interpolates smoothly between the two scenes (`0.0` = fully Scene A; `1.0` = fully Scene B).
* **Elektron-style Active Focus Deck:** Clicking the `A` or `B` scene buttons (which glow red to indicate focus) routes physical mouse edits and generative "Dice" actions only to the focused scene's snapshot in memory.
* **"Motorized Console" Visual Fader Physics:** When a control is idle, it smoothly follows the crossfader's interpolation in real-time. When a user actively drags a control, the update loop is temporarily bypassed (`.getThumbBeingDragged() < 0`) allowing direct baseline editing. On mouse release, the control glides/snaps back to its morphed visual position.

### 2. Custom Boutique-Hardware Aesthetics
The interface completely moves away from traditional software design to mimic custom hardware:
* **Pristine Cap Design:** Knob caps are rendered as solid, uniform silver metal cylinders, removing the clutter of drawn pointer needles and static anisotropic reflections.
* **Encoder-Style halos:** Circular LED rings display exactly **one lit indicator LED dot** in the active theme color representing the dial's true position.
* **LFO Corona Underglow:** Rather than crowding the interface with visual lines, LFO modulation is represented by a soft, backlit radial "corona" that glows *behind* each small knob cap. 
  * The glow breathes in sync with the real-time phase of the LFO.
  * Its maximum brightness is scaled by LFO depth.
  * Low depth values (like 10%) are mathematically boosted using a square-root curve so they remain highly visible to the user.
* **Symmetric Color Partitioning:** Left-side controls (Time/Rhythm) use the primary theme accent color, while right-side controls (Pitch/Melody) use a contrasting accent color to partition the engine logically.

### 3. Dynamic OLED Monitor Layout
The central monitor window changes context based on your current interaction:
* **Default State:** Displays a rotating 3D wireframe globe, deform-modulated by active LFO phases, with occasional retro space cameos (Voyager, JWST, UFOs) flying across the field. Below the globe, eight gapless, fat vertical columns completely fill the width (680px) showing step probabilities with analog-style phosphor decay.
* **HUD Dashboard Parameter Overlay:** Dragging any control triggers a snappy 1-second technical monitor overlay on the OLED screen. This view displays large, highly legible parameter headers, a digital read-out (formatted into beat divisions, signed octaves, or percentages), and dual 45-segment linear LED meters tracking unmodulated `[BASE]` settings and active `[LFO]` modulation limits.

### 4. Interactive Quick-Start Help Sidecar
To keep instructions accessible without cluttering the main panel, Navy Arp 2 features a collapsible sidecar panel toggled by a small `?` button at the top-right corner.
* Clicking `?` temporarily resizes the editor window from `1000px` to `1300px` wide.
* The sidecar displays a clean, low-clutter quick-start cheat sheet in high-contrast monospace Courier text.
* Each of the four instruction blocks is structured using highly legible, colored "Island Pill" header boxes (Coral, Purple, Cyan, and Amber) for immediate categorization.

### 5. Multi-Theme Panel Integration
The plugin offers three selectable color palettes that dynamically update all graphic elements, text overlays, and backlit coronas:
* **Navy:** Teal accents with a warm amber LFO contrast.
* **Monochrome:** Silver-white accents with a tech cyan LFO contrast.
* **Matrix:** Neon green accents with a hot magenta LFO contrast.

---

## File System & Codebase Layout

The core logic of the visual and DSP layout is implemented across these primary files:
* `PluginProcessor.cpp` / `PluginProcessor.h` — Manages the MIDI arpeggiation, diatonic chord generation, dual-scene snapshots, preset matrix serialization to/from XML, and multi-threaded LFO phase calculations.
* `PluginEditor.cpp` / `PluginEditor.h` — Manages child components, layout dimensions, the 30 fps visual update loop, mouse drag interceptors, and help sidecar rendering.
* `ChromaCapsLookAndFeel.cpp` / `ChromaCapsLookAndFeel.h` — Custom vector drawing class overriding slider knobs, linear faders, text button overrides, circular LED rings, and LFO under-cap radial glows.
* `OledDisplay.cpp` / `OledDisplay.h` — Custom monitor component rendering the 3D globe, gapless VU columns, space cameo simulations, and technical parameter overlay.
* `AppTheme.h` — Global configuration defining color structures for the selected themes.

---

## State Recall & DAW Serialization

To prevent state corruption during loading (especially inside Ableton Live), the serialization model is decoupled from standard real-time UI values:
* Active `sceneA` and `sceneB` states are serialized directly to dedicated `<CURRENT_SCENE_A>` and `<CURRENT_SCENE_B>` XML sub-elements.
* Preset slots 1–8 are serialized to their respective `<SCENE_A_PRESETS>` and `<SCENE_B_PRESETS>` trees, resolving duplicate node lookup errors.
* The `PluginEditor` constructor runs within a properties-bound `isUpdatingProgrammatically` block, shielding loaded scene state values from being overwritten by uninitialized slider attachments.

---

## Build Requirements

* **Framework:** JUCE 7 or JUCE 8 (Audio BASICS, GUI, and Processor modules).
* **Language Standard:** C++17 (or C++20).
* **Build System:** CMake (compatible with Xcode, Visual Studio, and Ninja generators).
