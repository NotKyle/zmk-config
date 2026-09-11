#!/usr/bin/env python3
"""Fail if keymap-reference.html has drifted from config/lily58.keymap, or if
the published artifact has drifted from keymap-reference.html.

The reference duplicates the keymap by hand, which is how the previous one
ended up documenting home row mods that had been removed two commits earlier.
This compares them position by position (0-57) across every layer.

The published artifact is a third copy, and it went stale the same way. CI
cannot fetch it — it is private and needs a Claude login — so instead the hash
of what was last published is recorded in keymap-reference.published and
compared against the file on disk. That catches the repo moving on without a
republish, which is the failure that actually happened. It does not see edits
made to the artifact from elsewhere.

    python3 scripts/check-keymap-reference.py            # check
    python3 scripts/check-keymap-reference.py --record   # after republishing
"""
import hashlib
import re
import sys

KEYMAP = "config/lily58.keymap"
SHEET = "keymap-reference.html"
PUBLISHED = "keymap-reference.published"
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


def sheet_hash():
    return hashlib.sha256(open(SHEET, "rb").read()).hexdigest()


def read_published():
    """-> (url, sha256), or (None, None) if the record is missing."""
    try:
        text = open(PUBLISHED).read()
    except FileNotFoundError:
        return None, None
    fields = dict(
        line.split(None, 1) for line in text.splitlines()
        if line.strip() and not line.startswith("#")
    )
    return fields.get("url", "").strip() or None, fields.get("sha256", "").strip() or None


def record():
    url, _ = read_published()
    if not url:
        sys.exit(
            f"{PUBLISHED}: no url recorded. Add one before --record, so the file\n"
            f"says which artifact the hash belongs to."
        )
    with open(PUBLISHED, "w") as f:
        f.write(
            "# Written by scripts/check-keymap-reference.py --record, straight after\n"
            "# republishing the artifact. A mismatch against keymap-reference.html\n"
            "# means the published page is behind the repo.\n"
            f"url    {url}\n"
            f"sha256 {sheet_hash()}\n"
        )
    print(f"{PUBLISHED}: recorded {sheet_hash()[:12]}… for {url}")


def check_published():
    url, recorded = read_published()
    if not recorded:
        return [f"{PUBLISHED} is missing or has no hash — run --record after publishing"]
    if recorded != sheet_hash():
        return [
            f"{SHEET} has changed since it was last published.",
            f"  recorded {recorded[:12]}…  on disk {sheet_hash()[:12]}…",
            f"  republish {url}, then run: python3 {sys.argv[0]} --record",
        ]
    return []


def main():
    if "--record" in sys.argv[1:]:
        record()
        return

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
            # &trans is legitimately drawn as a layer key where some lower
            # layer has one at that position — that is what it falls through
            # to. The Mouse layer's doorway, for instance, resolves to the
            # &mo 4 sitting on Nav, not on the base layer.
            falls_to_layer = any(fw[j][pos].startswith("&mo") for j in range(i))
            if transparent and marker != "t" and not falls_to_layer:
                problems.append(f"{name} pos {pos}: firmware {binding!r}, sheet {legend!r}")
            if not transparent and marker == "t":
                problems.append(f"{name} pos {pos}: sheet says transparent, firmware {binding!r}")

    if problems:
        print(f"{SHEET} has drifted from {KEYMAP}:\n")
        for p in problems:
            print(f"  {p}")
        sys.exit(1)
    print(f"{SHEET}: {len(fw)} layers x 58 keys match {KEYMAP}")

    stale = check_published()
    if stale:
        print()
        for line in stale:
            print(line)
        sys.exit(1)
    print(f"{PUBLISHED}: published artifact matches {SHEET}")


if __name__ == "__main__":
    main()
