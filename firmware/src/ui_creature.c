/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* Rumiboard's creatures: a sound's living picture, large on the play page (ui_play.c) and small at the right of
 * Stage's band (ui_stage.c stage_band). Each sound family gets its own (docs/creatures/README.md, the sheets in
 * docs/creatures/refs/): creature_of says which one a sound gets, creature_step moves it (once per CR_STEP_MS on the
 * clock, from the ISR's per-track state: a forced redraw shows the same frame), creature_draw_at draws it at any
 * size, creature_sig says when a redraw is due.
 *
 * Colours are roles: the body the track's colour (T_THEME), the accent (fins, gloves, a record) the track's first
 * companion shade (T_KNOB(1)), cut-outs (eyes, mouths) the background, a dark detail (pupils, a branch, a string)
 * the text colour. Built from discs, rounded rectangles and column spans (cv_vspan_aa: ellipses, triangles, arcs);
 * no sprites. All state is the struct below (RAM is scarce).
 *
 * The roster (refs/*.jpg), what a note does to each:
 *   CR_FISH      BASS            an anglerfish: a note opens its mouth (the harder, the wider), the lure flares with
 *                                sound rings, it lunges; held: mouth open; let go: closed; idle: floats.
 *   CR_BULLDOG   stabs (BRASS)   a boxy bulldog: a note squashes it, it jumps and barks (the harder, the higher) with
 *                                marks over its head, lands, wags its tail a while; idle: nods.
 *   CR_CHAMELEON PLUCK, LEAD     a chameleon on a branch: a note snaps its tongue out (velocity = reach), the pitch
 *                                bend bends it, let go it retracts; its eye follows the pitch.
 *   CR_CLOUD     PAD, STRING     a cloud: it grows with the sound's envelope, churns underneath, opens its eyes and
 *                                sparkles at full bloom, shrinks as the sound fades.
 *   CR_OCTOPUS   DRUM            an octopus at a pad box: a lane's hit lights its pad and a tentacle slams it, the
 *                                hats twiddle its knob; it sways with time.
 *   CR_HEDGEHOG  (felted piano)  a hedgehog pincushion with a mallet: a note swings the mallet into a string, which
 *                                vibrates while the note rings and stills when it is let go.
 *   CR_AXOLOTL   KEYS, ORGAN,    an axolotl in a bubble: a note starts a slow flip, the tail trailing; bubbles rise
 *                BELL            while notes are held; idle: floats.
 *   CR_SLOTH     (the lofi bus)  a sloth with a record on its back: the record turns, wobbles, the needle skips on a
 *                                peak; it blinks now and then.
 *   CR_BLOB      everything else a round blob: squashes on a note, springs back past round, settles; breathes idle.
 * The hedgehog and the sloth wait for the felted-piano preset and the lofi bus to be wired; cr_force (tests) shows
 * any creature regardless of the sound, for the review renders.
 * Onset: the amp envelope of the track's latest voice (mod.c m_env, Q15) rising by a sixth of full scale between
 * two steps, or the latest key changing while something sounds (a pad's slow attack gives no action); a DRUM track's
 * onsets are its lanes' hits (ui_stage.c stage.hit_t). */
enum { CR_NONE, CR_BLOB, CR_FISH, CR_BULLDOG, CR_CHAMELEON, CR_CLOUD, CR_OCTOPUS, CR_HEDGEHOG, CR_AXOLOTL, CR_SLOTH,
       CR_N };

static struct {
    int16_t p, v;                           /* a spring (the blob's squash, Q8: 256 = flat, negative = stretched; the
                                             * bulldog's squash / jump) and its velocity */
    int32_t env;                            /* m_env at the last step */
    uint8_t key, vel, trk, idle;            /* the latest key / velocity seen; the track (+ 1); steps at rest */
    uint8_t act, blast, lunge;              /* the action 0 .. 255 (a mouth, a tongue, a swing, a flip), effect steps
                                             * left (rings, marks, a wag), a displacement in px */
    uint8_t phase, lane;                    /* a step counter (sway, churn, the record); the last drum lane hit */
    int8_t bend;                            /* the pitch bend, -16 .. 16 px */
    uint32_t ms;                            /* fm1_ms at the last step */
} cr;
static uint8_t cr_force;                    /* tests/ui_render.c: a creature to show regardless of the sound (0: none) */
#define CR_STEP_MS 30u

static uint32_t creature_of(uint32_t cat)
{
    if (cr_force) return cr_force;
    switch (cat) {
    case CAT_BASS: return CR_FISH;
    case CAT_BRASS: return CR_BULLDOG;
    case CAT_PLUCK: case CAT_LEAD: return CR_CHAMELEON;
    case CAT_PAD: case CAT_STRING: return CR_CLOUD;
    case CAT_DRUM: return CR_OCTOPUS;
    case CAT_KEYS: case CAT_ORGAN: case CAT_BELL: return CR_AXOLOTL;
    default: return CR_BLOB;
    }
}

/* one step on the clock: the spring, the action's course, the effects' countdown, and a note's impulse */
static void creature_step(const track_t *t, int held)
{
    uint32_t sel = song.sel % NTRK + 1u, steps = (fm1_ms - cr.ms) / CR_STEP_MS, l;
    int32_t env = t->m_env, target = held ? 40 : 0, onset = 0, drum = drum_track(t);
    if (cr.trk != sel) {                                /* another track: start at rest (a note already sounding there
                                                         * counts as an onset: a move to say hello) */
        memset(&cr, 0, sizeof cr);
        cr.trk = (uint8_t)sel;
        cr.ms = fm1_ms;
        steps = 0;
    }
    if (drum) {                                         /* a lane just hit (stage_panel_hits: 10 frames lit) */
        for (l = 0; l < NLANE; l++)
            if (stage.hit_t[l] == 10u) { onset = 1; cr.lane = (uint8_t)l; }
        if (onset) { cr.vel = 110; cr.blast = 4; cr.lunge = 3; cr.idle = 0; }
    } else if (env - cr.env > 32767 / 6 || (t->m_key != cr.key && env > 32767 / 8)) {
        cr.vel = t->m_vel;
        cr.v = (int16_t)(cr.v + 30 + (int32_t)t->m_vel);   /* the spring's impulse: 30 .. 157 */
        cr.act = (uint8_t)(120 + t->m_vel);                 /* the action: 120 .. 247 */
        cr.blast = 7;
        cr.lunge = 5;
        cr.idle = 0;
        onset = 1;
    }
    cr.env = env;
    cr.key = t->m_key;
    cr.bend = (int8_t)clamp(t->bend_raw / 512, -16, 16);
    if (steps > 8u) steps = 8u;                         /* (a long gap: no catching up beyond a quarter second) */
    cr.ms += steps * CR_STEP_MS;
    while (steps--) {
        int32_t a = ((target - cr.p) >> 1) - (cr.v >> 2);   /* the spring: stiffness 1/2, damping 1/4 */
        int32_t at = held && env > 32767 / 8 ? 150 : 0;      /* the action's rest while held, else none */
        cr.v = (int16_t)clamp(cr.v + a, -300, 300);
        cr.p = (int16_t)clamp(cr.p + cr.v, -128, 200);
        cr.phase++;
        if (cr.blast) cr.blast--;
        if (cr.lunge) cr.lunge--;
        if (!onset || steps < 7u)                       /* (the first step after an onset keeps the action whole) */
            cr.act = (uint8_t)(cr.act + (at - (int32_t)cr.act) / 3);
        if (cr.p == target && !cr.v && !cr.blast && (int32_t)cr.act <= at + 2) {
            if (cr.idle < 255u) cr.idle++;
        } else
            cr.idle = 0;
    }
}

/* what the drawing depends on (the play page redraws the creature when this changes) */
static uint32_t creature_sig(void)
{
    uint32_t s = (uint32_t)(cr.p + 256) * 7u + cr.key * 131u + cr.trk + (cr.vel >= 96u && cr.env > 32767 / 4) * 7919u +
                 cr.act * 263u + cr.blast * 4099u + cr.lunge * 613u + cr.lane * 17u + (uint32_t)(cr.bend + 16) * 1009u +
                 (uint32_t)(cr.env >> 9) * 37u;
    if (cr.idle > 20u || cr.blast)
        s += cr.phase * 104729u;                        /* (sway, churn, a wag, the record turning) */
    return s;
}

/* ---- the parts: an ellipse, a triangle, a thick arc, a thick line, a chain of discs, by columns / steps */
static int32_t cr_isqrt(int32_t q)
{
    int32_t s = 0;
    while ((s + 1) * (s + 1) <= q) s++;
    return s;
}
/* sine, Q8, of a turn in 256ths (Bhaskara's approximation: within 0.2 %) */
static int32_t cr_sin(int32_t a)
{
    int32_t h = a & 255, neg = h >= 128, x;
    if (neg) h -= 128;
    x = h * (128 - h);
    x = 4096 * x / (81920 - 4 * x);
    return neg ? -x : x;
}
static int32_t cr_cos(int32_t a) { return cr_sin(a + 64); }
static void cr_ellipse(int32_t cx, int32_t cy, int32_t a, int32_t b, uint16_t c)
{
    int32_t x;
    if (a <= 0 || b <= 0) return;
    for (x = cx - a; x <= cx + a; x++) {
        int32_t dx = x - cx, h = b * cr_isqrt(16 * (a * a - dx * dx)) / a;   /* half height, 1/4 px */
        if (h <= 0) continue;
        cv_vspan_aa(x, cy * 16 - h * 4, cy * 16 + h * 4, c);
    }
}
/* a triangle with a vertical side at x0 (y0a .. y0b) and its tip at (x1, y1) */
static void cr_tri(int32_t x0, int32_t y0a, int32_t y0b, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t x, n = x1 - x0, d = n < 0 ? -n : n, i;
    if (!d) return;
    for (i = 0; i <= d; i++) {
        x = x0 + (n < 0 ? -i : i);
        cv_vspan_aa(x, (y0a * (d - i) + y1 * i) * 16 / d, (y0b * (d - i) + y1 * i) * 16 / d, c);
    }
}
/* the upper-left quarter of a circle (centre cx, cy, radius r), th px thick */
static void cr_arc(int32_t cx, int32_t cy, int32_t r, int32_t th, uint16_t c)
{
    int32_t x;
    for (x = cx - r; x <= cx; x++) {
        int32_t dx = x - cx, y = cr_isqrt(16 * (r * r - dx * dx));   /* 1/4 px */
        cv_vspan_aa(x, cy * 16 - y * 4 - th * 8, cy * 16 - y * 4 + th * 8, c);
    }
}
/* a line th px thick from (x0, y0) to (x1, y1): squares along it */
static void cr_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t th, uint16_t c)
{
    int32_t dx = x1 - x0, dy = y1 - y0, n = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy), i;
    if (th < 1) th = 1;
    for (i = 0; i <= n; i++)
        cv_rect(x0 + (n ? dx * i / n : 0) - th / 2, y0 + (n ? dy * i / n : 0) - th / 2, th, th, c);
}
/* discs from (x0, y0, d0) to (x1, y1, d1), n of them: a tentacle, a tail */
static void cr_chain(int32_t x0, int32_t y0, int32_t d0, int32_t x1, int32_t y1, int32_t d1, int32_t n, uint16_t c)
{
    int32_t i;
    for (i = 0; i <= n; i++) {
        int32_t d = (d0 * (n - i) + d1 * i) / n;
        cv_disc(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, d < 2 ? 2 : d, c, 0);
    }
}
/* the idle sway: a slow triangle of +-amp from the clock */
static int32_t cr_sway(int32_t amp)
{
    int32_t ph = (int32_t)((fm1_ms / 50u) % 48u);
    return ((ph < 24 ? ph : 48 - ph) - 12) * amp / 12;
}

