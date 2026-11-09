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

  0 bg (transparent)   1 body   2 accent   3 dark   4 light   5 body+bg edge   6 accent+bg edge   7 body+dark edge

The roles are classified by nearest colour against the sheet's own body and accent (the two most frequent non-
background colours, by hue), dark (near black / very dark) and light (near white). Output: one PNG preview per
creature (docs/creatures/sprites/<name>.png: the four poses in the sheet's colours, 4x) and a role-index sheet
(<name>.roles.png, greyscale indices, 1x) the firmware generator reads. Run from the repository root.
"""
import argparse
import colorsys
import os
from pathlib import Path

from PIL import Image

SHEETS = ["bass_anglerfish", "stab_bulldog", "pluck_chameleon", "pad_cloud", "drums_octopus", "piano_hedgehog",
          "keys_axolotl", "master_sloth"]
ROLE_RGB = {0: (234, 232, 226), 1: (166, 16, 32), 2: (176, 122, 0), 3: (26, 26, 30), 4: (250, 248, 244),
            5: (200, 124, 129), 6: (205, 177, 113), 7: (96, 21, 31)}       # the preview's colours (RUMI, red track)


def dist(a, b):
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2


def is_bg(p, bg, tol=28):
    return dist(p, bg) < tol * tol


def components(px, x0, x1, y0, y1, bg):
    """connected components (8-way) of non-background pixels in the box: [(count, bx0, by0, bx1, by1)]"""
    seen = set()
    out = []
    for sy in range(y0, y1):
        for sx in range(x0, x1):
            if (sx, sy) in seen or is_bg(px[sx, sy], bg):
                continue
            stack, n, bx0, by0, bx1, by1 = [(sx, sy)], 0, sx, sy, sx, sy
            seen.add((sx, sy))
            while stack:
                x, y = stack.pop()
                n += 1
                bx0, by0, bx1, by1 = min(bx0, x), min(by0, y), max(bx1, x), max(by1, y)
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        nx, ny = x + dx, y + dy
                        if x0 <= nx < x1 and y0 <= ny < y1 and (nx, ny) not in seen and not is_bg(px[nx, ny], bg):
                            seen.add((nx, ny))
                            stack.append((nx, ny))
            out.append((n, bx0, by0, bx1, by1))
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
    """body and accent: the two most frequent non-background colour clusters (6 clusters, far from bg, dark, light)"""
    crop = im.crop((boxes[0][0], boxes[0][1], boxes[-1][2], boxes[-1][3])).resize((300, 120), Image.BOX)
    q = crop.quantize(colors=12, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[:36]
    counts = sorted(q.getcolors(), reverse=True)
    cands = []
    for n, i in counts:
        c = tuple(pal[3 * i:3 * i + 3])
        if is_bg(c, bg, 40) or max(c) < 70 or min(c) > 225:
            continue
        for k, (m, d) in enumerate(cands):             # a shade of a colour already seen: the same colour
            if dist(c, d) < 45 * 45:
                cands[k] = (m + n, d)
                break
        else:
            cands.append((n, c))
    cands.sort(reverse=True)
    body = cands[0][1]
    accent = None
    for n, c in cands[1:]:                                # the accent: the next cluster clearly apart from the body
        if dist(c, body) > 45 * 45:
            accent = c
            break
    return body, accent or body


def classify(p, bg, body, accent):
    if is_bg(p, bg, 30): return 0
    if max(p) < 80: return 3
    if min(p) > 225: return 4
    cands = [(dist(p, body), 1), (dist(p, accent), 2), (dist(p, bg), 0), (dist(p, (20, 20, 20)), 3), (dist(p, (245, 245, 245)), 4)]
    d, r = min(cands)
    # an edge pixel: between two roles (far from both)
    db, da = dist(p, body), dist(p, bg)
    if r in (1, 0) and db > 40 * 40 and da > 40 * 40 and dist(p, tuple((a + b) // 2 for a, b in zip(body, bg))) < 40 * 40: return 5
    dac = dist(p, accent)
    if r in (2, 0) and dac > 40 * 40 and da > 40 * 40 and dist(p, tuple((a + b) // 2 for a, b in zip(accent, bg))) < 40 * 40: return 6
    return r


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
    blob, offs = bytearray(), []
    for name in SHEETS:
        im = Image.open(Path(a.refs) / f"{name}.jpg").convert("RGB")
        bg = im.getpixel((8, 8))
        boxes = find_panels(im, bg)
        if len(boxes) < 4:
            print(f"{name}: {len(boxes)} poses found (wanted 4)")
        body, accent = sheet_colours(im, bg, boxes)
        roles = Image.new("L", (a.w * 4, a.h), 0)
        prev = Image.new("RGB", (a.w * 4, a.h), ROLE_RGB[0])
        for k, (x0, y0, x1, y1) in enumerate(boxes[:4]):
            pose = im.crop((x0, y0, x1, y1))
            pose.thumbnail((a.w - 4, a.h - 4), Image.BOX)       # fit, a 2 px margin
            ox, oy = k * a.w + (a.w - pose.width) // 2, a.h - 2 - pose.height   # standing on the canvas's floor
            pp = pose.load()
            for y in range(pose.height):
                for x in range(pose.width):
                    r = classify(pp[x, y], bg, body, accent)
                    roles.putpixel((ox + x, oy + y), r)
                    prev.putpixel((ox + x, oy + y), ROLE_RGB[r])
        roles = despeckle(roles)
        prev = Image.new("RGB", roles.size, ROLE_RGB[0])
        rp = roles.load()
        for y in range(roles.height):
            for x in range(roles.width):
                prev.putpixel((x, y), ROLE_RGB[rp[x, y]])
        roles.save(Path(a.out) / f"{name}.roles.png")
        frames = [rle(roles.crop((k * a.w, 0, (k + 1) * a.w, a.h))) for k in range(4)]
        for fr in frames:
            offs.append(len(blob))
            blob += fr
        print(f"  compressed: {[len(f) for f in frames]} B, {sum(len(f) for f in frames)} B for the four poses")
        prev.resize((prev.width * 4, prev.height * 4), Image.NEAREST).save(Path(a.out) / f"{name}.png")
        rows.append(prev)
        print(f"{name}: {len(boxes)} poses, body {body}, accent {accent}, boxes {[(b[2]-b[0], b[3]-b[1]) for b in boxes]}")
    sheet = Image.new("RGB", (a.w * 4, a.h * len(rows)), ROLE_RGB[0])
    for i, r in enumerate(rows):
        sheet.paste(r, (0, i * a.h))
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(a.preview)
    print(f"preview {a.preview}")
    if a.header:
        offs.append(len(blob))
        lines = ["/* generated by tools/gen_creatures.py from docs/creatures/refs/ (the jpgs): the creatures' sprites (ui_creature.c).",
                 f" * {a.w} x {a.h} px frames, four poses a creature, in the order of CR_FISH .. CR_SLOTH; runs of a colour role:",
                 " * b = role << 5 | (n - 1) for n <= 31, else role << 5 | 31 then n - 32. Roles: 0 bg (transparent), 1 body, 2 accent,",
                 " * 3 dark, 4 light, 5 body+bg edge, 6 accent+bg edge, 7 body+dark edge. In the resource region (MELODEE_RES). */",
                 "#pragma once", "#include <stdint.h>",
                 f"#define CR_SPR_W {a.w}", f"#define CR_SPR_H {a.h}", f"#define CR_SPR_CREATURES {len(SHEETS)}", "#define CR_SPR_POSES 4",
                 "static const uint16_t CR_SPR_OFF[CR_SPR_CREATURES * CR_SPR_POSES + 1] = {" + ", ".join(str(o) for o in offs) + "};",
                 f"static const uint8_t MELODEE_RES CR_SPR[{len(blob)}] = {{"]
        for i in range(0, len(blob), 24):
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in blob[i:i + 24]) + ",")
        lines.append("};")
        Path(a.header).write_text("\n".join(lines) + "\n")
        print(f"header {a.header}: {len(blob)} B of sprites")


if __name__ == "__main__":
    main()
