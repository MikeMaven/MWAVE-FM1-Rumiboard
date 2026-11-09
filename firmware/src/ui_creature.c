/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* Rumiboard's creatures: a sound's living picture, large on the play page (ui_play.c) and small at the right of
 * Stage's band (ui_stage.c stage_band). Each sound family gets its own (docs/creatures/README.md, the sheets in
 * docs/creatures/refs/); this file is the pattern: creature_of says which one a sound gets, creature_step moves it
 * (once per CR_STEP_MS on the clock, from the ISR's per-track state: a forced redraw shows the same frame),
 * creature_draw_at draws it at any size, creature_sig says when a redraw is due.
 *
 * Colours are roles: the body the track's colour (T_THEME), the accent (fins, the lure) the track's first companion
 * shade (T_KNOB(1)), cut-outs (eyes, mouths) the background, a dark detail the text colour. Built from discs,
 * rounded rectangles and column spans (cv_vspan_aa: ellipses, fins, arcs); no sprites. State: the struct below.
 *
 * CR_FISH (BASS sounds, refs/bass_anglerfish.jpg): an anglerfish facing right. A note opens its mouth (the harder,
 * the wider: inhaling), then the lure flares with sound rings (the blast), the fish lunging forward; held, the mouth
 * stays open; let go, it closes. Idle, it floats (a slow bob) and the lure bobs.
 * CR_BLOB (every other synth sound, the fallback): a round blob that squashes on a note, springs back past round,
 * settles; sits a little squashed while held; breathes when idle; lower notes make it wider; squashed hard its eyes
 * squint, a hard note (velocity 96 or more) bares its teeth while it sounds.
 * Onset: the amp envelope of the track's latest voice (mod.c m_env, Q15) rising by a sixth of full scale between
 * two steps, or the latest key changing while something sounds (a pad's slow attack gives no action). */
enum { CR_NONE, CR_BLOB, CR_FISH };

static struct {
    int16_t p, v;                           /* the blob: squash (Q8: 256 = flat, negative = stretched) and its velocity */
    int32_t env;                            /* m_env at the last step */
    uint8_t key, vel, trk, idle;            /* the latest key / velocity seen; the track (+ 1); steps at rest */
    uint8_t mouth, blast, lunge;            /* the fish: the mouth 0 .. 255, blast steps left (the rings), lunge px */
    uint32_t ms;                            /* fm1_ms at the last step */
} cr;
#define CR_STEP_MS 30u

static uint32_t creature_of(uint32_t cat)
{
    if (cat == CAT_BASS) return CR_FISH;
    if (cat == CAT_DRUM) return CR_NONE;                /* (the drums' octopus is to come; the band shows the lanes) */
    return CR_BLOB;
}

/* one step on the clock: the blob's spring, the fish's mouth, and a note's impulse (held: the blob keeps a little
 * squash, the fish its mouth open) */
static void creature_step(const track_t *t, int held)
{
    uint32_t sel = song.sel % NTRK + 1u, steps = (fm1_ms - cr.ms) / CR_STEP_MS;
    int32_t env = t->m_env, target = held ? 40 : 0, onset = 0;
    if (cr.trk != sel) {                                /* another track: start at rest (a note already sounding there
                                                         * counts as an onset: a move to say hello) */
        memset(&cr, 0, sizeof cr);
        cr.trk = (uint8_t)sel;
        cr.ms = fm1_ms;
        steps = 0;
    }
    if (env - cr.env > 32767 / 6 || (t->m_key != cr.key && env > 32767 / 8)) {
        cr.vel = t->m_vel;
        cr.v = (int16_t)(cr.v + 30 + (int32_t)t->m_vel);   /* the blob's impulse: 30 .. 157 */
        cr.mouth = (uint8_t)(120 + t->m_vel);               /* the fish: inhaling, 120 .. 247 */
        cr.blast = 7;
        cr.lunge = 5;
        cr.idle = 0;
        onset = 1;
    }
    cr.env = env;
    cr.key = t->m_key;
    if (steps > 8u) steps = 8u;                         /* (a long gap: no catching up beyond a quarter second) */
    cr.ms += steps * CR_STEP_MS;
    while (steps--) {
        int32_t a = ((target - cr.p) >> 1) - (cr.v >> 2);   /* the blob: stiffness 1/2, damping 1/4 */
        int32_t mt = held && env > 32767 / 8 ? 150 : 0;      /* the fish: the mouth's rest while held, else closed */
        cr.v = (int16_t)clamp(cr.v + a, -300, 300);
        cr.p = (int16_t)clamp(cr.p + cr.v, -128, 200);
        if (cr.blast) cr.blast--;
        if (cr.lunge) cr.lunge--;
        if (!onset || steps < 7u)                       /* (the first step after an onset keeps the mouth wide) */
            cr.mouth = (uint8_t)(cr.mouth + (mt - (int32_t)cr.mouth) / 3);
        if (cr.p == target && !cr.v && !cr.blast && (int32_t)cr.mouth <= mt + 2) {
            if (cr.idle < 255u) cr.idle++;
        } else
            cr.idle = 0;
    }
}

/* what the drawing depends on (the play page redraws the creature when this changes) */
static uint32_t creature_sig(void)
{
    uint32_t s = (uint32_t)(cr.p + 256) * 7u + cr.key * 131u + cr.trk + (cr.vel >= 96u && cr.env > 32767 / 4) * 7919u +
                 cr.mouth * 263u + cr.blast * 4099u + cr.lunge * 613u;
    if (cr.idle > 20u)
        s += ((fm1_ms / 50u) % 48u) * 104729u;        /* (breathing, floating) */
    return s;
}

/* ---- the parts: a filled ellipse, a triangle, a thick arc, by columns (cv_vspan_aa: 1/16 px, anti-aliased) */
static int32_t cr_isqrt(int32_t q)
{
    int32_t s = 0;
    while ((s + 1) * (s + 1) <= q) s++;
    return s;
}
static void cr_ellipse(int32_t cx, int32_t cy, int32_t a, int32_t b, uint16_t c)
{
    int32_t x;
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
/* the upper-left quarter of a circle (centre cx, cy, radius r), th px thick: the lure's stalk */
static void cr_arc(int32_t cx, int32_t cy, int32_t r, int32_t th, uint16_t c)
{
    int32_t x;
    for (x = cx - r; x <= cx; x++) {
        int32_t dx = x - cx, y = cr_isqrt(16 * (r * r - dx * dx));   /* 1/4 px */
        cv_vspan_aa(x, cy * 16 - y * 4 - th * 8, cy * 16 - y * 4 + th * 8, c);
    }
}

/* the anglerfish, centred on cx, its body's floor on row bottom, h0 px tall at rest (the body; the lure rises above) */
static void creature_fish(int32_t cx, int32_t bottom, int32_t h0)
{
#define F(v) ((v) * h0 / 64)                            /* the sheet's units: a 64 px fish */
    uint16_t body = T_THEME, acc = T_KNOB(1), bg = T_BG;
    int32_t m = cr.mouth, bob = 0, a = F(36), b = F(19), cy, hx, x, lure_x, lure_y, i;
    if (cr.idle > 20u) {                                /* floating: a slow bob of the body, the lure after it */
        int32_t ph = (int32_t)((fm1_ms / 50u) % 48u);
        bob = ((ph < 24 ? ph : 48 - ph) - 12) * F(4) / 12;
    }
    cx += cr.lunge * F(1);
    cy = bottom - F(26) + bob / 2;
    /* the tail (accent): two fins in a V at the back */
    cr_tri(cx - a + F(6), cy - F(4), cy + F(4), cx - a - F(14), cy - F(16), acc);
    cr_tri(cx - a + F(6), cy - F(4), cy + F(4), cx - a - F(14), cy + F(16), acc);
    /* the dorsal fin (accent), then the body over the fins' roots */
    cr_tri(cx - F(26), cy - b + F(8), cy - b + F(2), cx - F(8), cy - b - F(12), acc);
    cr_ellipse(cx, cy, a, b, body);
    cr_ellipse(cx - F(4), cy + F(12), F(9), F(4), acc);   /* the pectoral fin */
    /* the mouth: a wedge cut out of the front from the hinge, open by m; closed: a seam. Teeth (the body's colour)
     * along both jaws once it is open */
    hx = cx + F(6);
    if (m > 24) {
        int32_t open = F(18) * m / 255, len = cx + a + F(2) - hx;
        for (x = hx; x <= cx + a + F(2); x++) {
            int32_t o = open * (x - hx) / len;
            cv_vspan_aa(x, (cy + F(2)) * 16 - o * 16, (cy + F(2)) * 16 + o * 16, bg);
        }
        if (open >= F(6)) {
            int32_t tw = F(5) < 3 ? 3 : F(5);
            for (i = 0; i < 3; i++) {
                int32_t tx = hx + len * (2 + 3 * i) / 10, o = open * (tx - hx) / len;
                cr_tri(tx, cy + F(2) - o, cy + F(2) - o + 1, tx + tw, cy + F(2) - o + tw, body);   /* upper jaw's */
                cr_tri(tx + tw, cy + F(2) + o - 1, cy + F(2) + o, tx + 2 * tw, cy + F(2) + o - tw, body);   /* lower's */
            }
        }
    } else {
        cv_rect(hx + F(8), cy + F(3), a - F(10), h0 >= 48 ? 2 : 1, bg);
    }
    /* the eye (a cut-out, its pupil dark) and two spots on the cheek */
    cv_disc(cx + F(18), cy - F(7), F(8) < 3 ? 3 : F(8), bg, 0);
    if (h0 >= 40) cv_disc(cx + F(19), cy - F(7), F(3), T_TEXT, 0);
    if (h0 >= 40) {
        cv_disc(cx - F(2), cy - F(3), F(4), bg, 0);
        cv_disc(cx + F(5), cy - F(3), F(4), bg, 0);
    }
    /* the lure: a stalk arcing up from the forehead to the bulb; blasting: rings round the bulb */
    lure_x = cx + F(30);
    lure_y = cy - b - F(16) - bob;
    if (h0 >= 40) {
        cr_arc(lure_x, cy - b + F(2), F(20), h0 >= 48 ? 2 : 1, acc);
        lure_y = cy - b + F(2) - F(20);
    }
    cv_disc(lure_x, lure_y, cr.blast ? F(10) : F(7) < 3 ? 3 : F(7), acc, 0);
    if (cr.blast && h0 >= 40)
        for (i = 0; i < 2; i++) {
            int32_t d = F(14) + (7 - (int32_t)cr.blast) * F(3) + i * F(8);
            if (d > F(40)) continue;
            cv_disc(lure_x, lure_y, d, acc, 1);
        }
#undef F
}

/* the blob, centred on cx, standing on row bottom, h0 px tall at rest */
static void creature_blob(int32_t cx, int32_t bottom, int32_t h0)
{
    int32_t s = cr.p, key = clamp(cr.key ? cr.key : 48, 24, 72), w0 = h0 + (72 - key) * h0 / 88;
    int32_t w, h, y0, r, ey, ex, my, mw, mh, ed = h0 / 7 < 3 ? 3 : h0 / 7, th = h0 / 10 < 2 ? 2 : h0 / 10;
    if (cr.idle > 20u) {                                /* at rest: breathing, a slow triangle of +-6 */
        int32_t ph = (int32_t)((fm1_ms / 50u) % 48u);
        s = (ph < 24 ? ph : 48 - ph) - 12;
        s = s / 2;
    }
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
    if (cr.vel >= 96u && cr.env > 32767 / 4) {         /* (while the hard note sounds) */
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

/* the creature centred on cx, standing on row bottom, h0 px tall at rest (the band: 22; the play page: 64) */
static void creature_draw_at(uint32_t kind, int32_t cx, int32_t bottom, int32_t h0)
{
    if (kind == CR_FISH)
        creature_fish(cx, bottom, h0);
    else if (kind == CR_BLOB)
        creature_blob(cx, bottom, h0);
}

/* the band's creature (ui_stage.c stage_band): right-aligned to x1 standing on row bottom, 22 px tall at rest;
 * returns the x where the notes must end */
static int32_t creature_draw(uint32_t kind, int32_t x1, int32_t bottom)
{
    if (kind == CR_NONE)
        return x1;
    creature_draw_at(kind, x1 - 30, bottom, 22);
    return x1 - 64;
}