#define F(v) ((v) * h0 / 64)                            /* the sheets' units: a 64 px creature */
#define FM(v, m) ((v) * h0 / 64 < (m) ? (m) : (v) * h0 / 64)   /* .. at least m px (dots, lines) */

/* ---- the anglerfish (BASS), facing right; its lure rises above the body */
static void creature_fish(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t m = cr.act, bob = cr.idle > 20u ? cr_sway(F(4)) : 0, a = F(36), b = F(19), cy, hx, x, lx, ly, i;
    cx += cr.lunge * F(1);
    cy = bottom - F(26) + bob / 2;
    cr_tri(cx - a + F(6), cy - F(4), cy + F(4), cx - a - F(14), cy - F(16), acc);   /* the tail's V */
    cr_tri(cx - a + F(6), cy - F(4), cy + F(4), cx - a - F(14), cy + F(16), acc);
    cr_tri(cx - F(26), cy - b + F(8), cy - b + F(2), cx - F(8), cy - b - F(12), acc);   /* the dorsal fin */
    cr_ellipse(cx, cy, a, b, body);
    cr_ellipse(cx - F(4), cy + F(12), F(9), F(4), acc);   /* the pectoral fin */
    hx = cx + F(6);                                     /* the mouth: a wedge from the hinge, open by m; shut: a seam */
    if (m > 24) {
        int32_t open = F(18) * m / 255, len = cx + a + F(2) - hx;
        for (x = hx; x <= cx + a + F(2); x++) {
            int32_t o = open * (x - hx) / len;
            cv_vspan_aa(x, (cy + F(2)) * 16 - o * 16, (cy + F(2)) * 16 + o * 16, bg);
        }
        if (open >= F(6)) {                             /* teeth along both jaws */
            int32_t tw = FM(5, 3);
            for (i = 0; i < 3; i++) {
                int32_t tx = hx + len * (2 + 3 * i) / 10, o = open * (tx - hx) / len;
                cr_tri(tx, cy + F(2) - o, cy + F(2) - o + 1, tx + tw, cy + F(2) - o + tw, body);
                cr_tri(tx + tw, cy + F(2) + o - 1, cy + F(2) + o, tx + 2 * tw, cy + F(2) + o - tw, body);
            }
        }
    } else {
        cv_rect(hx + F(8), cy + F(3), a - F(10), h0 >= 48 ? 2 : 1, bg);
    }
    cv_disc(cx + F(18), cy - F(7), FM(8, 3), bg, 0);    /* the eye, its pupil; two spots on the cheek */
    if (h0 >= 40) {
        cv_disc(cx + F(19), cy - F(7), F(3), T_TEXT, 0);
        cv_disc(cx - F(2), cy - F(3), F(4), bg, 0);
        cv_disc(cx + F(5), cy - F(3), F(4), bg, 0);
    }
    lx = cx + F(30);                                    /* the lure: a stalk arcing to the bulb; blasting: rings */
    ly = cy - b - F(16) - bob;
    if (h0 >= 40) {
        cr_arc(lx, cy - b + F(2), F(20), h0 >= 48 ? 2 : 1, acc);
        ly = cy - b + F(2) - F(20);
    }
    cv_disc(lx, ly, cr.blast ? F(10) : FM(7, 3), acc, 0);
    if (cr.blast && h0 >= 40)
        for (i = 0; i < 2; i++) {
            int32_t d = F(14) + (7 - (int32_t)cr.blast) * F(3) + i * F(8);
            if (d <= F(40)) cv_disc(lx, ly, d, acc, 1);
        }
}

