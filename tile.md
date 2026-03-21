# Tile — Workspace Layout System

The tile system is the DAW's window management layer. It allows users to arrange
the five core panels (Playlist, Piano Roll, FX Rack, Mixer, File Browser) across
named layers using tiled splits, floating windows, or overlapping stacks — all
persisted in a human-readable TOML config.

The design is deliberately inspired by tiling window managers (Hyprland, i3) and
intentionally avoids the fixed-layout assumptions of FL Studio or Ableton.

---

## Concepts

### Panel

A panel is a logical, stateful UI component. There are five:

| ID             | Description                     |
|----------------|---------------------------------|
| `playlist`     | Arrangement / clip launcher     |
| `piano_roll`   | MIDI note editor                |
| `fx_rack`      | Effect chain / plugin browser   |
| `mixer`        | Channel strip mixer             |
| `file_browser` | File/sample/preset browser      |

Panels are **permanent singleton objects** owned by the `WorkspaceComponent`.
They are created once at startup and never destroyed. A layer does not own a
panel — it only describes where and how to display it. The same panel can appear
in multiple layers with different sizes.

### Layer

A layer is a named viewport configuration: a tree describing how panels are
arranged within the screen bounds. Layers are analogous to workspaces in a
tiling WM. The user switches between them via keybind.

### Workspace

The workspace is the top-level object that owns all panels and all layers, and
tracks which layer is currently active. It is the root JUCE component that fills
the application window.

---

## Layout Tree

Each layer holds a layout tree. Every node in the tree is one of three variants:

```
LayoutNode = SplitNode | PanelLeaf | FloatNode
```

### SplitNode

Divides its allocated rectangle into N sub-regions along one axis. Each child
has an associated ratio (all ratios in a node sum to 1.0). Children are
themselves `LayoutNode`s, allowing arbitrary nesting.

```
SplitNode {
  direction : Horizontal | Vertical
  children  : [(LayoutNode, ratio: f32)]
}
```

### PanelLeaf

A terminal node that occupies its allocated rectangle with a single panel.

```
PanelLeaf {
  panel   : PanelId
  visible : bool
}
```

`visible = false` reserves the panel's slot in the tree but renders nothing.
This allows a panel to be hidden without restructuring the tree.

### FloatNode

A panel that lives outside the tiled tree, positioned absolutely within the
screen bounds. Float nodes are stored in a separate ordered list per layer (not
inside the split tree) and rendered on top of the tiled content.

```
FloatNode {
  panel   : PanelId
  x       : f32   // normalized 0..1 relative to screen width
  y       : f32   // normalized 0..1 relative to screen height
  w       : f32
  h       : f32
  z_order : i32   // higher = rendered on top
}
```

---

## Layout Engine

The layout engine is a pure computation layer with no JUCE dependency. Given a
`LayoutNode` tree and a bounding `Rectangle<int>`, it recursively computes a
flat map of `PanelId -> Rectangle<int>`.

```cpp
// layout_engine.h
class LayoutEngine {
public:
  using BoundsMap = std::unordered_map<PanelId, juce::Rectangle<int>>;

  BoundsMap compute(const LayoutNode& root,
                    juce::Rectangle<int> screen) const;

private:
  void visit(const LayoutNode& node,
             juce::Rectangle<int> rect,
             BoundsMap& out) const;
};
```

`WorkspaceComponent::resized()` calls `compute()` and calls `setBounds()` on
each panel component using the resulting map. Float nodes are handled separately:
their normalized coordinates are multiplied by screen dimensions at resize time.

This separation means the entire layout logic is unit-testable without a display.

---

## Layer Mutations

Three operations mutate a layer's tree:

### Toggle panel

`workspace.toggle` has two modes, determined by the panel's configuration:

**Tree-insert mode** (default for `playlist`, `piano_roll`, `fx_rack`, `mixer`):
Inserts or removes a `PanelLeaf` from the tiled tree. When inserting, the new
leaf is added as a sibling of a specified anchor panel within its parent
`SplitNode`, and sibling ratios are rebalanced proportionally. When removing,
sibling ratios expand to fill the vacated space.

The mixer toggle is expressed as:

```
// Base state of layer "arrange"
SplitH [
  (playlist, 0.8),
  (fx_rack,  0.2)
]

// After toggle_panel(mixer, anchor=fx_rack, side=Left)
SplitH [
  (playlist, 0.6),
  (mixer,    0.2),
  (fx_rack,  0.2)
]
```

The inserted ratio and rebalancing strategy are configurable per toggle binding.

**Transient-float mode** (default for `file_browser`):
Shows or hides the panel as a temporary float without modifying the layer's
tiled tree. The panel slides in from a configured edge, overlays the existing
layout, and dismisses without leaving any trace in the tree. Controlled via the
`[panels.<id>]` config block (see Config Format). Any panel can opt into this
mode by setting `toggle_mode = "float"` in its panel config.

### Resize split

Adjusts the ratios of two adjacent siblings in a `SplitNode`. Triggered by the
user dragging a resize handle. The ratio delta is clamped so neither child falls
below a minimum size (default: 60px).

### Move panel to float

Removes a `PanelLeaf` from the tiled tree and creates a corresponding `FloatNode`
at a specified position. The inverse operation (float to tile) inserts it back at
a target location.

---

## Config Format

Layouts are stored in `~/.config/daw/layout.toml`. The file is written on every
resize-handle release and layer mutation. Users can edit it by hand; the app
reloads it on focus regain if the mtime has changed.

### Example

```toml
[workspace]
active_layer = "arrange"

[layers.piano_roll]
[layers.piano_roll.root]
type  = "panel"
panel = "piano_roll"

[layers.arrange]
[layers.arrange.root]
type      = "split"
direction = "H"

[[layers.arrange.root.children]]
panel = "playlist"
ratio = 0.8

[[layers.arrange.root.children]]
panel = "fx_rack"
ratio = 0.2

[layers.arrange.mixer_variant]
# A named variant of the arrange layer with mixer injected.
# Activated by a keybind rather than being a separate layer.
insert_panel  = "mixer"
anchor        = "fx_rack"
side          = "left"
ratio         = 0.2
# Remaining siblings rebalance proportionally unless overridden.

[layers.arrange.floats]
# Optional floating panels on this layer.
[[layers.arrange.floats]]
panel   = "fx_rack"
x       = 0.65
y       = 0.05
w       = 0.34
h       = 0.90
z_order = 1
```

### Schema rules

- `ratio` values within a `children` array must sum to `1.0`. The loader
  normalizes them if they do not, and logs a warning.
- `panel` values must be one of: `playlist`, `piano_roll`, `fx_rack`, `mixer`,
  `file_browser`.
- A panel may appear at most once in the tiled tree per layer. It may
  additionally appear as a float on the same layer.
- `z_order` values do not need to be contiguous; only relative order matters.
- `[panels.<id>]` blocks configure per-panel defaults and toggle behavior.
  Valid keys: `toggle_mode` (`"tile"` | `"float"`), `default_float`
  (`{x, y, w, h}` normalized), `slide_from` (`"left"` | `"right"` | `"top"` |
  `"bottom"`). Omitting a `[panels.<id>]` block uses built-in defaults.

---

## Keybind Integration

Layer switching and panel toggles are first-class keybind actions. The keybind
system (see `keybinds.md`, forthcoming) dispatches named actions to the
workspace:

| Action                        | Description                                 |
|-------------------------------|---------------------------------------------|
| `workspace.layer <name>`      | Switch to named layer                       |
| `workspace.layer_next`        | Cycle to next layer                         |
| `workspace.layer_prev`        | Cycle to previous layer                     |
| `workspace.toggle <panel>`    | Toggle panel visibility in current layer    |
| `workspace.float <panel>`     | Move panel to float on current layer        |
| `workspace.tile <panel>`      | Return floating panel to tiled tree         |
| `workspace.variant <name>`    | Activate a named variant of current layer   |

