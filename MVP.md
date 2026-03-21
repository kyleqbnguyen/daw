# MVP — Technical Breakdown

This document defines the technical scope, architecture, and implementation plan
for the MVP release. It is the authoritative reference for what is and is not
being built, and in what order.

---

## Scope

### In Scope

| Area | Features |
|------|----------|
| **Workspace / Tile** | Full system: TOML config, tiled + float + overlap layouts, named layers, layer switching, drag-to-resize, keybinds, ratio persistence. See `tile.md` for full spec. |
| **Transport** | Play / Stop / Record, BPM, time signature, loop region, playhead scrubbing |
| **Playlist** | Audio + MIDI tracks, clip display, timeline ruler, track add/remove/reorder, waveform rendering, audio recording |
| **Piano Roll** | Note draw / erase / move, velocity bar editor, quantize, playback preview. No live MIDI recording. |
| **Mixer** | Per-track fader, pan, mute/solo, VU meters, send routing, master bus strip |
| **FX Rack** | Per-track insert chain, VST3/AU plugin scan and hosting, plugin editor window |
| **Keybinds** | `keybinds.toml` load/save, named action dispatch, platform modifier mapping |
| **Config** | `layout.toml` and `keybinds.toml` under `~/.config/daw/`, hot-reload on external edit |

### Explicitly Deferred

| Feature | Reason |
|---------|--------|
| File Browser panel | Convenience — not a workflow blocker for MVP |
| Live MIDI recording | Note drawing covers MVP; hardware input is added complexity |
| Automation lanes | Significant standalone feature |
| Comping / multi-take lanes | Post-MVP |
| Time stretching / warping | Post-MVP |
| Theme configuration | Ships with one default dark theme; `theme.toml` deferred |

---

## Architecture Overview

```
DAWApplication  (juce::JUCEApplication)
└── MainWindow  (juce::DocumentWindow)
    └── WorkspaceComponent  (root UI, owns all panels and layers)
        ├── TiledContainer   (renders active layer's split tree)
        │   ├── SplitHandle  (drag-to-resize dividers)
        │   └── [panel components — bounds set by LayoutEngine]
        ├── FloatContainer   (float nodes, rendered above tiled content)
        │   └── FloatFrame   (drag/resize wrapper)
        └── [Panel singletons — created once, never destroyed]
            ├── PlaylistPanel
            ├── PianoRollPanel
            ├── MixerPanel
            └── FxRackPanel
```

**Key invariants:**

- `WorkspaceComponent` is the single owner of all panel components. Panels are
  never destroyed or re-parented when switching layers — only `setVisible()` and
  `setBounds()` are called.
- `LayoutEngine` has zero JUCE dependency. It is a pure C++ computation layer
  that maps a `LayoutNode` tree and a screen rectangle to a flat
  `PanelId → Rectangle<int>` map.
- All config I/O (layout, keybinds) is isolated to a `ConfigManager` class.
  No panel or workspace component reads or writes files directly.
- Tracktion Engine objects (`te::Engine`, `te::Edit`, `te::TransportControl`) are
  owned by a `Project` class, not by any UI component. Panels hold non-owning
  references.

---

## Component Specifications

### 1. Layout Engine

Pure C++ — no JUCE headers. See `tile.md` for the full data model and algorithm.

**Relevant types:**

```cpp
enum class PanelId { Playlist, PianoRoll, Mixer, FxRack };

struct SplitNode { ... };
struct PanelLeaf { ... };
struct FloatNode  { ... };
using  LayoutNode = std::variant<SplitNode, PanelLeaf, FloatNode>;

class LayoutEngine {
public:
  using BoundsMap = std::unordered_map<PanelId, juce::Rectangle<int>>;
  BoundsMap compute(const LayoutNode& root,
                    juce::Rectangle<int> screen) const;
};
```

**Files:** `src/layout/layout_node.h`, `src/layout/layout_engine.h/.cpp`

---

### 2. Workspace / Tile System

Implements the full spec from `tile.md`. MVP includes:

- Tiled layouts (SplitNode tree, arbitrary nesting)
- Float layouts (FloatNode list per layer, z-ordered)
- Named layers with TOML-persisted configs
- Layer switching via keybind
- `SplitHandle` drag-to-resize (min panel size: 60px, ratio persisted on release)
- `workspace.toggle` in both tree-insert and transient-float modes

MVP excludes:
- Hot-reload of `layout.toml` on external edit (deferred to post-MVP polish)
- Visual layout editor