/* ---- the bulldog (stabs): boxy, a brow stripe, it squashes, jumps and barks */
static void creature_bulldog(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t s = cr.p, sq = s > 0 ? s : 0, jump = s < 0 ? -s * F(44) / 128 : 0, bark = cr.act;
    int32_t bw = F(40) + sq * F(8) / 256, bh = F(30) - sq * F(8) / 256, hw = F(46), hh = F(34) - sq * F(6) / 256;
    int32_t by = bottom - jump - bh, hy = by - hh + F(6), nod = cr.idle > 20u ? cr_sway(F(2)) : 0, i;
    hy += nod;
    cv_rrect(cx - F(16), bottom - jump - F(8), F(10), F(8), F(3), body, bg);   /* the legs, their paws */
    cv_rrect(cx + F(6), bottom - jump - F(8), F(10), F(8), F(3), body, bg);
    cv_rrect(cx - F(16), bottom - jump - F(4), F(10), F(4), F(2), acc, body);
    cv_rrect(cx + F(6), bottom - jump - F(4), F(10), F(4), F(2), acc, body);
    if (cr.blast || cr.idle > 20u) {                    /* the tail: wagging after a note, slow when idle */
        int32_t wag = cr.blast ? ((cr.phase & 1u) ? F(5) : -F(5)) : cr_sway(F(3));
        cv_rrect(cx - bw / 2 - F(6) + wag / 2, by + F(6), F(8), F(5), F(2), acc, bg);
    }
    cv_rrect(cx - bw / 2, by, bw, bh, F(8), body, bg);  /* the body, the head over it */
    cr_tri(cx - hw / 2 + F(4), hy + F(12), hy + F(2), cx - hw / 2 - F(4), hy - F(8), acc);   /* the ears */
    cr_tri(cx + hw / 2 - F(4), hy + F(12), hy + F(2), cx + hw / 2 + F(4), hy - F(8), acc);
    cv_rrect(cx - hw / 2, hy, hw, hh, F(10), body, bg);
    cv_rect(cx - F(12), hy + F(3), F(24), FM(4, 2), acc);   /* the brow stripe, notched */
    if (h0 >= 40) for (i = 0; i < 3; i++) cv_rect(cx - F(7) + i * F(7), hy + F(3), FM(2, 1), FM(4, 2), body);
    if (bark > 60) {                                    /* barking: eyes shut tight, the mouth open with teeth */
        cv_rect(cx - F(13), hy + F(13), F(8), FM(2, 1), bg);
        cv_rect(cx + F(5), hy + F(13), F(8), FM(2, 1), bg);
    } else {
        cv_disc(cx - F(9), hy + F(14), FM(7, 3), bg, 0);   /* the eyes: open; idle, sleepy lids */
        cv_disc(cx + F(9), hy + F(14), FM(7, 3), bg, 0);
        if (cr.idle > 20u && h0 >= 40) {
            cv_rect(cx - F(13), hy + F(11), F(8), F(3), body);
            cv_rect(cx + F(5), hy + F(11), F(8), F(3), body);
        }
    }
    cv_rrect(cx - F(5), hy + F(19), F(10), FM(6, 3), F(2), bg, body);   /* the nose */
    if (bark > 60) {
        int32_t mh = F(10) * bark / 255;
        cv_rrect(cx - F(8), hy + F(26), F(16), mh < 3 ? 3 : mh, F(3), bg, body);
        if (h0 >= 40 && mh > F(4)) { cv_rect(cx - F(5), hy + F(26), F(3), F(3), body); cv_rect(cx + F(2), hy + F(26), F(3), F(3), body); }
    } else {
        cv_rect(cx - F(6), hy + F(27), F(12), FM(2, 1), bg);   /* shut: a line */
    }
    if (cr.blast > 3 && h0 >= 40) {                    /* the bark's marks */
        cr_line(cx + hw / 2 + F(2), hy - F(2), cx + hw / 2 + F(8), hy - F(10), FM(2, 1), acc);
        cr_line(cx + hw / 2 + F(8), hy + F(4), cx + hw / 2 + F(14), hy, FM(2, 1), acc);
    }
}

