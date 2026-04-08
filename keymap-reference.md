# Lily58 Keymap Reference

> **Nice Nano v2** — ZMK Firmware
> 4 layers · Home Row Mods (left hand) · Caps Word on F hold · Nav Layer

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
│ LSFT │ ⌘/A │ ⌥/S │ ⌃/D │CW/F │  G  │                 │  H  │  J  │  K  │  L  │  ;  │  '   │
├──────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼─────┼──────┤
│ CTRL │  Z  │  X  │  C  │  V  │  B  │ [L3] │   │ [L2] │  N  │  M  │  ,  │  .  │  /  │ RSFT │
└──────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴──────┘
                       │ GUI │ RET │ ALT │ SPC │   │ ENT │ BSP │ DEL │ GUI │
                       └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
                                                     └L1 hold┘└L3 hold┘
```

> **LSFT** = left shift (outer left homerow, muscle-memory position)
> **CW/F** = tap F · hold Caps Word — intentional caps_word, no accidental triggers while typing camelCase
> **[L3]** = hold for Nav layer · **[L2]** = hold for Numbers+Fn layer
> **ENT** (right thumb) = tap Enter, hold for Symbols layer

---

## Home Row Mods — Left Hand Only

Hold to activate modifier. Tap for the letter.
Trigger threshold: **200ms** · Prior idle required: **150ms**

```
Left hand only
┌──────┬──────┬───────┬──────────┐
│  ⌘   │  ⌥   │   ⌃   │  CapsWrd │
│  A   │  S   │   D   │    F     │
└──────┴──────┴───────┴──────────┘
 LGUI   LALT   LCTRL   caps_word (hold)
```

Right-hand home row (`H J K L ;`) are plain keys — no hold behaviour — so Neovim navigation works without interference.

**For shortcuts:** use the left-hand mod and reach with your right hand, e.g.:
- `⌃W` (split navigation): hold `D`, tap `W`
- `⌘S` (save): hold `A`, tap `S`
- Shift when typing: outer-left `LSFT` pinky key (restored)
- Caps Word (whole word): hold `F`

**Tips:**
- If mods misfire while typing fast, increase `require-prior-idle-ms`
- If mods feel slow to trigger, lower `tapping-term-ms` (try 180)

---

## Layer 1 — Symbols

*Access: hold right-thumb `ENT`*

Brackets are **paired by finger position** — opening on left hand, closing on right hand at the same finger.

```
┌───────┬─────┬─────┬─────┬─────┬─────┐                 ┌─────┬─────┬─────┬─────┬─────┬──────┐
│ BT CLR│ BT0 │ BT1 │ BT2 │ BT3 │ BT4 │                 │     │     │     │     │     │      │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│       │  !  │  @  │  #  │  $  │  %  │                 │  ^  │  &  │  *  │  +  │  =  │  ~   │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│       │  `  │  [  │  {  │  (  │  <  │                 │  >  │  )  │  }  │  ]  │  /  │  \   │
├───────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼─────┼──────┤
│       │  ~  │  |  │  _  │  -  │  :  │      │   │      │     │     │     │     │     │      │
└───────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴──────┘
                        │     │     │     │ L2  │   │ L1  │     │     │     │
                        └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
```

**Bracket pairs (same finger, mirrored):**
```
Left F → (    Right J → )    ← index finger
Left D → {    Right K → }    ← middle finger
Left S → [    Right L → ]    ← ring finger
Left G → <    Right H → >    ← inner index
Left A → `    Right ; → /
              Right ' → \
```

**Bluetooth:** `BT CLR` clears current device · `BT0–BT4` selects paired device

---

## Layer 2 — Numbers + Fn

*Access: `SPC` + `ENT` combo (75ms) · or hold inner-right `[L2]` key*

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
──────────────────────────────────────────────────────
A                  a           ⌘ (LGUI)
S                  s           ⌥ (LALT)
D                  d           ⌃ (LCTRL)
F                  f           Caps Word
LSFT position      Shift       Shift (outer left pinky, restored)
Right ENT          Enter       Layer 1 (Symbols)
Right BSP          Backspace   Layer 3 (Navigation)  ← one-handed nav
Inner-left [L3]    —           Layer 3 (Navigation)
Inner-right [L2]   —           Layer 2 (Numbers+Fn)
SPC + ENT combo    —           Layer 2 (Numbers+Fn)
```
