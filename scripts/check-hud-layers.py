#!/usr/bin/env python3
"""Fail if the Hammerspoon HUD's LAYER_META has drifted from config/lily58.keymap.

LAYER_META is indexed by the layer number the firmware reports, and is kept by
hand. It broke three times when layers were added: the HUD looked up an index
it did not have. This checks each entry's strip label reads "L<i> · <name>"
for the keymap's display-name at that index, and that its colour var exists.

    python3 scripts/check-hud-layers.py [HUD]
"""
import re
import sys

KEYMAP = "config/lily58.keymap"
HUD = sys.argv[1] if len(sys.argv) > 1 else "hammerspoon/lily58-hud.html"


def firmware():
    src = re.sub(r"//[^\n]*", "", open(KEYMAP).read())
    keymap = src[src.index('compatible = "zmk,keymap"'):]
    return re.findall(r'display-name\s*=\s*"([^"]+)"', keymap)


def hud():
    html = open(HUD).read()
    i = html.index("const LAYER_META = [")
    data = html[i:html.index("\n];", i)]
    entries = re.findall(r'strip:"([^"]+)"', data)
    colours = re.findall(r'color:"var\((--[\w-]+)\)"', data)
    defined = set(re.findall(r"(--[\w-]+):", html))
    return entries, colours, defined


def main():
    names = firmware()
    strips, colours, defined = hud()

    problems = []
    if len(strips) != len(names):
        problems.append(f"LAYER_META has {len(strips)} entries, keymap has {len(names)} layers")
    for i, name in enumerate(names):
        want = f"L{i} · {name}"
        got = strips[i] if i < len(strips) else None
        if got != want:
            problems.append(f"layer {i}: keymap {want!r}, HUD strip {got!r}")
    for i, c in enumerate(colours):
        if c not in defined:
            problems.append(f"layer {i}: colour {c} is not defined in the stylesheet")

    if problems:
        print(f"{HUD} has drifted from {KEYMAP}:\n")
        for p in problems:
            print(f"  {p}")
        sys.exit(1)
    print(f"{HUD}: {len(names)} layers match {KEYMAP}")


if __name__ == "__main__":
    main()