/* ---- the chameleon (plucks, leads) on a branch, its tongue the action, bent by the pitch bend */
static void creature_chameleon(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t by = bottom - F(10), cy = bottom - F(24), hx = cx + F(14), hy = bottom - F(28), i, look;
    cv_rect(cx - F(52), by, F(96), h0 >= 48 ? 2 : 1, T_TEXT);   /* the branch */
    cv_disc(cx - F(32), cy + F(6), F(14), body, 1);     /* the curled tail */
    cr_ellipse(cx - F(6), cy, F(24), F(13), body);      /* the body, the head */
    cr_ellipse(hx, hy, F(12), F(10), body);
    for (i = 0; i < 3; i++)                             /* the crest */
        cv_disc(cx - F(16) + i * F(10), cy - F(13) + (i == 1 ? -F(1) : 0), FM(6, 3), acc, 0);
    cv_rect(cx - F(14), cy + F(10), F(5), F(5), body);  /* the legs */
    cv_rect(cx + F(4), cy + F(10), F(5), F(5), body);
    look = clamp(((int32_t)cr.key - 48) / 8, -3, 3);    /* the eye follows the pitch; a snap: it looks at the tongue */
    cv_disc(hx + F(2), hy - F(2), FM(9, 3), bg, 0);
    if (h0 >= 40) cv_disc(hx + F(2) + (cr.act > 60 ? F(3) : F(look)), hy - F(2) - (cr.act > 60 ? 0 : F(1)), F(3), T_TEXT, 0);
    cv_rect(hx + F(6), hy + F(4), F(7), FM(2, 1), bg);  /* the mouth */
    if (cr.act > 24) {                                  /* the tongue: out by the action, its tip bent by the bend */
        int32_t x0 = hx + F(12), y0 = hy + F(4), len = F(32) * cr.act / 255, th = FM(3, 2), seg;
        for (seg = 0; seg < 4; seg++) {
            int32_t xa = x0 + len * seg / 4, xb = x0 + len * (seg + 1) / 4;
            int32_t ya = y0 - cr.bend * seg * seg * F(1) / 16, yb = y0 - cr.bend * (seg + 1) * (seg + 1) * F(1) / 16;
            cr_line(xa, ya, xb, yb, th, acc);
        }
        cv_disc(x0 + len, y0 - cr.bend * F(1), FM(5, 3), acc, 0);
    }
}