**Files:** `src/workspace/workspace_component.h/.cpp`,
`src/workspace/tiled_container.h/.cpp`,
`src/workspace/split_handle.h/.cpp`,
`src/workspace/float_container.h/.cpp`,
`src/workspace/float_frame.h/.cpp`

---

### 3. Keybind System

Actions are named strings dispatched to registered handlers. No hardcoded
shortcuts exist in panel code — all bindings go through the keybind system.

**Action format:** `<domain>.<action> [arg]`
Examples: `workspace.layer arrange`, `workspace.toggle mixer`,
`transport.play`, `piano_roll.quantize`

**Config:** `~/.config/daw/keybinds.toml`

```toml
[keybinds]
"Cmd+1" = "workspace.layer piano_roll"
"Cmd+2" = "workspace.layer arrange"
"Cmd+M" = "workspace.variant mixer_variant"
"Cmd+F" = "workspace.toggle file_browser"
"Space" = "transport.play_stop"
"Cmd+R" = "transport.record"
```

Platform modifier mapping: `Cmd` (macOS config canonical) → `Ctrl` on
Windows/Linux. Resolved at load time; panels and actions never see raw
platform modifiers.

**Files:** `src/keybinds/keybind_manager.h/.cpp`,
`src/keybinds/action_dispatcher.h/.cpp`

---

### 4. Config Manager

Owns all file I/O for `layout.toml` and `keybinds.toml`. Writes atomically
(write to `.tmp` then `rename()`). Exposes a simple load/save API; no panel
reads a file directly.

TOML parsing via a vendored or FetchContent-fetched header-only library
(`toml++` recommended — single header, no dependencies, C++17/20).

**Files:** `src/config/config_manager.h/.cpp`

---

### 5. Transport

Thin UI layer over Tracktion's `te::TransportControl`. The transport bar is a
fixed component docked at the top of `WorkspaceComponent` (not part of the
tiling tree — always visible).

**Controls:**
- Play / Stop / Record buttons
- BPM spinner (double-click to type, scroll to nudge)
- Time signature selector
- Loop toggle + loop region (set via drag on the playlist ruler)
- Playhead position display (bars:beats:ticks and MM:SS)
- Metronome toggle

**Tracktion integration:**

```cpp
// Playback
edit.getTransport().play(false);
edit.getTransport().stop(false, false);

// Recording
edit.getTransport().record(false);

// BPM
edit.tempoSequence.getTempos()[0]->setBpm(bpm);
```

**Files:** `src/transport/transport_bar.h/.cpp`

---

### 6. Playlist Panel

Arrangement view with a vertical track list and a horizontal timeline.

**Subcomponents:**

| Component | Responsibility |
|-----------|---------------|
| `TrackHeader` | Track name, arm button, mute/solo, height resize handle |
| `TrackLane` | Clip rendering area for one track |
| `AudioClipView` | Waveform thumbnail (JUCE `AudioThumbnail`) |
| `MidiClipView` | Note density preview (mini piano roll overview) |
| `TimelineRuler` | Bar/beat markers, loop region drag handles |
| `PlayheadOverlay` | Animated playhead line, redraws on timer |

**Tracktion integration:**

```cpp
// Add audio track
auto* track = edit.insertNewAudioTrack(
    te::TrackInsertPoint{nullptr, edit.getTopLevelTracks().getLast()}, nullptr);

// Add MIDI track
auto* track = edit.insertNewMidiTrack(...);

// Audio recording: arm track
track->setCurrentlyRecording(true);

// Enumerate clips
for (auto* clip : track->getClips())
    renderClip(clip);
```

Waveform thumbnails are generated asynchronously via JUCE's
`AudioThumbnailCache`. Each `AudioClipView` registers as a
`ChangeListener` on its thumbnail and repaints when data arrives.

**Files:** `src/panels/playlist/playlist_panel.h/.cpp`,
`src/panels/playlist/track_header.h/.cpp`,
`src/panels/playlist/track_lane.h/.cpp`,
`src/panels/playlist/audio_clip_view.h/.cpp`,
`src/panels/playlist/midi_clip_view.h/.cpp`,
`src/panels/playlist/timeline_ruler.h/.cpp`

---

### 7. Piano Roll Panel

Opens when a MIDI clip is double-clicked in the Playlist. Displays notes as
rectangles on a pitch/time grid.

**Subcomponents:**