Default bindings (all overridable in `keybinds.toml`):

```toml
[keybinds]
"Cmd+1" = "workspace.layer piano_roll"
"Cmd+2" = "workspace.layer arrange"
"Cmd+3" = "workspace.layer mixer_full"
"Cmd+M" = "workspace.variant mixer_variant"
"Cmd+F" = "workspace.toggle file_browser"
```

Modifier key names follow platform convention: `Cmd` on macOS, `Ctrl` on
Windows and Linux. The keybind system resolves these automatically — config
files use the macOS name as the canonical form (see `keybinds.md`).

### File browser toggle behavior

The file browser is designed as an **overlay toggle**, not a persistent layer
member. When `workspace.toggle file_browser` fires:

- If not visible: the browser slides in as a float anchored to the left edge of
  the screen (default width: 20% of screen, full height). It does not disturb
  the tiled layout of the active layer.
- If already visible: it dismisses. The previously focused panel regains focus.

Users who prefer it docked can override the default by adding it to a layer's
tiled tree in `layout.toml` like any other panel.

The default float position for the browser is stored as a special key so it
persists between toggles without being a permanent float entry:

```toml
[panels.file_browser]
default_float = { x = 0.0, y = 0.0, w = 0.20, h = 1.0 }
slide_from    = "left"   # animation direction: left | right | top | bottom
```

---

## JUCE Component Hierarchy

```
WorkspaceComponent  (fills MainWindow, owns all panels and layers)
├── TiledContainer  (renders the active layer's split tree)
│   ├── SplitHandle (draggable divider between siblings, one per split boundary)
│   └── [panel components as direct children, bounds set by LayoutEngine]
└── FloatContainer  (renders float nodes for the active layer, drawn on top)
    └── FloatFrame  (draggable/resizable wrapper around a panel)
        └── [panel component]
```

`WorkspaceComponent` never deletes or re-parents panel components when switching
layers. It only calls `setVisible(false)` on panels not present in the incoming
layer and `setBounds()` / `setVisible(true)` on panels that are.

---

## Resize Persistence

When the user releases a `SplitHandle` after dragging:

1. The new pixel position is converted back to a ratio within the parent
   `SplitNode`.
2. The ratio is clamped (minimum panel size enforced).
3. The in-memory `LayoutNode` tree is updated.
4. `layout.toml` is written atomically (write to `.tmp`, then rename).

Float panel bounds follow the same pattern: on drag/resize end, normalized
coordinates are computed and written to config.

This means the config file is always a complete, valid snapshot of the current
layout. Losing the process mid-session loses at most one drag gesture.

---

## Panel Focus

At any time, exactly one panel holds keyboard focus. The focused panel is tracked
by `WorkspaceComponent` and used to resolve panel-relative keybind actions (e.g.
`workspace.float focused_panel`). Focus transfers on click or via a configurable
keybind (`Super+Tab` by default cycles focus through visible panels in the active
layer).

The focused panel renders a configurable highlight border (color defined in
`theme.toml`).

---

## Implementation Phases

| Phase | Deliverable |
|-------|-------------|
| 1 | `LayoutNode` data model, `LayoutEngine::compute()`, full unit test coverage |
| 2 | TOML serialization — load and save round-trips cleanly |
| 3 | `WorkspaceComponent` + `TiledContainer` — tiled layout rendering, layer switching |
| 4 | `SplitHandle` — drag to resize, ratio persistence |
| 5 | `FloatContainer` + `FloatFrame` — floating panels, drag/resize, z-order |
| 6 | Keybind actions — toggle, float, tile, variant |
| 7 | Hot-reload — detect external edits to `layout.toml`, reload without restart |