/* ---- the cloud (pads): grows with the envelope, churns, blooms */
static void creature_cloud(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t k = 154 + 102 * cr.env / 32767, cy = bottom - F(30), churn = cr_sway(F(3)), i;   /* k: Q8 size, 0.6 .. 1 */
    int32_t bloom = cr.env > 26000;
#define K(v) (F(v) * k / 256)
    for (i = 0; i < 3; i++)                             /* the underbelly (accent), churning */
        cv_disc(cx - K(16) + i * K(16) + (i == 1 ? -churn : churn), cy + K(14), K(16) < 3 ? 3 : K(16), acc, 0);
    cv_disc(cx - K(18), cy + K(2), K(30) < 3 ? 3 : K(30), body, 0);   /* the body: a heap of discs */
    cv_disc(cx + K(18), cy + K(4), K(28) < 3 ? 3 : K(28), body, 0);
    cv_disc(cx, cy - K(10), K(28) < 3 ? 3 : K(28), body, 0);
    cv_disc(cx, cy + K(4), K(44) < 3 ? 3 : K(44), body, 0);
    cv_rrect(cx - K(30), cy + K(6), K(60), K(10) < 2 ? 2 : K(10), K(4), body, bg);
    if (bloom) {                                        /* full bloom: eyes open, sparkles */
        cv_disc(cx - K(7), cy - K(2), FM(6, 3), bg, 0);
        cv_disc(cx + K(7), cy - K(2), FM(6, 3), bg, 0);
        if (h0 >= 40) for (i = 0; i < 3; i++)
            cv_disc(cx - K(34) + i * K(34), cy - K(28) + (i == 1 ? -F(4) : F(2)) + ((cr.phase + i * 5) % 8u < 4u ? 0 : F(2)), FM(3, 2), acc, 0);
    } else {                                            /* resting: sleepy lids */
        cv_rect(cx - K(10), cy - K(2), K(6) < 3 ? 3 : K(6), FM(2, 1), bg);
        cv_rect(cx + K(4), cy - K(2), K(6) < 3 ? 3 : K(6), FM(2, 1), bg);
    }
    cv_rect(cx - FM(2, 1), cy + K(5), FM(4, 2), FM(2, 1), bg);   /* a small mouth */
#undef K
}

