#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Mike Maven and Rumiboard contributors
"""The creatures' sprites from the concept sheets (docs/creatures/refs/*.jpg).

  gen_creatures.py [--out DIR] [--preview PNG] [--w 120 --h 88]

Each sheet is two rows of four poses on a light background (the top row is used). The poses are found by the
background: columns that are all background separate the panels, the caption under each pose is cut off, and each
pose's bounding box is scaled to fit the creature's canvas (120 x 88 by default). Pixels become colour ROLES, so the
device recolours a sprite with the palette (body = the track's colour, accent = its companion shade, dark = the text
colour, light = a light detail, 0 = transparent background):

  0 bg (transparent)   1 body   2 accent   3 dark   4 light   5 second accent   6 third accent   (7 unused)

The roles are classified by nearest colour against the sheet's own colours: body, accent, a second and a third
accent (the four most frequent non-background colour clusters, shades merged), dark (near black) and light (near
white). The
sheet's colours travel with the sprite (the firmware draws each creature in its own colours, as designed; the
track's colour stays on the chrome). Output: one PNG preview per creature (docs/creatures/sprites/<name>.png, 4x),
a role-index sheet (<name>.roles.png) and, with --header, the firmware's table. Run from the repository root.
"""
import argparse
import colorsys
import os
from pathlib import Path

from PIL import Image

SHEETS = ["bass_anglerfish", "stab_bulldog", "pluck_chameleon", "pad_cloud", "drums_octopus", "piano_hedgehog",
          "keys_axolotl", "master_sloth"]
BG_RGB, DARK_RGB = (234, 232, 226), (26, 26, 30)       # the preview's background (RUMI) and the dark role
# Corrections to the sheets, on the role map (a frame's own px, 0 .. 119 x 0 .. 87): ("fill", poses or None for all,
# x0, y0, x1, y1, roles, role):
# pixels of the given roles in the box become the role. The sloth: the paw tucked under its body left of the face
# is a third arm (the one reaching up is that arm); its seams (cut-outs) become body, so the arm vanishes into it.
PATCHES = {
    # The sloth: the arm reaching up to the record is its left arm, so the left front limb drawn resting under its
    # head (with the claws at the bottom) is a third arm. It goes: its paw and forearm below the head become
    # background, its upper outline (light seams) becomes body, so the body's belly is the bottom edge there.
    # ("cut", poses, x0, y0, x1, y1): pixels below a curve from (x0, y0) bowing down to (x1, y1) become background: the
    # belly's edge, continuing the body's curve from the front limb's foot up to under the head.
    "master_sloth": [("fill", [0], 43, 56, 52, 74, (0, 4, 5, 6), 1), ("cut", [0], 52, 84, 104, 72),
                     ("fill", [1], 43, 56, 53, 73, (0, 4, 5, 6), 1), ("cut", [1], 50, 86, 108, 73),
                     ("fill", [2], 57, 56, 70, 74, (0, 4, 5, 6), 1), ("cut", [2], 48, 85, 106, 73),
                     ("fill", [3], 57, 56, 70, 74, (0, 4, 5, 6), 1), ("cut", [3], 48, 85, 110, 72)],
    # the chameleon's last frame (the bend): its eye white with a dark pupil, as at the perch, not orange
    "pluck_chameleon": [("fill", [3], 41, 34, 55, 48, (5,), 4), ("fill", [3], 47, 39, 52, 44, (0, 4, 5), 3),
                        ("fill", [2], 49, 42, 60, 53, (5,), 4), ("fill", [2], 52, 45, 57, 50, (0, 4, 5), 3)],   # the reach's too
}
# Single frames that replace a sheet's pose (0 .. 3): an image of one figure, possibly inside a mock of the screen
# (docs/creatures/refs/frames/), or (path, (fx0, fy0, fx1, fy1)): that part of the image, as fractions (an image
# holding two mocks side by side: the left half, the right half). The figure is the largest connected piece whose
# pixels are mostly the creature's colours (a drawn device frame and text are dark) and whose box spans less than
# 90 % of the (part of the) image's width.
FRAMES = {
    "bass_anglerfish": {1: "docs/creatures/refs/frames/bass_anglerfish_2.jpg"},
    "stab_bulldog": {1: ("docs/creatures/refs/frames/stab_bulldog_ab.jpg", (0, 0, 0.5, 1)),      # A: the squash
                     2: ("docs/creatures/refs/frames/stab_bulldog_ab.jpg", (0.5, 0, 1, 1))},     # B: the launch
    "pluck_chameleon": {1: ("docs/creatures/refs/frames/pluck_chameleon_ab.jpg", (0, 0, 0.5, 1)),   # A: the snap
                        2: ("docs/creatures/refs/frames/pluck_chameleon_ab.jpg", (0.5, 0, 1, 1))},  # B: the reach
}
# A sheet's poses in another order (the sheet's indices for the four frames), before FRAMES replace any: the
# terrier keeps idle and the bark and drops its own squash and tail wag for the in-betweens.
ORDER = {"stab_bulldog": [0, 1, 2, 2],             # idle, (A), (B), the bark
         "pluck_chameleon": [0, 1, 2, 2]}          # perch, (A), (B), the bend (the sheet's left-facing fourth dropped)


