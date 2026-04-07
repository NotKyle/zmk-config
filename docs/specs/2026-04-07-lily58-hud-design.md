# Lily58 Keyboard HUD — Design Spec

**Date:** 2026-04-07  
**Status:** Approved  

---

## Overview

A floating always-on-top keyboard HUD for macOS that shows the user's Lily58 split keyboard layout in real time. Built with Hammerspoon (Lua) + WebView (HTML/CSS/JS). Helps the user learn their custom ZMK keymap by visualising the current layer and highlighting keys as they are pressed.

---

## Goals

- Show the full Lily58 layout in a bottom-right corner overlay
- Highlight the last pressed key by physical position
- Infer the active ZMK layer from key output and update the display
- Three switchable view modes (selectable in-HUD)
- No build step, no dependencies beyond Hammerspoon

---

## Non-Goals

- Direct ZMK firmware communication
- Perfect layer detection accuracy (inference is best-effort)
- Tracking held keys / chords beyond single-key highlight
- Working inside apps with Secure Input enabled (VM, some terminals) — documented as a known limitation

---

## Architecture

Two files, both in `~/.hammerspoon/`:

```
~/.hammerspoon/
  init.lua              # existing — add: require("lily58-hud")
  lily58-hud.lua        # event logic, layer inference, WebView management
  lily58-hud.html       # keyboard layout, CSS, JS update handler (single self-contained file)
```

`lily58-hud.html` must be fully self-contained (no external CSS or JS files) because WebKit blocks cross-file local resource loads from `file://` URLs.

**Data flow:**

```
macOS key event
  → hs.eventtap keyDown (lily58-hud.lua)
    → getKeyID(keyCode) → key position ID
    → inferLayer(keyCode, modifiers) → layer index
    → debounced evaluateJavaScript("hud.update(keyID, layerIdx, viewMode)")
      → lily58-hud.html updates DOM
```

---

## Components

### 1. Key Event Watcher

Uses `hs.eventtap.new({hs.eventtap.event.types.keyDown}, handler)`. The handler always returns `false` so events pass through to the active app unmodified.

For each event:
- `keyCode = event:getKeyCode()` — integer HID usage code
- `mods = event:getFlags()` — table of active modifiers
- Calls inferLayer, then debounces the JS update call (see §5)

`event:getCharacters()` is **not** used for layer inference — keyCodes are used directly (see §3).

### 2. Key Position ID Scheme

The HUD needs to map a `keyCode` to a DOM element. Each key `<div>` in the HTML has an `id` of the form:

```
id="k-{side}-{row}-{col}"
```

Where `side` is `L` or `R`, `row` is 0–4, `col` is 0–5 (plus thumb positions `T0`–`T3`). Examples: `k-L-1-2` (left row 1 col 2 = W), `k-R-2-0` (right row 2 col 0 = H).

`lily58-hud.lua` contains a lookup table `KEY_POSITIONS` mapping macOS keyCodes to these IDs:

```lua
KEY_POSITIONS = {
  [6]  = "k-L-1-1",  -- Z
  [13] = "k-L-1-2",  -- W
  -- ... all 58 physical key positions
}
```

Keys not in the table (e.g. media keys) are silently ignored.

### 3. Layer Inferencer

Layer is inferred from `keyCode` and `mods`, **not** from the output character. This avoids the ambiguity of Shift+number producing `!` etc.

Rules checked in priority order:

| Condition | Inferred layer | Display timeout |
|-----------|---------------|-----------------|
| keyCode in F1–F12 range (0x7a, 0x78, 0x63, 0x76, 0x60, 0x61, 0x62, 0x64, 0x65, 0x6d, 0x67, 0x6f) | L2 Numbers+Fn | 2s |
| keyCode in arrow/Home/End/PgUp/PgDn set | L3 Navigation | 2s |
| keyCode maps to a key that exists **only** on L1 (e.g. keycodes for `[`, `]`, `\` without shift — physical keys that produce L1 symbols directly from ZMK) | L1 Symbols | 2s |
| Anything else | L0 Default | permanent |

The L1 keycode set is the physical keys whose L1 binding differs from their default-layer binding and whose L1 output has no Shift equivalent on L0. This set is defined as a Lua table in `lily58-hud.lua` alongside the layer data.

After the timeout with no further layer-specific keys, the display reverts to L0. Each layer-specific keypress resets the timer.

### 4. WebView Manager

```lua
local wv = hs.webview.new(frame, {developerExtrasEnabled = false})
wv:windowStyle(hs.webview.windowMasks.borderless)
wv:level(hs.canvas.windowLevels.floating)  -- hs.canvas, NOT deprecated hs.drawing
wv:alpha(0.92)
wv:allowTextEntry(false)
wv:transparent(true)
wv:url("file://" .. hs.configdir .. "/lily58-hud.html")
wv:show()
```

**Position:** bottom-right of `hs.screen.mainScreen()` (the screen with the currently focused window), 20px from right and bottom edges. On reload, stored `{x, y}` is validated against current screen bounds; if out of bounds, defaults to bottom-right of mainScreen.

**Dragging:** Implemented via JS `mousedown`/`mousemove` inside the HTML. The HTML sends the new position to Lua via a `lily58hud://move?x=N&y=N` URL navigation, intercepted by `wv:navigationCallback`. Lua calls `wv:topLeft({x, y})` and persists the value to `hs.settings`.

### 5. Debouncing JS Updates

`evaluateJavaScript` is asynchronous. To avoid queuing updates faster than the WebView can process at high typing speeds, updates are debounced at 16ms (one frame at 60fps):

```lua
local pending = nil
local function scheduleUpdate(keyID, layer, mode)
  if pending then pending:stop() end
  pending = hs.timer.doAfter(0.016, function()
    wv:evaluateJavaScript(
      string.format("hud.update('%s',%d,'%s')", keyID, layer, mode),
      function() end  -- discard result
    )
    pending = nil
  end)
end
```

### 6. HUD Renderer (`lily58-hud.html`)

Single self-contained file. Exposes one global function:

```js
hud.update(keyID, layerIdx, viewMode)
// keyID:    string matching a DOM id, e.g. "k-L-1-5"
// layerIdx: 0|1|2|3
// viewMode: "full"|"diff"|"strip"
```

On each call:
1. Remove `.active` class from previously highlighted key
2. Apply `.active` class to `document.getElementById(keyID)`
3. Set a 600ms timeout to remove `.active`
4. If `layerIdx` changed: swap the visible layer dataset (each key `<div>` carries `data-l0`, `data-l1`, `data-l2`, `data-l3` label attributes; JS updates `textContent` and classes from these)
5. Update the layer label colour (L0=blue, L1=pink, L2=green, L3=orange)
6. Apply `viewMode` class to the root container

### 7. Layer Data Schema

Each key entry in the JS `LAYERS` constant is an object:

```js
{ label: "A", mod: "⌘", modColor: "#4fc3f7" }
// label:    display string for the key in this layer
// mod:      optional modifier badge (HRM or layer indicator)
// modColor: badge colour (omit if no badge)
```

Empty/transparent keys: `{ label: "—", dim: true }`

The full `LAYERS` array (indices 0–3) is defined once in the HTML and never changes at runtime.

### 8. View Modes

Three modes, cycled by clicking the `■■` icon in the HUD:

| Mode | Rendered content |
|------|-----------------|
| `full` | Both halves, all 58 keys, full layer overlay |
| `diff` | Only keys where `layers[current][i].label !== layers[0][i].label` — i.e. keys whose binding changes from L0. Dim keys are excluded. |
| `strip` | No keyboard grid — layer name, last key label (large), active mod indicators (⌘⌥⌃⇧ lit when inferred from HRM positions), one hint line |

Mode is persisted via `hs.settings.set("lily58hud.viewMode", mode)` and read on init.

---

## Teardown & Reload Safety

The module uses a guard at the top of `lily58-hud.lua`:

```lua
if _G._lily58hud then
  _G._lily58hud.eventtap:stop()
  _G._lily58hud.webview:delete()
end
_G._lily58hud = { eventtap = ..., webview = ... }
```

This ensures a clean restart on every Hammerspoon config reload with no duplicate watchers or orphaned windows.

---

## Settings Persistence

`hs.settings` keys:

| Key | Type | Default |
|-----|------|---------|
| `lily58hud.viewMode` | string | `"full"` |
| `lily58hud.position` | table `{x,y}` | bottom-right of mainScreen |

---

## Known Limitations

- **Secure Input apps** (some terminals, password managers, VMs): `hs.eventtap` receives no events. HUD will be frozen. No workaround — document in README.
- **Layer inference is reactive**: the display shows the inferred layer *after* a key on that layer is typed, not before.
- **Accessibility permission**: if revoked at runtime, events stop silently. No watchdog is implemented — the user must notice and re-grant.
- **Multi-monitor hot-unplug**: if the monitor holding the HUD is disconnected, macOS moves the window off-screen. On next reload, the bounds check corrects position to mainScreen.

---

## Installation

1. `brew install --cask hammerspoon`
2. Open Hammerspoon → System Settings → Privacy → Accessibility → enable
3. Copy `lily58-hud.lua` and `lily58-hud.html` to `~/.hammerspoon/`
4. Add `require("lily58-hud")` to `~/.hammerspoon/init.lua`
5. Reload: menubar icon → Reload Config (or `⌘⌥⌃R`)