/* ---- the octopus (drums) at its pad box: a hit lights a pad and a tentacle slams it */
static void creature_octopus(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t sway = cr_sway(F(3)), hx = cx + sway, hy = bottom - F(64), hw = F(40), hh = F(34), i;
    int32_t bx = cx - F(28), byy = bottom - F(14), bw = F(56), bh = F(14), lane = cr.lane % NLANE;
    int32_t px = bx + F(5) + (lane % 4) * F(12), py = byy + F(3) + (lane / 4) * F(6);   /* the hit lane's pad */
    int32_t lit = cr.blast != 0u;
    for (i = 0; i < 3; i++) {                           /* three tentacles a side, drooping to the box */
        int32_t x0 = hx - hw / 2 + F(6) + i * F(4), x1 = cx - F(44) + i * F(14), y1 = byy - F(2) + (i == 1 ? F(2) : 0);
        int32_t x0r = hx + hw / 2 - F(6) - i * F(4), x1r = cx + F(44) - i * F(14);
        if (lit && lane % 4 < 2 && i == 2) { x1 = px; y1 = py; }       /* .. one slams the hit pad */
        if (lit && lane % 4 >= 2 && i == 2) { x1r = px; }
        cr_chain(x0, hy + hh - F(4), FM(8, 2), x1, y1, FM(4, 2), 14, body);
        cr_chain(x0r, hy + hh - F(4), FM(8, 2), x1r, lit && lane % 4 >= 2 && i == 2 ? py : y1, FM(4, 2), 14, body);
    }
    cv_rrect(hx - hw / 2, hy + (lit ? F(2) : 0), hw, hh, F(10), body, bg);   /* the head */
    cv_disc(hx - F(8), hy + F(14), FM(8, 3), bg, 0);   /* the eyes: wide on a hit */
    cv_disc(hx + F(8), hy + F(14), FM(8, 3), bg, 0);
    if (h0 >= 40) {
        cv_disc(hx - F(8) + (lit ? 0 : F(1)), hy + F(15), lit ? F(4) : F(3), T_TEXT, 0);
        cv_disc(hx + F(8) + (lit ? 0 : F(1)), hy + F(15), lit ? F(4) : F(3), T_TEXT, 0);
    }
    if (lit) cv_disc(hx, hy + F(25), FM(4, 2), bg, 0);  /* the mouth: an O on a hit, a smile line otherwise */
    else cv_rect(hx - F(4), hy + F(25), F(8), FM(2, 1), bg);
    cv_rrect(bx, byy, bw, bh, F(3), body, bg);          /* the pad box, its 4 x 2 pads (the lanes), its knob */
    for (i = 0; i < NLANE; i++) {
        int32_t x = bx + F(5) + (i % 4) * F(12), y = byy + F(3) + (i / 4) * F(6);
        cv_rrect(x, y, FM(9, 2), FM(4, 2), FM(1, 1), stage.hit_t[i] ? acc : bg, body);
    }
    cv_disc(bx + bw - F(5), byy + bh / 2, FM(6, 3), acc, 0);
    if (h0 >= 40) cv_disc(bx + bw - F(5) + ((cr.phase & 2u) ? F(2) : -F(2)) * (lit && (lane == 3 || lane == 4)), byy + bh / 2 - F(2), FM(2, 1), T_TEXT, 0);
}

/* ---- the hedgehog (felted piano): a mallet swings into a string that vibrates while the note rings */
static void creature_hedgehog(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t bx = cx - F(12), bcy = bottom - F(26), sx = cx + F(36), i, amp;
    int32_t swing = cr.act, ang = 96 - 96 * swing / 255;  /* the mallet's angle: 135 deg at guard .. 0 at the string */
    int32_t shx = cx + F(2), shy = bottom - F(30), hl = F(26), hx = shx + hl * cr_cos(ang) / 256, hy = shy - hl * cr_sin(ang) / 256;
    amp = cr.env > 4000 ? F(3) * cr.env / 32767 : 0;
    for (i = 0; i < F(60); i += FM(4, 2))                /* the string, vibrating */
        cv_rect(sx + (amp ? ((i / FM(4, 2) + cr.phase) & 1 ? amp : -amp) : 0), bottom - F(60) + i, h0 >= 48 ? 2 : 1, FM(4, 2), T_TEXT);
    for (i = 0; i < 5; i++)                             /* the spines: loops over the back */
        cv_disc(bx - F(16) + i * F(7), bcy - F(22) + (i < 2 ? F(4) - i * F(2) : (i - 2) * F(3)), FM(8, 3), acc, 1);
    cr_ellipse(bx, bcy, F(20), F(26), body);            /* the body */
    cv_rrect(bx - F(14), bottom - F(5), F(10), F(5), F(2), body, bg);   /* the feet */
    cv_rrect(bx + F(2), bottom - F(5), F(10), F(5), F(2), body, bg);
    if (h0 >= 40) for (i = 0; i < 4; i++) cv_rect(bx - F(6), bcy - F(14) + i * F(8), FM(2, 1), F(4), bg);   /* a seam */
    cv_disc(bx + F(8), bcy - F(10), FM(5, 3), bg, 0);   /* the eye (shut when idle), the nose */
    if (cr.idle > 20u) cv_rect(bx + F(5), bcy - F(10), F(6), F(3), body);
    cv_disc(bx + F(18), bcy - F(6), FM(4, 2), T_TEXT, 0);
    cr_line(shx, shy, hx, hy, FM(3, 2), acc);           /* the mallet: handle and head */
    cv_disc(hx, hy, FM(9, 3), acc, 0);
    cv_disc(shx, shy, FM(5, 3), body, 0);               /* the paw holding it */
}

