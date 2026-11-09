/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* Rumiboard's creatures: a sound's living picture on the play page (ui_play.c), drawn from the concept sheets.
 * tools/gen_creatures.py turns each sheet (docs/creatures/refs/, the jpgs) into four 120 x 88 px poses in colour roles
 * (build/gen/creature_sprites.h, in the resource region): the firmware plays the poses to the music and moves the
 * picture a little (a bob, a lunge, a jump), so the art is the artist's and the motion is the sound's. Each creature
 * keeps its sheet's own colours (CR_SPR_COLS: body, accent, a second accent, dark; cut-outs and light the
 * background), as designed; the track's colour stays on the chrome around it. In the tests' GRAY palette they are
 * drawn as greys. The blob (CR_BLOB), drawn from shapes in the track's colour, stands in for sounds without a sheet.
 *
 * creature_of says which creature a sound gets, creature_step moves it (once per CR_STEP_MS on the clock, from the
 * ISR's per-track state: a forced redraw shows the same frame), creature_draw_canvas draws it, creature_sig says when
 * a redraw is due. All state is the struct below (RAM is scarce).
 *
 * The poses (the sheets' four, left to right) and what a note does:
 *   FISH       ready / inhaling / sound blast / return        a note: inhaling, then the blast while it sounds, return
 *   BULLDOG    idle / squash / bark / tail wag                a note: squash, then the bark (jumping) while it sounds, wag
 *   CHAMELEON  perch / snap / bend / retract                  a note: snap, then bent while it sounds, retract
 *   CLOUD      resting / churning / full bloom / dissipating  the envelope: churning as it rises, bloom at the top,
 *                                                             dissipating as it falls
 *   OCTOPUS    swing left / pad slam / knob twiddle / swing right   kick, snare, perc lanes: the slam; hats: the twiddle;
 *                                                             else swinging left and right with time
 *   HEDGEHOG   at guard / strike / vibration / damp           a note: the strike, the vibration while it rings, damp
 *   AXOLOTL    float / initiate flip / flip & trail / settle  a note: the flip, trailing while it sounds, settle
 *   SLOTH      lazy wobble / adjust weight / needle skip / return    idle: wobble and adjust in turn; a note (a peak,
 *                                                             later): the skip, then return
 * Onset: the amp envelope of the track's latest voice (mod.c m_env, Q15) rising by a sixth of full scale between
 * two steps, or the latest key changing while something sounds; a DRUM track's onsets are its lanes' hits
 * (ui_stage.c stage.hit_t). cr_force (tests, the emulator's picker) shows any creature regardless of the sound. */
#ifndef MELODEE_RES
#define MELODEE_RES                                     /* (host builds: the sprites in the program) */
#endif
#include "creature_sprites.h"

enum { CR_NONE, CR_BLOB, CR_FISH, CR_BULLDOG, CR_CHAMELEON, CR_CLOUD, CR_OCTOPUS, CR_HEDGEHOG, CR_AXOLOTL, CR_SLOTH,
       CR_N };
_Static_assert(CR_SLOTH - CR_FISH + 1 == CR_SPR_CREATURES, "a sheet per creature, in the enum's order");

static struct {
    int16_t p, v;                           /* the blob's spring: squash (Q8: 256 = flat, negative = stretched), velocity */
    int32_t env;                            /* m_env at the last step */
    uint8_t key, vel, trk, idle;            /* the latest key / velocity seen; the track (+ 1); steps at rest */
    uint8_t act;                            /* steps since the onset (255: none in a while) */
    uint8_t rel;                            /* steps since the sound ended (255: long ago) */
    uint8_t phase, lane, held, rising;      /* a step counter; the last drum lane hit; notes held; the envelope rising */
    int8_t bend;                            /* the pitch bend, -16 .. 16 (the chameleon's tongue waves) */
    uint8_t peak;                           /* the envelope's recent peak (m_env >> 7), falling slowly: the cloud's scale */
    uint32_t ms;                            /* fm1_ms at the last step */
} cr;
static uint8_t cr_force;                    /* tests, the emulator: a creature to show regardless of the sound (0: none) */
#define CR_STEP_MS 30u

/* a sound's creature by its category (the fallback) */
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

/* a sound names its creature: a word of its name (the first rule whose word starts a word of the name, '|': and
 * ends it; browse.c name_has_word), else a letter per factory preset of its engine (CREATURE_LETTER's index is the
 * creature; '.': none), else its category. Our own presets name theirs this way (and will carry an id when the
 * RUMI / 1984 banks exist). */
static const struct { uint8_t cr; const char *w; } CREATURE_WORDS[] = {
    {CR_HEDGEHOG, "PIANO"}, {CR_HEDGEHOG, "FELT"}, {CR_HEDGEHOG, "HONKY"}, {CR_HEDGEHOG, "HARPSI"}, {CR_HEDGEHOG, "CLAV"},
    {CR_AXOLOTL, "EP|"}, {CR_AXOLOTL, "E.P"}, {CR_AXOLOTL, "TINE"}, {CR_AXOLOTL, "RHODES"}, {CR_AXOLOTL, "WURL"},
    {CR_AXOLOTL, "SINE"}, {CR_AXOLOTL, "BELL"}, {CR_AXOLOTL, "KEYS"}, {CR_AXOLOTL, "TOY"},
    {CR_BULLDOG, "STAB"}, {CR_BULLDOG, "BRASS"}, {CR_BULLDOG, "HORN"}, {CR_BULLDOG, "CHORD"}, {CR_BULLDOG, "COMP"},
    {CR_BULLDOG, "HOUSE"}, {CR_BULLDOG, "PUNCH"},
    {CR_FISH, "SUB|"}, {CR_FISH, "BASS"}, {CR_FISH, "REESE"}, {CR_FISH, "808|"},
    {CR_CHAMELEON, "PLUCK"}, {CR_CHAMELEON, "LEAD"}, {CR_CHAMELEON, "LD|"}, {CR_CHAMELEON, "BEND"}, {CR_CHAMELEON, "GLIDE"},
    {CR_CHAMELEON, "SOLO"}, {CR_CHAMELEON, "ARP"}, {CR_CHAMELEON, "SYNC"},
    {CR_CLOUD, "PAD"}, {CR_CLOUD, "CLOUD"}, {CR_CLOUD, "SWELL"}, {CR_CLOUD, "DREAM"}, {CR_CLOUD, "ATMOS"}, {CR_CLOUD, "STRING"},
    {CR_CLOUD, "CHOIR"}, {CR_CLOUD, "VOX"}, {CR_CLOUD, "DRONE"}, {CR_CLOUD, "HAZE"},
    {CR_SLOTH, "TAPE"}, {CR_SLOTH, "LOFI"}, {CR_SLOTH, "LO-FI"}, {CR_SLOTH, "VINYL"}, {CR_SLOTH, "CASSETTE"}, {CR_SLOTH, "VHS"},
    {CR_OCTOPUS, "KIT|"}, {CR_OCTOPUS, "DRUM"}, {CR_OCTOPUS, "PERC"},
};
static const char CREATURE_LETTER[CR_N + 1u] = ".LFBCPOHAS";   /* a letter's index is its creature */
static const char *const PRESET_CREATURE[NENGINES] = {0};      /* a letter per factory preset of an engine; none yet */
static uint32_t creature_of_entry(uint32_t src, uint32_t k)
{
    static uint16_t last_src = 0xFFFFu, last_k;
    static uint8_t last_cr;
    uint32_t i, e;
    char tag[6], nm[24];
    if (cr_force) return cr_force;
    if (src == last_src && k == last_k) return last_cr;
    e = src_engine(src, k);
    entry_label(src, k, tag, nm);
    last_cr = 0;
    for (i = 0; i < NELEM(CREATURE_WORDS) && !last_cr; i++)
        if (name_has_word(nm, CREATURE_WORDS[i].w))
            last_cr = CREATURE_WORDS[i].cr;
    if (!last_cr && !src_slot(src) && e < NENGINES && PRESET_CREATURE[e] && k < ENGINES[e]->npresets) {
        for (i = 1; i < CR_N && CREATURE_LETTER[i] != PRESET_CREATURE[e][k]; i++)
            ;
        if (i < CR_N) last_cr = (uint8_t)i;
    }
    if (!last_cr)
        last_cr = (uint8_t)creature_of(entry_cat(src, k) % CAT_N);
    if (e == ENGI_DRUM) last_cr = CR_OCTOPUS;           /* (a drum kit is the octopus whatever its name) */
    last_src = (uint16_t)src; last_k = (uint16_t)k;
    return last_cr;
}

/* one step on the clock: the counters, the blob's spring, and a note's onset */
static void creature_step(const track_t *t, int held)
{
    uint32_t sel = song.sel % NTRK + 1u, steps = (fm1_ms - cr.ms) / CR_STEP_MS, l;
    int32_t env = t->m_env, target = held ? 40 : 0, onset = 0, drum = drum_track(t), sounding;
    if (cr.trk != sel) {                                /* another track: start at rest (a note already sounding there
                                                         * counts as an onset: a move to say hello) */
        memset(&cr, 0, sizeof cr);
        cr.trk = (uint8_t)sel;
        cr.ms = fm1_ms;
        cr.act = cr.rel = 255;
        steps = 0;
    }
    if (drum) {                                         /* a lane just hit (stage_panel_hits: 10 frames lit, one less each
                                                         * frame; this runs every other frame, so 10 or 9 is a new hit) */
        for (l = 0; l < NLANE; l++)
            if (stage.hit_t[l] >= 9u) { onset = 1; cr.lane = (uint8_t)l; }
        if (onset) cr.vel = 110;
    } else if (env - cr.env > 32767 / 6 || (t->m_key != cr.key && env > 32767 / 8)) {
        cr.vel = t->m_vel;
        cr.v = (int16_t)(cr.v + 30 + (int32_t)t->m_vel);   /* the blob's impulse: 30 .. 157 */
        onset = 1;
    }
    if (onset) { cr.act = 0; cr.rel = 255; cr.idle = 0; }
    cr.rising = (uint8_t)(env > cr.env + 200);
    cr.bend = (int8_t)clamp(t->bend_raw / 512, -16, 16);
    if ((env >> 7) > cr.peak) cr.peak = (uint8_t)(env >> 7);
    sounding = drum ? (cr.act < 10u) : (held || env > 32767 / 8);
    cr.held = (uint8_t)sounding;
    cr.env = env;
    cr.key = t->m_key;
    if (steps > 8u) steps = 8u;                         /* (a long gap: no catching up beyond a quarter second) */
    cr.ms += steps * CR_STEP_MS;
    while (steps--) {
        int32_t a = ((target - cr.p) >> 1) - (cr.v >> 2);   /* the blob: stiffness 1/2, damping 1/4 */
        cr.v = (int16_t)clamp(cr.v + a, -300, 300);
        cr.p = (int16_t)clamp(cr.p + cr.v, -128, 200);
        cr.phase++;
        if (cr.peak > 8u && !(cr.phase & 15u)) cr.peak--;   /* (the peak forgets slowly: a quieter sound still blooms) */
        if (cr.act < 255u) cr.act++;
        if (!sounding && cr.rel < 255u) cr.rel++;
        if (sounding) cr.rel = 0;
        if (!sounding && cr.act > 20u && cr.rel > 20u && cr.p == target && !cr.v) {
            if (cr.idle < 255u) cr.idle++;
        } else
            cr.idle = 0;
    }
}

/* a note's frames, per creature: the attack frames (CR_FRAME_STEPS steps each), the frame held while the sound
 * lasts, the release frames (0xFF: none). The frames in order: the terrier's idle, squash, launch, bark; the fish's
 * ready, opening, blast, return; the chameleon's perch, snap, reach, and the bend (held with the pitch wheel moved). */
typedef struct { uint8_t att[2], hold, rel[2]; } cr_seq_t;
static const cr_seq_t CR_SEQ[CR_N] = {
    [CR_FISH] = {{1, 0xFF}, 2, {3, 0xFF}},
    [CR_BULLDOG] = {{1, 2}, 3, {2, 1}},
    [CR_CHAMELEON] = {{1, 0xFF}, 2, {1, 0xFF}},
    [CR_HEDGEHOG] = {{1, 0xFF}, 2, {3, 0xFF}},
    [CR_AXOLOTL] = {{1, 0xFF}, 2, {3, 0xFF}},
};
#define CR_FRAME_STEPS 3u

/* the pose a creature shows now (0 .. 3, the sheet's), from the counters */
static uint32_t creature_pose(uint32_t kind)
{
    uint32_t a = cr.act, r = cr.rel, i;
    const cr_seq_t *q = &CR_SEQ[kind < CR_N ? kind : 0];
    if (kind == CR_CHAMELEON && cr.held && a >= CR_FRAME_STEPS && (cr.bend > 3 || cr.bend < -3))
        return 3;                                       /* held with the pitch wheel moved: the tongue waves */
    switch (kind) {
    case CR_CLOUD: {                                    /* the envelope's course, against its recent peak: churning as
                                                         * it rises, full bloom near the top, dissipating as it falls */
        int32_t e = cr.env >> 7, pk = cr.peak < 16u ? 16 : cr.peak;
        if (e * 10 >= pk * 7) return 2;
        if (e * 10 < pk * 2) return r < 40u && cr.act != 255u ? 3 : 0;
        return cr.rising || a < 12u ? 1 : 3;
    }
    case CR_OCTOPUS:                                    /* the lanes */
        if (a < 6u) return cr.lane == 3u || cr.lane == 4u ? 2 : 1;
        return (cr.phase / 16u) & 1u ? 3 : 0;
    case CR_SLOTH:                                      /* lazy in turn; a note: the skip */
        if (a < 6u) return 2;
        if (a < 16u) return 3;
        return (cr.phase / 24u) & 1u;
    default:                                            /* the attack frames, the held frame while it sounds, then the
                                                         * release frames, then rest */
        for (i = 0; i < 2u; i++)
            if (q->att[i] != 0xFFu && a < CR_FRAME_STEPS * (i + 1u))
                return q->att[i];
        if (cr.held || a < CR_FRAME_STEPS * 2u + 1u)
            return q->hold;
        for (i = 0; i < 2u; i++)
            if (q->rel[i] != 0xFFu && r < CR_FRAME_STEPS * (i + 1u))
                return q->rel[i];
        return 0;
    }
}

/* what the drawing depends on (the play page redraws the creature when this changes) */
static uint32_t creature_sig(void)
{
    uint32_t s = (uint32_t)(cr.p + 256) * 7u + cr.key * 131u + cr.trk + (cr.vel >= 96u && cr.env > 32767 / 4) * 7919u +
                 (cr.act < 16u ? cr.act : 16u) * 263u + (cr.rel < 16u ? cr.rel : 16u) * 613u + cr.lane * 17u + cr.held * 5u +
                 (uint32_t)(cr.env >> 10) * 37u + cr.rising * 3u + cr.peak * 11u + (uint32_t)(cr.bend + 16) * 1009u;
    if (cr.idle > 20u || cr.act < 16u)
        s += cr.phase * 104729u;                        /* (a bob, a jump, the sway, the record) */
    return s;
}

/* the idle sway: a slow triangle of +-amp from the clock */
static int32_t cr_sway(int32_t amp)
{
    int32_t ph = (int32_t)((fm1_ms / 50u) % 48u);
    return ((ph < 24 ? ph : 48 - ph) - 12) * amp / 12;
}

/* ---- the blob (the fallback), drawn from shapes: squash, spring back, breathe */
static void creature_blob(int32_t cx, int32_t bottom, int32_t h0)
{
    int32_t s = cr.p, key = clamp(cr.key ? cr.key : 48, 24, 72), w0 = h0 + (72 - key) * h0 / 88;
    int32_t w, h, y0, r, ey, ex, my, mw, mh, ed = h0 / 7 < 3 ? 3 : h0 / 7, th = h0 / 10 < 2 ? 2 : h0 / 10;
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

/* the creature on the current canvas (w x h, the play page's 120 x 88): its pose from the sheet, moved a little */
static void creature_draw_canvas(uint32_t kind, int32_t w, int32_t h)
{
    uint16_t cols[8];
    uint32_t pose, i;
    int32_t dx = 0, dy = 0;
    if (kind == CR_BLOB) {
        creature_blob(w / 2, h - 4, 64);
        return;
    }
    if (kind < CR_FISH || kind > CR_SLOTH)
        return;
    pose = creature_pose(kind);
    for (i = 0; i < 8u; i++) {                          /* the sheet's colours; cut-outs and light: the background */
        uint16_t c = CR_SPR_COLS[kind - CR_FISH][i];
        cols[i] = c ? c : T_BG;
#ifdef UI_TEST_PALETTE
        if (c && ux.pal == UI_GRAY_INDEX) {             /* (the tests' GRAY palette: every pixel a grey) */
            uint32_t l = ux_luma(c) >> 3;
            cols[i] = (uint16_t)((l << 11) | (l << 6) | l);
        }
#endif
    }
    i = (kind - CR_FISH) * CR_SPR_POSES + pose;
    if (cr.idle > 20u)                                  /* at rest: a slow bob */
        dy = cr_sway(kind == CR_CLOUD || kind == CR_AXOLOTL ? 3 : 2);
    if (cr.act < 12u) {                                 /* the note's motion: a lunge, a jump, a hop */
        int32_t t = (int32_t)cr.act;
        if (kind == CR_FISH || kind == CR_CHAMELEON) dx = t < 4 ? t * 2 : t < 10 ? 8 - (t - 4) : 2 - (t - 10);
        if (kind == CR_BULLDOG) dy = t < 3 ? t * 2 : t < 9 ? -(6 - (t - 3)) * 2 : 0;
        if (kind == CR_HEDGEHOG) dx = t < 4 ? t : t < 8 ? 4 - (t - 4) : 0;
        if (kind == CR_OCTOPUS) dy = t < 2 ? 3 : t < 5 ? 1 : 0;
    }
    cv_sprite(dx, dy, CR_SPR + CR_SPR_OFF[i], CR_SPR_OFF[i + 1] - CR_SPR_OFF[i], CR_SPR_W, CR_SPR_H, cols);
    (void)w; (void)h;
}

/* the band (ui_stage.c stage_band) shows no creature: they live on the play page */
static int32_t creature_draw(uint32_t kind, int32_t x1, int32_t bottom)
{
    (void)kind; (void)bottom;
    return x1;
}
