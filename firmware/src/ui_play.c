/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* HOME's play page (HOME_PG_PLAY, the page at power-on): the selected track's sound as its creature, large, standing
 * over its name; the track's four knobs always out above it (the strip: ui_stage.c stage_columns, the same cards
 * Stage drops in) and the band below as on Stage (the chord and the notes; no small creature there, the big one is
 * here). A sound without a creature yet (ui_creature.c creature_of) shows its waveform in the creature's place. HOME
 * turns the pages: Stage's rows, the chord view (ui_view.c), this one. ALGORITHM picks the track, PRESETS browses its
 * sounds, the knobs edit the sound, all as on Stage. Included by ui_draw.c after ui_stage.c and ui_view.c.
 * Redraws: the creature when its state changes (every other frame while it moves, now and then while it breathes), the
 * name when it changes, the knobs as Stage's cards; the band as on Stage. */
#define PL_CR_W 120                                     /* the creature's canvas: x 60 .. 180 */
#define PL_CR_H 88                                      /* .. rows 88 .. 176 */
#define PL_CR_X 60
#define PL_CR_Y (Y_GRAPH + 4)
#define PL_CR_SIZE 64                                   /* the blob at rest, px tall */
#define PL_NAME_H 26
#define PL_NAME_Y (ST_BAND_Y - PL_NAME_H - 2)           /* the name's line: rows 182 .. 207 */

static struct { uint32_t cr, name; } play;             /* what each part drew */

static void play_creature(uint32_t kind)
{
    uint32_t sig = ux.gen * 977u + ux.pal * 31u + (song.sel % NTRK) * 613u + kind * 31u, band, i, held = 0;
    if (kind) {
        fm1_irq_off();
        for (i = 0; i < 4u; i++) held |= live_held[song.sel % NTRK][i] != 0u;
        fm1_irq_on();
        creature_step(TSEL, held);
        sig += creature_sig();
        if (!ui.force && sig == play.cr)
            return;
        play.cr = sig;
        for (band = 0; band < (uint32_t)PL_CR_H; band += CV_MAX / (uint32_t)PL_CR_W) {
            cv_begin_band(PL_CR_W, PL_CR_H, T_BG, band);
            creature_draw_at(kind, PL_CR_W / 2, PL_CR_H - 4, PL_CR_SIZE);
            cv_blit_band(PL_CR_X, PL_CR_Y);
        }
        return;
    }
    play.cr = 0;                                        /* no creature: the waveform across, every other frame as Stage's */
    for (band = 0; band < (uint32_t)PL_CR_H; band += CV_MAX / 240u) {
        cv_begin_band(240, PL_CR_H, T_BG, band);
        stage_wave(ux_mix(T_BG, T_THEME, 70), 8, PL_CR_H - 16);
        cv_blit_band(0, PL_CR_Y);
    }
}

/* the sound's name under the creature (browsing: the sound the list shows, in the accent, not yet loaded) */
static void play_name(void)
{
    char nm[24];
    uint32_t sig;
    int pending = browse_pending();
    const aafont_t *f;
    if (pending) {
        char tag[6];
        uint32_t src, kk;
        src = browse_shown(&kk);
        entry_label(src, kk, tag, nm);
        if (!nm[0]) str_cpy(nm, tag, sizeof nm);
    } else {
        sound_name(TSEL, nm);
        if (!nm[0]) str_cpy(nm, ENGINES[eng_idx(TSEL->eng_req)]->name, sizeof nm);
    }
    sig = str_hash(ux.gen * 977u + ux.pal * 31u + (song.sel % NTRK) * 613u + (uint32_t)pending * 5u, nm);
    if (!ui.force && sig == play.name)
        return;
    play.name = sig;
    f = ty_fit(TY_TX, nm, 228);
    cv_begin(240, PL_NAME_H, T_BG);
    cv_text_c(120, ty_y(PL_NAME_H / 2, f), f, nm, pending ? T_ACCENT : T_TEXT, T_BG);
    cv_blit(0, PL_NAME_Y);
}

static void play_draw(void)
{
    stage_panel_hits();                                 /* (the DRUM lanes' flashes, for the band) */
    if (ui.force)
        memset(ui.col, 0, sizeof ui.col);               /* (the cards again after a page turn's clear) */
    stage_columns();                                    /* the knobs, always out */
    if (!ui.force && (ui.frame & 1u))
        return;
    play_creature(creature_of(stage_sel_cat()));
    play_name();
    stage_band();
}