| Component | Responsibility |
|-----------|---------------|
| `NoteGrid` | Main edit area — draw, erase, move, resize notes |
| `PianoKeyboard` | Left-side keyboard strip, highlights active notes during playback |
| `VelocityEditor` | Bottom strip — bars per note, draggable |
| `PianoRollRuler` | Top timeline ruler, zoom control |

**Edit modes** (toggled via keybind or toolbar):
- Draw — click to create note, drag to set length
- Erase — click to delete
- Select — marquee select, then move/resize/delete

**Quantize:** snaps note start and length to a configurable grid
(1/4, 1/8, 1/16, 1/32, triplets). Applied on note creation and via
explicit quantize action.

**Tracktion integration:**

```cpp
// Active MIDI clip
auto* midiClip = dynamic_cast<te::MidiClip*>(selectedClip);
auto& seq = midiClip->getSequence();

// Add note
seq.addNote(pitch, startBeat, lengthInBeats, velocity, 0, nullptr);

// Remove note
seq.removeNote(*note, nullptr);
```

**Files:** `src/panels/piano_roll/piano_roll_panel.h/.cpp`,
`src/panels/piano_roll/note_grid.h/.cpp`,
`src/panels/piano_roll/piano_keyboard.h/.cpp`,
`src/panels/piano_roll/velocity_editor.h/.cpp`

---

### 8. Mixer Panel

One channel strip per track, plus a master bus strip. Laid out horizontally.

**Channel strip subcomponents:**

| Control | Tracktion API |
|---------|--------------|
| Fader (dB) | `track->getVolumeAndPanPlugin()->setVolumeDb(db)` |
| Pan knob | `track->getVolumeAndPanPlugin()->setPan(pan)` |
| Mute button | `track->setMute(true)` |
| Solo button | `edit.setTrackSolo(track, true)` |
| VU meter | Read peak levels from `te::LevelMeasurer` on audio node output |
| Send knob | `te::AuxSendPlugin` volume per send bus |
| Track label | Mirrors track name from the Edit |

VU meters update on a `juce::Timer` at ~30fps. Peak hold decays over 2 seconds.

**Files:** `src/panels/mixer/mixer_panel.h/.cpp`,
`src/panels/mixer/channel_strip.h/.cpp`,
`src/panels/mixer/vu_meter.h/.cpp`

---

### 9. FX Rack Panel

Per-track insert chain. Displays the plugin chain for the focused track
(track focus shared with Playlist via a `FocusedTrackBroadcaster`).

**Layout:**

```
┌──────────────────────────────────┐
│  Track: Synth Lead               │
├──────────────────────────────────┤
│  1. EQ Eight        [Edit] [x]   │
│  2. Compressor      [Edit] [x]   │
│  3. Reverb          [Edit] [x]   │
│  + Add Plugin                    │
└──────────────────────────────────┘
```

**Plugin management:**

```cpp
// Scan plugins (run once, cache results)
engine.getPluginManager().knownPluginList.scanAndAddFile(...);

// Add plugin to track
auto* plugin = edit.getPluginCache().createNewPlugin(
    te::ExternalPlugin::xmlTypeName, pluginDescription);
track->pluginList.insertPlugin(plugin, insertIndex, nullptr);

// Open editor
plugin->showWindowExplicitly();
```

Plugin scan results are persisted to `~/.config/daw/plugins.xml` (JUCE's
`KnownPluginList` serialization). Scan runs in a background thread on first
launch and on explicit re-scan. UI shows progress.

**Files:** `src/panels/fx_rack/fx_rack_panel.h/.cpp`,
`src/panels/fx_rack/plugin_slot.h/.cpp`,
`src/panels/fx_rack/plugin_scanner.h/.cpp`

---

### 10. Project / Edit Management

Owns the Tracktion Engine, the active Edit, and the audio device manager.
All panels hold a non-owning `Project&` reference.

