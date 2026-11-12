/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* The groove (docs/GROOVE.md) on hits entered through the UI, on the UI test's bench (tests/ui_test.c):
 *   build/host/groove_ui_test
 * SEQ > DRUM HIT enters a hit as a recorded event (seq_notes.c drum_hit_edit: the step is SF_RECORDED, its grid cell an
 * overview); SEQ > STEP (the grid) enters hit bits. Both must play each hit once with a FEEL on, at its shifted time:
 * the off-beat hats 3 pulses late, a RUSH snare 10 pulses early (no flam of the recording's copy and the groove's). */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

typedef struct { uint8_t trk, note; uint32_t at; } ev_t;
static uint32_t step_at[16];

static uint32_t play_bar(ev_t *ev, uint32_t max, uint32_t samples)      /* (as tests/groove_test.c) */
{
    uint32_t f, n = 0, k, i, last_age = vage, last_idx = 0xFFFFu;
    int32_t o[2 * CTL];
    transport_req = 1;
    for (f = 0; f < samples; f += CTL) {
        mix_block(o, CTL);
        if (trk[0].seq_idx != last_idx) {
            last_idx = trk[0].seq_idx;
            if (last_idx < 16u) step_at[last_idx] = f;
        }
        for (k = 0; k < NPART; k++)
            for (i = 0; i < NVOICE; i++) {
                voice_t *v = &trk[k].v[i];
                if (v->active && v->age > last_age && n < max) {
                    ev[n].trk = (uint8_t)k; ev[n].note = v->note; ev[n].at = f; n++;
                }
            }
        last_age = vage;
    }
    transport_req = 2;
    mix_block(o, CTL);
    return n;
}

static uint32_t count_in(const ev_t *ev, uint32_t n, uint32_t note, uint32_t from, uint32_t to)
{
    uint32_t i, c = 0;
    for (i = 0; i < n; i++)
        if (ev[i].trk == 0 && ev[i].note == note && ev[i].at + 1u > from && ev[i].at < to) c++;
    return c;
}
static uint32_t first_at(const ev_t *ev, uint32_t n, uint32_t note, uint32_t from, uint32_t to)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        if (ev[i].trk == 0 && ev[i].note == note && ev[i].at + 1u > from && ev[i].at < to) return ev[i].at;
    return 0xFFFFFFFFu;
}
static int near(uint32_t at, int32_t want, int32_t tol)
{
    return at != 0xFFFFFFFFu && (int32_t)at >= want - tol && (int32_t)at <= want + tol;
}

/* the drum track on its page, the hats on every 8th and a snare on step 5, entered with the white keys */
static void enter(uint32_t graph)
{
    uint32_t p, s;
    ui_power_on();
    set_engine_of(&trk[0], ENGI_DRUM);
    trk[0].engine = trk[0].eng_req;
    song.g[G_BPM] = 90;
    open_family(FAM_SEQ);
    go_page(graph);
    frame();
    for (s = 0; s < drum_sound_count(&trk[0]) && drum_sound_note(&trk[0], s) != 42u; s++) ;
    ui.drum_sound = (uint8_t)s;                         /* DRUM HIT: KNOB 2's drum is HAT C */
    ui.lane = DV_HATC;                                 /* the grid: lane 4 (black key 4 would) */
    for (p = 0; p < 16u; p += 2) tap_key(key_at(0, p));
    for (s = 0; s < drum_sound_count(&trk[0]) && drum_sound_note(&trk[0], s) != 38u; s++) ;
    ui.drum_sound = (uint8_t)s;
    ui.lane = DV_SNARE;
    tap_key(key_at(0, 4));
}

static int run(const char *how, uint32_t graph, int recorded)
{
    static ev_t ev[512];
    int bad = 0;
    uint32_t n, pulse = (uint32_t)FS * 60u / 90u / 96u, tol = 2u * CTL, k, hats = 0, rec = 1, hit = 1;
    char what[160];
    enter(graph);
    for (k = 0; k < 16u; k += 2) {
        hats += (step_lanes(&trk[0].step[k]) >> DV_HATC) & 1u;
        rec &= (trk[0].step[k].flags & SF_RECORDED) != 0;
        hit &= (trk[0].step[k].hit >> DV_HATC) & 1u;
    }
    snprintf(what, sizeof what, "%s: eight hats and a snare entered%s", how, recorded ? " as recorded events (the cells an overview)" : " as hit bits");
    bad += check(what, hats == 8u && (step_lanes(&trk[0].step[4]) >> DV_SNARE & 1u) && (recorded ? rec : hit && !rec));
    song.g[G_GROOVE] = GROOVE_OFF;
    n = play_bar(ev, 512, FS * 3u);
    snprintf(what, sizeof what, "%s, OFF: every hat once, on its step", how);
    bad += check(what, count_in(ev, n, 42, 0, step_at[8]) == 4u && near(first_at(ev, n, 42, step_at[2] - 100u, step_at[3]), (int32_t)step_at[2], (int32_t)tol));
    song.g[G_GROOVE] = GROOVE_DILLA;
    n = play_bar(ev, 512, FS * 3u);
    snprintf(what, sizeof what, "%s, DILLA: every hat once (no flam), the off-beat ones 3 pulses late", how);
    bad += check(what, count_in(ev, n, 42, 0, step_at[8]) == 4u && count_in(ev, n, 42, step_at[8], 2u * step_at[8] - 200u) == 4u &&
                 near(first_at(ev, n, 42, step_at[2] - 100u, step_at[3]), (int32_t)(step_at[2] + 3u * pulse), (int32_t)tol) &&
                 near(first_at(ev, n, 42, 0, step_at[2]), 0, (int32_t)tol) &&   /* (step_at[0]: the second bar's, by now) */
                 count_in(ev, n, 38, 0, step_at[8]) == 1u);
    song.g[G_GROOVE] = GROOVE_RUSH;
    n = play_bar(ev, 512, FS * 3u);
    snprintf(what, sizeof what, "%s, RUSH: the snare once (no flam), 10 pulses early", how);
    bad += check(what, count_in(ev, n, 38, 0, step_at[8]) == 1u && near(first_at(ev, n, 38, step_at[4] - 6000u, step_at[5]), (int32_t)(step_at[4] - 10u * pulse), (int32_t)tol));
    return bad;
}

int main(void)
{
    int bad = 0;
    bad += run("SEQ > DRUM HIT", GR_DRUMHIT, 1);
    bad += run("SEQ > STEP (the grid)", GR_ROLL, 0);
    printf(bad ? "groove ui: %d FAILED\n" : "groove ui: all passed\n", bad);
    return bad != 0;
}
