# Lily58 Keymap Reference

> **Nice Nano v2** — ZMK Firmware
> 4 layers · no hold-taps on letters · layers on thumbs · tuned for Doom Emacs / Neovim

---

## Design rules

1. **No timing-based behaviour on any letter key.** Every alpha is a plain `&kp`.
   Nothing to mistime, nothing to misfire.
2. **Layers live on thumbs**, as plain `&mo`. A thumb hold is instant — no
   tapping term, no "did it decide yet".
3. **Each layer is held by the hand that isn't doing the work.**
   Nav is held left, arrows are right. Symbols is held right, most symbols are left.
4. **Anything destructive sits behind two thumbs.** Bluetooth, reset, bootloader
   are on the Numbers layer only.

---

## Layer Overview

| # | Name | Access |
|---|------|--------|
| 0 | Default | Always active |
| 1 | Symbols | Hold right-inner thumb `[SYM]` |
| 2 | Numbers + Fn | Hold **both** thumb layer keys, or inner-right `[NUM]` |
| 3 | Navigation | Hold left-inner thumb `[NAV]` |

Layer 2 is a **conditional layer** (`1 + 3 → 2`), not a combo. Holding both
thumbs is unambiguous and has no timeout.

---

## Layer 0 — Default

```
┌──────┬─────┬─────┬─────┬─────┬─────┐                 ┌─────┬─────┬─────┬─────┬─────┬──────┐
│ ESC  │  1  │  2  │  3  │  4  │  5  │                 │  6  │  7  │  8  │  9  │  0  │  `   │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│ TAB  │  Q  │  W  │  E  │  R  │  T  │                 │  Y  │  U  │  I  │  O  │  P  │  -   │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│ LSFT │  A  │  S  │  D  │  F  │  G  │                 │  H  │  J  │  K  │  L  │  ;  │  '   │
├──────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼─────┼──────┤
│ CTRL │  Z  │  X  │  C  │  V  │  B  │ ESC  │   │[NUM] │  N  │  M  │  ,  │  .  │  /  │ RSFT │
└──────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴──────┘
                       │ ALT │ RET │[NAV]│ SPC │   │ BSP │[SYM]│ ⌘   │ ⌃   │
                       └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
```

**Why the thumbs are laid out this way**

| Key | Reason |
|-----|--------|
| `SPC` (left home thumb) | Doom's leader. Plain key — nothing intercepts it. |
| `[NAV]` (left, one in) | Held by the left thumb so the right hand does the arrows. |
| `BSP` (right home thumb) | Promoted from the outer position it used to sit in. |
| `[SYM]` (right, one in) | Held by the right thumb so the left hand types the symbols. |
| `⌘` / `⌃` (right outer) | Thumb + same-hand fingers is fine, so these reach every letter. |
| `ESC` (left inner index) | Second Escape, off the pinky corner. For `jk`-free normal mode. |

Ctrl is available twice: left pinky (`CTRL`, bottom outer) and right thumb.
Use whichever hand isn't typing the letter.

---

## Layer 1 — Symbols

*Access: hold right-inner thumb `[SYM]`*

Brackets are **paired by finger position** — opening on the left hand, closing
on the right hand at the same finger.

```
┌───────┬─────┬─────┬─────┬─────┬─────┐                 ┌─────┬─────┬─────┬─────┬─────┬──────┐
│       │     │     │     │     │     │                 │     │     │     │     │     │      │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│       │  !  │  @  │  #  │  $  │  %  │                 │  ^  │  &  │  *  │  +  │  =  │  ~   │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼─────┼──────┤
│ CAPSW │  `  │  [  │  {  │  (  │  <  │                 │  >  │  )  │  }  │  ]  │  /  │  \   │
├───────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼─────┼──────┤
│       │  ~  │  |  │  _  │  -  │  :  │      │   │      │     │     │     │     │     │      │
└───────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴──────┘
                        │     │     │     │     │   │     │ held│     │     │
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

`CAPSW` = Caps Word — types the next word in caps, ends at the first space.
It lives on the left pinky here instead of on a letter hold.

---

## Layer 2 — Numbers + Fn

*Access: hold **both** thumb layer keys (`[NAV]` + `[SYM]`), or the inner-right `[NUM]` key*

Everything here is deliberately behind two thumbs, because it is everything
that can ruin your day.

```
┌───────┬─────┬─────┬─────┬─────┬─────┐                 ┌─────┬─────┬─────┬─────┬───────┬───────┐
│  BT0  │ BT1 │ BT2 │ BT3 │ BT4 │     │                 │     │     │     │     │ RESET │ BOOTL │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼───────┼───────┤
│   `   │  1  │  2  │  3  │  4  │  5  │                 │  6  │  7  │  8  │  9  │   0   │       │
├───────┼─────┼─────┼─────┼─────┼─────┤                 ├─────┼─────┼─────┼─────┼───────┼───────┤
│  F1   │  F2 │  F3 │  F4 │  F5 │  F6 │                 │ F7  │ F8  │ F9  │ F10 │  F11  │  F12  │
├───────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼─────┼─────┼─────┼─────┼───────┼───────┤
│ BTCLR │     │     │     │     │     │      │   │      │     │     │     │     │       │       │
└───────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴─────┴───────┴───────┘
                        │     │     │ held│     │   │     │ held│     │     │
                        └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
```