**Responsibilities:**
- New project: creates a blank `te::Edit`
- Open project: deserializes Edit from `.daw` file (Tracktion's XML format)
- Save / Save As: serializes Edit to file
- Audio device setup: exposes `te::Engine::getDeviceManager()`

**File format:** `.daw` — Tracktion's native XML-based Edit format.
No custom format needed for MVP; Tracktion handles serialization.

**Files:** `src/project/project.h/.cpp`

---

## File Structure

```
src/
├── main.cpp
├── main_component.h/.cpp          (transitional — replaced by WorkspaceComponent)
├── layout/
│   ├── layout_node.h
│   ├── layout_engine.h/.cpp
│   └── layer.h
├── workspace/
│   ├── workspace_component.h/.cpp
│   ├── tiled_container.h/.cpp
│   ├── split_handle.h/.cpp
│   ├── float_container.h/.cpp
│   └── float_frame.h/.cpp
├── keybinds/
│   ├── keybind_manager.h/.cpp
│   └── action_dispatcher.h/.cpp
├── config/
│   └── config_manager.h/.cpp
├── transport/
│   └── transport_bar.h/.cpp
├── project/
│   └── project.h/.cpp
└── panels/
    ├── playlist/
    │   ├── playlist_panel.h/.cpp
    │   ├── track_header.h/.cpp
    │   ├── track_lane.h/.cpp
    │   ├── audio_clip_view.h/.cpp
    │   ├── midi_clip_view.h/.cpp
    │   └── timeline_ruler.h/.cpp
    ├── piano_roll/
    │   ├── piano_roll_panel.h/.cpp
    │   ├── note_grid.h/.cpp
    │   ├── piano_keyboard.h/.cpp
    │   └── velocity_editor.h/.cpp
    ├── mixer/
    │   ├── mixer_panel.h/.cpp
    │   ├── channel_strip.h/.cpp
    │   └── vu_meter.h/.cpp
    └── fx_rack/
        ├── fx_rack_panel.h/.cpp
        ├── plugin_slot.h/.cpp
        └── plugin_scanner.h/.cpp
```

---

## Implementation Phases

| Phase | Deliverable | Depends On |
|-------|-------------|------------|
| 1 | `LayoutNode` model, `LayoutEngine::compute()`, unit tests | — |
| 2 | TOML serialization — `ConfigManager`, layout load/save round-trip | Phase 1 |
| 3 | `WorkspaceComponent` + `TiledContainer` — panel stubs, layer switching | Phase 2 |
| 4 | `SplitHandle` — drag-to-resize, ratio persistence | Phase 3 |
| 5 | `KeybindManager` + `ActionDispatcher` — keybinds.toml, action routing | Phase 3 |
| 6 | `Project` — Engine init, new/open/save Edit, audio device setup | Phase 3 |
| 7 | `TransportBar` — play/stop/record/BPM wired to Tracktion | Phase 6 |
| 8 | `PlaylistPanel` — tracks, clip display, timeline ruler | Phase 7 |
| 9 | Audio recording — arm track, record, waveform display | Phase 8 |
| 10 | `MixerPanel` — channel strips, fader/pan/mute/solo, VU meters | Phase 8 |
| 11 | `FxRackPanel` — plugin scan, insert chain, open editor window | Phase 8 |
| 12 | `PianoRollPanel` — note grid, velocity editor, quantize | Phase 8 |
| 13 | `FloatContainer` + `FloatFrame` — float layouts, drag/resize/z-order | Phase 4 |
| 14 | Integration pass — cross-panel focus, send routing, end-to-end test | Phase 12 |

---

## Third-Party Dependencies to Add

| Library | Purpose | Integration |
|---------|---------|-------------|
| `toml++` | TOML parsing for layout and keybind configs | FetchContent, header-only |

All other functionality is covered by JUCE 8 and Tracktion Engine 3 which are
already in the build.

---

## Key Technical Decisions

**`LayoutEngine` is JUCE-free.** This keeps it unit-testable without a display
and decouples the layout math from the rendering layer entirely.

**Panels are permanent singletons.** `WorkspaceComponent` never destroys or
re-parents a panel on layer switch. Only `setVisible()` and `setBounds()` change.
This avoids teardown/rebuild cost and preserves panel state (scroll position,
selection, zoom) across layer switches.

**Transport bar is outside the tile tree.** It docks at the top of
`WorkspaceComponent` and always remains visible regardless of the active layer.
Its height is subtracted from the rectangle passed to `LayoutEngine`.

**Focused track is shared state.** `PlaylistPanel`, `MixerPanel`, and
`FxRackPanel` all reflect the same focused track. This is managed via a
`FocusedTrackBroadcaster` (a `juce::ChangeBroadcaster` subclass) owned by
`WorkspaceComponent`. Panels register as listeners and update on change.

**Plugin editor windows are native.** Tracktion opens VST3/AU editors in their
own OS windows via `plugin->showWindowExplicitly()`. No attempt is made to
embed them inside the DAW window for MVP.

**Config writes are atomic.** All writes to `layout.toml` and `keybinds.toml`
go through `ConfigManager` which writes to a `.tmp` sibling then calls
`juce::File::moveFileTo()` to replace atomically. This prevents corrupt configs
on crash.
