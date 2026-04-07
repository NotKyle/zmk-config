# Lily58 Keymap Reference

> **Nice Nano v2** — ZMK Firmware  
> 4 layers · Home Row Mods · Caps Word · Nav Layer

---

## Layer Overview

| # | Name | Access |
|---|------|--------|
| 0 | Default | Always active |
| 1 | Symbols | Hold `ENT` (right thumb) |
| 2 | Numbers + Fn | `SPC` + `ENT` combo, or hold inner-right key |
| 3 | Navigation | Hold inner-left key |

---

## Layer 0 — Default

```
┌──────┬─────┬─────┬─────┬─────┬─────┐                 ┌─────┬─────┬─────┬─────┬─────┬──────┐
│ ESC  │  1  │  2  │  3  │  4  │  5  │                 │  6  │  7  │  8  │  9  │  0  │  `   │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│ TAB  │  Q  │  W  │  E  │  R  │  T  │                 │  Y  │  U  │  I  │  O  │  P  │  -   │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│ CAPS │ ⌘/A │ ⌥/S │ ⌃/D │ ⇧/F │  G  │                 │  H  │ ⇧/J │ ⌃/K │ ⌥/L │ ⌘/; │  '   │
├──────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼─────┼──────┤
│ CTRL │  Z  │  X  │  C  │  V  │  B  │ [L3] │   │ [L2] │  N  │  M  │  ,  │  .  │  /  │ SHFT │
└──────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴──────┘
                       │ GUI │ RET │ ALT │ SPC │   │ ENT │ BSP │ DEL │ GUI │
                       └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
                                                     └L1 hold┘
```

> **CAPS** key = `Caps Word` — type a word in ALL_CAPS then auto-disables on space/symbol  
> **[L3]** = hold for Nav layer · **[L2]** = hold for Numbers+Fn layer  
> **ENT** (right thumb) = tap Enter, hold for Symbols layer

---

## Home Row Mods

Hold any home row key for its modifier. Tap for the letter.  
Trigger threshold: **200ms** · Prior idle required: **150ms**

```
Left hand                        Right hand
┌──────┬──────┬───────┬───────┐  ┌────────┬────────┬──────┬──────┐
│  ⌘   │  ⌥   │   ⌃   │   ⇧   │  │   ⇧    │   ⌃    │  ⌥   │  ⌘   │
│  A   │  S   │   D   │   F   │  │   J    │   K    │  L   │  ;   │
└──────┴──────┴───────┴───────┘  └────────┴────────┴──────┴──────┘
 LGUI   LALT   LCTRL   LSHIFT     RSHIFT   RCTRL   RALT   RGUI
```

**Tips:**
- Rest your fingers on home row — mods won't fire unless you *hold*
- If mods misfire while typing fast, you need more `require-prior-idle-ms`
- If mods feel slow to trigger, lower `tapping-term-ms` (try 180)
- For shortcuts like `⌘C`: hold S (⌥) or D (⌃) with right hand, tap letter with left

---

## Layer 1 — Symbols

*Access: hold right-thumb `ENT`*

```
┌───────┬─────┬─────┬─────┬─────┬─────┐                 ┌─────┬─────┬─────┬─────┬─────┬──────┐
│ BT CLR│ BT0 │ BT1 │ BT2 │ BT3 │ BT4 │                 │     │     │     │     │     │      │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│       │  !  │  @  │  #  │  $  │  %  │                 │  ^  │  &  │  *  │  (  │  )  │  ~   │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│   `   │     │     │     │     │     │                 │     │  _  │  +  │  {  │  }  │  |   │
├───────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼─────┼──────┤
│       │     │     │     │     │     │      │   │      │     │  -  │  =  │  [  │  ]  │  \   │
└───────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴──────┘
                        │     │     │     │ L2  │   │ L1  │     │     │     │
                        └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
```

**Bluetooth:** `BT CLR` clears current device · `BT0–BT4` selects paired device

---

## Layer 2 — Numbers + Fn

*Access: `SPC` + `ENT` combo (50ms) · or hold inner-right `[L2]` key*

```
┌──────┬─────┬─────┬─────┬─────┬─────┐                 ┌─────┬─────┬─────┬─────┬─────┬──────┐
│      │     │     │     │     │     │                 │     │     │     │     │     │      │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│  `   │  1  │  2  │  3  │  4  │  5  │                 │  6  │  7  │  8  │  9  │  0  │      │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│  F1  │  F2 │  F3 │  F4 │  F5 │  F6 │                 │  F7 │  F8 │  F9 │ F10 │ F11 │  F12 │
├──────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼─────┼──────┤
│      │     │     │     │     │     │      │   │      │     │     │     │     │     │      │
└──────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴──────┘
                       │     │     │     │     │   │     │     │     │     │
                       └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
```

---

## Layer 3 — Navigation

*Access: hold inner-left `[L3]` key (left hand holds, right hand navigates)*

```
┌──────┬─────┬─────┬─────┬─────┬─────┐                 ┌──────┬──────┬──────┬─────┬─────┬──────┐
│      │     │     │     │     │     │                 │      │      │      │     │     │      │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├──────┼──────┼──────┼─────┼─────┼──────┤
│      │     │     │     │     │     │                 │ HOME │ PGDN │ PGUP │ END │     │      │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├──────┼──────┼──────┼─────┼─────┼──────┤
│      │     │     │     │     │     │                 │  ←   │  ↓   │  ↑   │  →  │     │      │
├──────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼──────┼──────┼──────┼─────┼─────┼──────┤
│      │     │     │     │     │     │ [L3] │   │      │      │      │      │     │     │      │
└──────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┴──┬──┴──┬──┴─────┴─────┴──────┘
                       │     │     │     │     │   │     │     │     │     │
                       └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
```

**Right hand nav positions:**
```
Y = HOME    U = PG DN    I = PG UP    O = END
H = ←       J = ↓        K = ↑        L = →
```

---

## Combos

| Keys | Output | Timeout |
|------|--------|---------|
| `SPC` + `ENT` | Layer 2 (Numbers+Fn) momentary | 75ms |

---

## Quick Reference Card

```
DEFAULT THUMB CLUSTER
┌─────┬─────┬─────┬─────┐   ┌──────────┬─────┬─────┬─────┐
│ GUI │ RET │ ALT │ SPC │   │ ENT (L1) │ BSP │ DEL │ GUI │
└─────┴─────┴─────┴─────┘   └──────────┴─────┴─────┴─────┘

KEY                TAP         HOLD
──────────────────────────────────────
A                  a           ⌘ (LGUI)
S                  s           ⌥ (LALT)
D                  d           ⌃ (LCTRL)
F                  f           ⇧ (LSHIFT)
J                  j           ⇧ (RSHIFT)
K                  k           ⌃ (RCTRL)
L                  l           ⌥ (RALT)
;                  ;           ⌘ (RGUI)
Right ENT          Enter       Layer 1 (Symbols)
Inner-left [L3]    —           Layer 3 (Navigation)
Inner-right [L2]   —           Layer 2 (Numbers+Fn)
CAPS position      Caps Word   —
SPC + ENT combo    —           Layer 2 (Numbers+Fn)
```