/* ---- the axolotl (keys) in a bubble: a note flips it, the tail trailing; bubbles rise while held */
static void creature_axolotl(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t bcx = cx, bcy = bottom - F(32), r = F(14), flip, ah, at, hxp, hyp, txp, typ, i, gx, gy;
    cv_disc(bcx, bcy, F(62), acc, 1);                   /* the bubble, a highlight */
    if (h0 >= 40) cr_line(bcx - F(22), bcy - F(18), bcx - F(16), bcy - F(24), FM(2, 1), acc);
    flip = 255 - cr.act;                                /* the flip: a turn as the action fades (the head leads) */
    ah = cr.act > 4 ? flip : 0;
    at = ah > 24 ? ah - 24 : 0;
    hxp = bcx - r * cr_cos(ah) / 256; hyp = bcy - r * cr_sin(ah) / 256 + (cr.idle > 20u ? cr_sway(F(2)) : 0);
    txp = bcx + r * cr_cos(at) / 256; typ = bcy + r * cr_sin(at) / 256;
    cr_ellipse(txp + (txp - bcx) / 2, typ + (typ - bcy) / 2, F(8), F(5), acc);   /* the tail fin */
    cr_chain(hxp, hyp, F(16), txp, typ, F(8), 5, body); /* the body: head to tail */
    for (i = 0; i < 3; i++) {                           /* the gills, each side of the head */
        gx = hxp + (hyp - bcy) * (i - 1) / 6; gy = hyp - (hxp - bcx) * (i - 1) / 6;
        cv_disc(gx + (hxp - bcx) * -3 / 14 - (hyp - bcy) * 5 / 14, gy + (hyp - bcy) * -3 / 14 + (hxp - bcx) * 5 / 14, FM(4, 2), acc, 0);
        cv_disc(gx + (hxp - bcx) * -3 / 14 + (hyp - bcy) * 5 / 14, gy + (hyp - bcy) * -3 / 14 - (hxp - bcx) * 5 / 14, FM(4, 2), acc, 0);
    }
    cv_disc(hxp - F(3), hyp - F(2), FM(3, 2), bg, 0);   /* the face */
    cv_disc(hxp + F(3), hyp - F(2), FM(3, 2), bg, 0);
    cv_rect(hxp - F(2), hyp + F(3), F(5), FM(1, 1), bg);
    if (cr.env > 8000 && h0 >= 40)                      /* bubbles while it sounds */
        for (i = 0; i < 3; i++) {
            int32_t ph = (cr.phase * 2 + i * 20) % 60;
            cv_disc(bcx + F(18) + i * F(5), bcy + F(24) - ph * F(1), FM(3, 2) + i, acc, 1);
        }
}

/* ---- the sloth (the lofi bus): a record turning on its back, a tonearm, a slow blink */
static void creature_sloth(int32_t cx, int32_t bottom, int32_t h0)
{
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t rx = cx - F(12) + (cr.idle > 20u ? cr_sway(F(1)) : 0), ry = bottom - F(40), rr = F(15), ang = cr.phase * 6, mx, my;
    int32_t hx = cx + F(18), hy = bottom - F(26), blink = (cr.phase % 90u) < 3u;
    cr_ellipse(cx, bottom - F(20), F(40), F(20), body); /* the body, lying */
    cv_disc(hx, hy, F(26), body, 0);                    /* the head, its face mask */
    cr_ellipse(hx + F(2), hy + F(1), F(9), F(7), bg);
    if (blink) { cv_disc(hx - F(2), hy - F(1), FM(3, 2), T_TEXT, 0); cv_disc(hx + F(6), hy - F(1), FM(3, 2), T_TEXT, 0); }
    else { cv_rect(hx - F(4), hy - F(1), F(4), FM(1, 1), T_TEXT); cv_rect(hx + F(4), hy - F(1), F(4), FM(1, 1), T_TEXT); }
    cv_rect(hx + F(1), hy + F(3), F(3), FM(2, 1), T_TEXT);   /* the nose */
    cv_disc(rx, ry, rr * 2, acc, 0);                    /* the record, its label, the hole, a mark turning */
    cv_disc(rx, ry, F(10), bg, 0);
    cv_disc(rx, ry, FM(3, 2), body, 0);
    mx = rx + F(11) * cr_cos(ang) / 256; my = ry + F(5) * cr_sin(ang) / 256;
    if (h0 >= 40) cv_disc(mx, my, FM(2, 1), bg, 0);
    cr_line(hx - F(6), ry - F(16) - (cr.blast ? F(4) : 0), rx + F(6), ry - F(4) - (cr.blast ? F(4) : 0), FM(2, 1), T_TEXT);   /* the arm */
    cv_disc(hx - F(6), ry - F(16) - (cr.blast ? F(4) : 0), FM(4, 2), T_TEXT, 0);
    if (cr.blast && h0 >= 40) {                         /* the needle skipping: sparks */
        cv_disc(rx + F(8), ry - F(12), FM(2, 1), acc, 0);
        cv_disc(rx + F(14), ry - F(16), FM(2, 1), acc, 0);
        cv_disc(rx + F(4), ry - F(18), FM(2, 1), acc, 0);
    }
    cv_disc(cx - F(34), bottom - F(10), F(10), body, 0);   /* a paw hugging the record's side */
}

