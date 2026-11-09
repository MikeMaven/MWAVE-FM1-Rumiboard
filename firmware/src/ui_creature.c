/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* Rumiboard's creatures: a sound's living picture in Stage's band (ui_stage.c stage_band), drawn in the track's
 * colour at the band's right, the chord and notes keeping to its left. Each kind of sound gets its own (docs/
 * UI_DIRECTION.md "The playful layer"); this file is the pattern: creature_of says which one a sound gets,
 * creature_step moves it (once a band frame, from the ISR's per-track state), creature_draw draws it.
 *
 * CR_BLOB (BASS sounds): a round blob sitting on the band's floor. It squashes on every note (the harder the note,
 * the flatter), overshoots tall and settles: a damped spring on its squash, in Q8, stepped on the clock (one step
 * per 30 ms, however often the band draws: a forced redraw shows the same frame). Held, it stays a little squashed
 * (sitting on the note); let go, it rounds out; idle, it breathes. Lower notes make it wider. Its eyes squint while
 * it is squashed hard; a hard note (velocity 96 or more) bares its teeth. (The sub-bass engine's BITE will take
 * over the teeth when it exists; velocity stands in until then.)
 * Onset: the amp envelope of the track's latest voice (mod.c m_env, Q15) rising by a sixth of full scale between
 * two band frames, or the latest key changing while something sounds: a slow attack (a pad) gives no squash.
 * Cost: one rounded rectangle, two dots and a mouth; no RAM beyond the state below. */
enum { CR_NONE, CR_BLOB };

static struct {
    int16_t p, v;                           /* squash (Q8: 256 = flat, negative = stretched) and its velocity */
    int32_t env;                            /* m_env at the last step */
    uint8_t key, vel, trk, idle;            /* the latest key / velocity seen; the track (+ 1); steps at rest */
    uint32_t ms;                            /* fm1_ms at the last step */
} cr;
#define CR_STEP_MS 30u

static uint32_t creature_of(uint32_t cat) { return cat == CAT_BASS ? CR_BLOB : CR_NONE; }

/* one band frame: the spring, and a note's impulse (held: it keeps a little squash) */
static void creature_step(const track_t *t, int held)
{
    uint32_t sel = song.sel % NTRK + 1u, steps = (fm1_ms - cr.ms) / CR_STEP_MS;
    int32_t env = t->m_env, target = held ? 40 : 0;
    if (cr.trk != sel) {                                /* another track: start round (a note already sounding there
                                                         * counts as an onset: a squash to say hello) */
        memset(&cr, 0, sizeof cr);
        cr.trk = (uint8_t)sel;
        cr.ms = fm1_ms;
        steps = 0;
    }
    if (env - cr.env > 32767 / 6 || (t->m_key != cr.key && env > 32767 / 8)) {
        cr.vel = t->m_vel;
        cr.v = (int16_t)(cr.v + 30 + (int32_t)t->m_vel);   /* the impulse: 30 .. 157 */
        cr.idle = 0;
    }
    cr.env = env;
    cr.key = t->m_key;
    if (steps > 8u) steps = 8u;                         /* (a long gap: no catching up beyond a quarter second) */
    cr.ms += steps * CR_STEP_MS;
    while (steps--) {
        int32_t a = ((target - cr.p) >> 1) - (cr.v >> 2);   /* stiffness 1/2, damping 1/4: flat, up past round, settle */
        cr.v = (int16_t)clamp(cr.v + a, -300, 300);
        cr.p = (int16_t)clamp(cr.p + cr.v, -128, 200);
        if (cr.p == target && !cr.v) {
            if (cr.idle < 255u) cr.idle++;
        } else
            cr.idle = 0;
    }
}

/* what the drawing depends on (the play page redraws the creature when this changes) */
static uint32_t creature_sig(void)
{
    uint32_t s = (uint32_t)(cr.p + 256) * 7u + cr.key * 131u + cr.trk + (cr.vel >= 96u && cr.env > 32767 / 4) * 7919u;
    if (cr.idle > 20u)
        s += ((fm1_ms / 50u) % 48u) * 104729u;        /* (breathing) */
    return s;
}

/* the creature centred on cx, standing on row bottom, h0 px tall at rest (the band: 22; the play page: 64) */
static void creature_draw_at(uint32_t kind, int32_t cx, int32_t bottom, int32_t h0)
{
    int32_t s = cr.p, key = clamp(cr.key ? cr.key : 48, 24, 72), w0 = h0 + (72 - key) * h0 / 88;
    int32_t w, h, y0, r, ey, ex, my, mw, mh, ed = h0 / 7 < 3 ? 3 : h0 / 7, th = h0 / 10 < 2 ? 2 : h0 / 10;
    if (kind != CR_BLOB)
        return;
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

/* the band's creature (ui_stage.c stage_band): right-aligned to x1 standing on row bottom, 22 px tall at rest;
 * returns the x where the notes must end */
static int32_t creature_draw(uint32_t kind, int32_t x1, int32_t bottom)
{
    if (kind == CR_NONE)
        return x1;
    creature_draw_at(kind, x1 - 26, bottom, 22);
    return x1 - 56;
}