- `BT0–BT4` — switch Bluetooth profile
- `BTCLR` — **unpair the current profile**. Bottom-left corner, as far from
  anything as it gets.
- `RESET` / `BOOTL` — soft reset / bootloader, no more double-tapping the
  physical button to flash.

---

## Layer 3 — Navigation

*Access: hold left-inner thumb `[NAV]` — left hand holds, right hand moves*

```
┌──────┬─────┬─────┬─────┬─────┬─────┐                 ┌──────┬──────┬──────┬──────┬─────┬──────┐
│      │     │     │     │     │     │                 │      │      │      │      │     │      │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├──────┼──────┼──────┼──────┼─────┼──────┤
│      │     │     │     │     │     │                 │ HOME │ PGDN │ PGUP │ END  │     │      │
├──────┼─────┼─────┼─────┼─────┼─────┤                 ├──────┼──────┼──────┼──────┼─────┼──────┤
│      │     │     │     │     │     │                 │  ←   │  ↓   │  ↑   │  →   │     │      │
├──────┼─────┼─────┼─────┼─────┼─────┼──────┐   ┌──────┼──────┼──────┼──────┼──────┼─────┼──────┤
│      │     │     │     │     │     │      │   │      │  ⌘←  │  ⌥←  │  ⌥→  │  ⌘→  │     │      │
└──────┴─────┴─────┴──┬──┴──┬──┴──┬──┴──┬───┘   └───┬──┴──┬──┴──┬──┴──┬──┴──────┴─────┴──────┘
                       │     │     │ held│     │   │ ⌥BSP│     │ DEL │     │
                       └─────┴─────┴─────┴─────┘   └─────┴─────┴─────┴─────┘
```

**Right hand nav positions:**
```
Y = HOME    U = PG DN    I = PG UP    O = END     ← paging
H = ←       J = ↓        K = ↑        L = →       ← same fingers as Vim hjkl
N = ⌘←      M = ⌥←       , = ⌥→       . = ⌘→      ← line start/end, word left/right
```

Thumb extras while `[NAV]` is held: `⌥BSP` (delete word back) under Backspace,
and `DEL` on the right outer thumb.

---

## Combos

None. The old `SPC` + `ENT` combo is gone — at a 75 ms window it fired during
normal typing, swallowing both keys, and it collided with the Doom leader.
Layer 2 is a conditional layer instead.

---

## Quick Reference Card

```
DEFAULT THUMB CLUSTER
┌─────┬─────┬───────┬─────┐   ┌─────┬───────┬─────┬─────┐
│ ALT │ RET │ [NAV] │ SPC │   │ BSP │ [SYM] │  ⌘  │  ⌃  │
└─────┴─────┴───────┴─────┘   └─────┴───────┴─────┴─────┘
                  └──────── both held = [NUM] ────────┘

WANT                        PRESS
──────────────────────────────────────────────────────────
Symbols / brackets          hold [SYM] (right thumb)
Arrows, paging, word jumps  hold [NAV] (left thumb)
Numbers, F-keys             hold both thumbs
Bluetooth / reset           hold both thumbs, top row
Caps Word                   hold [SYM], left pinky
Escape                      top-left, or inner-left index
Delete word back            hold [NAV], Backspace
```

---

## Tuning

**Chatter (a key repeating or dropping).** `config/lily58.conf`:

```
CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS=5
CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS=10
```

Release debounce is the one that matters for doubled letters. Raise it to
12–15 if it persists. If only one or two keys misbehave that is a worn switch
or a cold solder joint, and no config value will fix it.

**ZMK Studio.** `CONFIG_ZMK_STUDIO=y` is on. Any keymap edit made in Studio is
saved to settings and **overrides this file**. If a freshly flashed keymap
doesn't take effect, flash the `settings_reset` firmware to both halves first,
then reflash left and right.