def frame_image(spec):
    """a FRAMES entry's image: the file, or the part of it"""
    path, box = (spec, None) if isinstance(spec, str) else spec
    fim = Image.open(path).convert("RGB")
    if box:
        w, h = fim.size
        fim = fim.crop((int(w * box[0]), int(h * box[1]), int(w * box[2]), int(h * box[3])))
    return fim


def find_figure(im, bg, cols):
    """one figure in an image (a mock of the screen round it, text, a drawn frame): the largest connected piece
    that is mostly in the creature's colours and spans less than 90 % of the width (a drawn frame spans it all); its
    box in the image's pixels"""
    w, h = im.size
    s = 4 if w > 800 else 1
    small = im.resize((w // s, h // s), Image.BOX) if s > 1 else im
    px = small.load()
    is_fig = lambda p: any(dist(p, c) < 60 * 60 for c in cols)
    comps = [c[:5] for c in components(px, 0, small.width, 0, small.height, bg, is_fig)
             if (c[3] - c[1]) < small.width * 0.9 and c[0] * 2 > c[5]]
    comps.sort(reverse=True)
    n, bx0, by0, bx1, by1 = comps[0]
    fh, fw = by1 - by0, bx1 - bx0
    for m, cx0, cy0, cx1, cy1 in comps[1:]:            # its nearby pieces (rings, marks): at least a fifth of its
        if m < max(6, n // 40) or m > n // 2: continue    # height and a 40th of its area (letters are smaller),
        if (cy1 - cy0) < fh // 5 or (cx1 - cx0) > fw // 2: continue   # narrower than half of it, beside or above
        if cx1 < bx0 - fw // 4 or cx0 > bx1 + fw // 4: continue        # it (a creature stands on its floor: nothing
        if cy1 < by0 - fh // 4 or cy0 > by1 - fh // 8: continue        # below it; a mock's caption is below)
        bx0, by0, bx1, by1 = min(bx0, cx0), min(by0, cy0), max(bx1, cx1), max(by1, cy1)
    return (bx0 * s, by0 * s, bx1 * s + s - 1, by1 * s + s - 1)


def to565(c):
    return ((c[0] >> 3) << 11) | ((c[1] >> 2) << 5) | (c[2] >> 3)


def dist(a, b):
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2


def is_bg(p, bg, tol=28):
    return dist(p, bg) < tol * tol


def components(px, x0, x1, y0, y1, bg, is_fig=None):
    """connected components (8-way) of non-background pixels in the box: [(count, bx0, by0, bx1, by1)]; with is_fig,
    count is the number of pixels is_fig accepts (the creature's colours), the sixth item the pixels in all"""
    seen = set()
    out = []
    for sy in range(y0, y1):
        for sx in range(x0, x1):
            if (sx, sy) in seen or is_bg(px[sx, sy], bg):
                continue
            stack, n, nf, bx0, by0, bx1, by1 = [(sx, sy)], 0, 0, sx, sy, sx, sy
            seen.add((sx, sy))
            while stack:
                x, y = stack.pop()
                n += 1
                if is_fig and is_fig(px[x, y]): nf += 1
                bx0, by0, bx1, by1 = min(bx0, x), min(by0, y), max(bx1, x), max(by1, y)
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        nx, ny = x + dx, y + dy
                        if x0 <= nx < x1 and y0 <= ny < y1 and (nx, ny) not in seen and not is_bg(px[nx, ny], bg):
                            seen.add((nx, ny))
                            stack.append((nx, ny))
            out.append((nf, bx0, by0, bx1, by1, n) if is_fig else (n, bx0, by0, bx1, by1))
    return out


def find_panels(im, bg):
    """the top row's four poses: (x0, y0, x1, y1) boxes in the sheet's pixels, titles and captions left out"""
    w, h = im.size
    small = im.resize((w // 4, h // 4), Image.BOX)
    sw, sh = small.size
    px = small.load()
    top, bot = int(sh * 0.12), int(sh * 0.465)            # the top row's figures (titles above, captions below)
    col = [any(not is_bg(px[x, y], bg) for y in range(top, bot)) for x in range(sw)]
    panels, x = [], 0
    while x < sw:
        while x < sw and not col[x]: x += 1
        if x >= sw: break
        x0 = x
        gap = 0
        while x < sw and gap < 6:                           # a panel ends at a gap of 6 small px (24 px)
            gap = gap + 1 if not col[x] else 0
            x += 1
        panels.append((x0, x - gap))
    panels = [p for p in panels if p[1] - p[0] > sw // 12]  # (labels and stray marks are narrow)
    # two poses bridged by a detail (sound rings, a tongue): a panel far wider than the rest splits at the thinnest
    # column of its middle half
    if panels:
        med = sorted(p[1] - p[0] for p in panels)[len(panels) // 2]
        split = []
        for x0, x1 in panels:
            if x1 - x0 > med * 8 // 5:
                best, bx = None, None
                for x in range(x0 + (x1 - x0) // 4, x1 - (x1 - x0) // 4):
                    n = sum(1 for y in range(top, bot) if not is_bg(px[x, y], bg))
                    if best is None or n < best: best, bx = n, x
                split += [(x0, bx), (bx + 1, x1)]
            else:
                split.append((x0, x1))
        panels = split
    boxes = []
    for x0, x1 in panels[:4]:
        # the figure: the largest connected piece, with the pieces near it (marks, rings, bubbles) that are not text:
        # text sits below the figure or is a short wide strip beside it
        comps = components(px, x0, x1 + 1, top, bot, bg)
        if not comps: continue
        comps.sort(reverse=True)
        n, bx0, by0, bx1, by1 = comps[0]
        fh = by1 - by0
        for m, cx0, cy0, cx1, cy1 in comps[1:]:
            if m < 6: continue
            textlike = (cy1 - cy0) < sh * 0.035 and (cx1 - cx0) > 2.5 * (cy1 - cy0 + 1)
            tiny = m < n / 40 and (cy1 - cy0) < sh * 0.03   # letters, dots, specks (the marks are drawn by the firmware)
            below = cy0 > by1 + 2
            if below or textlike or tiny: continue
            if cx1 < bx0 - (x1 - x0) // 3 or cx0 > bx1 + (x1 - x0) // 3: continue
            bx0, by0, bx1, by1 = min(bx0, cx0), min(by0, cy0), max(bx1, cx1), max(by1, cy1)
        boxes.append((bx0 * 4, by0 * 4, bx1 * 4 + 3, by1 * 4 + 3))
    return boxes


def sheet_colours(im, bg, boxes):
    """body, accent, a second and a third accent: the most frequent non-background colour clusters (shades merged;
    not bg, dark or light); a sheet with fewer repeats its last"""
    # the figures' own pixels (no background, dark or light), binned to 16 levels a channel, the bins ranked
    crop = im.crop((boxes[0][0], boxes[0][1], boxes[-1][2], boxes[-1][3]))
    crop.thumbnail((900, 400), Image.NEAREST)
    bins = {}
    for p in crop.getdata():
        if is_bg(p, bg, 40) or max(p) < 70 or min(p) > 225:
            continue
        k = (p[0] >> 4, p[1] >> 4, p[2] >> 4)
        n, s = bins.get(k, (0, (0, 0, 0)))
        bins[k] = (n + 1, (s[0] + p[0], s[1] + p[1], s[2] + p[2]))
    ranked = sorted(((n, tuple(v // n for v in s)) for n, s in bins.values()), reverse=True)
    cands = []
    sat = lambda c: max(c) - min(c)
    for n, c in ranked:                                   # a shade of a colour already seen: the same colour (near it,
        for k, (m, d) in enumerate(cands):                # and as saturated: a dusky indigo is not a shade of grey)
            if dist(c, d) < 48 * 48 and abs(sat(c) - sat(d)) < 20:
                cands[k] = (m + n, d)
                break
        else:
            cands.append((n, c))
    cands.sort(reverse=True)
    total = sum(n for n, _ in cands)
    cands = [(n, c) for n, c in cands if n > total * 0.015]   # (a colour of at least 1.5 % of the figures)
    apart = lambda c, k: dist(c, k) > 45 * 45 or abs(sat(c) - sat(k)) > 20   # (a different colour, or a different chroma)
    cols = [cands[0][1]]
    for n, c in cands[1:]:                                # the accent: the next cluster clearly apart from the body
        if apart(c, cols[0]):
            cols.append(c)
            break
    # the second accent: a saturated colour if the sheet has one left (a grey sloth keeps its indigo record), else
    # the next cluster apart from both
    rest = [c for n, c in cands if all(apart(c, k) for k in cols)]
    sats = [c for c in rest if max(c) - min(c) > 32]
    if len(cols) < 2: cols.append(cols[0])
    cols.append((sats or rest or [cols[-1]])[0])
    rest = [c for c in rest if all(apart(c, k) for k in cols)]   # the third accent: the next apart from all three
    cols.append((rest or [cols[-1]])[0])
    return cols


def classify(p, bg, cols):
    if is_bg(p, bg, 30): return 0
    if max(p) < 80: return 3
    if min(p) > 225: return 4
    cands = [(dist(p, cols[0]), 1), (dist(p, cols[1]), 2), (dist(p, cols[2]), 5), (dist(p, cols[3]), 6), (dist(p, bg), 0),
             (dist(p, (20, 20, 20)), 3), (dist(p, (245, 245, 245)), 4)]
    return min(cands)[1]


def rle(frame):
    """the firmware's sprite bytes: runs of a role, b = role << 5 | (n - 1) for n <= 31, else role << 5 | 31 then
    n - 32 (runs up to 287 px); row-major over the frame"""
    px = list(frame.getdata())
    out = bytearray()
    i = 0
    while i < len(px):
        r = px[i]
        n = 1
        while i + n < len(px) and px[i + n] == r and n < 287:
            n += 1
        if n <= 31:
            out.append((r << 5) | (n - 1))
        else:
            out.append((r << 5) | 31)
            out.append(n - 32)
        i += n
    return bytes(out)


def despeckle(roles):
    """a role pixel that differs from all four neighbours takes the majority among them (JPEG specks)"""
    w, h = roles.size
    src = roles.load()
    out = roles.copy()
    dst = out.load()
    for y in range(1, h - 1):
        for x in range(1, w - 1):
            n = [src[x - 1, y], src[x + 1, y], src[x, y - 1], src[x, y + 1]]
            if src[x, y] not in n:
                dst[x, y] = max(set(n), key=n.count)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--refs", default="docs/creatures/refs")
    ap.add_argument("--out", default="docs/creatures/sprites")
    ap.add_argument("--w", type=int, default=120)
    ap.add_argument("--h", type=int, default=88)
    ap.add_argument("--preview", default="docs/creatures/sprites/all.png")
    ap.add_argument("--header", help="write the firmware's sprite table (C) here, e.g. build/gen/creature_sprites.h")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rows = []
    blob, offs, palettes = bytearray(), [], []
    for name in SHEETS:
        im = Image.open(Path(a.refs) / f"{name}.jpg").convert("RGB")
        bg = im.getpixel((8, 8))
        boxes = find_panels(im, bg)
        if len(boxes) < 4:
            print(f"{name}: {len(boxes)} poses found (wanted 4)")
        cols = sheet_colours(im, bg, boxes)
        role_rgb = {0: BG_RGB, 1: cols[0], 2: cols[1], 3: DARK_RGB, 4: BG_RGB, 5: cols[2], 6: cols[3], 7: BG_RGB}
        roles = Image.new("L", (a.w * 4, a.h), 0)
        prev = Image.new("RGB", (a.w * 4, a.h), BG_RGB)
        order = ORDER.get(name, [0, 1, 2, 3])
        for k in range(4):
            x0, y0, x1, y1 = boxes[order[k]] if order[k] < len(boxes) else boxes[-1]
            pose = im.crop((x0, y0, x1, y1))
            fbg = bg
            if k in FRAMES.get(name, {}):              # a frame of its own replaces this pose
                fim = frame_image(FRAMES[name][k])
                fbg = fim.getpixel((8, 8))
                pose = fim.crop(find_figure(fim, fbg, cols))
            pose.thumbnail((a.w - 4, a.h - 4), Image.BOX)       # fit, a 2 px margin
            ox, oy = k * a.w + (a.w - pose.width) // 2, a.h - 2 - pose.height   # standing on the canvas's floor
            pp = pose.load()
            for y in range(pose.height):
                for x in range(pose.width):
                    r = classify(pp[x, y], fbg, cols)
                    roles.putpixel((ox + x, oy + y), r)
        roles = despeckle(roles)
        rp = roles.load()
        for patch in PATCHES.get(name, []):
            op, poses = patch[0], patch[1]
            for k in (range(4) if poses is None else poses):
                if op == "fill":
                    _, _, x0, y0, x1, y1, from_roles, to_role = patch
                    for y in range(y0, y1):
                        for x in range(k * a.w + x0, k * a.w + x1):
                            if rp[x, y] in from_roles:
                                rp[x, y] = to_role
                elif op == "cut":
                    _, _, x0, y0, x1, y1 = patch
                    for x in range(x0, x1):                   # (a belly: the line bows down, rising near its end)
                        t = (x - x0) / (x1 - x0)
                        yc = round(y0 + (y1 - y0) * t * t)
                        for y in range(yc, a.h):
                            rp[k * a.w + x, y] = 0
        for y in range(roles.height):
            for x in range(roles.width):
                prev.putpixel((x, y), role_rgb[rp[x, y]])
        palettes.append([to565(role_rgb[r]) for r in range(8)])
        roles.save(Path(a.out) / f"{name}.roles.png")
        frames = [rle(roles.crop((k * a.w, 0, (k + 1) * a.w, a.h))) for k in range(4)]
        for fr in frames:
            offs.append(len(blob))
            blob += fr
        print(f"  compressed: {[len(f) for f in frames]} B, {sum(len(f) for f in frames)} B for the four poses")
        prev.resize((prev.width * 4, prev.height * 4), Image.NEAREST).save(Path(a.out) / f"{name}.png")
        rows.append(prev)
        print(f"{name}: {len(boxes)} poses, colours {cols}, boxes {[(b[2]-b[0], b[3]-b[1]) for b in boxes]}")
    sheet = Image.new("RGB", (a.w * 4, a.h * len(rows)), BG_RGB)
    for i, r in enumerate(rows):
        sheet.paste(r, (0, i * a.h))
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(a.preview)
    print(f"preview {a.preview}")
    if a.header:
        offs.append(len(blob))
        lines = ["/* generated by tools/gen_creatures.py from docs/creatures/refs/ (the jpgs): the creatures' sprites (ui_creature.c).",
                 f" * {a.w} x {a.h} px frames, four poses a creature, in the order of CR_FISH .. CR_SLOTH; runs of a colour role:",
                 " * b = role << 5 | (n - 1) for n <= 31, else role << 5 | 31 then n - 32. Roles: 0 bg (transparent), 1 body, 2 accent,",
                 " * 3 dark, 4 light, 5 second accent, 6 third accent. CR_SPR_COLS: each creature's role colours (RGB565) from its sheet; roles 0 and 4",
                 " * are 0 there: the background. In the resource region (MELODEE_RES). */",
                 "#pragma once", "#include <stdint.h>",
                 f"#define CR_SPR_W {a.w}", f"#define CR_SPR_H {a.h}", f"#define CR_SPR_CREATURES {len(SHEETS)}", "#define CR_SPR_POSES 4",
                 "static const uint16_t CR_SPR_OFF[CR_SPR_CREATURES * CR_SPR_POSES + 1] = {" + ", ".join(str(o) for o in offs) + "};",
                 "static const uint16_t CR_SPR_COLS[CR_SPR_CREATURES][8] = {" + ", ".join("{" + ", ".join(f"0x{c:04x}" if r not in (0, 4, 7) else "0" for r, c in enumerate(p)) + "}" for p in palettes) + "};",
                 f"static const uint8_t MELODEE_RES CR_SPR[{len(blob)}] = {{"]
        for i in range(0, len(blob), 24):
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in blob[i:i + 24]) + ",")
        lines.append("};")
        Path(a.header).write_text("\n".join(lines) + "\n")
        print(f"header {a.header}: {len(blob)} B of sprites")


if __name__ == "__main__":
    main()
