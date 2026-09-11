#!/usr/bin/env python3
"""Fail if keymap-reference.html has drifted from config/lily58.keymap.

The reference duplicates the keymap by hand, which is how the previous one
ended up documenting home row mods that had been removed two commits earlier.
This compares them position by position (0-57) for all four layers.
"""
import re
import sys

KEYMAP = "config/lily58.keymap"
SHEET = "keymap-reference.html"
ROWS = [12, 12, 12, 14, 8]  # bindings per row; 58 total


def firmware():
    src = re.sub(r"//[^\n]*", "", open(KEYMAP).read())
    blocks = re.findall(r"\n      bindings = <(.*?)>;", src, re.S)
    layers = []
    for b in blocks:
        counts = [len(re.findall(r"&\w+", r)) for r in b.strip().split("\n") if r.strip()]
        if counts != ROWS:
            sys.exit(f"{KEYMAP}: layer has rows {counts}, expected {ROWS}")
        layers.append([
            t.strip() for t in
            re.findall(r"&\S+(?:\s+(?:BT_SEL\s+\d|BT_CLR|\w+\([^)]*\)|[A-Z0-9_]+))?", b)
        ])
    return layers


def sheet():
    """Pull the per-half legend rows out of the page's LAYERS table, in order."""
    html = open(SHEET).read()
    i = html.index("var LAYERS = [")
    data = html[i:html.index("\n  ];", i)]
    names = re.findall(r'name:\s*"([^"]+)"', data)
    halves = re.findall(r"(?:left|right):\s*\[([\s\S]*?)\n      \]", data)
    rows = [
        [re.findall(r'"((?:[^"\\]|\\.)*)"', r)
         for r in re.findall(r'\[((?:\s*"(?:[^"\\]|\\.)*"\s*,?)+)\]', h)]
        for h in halves
    ]
    return names, rows


def main():
    fw, (names, rows) = firmware(), sheet()
    if len(rows) != 2 * len(fw):
        sys.exit(f"{SHEET}: {len(rows)} halves for {len(fw)} layers")

    problems = []
    for i, (kmap, name) in enumerate(zip(fw, names)):
        left, right = rows[i * 2], rows[i * 2 + 1]
        flat = []
        for r in range(4):
            flat += left[r] + right[r]
        flat += left[4] + right[4]
        if len(flat) != 58:
            problems.append(f"{name}: sheet has {len(flat)} keys, not 58")
            continue
        for pos, (binding, legend) in enumerate(zip(kmap, flat)):
            # Legends are "<marker>:<label>" — t transparent, L layer, H held,
            # W destructive — or a bare label. Two-char markers, because "~",
            # "+", "*" and "!" are themselves keys on the Symbols layer.
            m = re.match(r"^([tLHW]):([\s\S]*)$", legend)
            marker, label = (m.group(1), m.group(2)) if m else (None, legend)
            if label == "":
                problems.append(f"{name} pos {pos}: blank keycap ({legend!r})")
            transparent = binding == "&trans"
            # &trans is legitimately drawn as a layer key where the base layer
            # has one at that position — that is what it falls through to.
            if transparent and marker != "t" and not fw[0][pos].startswith("&mo"):
                problems.append(f"{name} pos {pos}: firmware {binding!r}, sheet {legend!r}")
            if not transparent and marker == "t":
                problems.append(f"{name} pos {pos}: sheet says transparent, firmware {binding!r}")

    if problems:
        print(f"{SHEET} has drifted from {KEYMAP}:\n")
        for p in problems:
            print(f"  {p}")
        sys.exit(1)
    print(f"{SHEET}: {len(fw)} layers x 58 keys match {KEYMAP}")


if __name__ == "__main__":
    main()