/* ---- the blob (the fallback): squash, spring back, breathe */
static void creature_blob(int32_t cx, int32_t bottom, int32_t h0)
{
    int32_t s = cr.p, key = clamp(cr.key ? cr.key : 48, 24, 72), w0 = h0 + (72 - key) * h0 / 88;
    int32_t w, h, y0, r, ey, ex, my, mw, mh, ed = FM(9, 3), th = FM(6, 2);
    if (cr.idle > 20u) s = cr_sway(6);                  /* at rest: breathing */
    h = h0 - h0 * s * (s < 0 ? 33 : 11) / (25 * 256);   /* 44 % flatter at full squash; stretched, three times as
                                                         * much per step the other way, so the rebound shows */
    w = w0 + w0 * s * (s < 0 ? 33 : 11) / (25 * 256);
    if (h < h0 * 2 / 5) h = h0 * 2 / 5;
    if (w < h0 / 2) w = h0 / 2;
    y0 = bottom - h;
    r = (w < h ? w : h) / 2;
    cv_rrect(cx - w / 2, y0, w, h, r, T_THEME, T_BG);
    ey = y0 + h * 2 / 5;                                /* the eyes: dots; squashed hard, squinting lines */
    ex = w / 5;
    if (s > 140) {
        cv_rect(cx - ex - ed / 2, ey, ed, th / 2 < 1 ? 1 : th / 2, T_INK);
        cv_rect(cx + ex - ed / 2, ey, ed, th / 2 < 1 ? 1 : th / 2, T_INK);
    } else {
        cv_disc(cx - ex, ey, ed, T_INK, 0);
        cv_disc(cx + ex, ey, ed, T_INK, 0);
    }
    my = y0 + h * 3 / 5 + th / 2;                       /* the mouth: a line; a hard note, open with teeth */
    if (cr.vel >= 96u && cr.env > 32767 / 4) {
        int32_t tx;
        mw = w / 2;
        mh = clamp(h / 4, th + 1, h0 / 4);
        if (my + mh > bottom - 1) mh = bottom - 1 - my;
        cv_rrect(cx - mw / 2, my, mw, mh, th, T_INK, T_THEME);
        for (tx = cx - mw / 2 + th; tx + th <= cx + mw / 2 - th; tx += 2 * th)
            cv_rect(tx, my, th, mh > th + 1 ? th : 1, T_THEME);
    } else {
        cv_rect(cx - ed, my, 2 * ed - 1, th / 2 < 1 ? 1 : th / 2, T_INK);
    }
}
#undef F
#undef FM

/* the creature centred on cx, standing on row bottom, h0 px tall at rest (the band: 22; the play page: 64 or so) */
static void creature_draw_at(uint32_t kind, int32_t cx, int32_t bottom, int32_t h0)
{
    switch (kind) {
    case CR_FISH: creature_fish(cx, bottom, h0); break;
    case CR_BULLDOG: creature_bulldog(cx, bottom, h0); break;
    case CR_CHAMELEON: creature_chameleon(cx, bottom, h0); break;
    case CR_CLOUD: creature_cloud(cx, bottom, h0); break;
    case CR_OCTOPUS: creature_octopus(cx, bottom, h0); break;
    case CR_HEDGEHOG: creature_hedgehog(cx, bottom, h0); break;
    case CR_AXOLOTL: creature_axolotl(cx, bottom, h0); break;
    case CR_SLOTH: creature_sloth(cx, bottom, h0); break;
    case CR_BLOB: creature_blob(cx, bottom, h0); break;
    default: break;
    }
}
/* the play page's size for a creature (its body; some rise above or spread wider than they are tall) */
static int32_t creature_size(uint32_t kind)
{
    return kind == CR_FISH ? 76 : kind == CR_CLOUD || kind == CR_SLOTH ? 72 : 64;
}

/* the band's creature (ui_stage.c stage_band): right-aligned to x1 standing on row bottom, 22 px tall at rest;
 * returns the x where the notes must end */
static int32_t creature_draw(uint32_t kind, int32_t x1, int32_t bottom)
{
    if (kind == CR_NONE || kind == CR_OCTOPUS)          /* (a DRUM track's band shows its lanes) */
        return x1;
    creature_draw_at(kind, x1 - 30, bottom, 22);
    return x1 - 64;
}
