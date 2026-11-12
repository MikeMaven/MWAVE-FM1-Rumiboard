/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* Host test of the groove (seq.c GROOVES, docs/GROOVE.md), same sources as the firmware (through hostsim.c):
 *   build/host/groove_test
 * 1. OFF: every element plays at its step's boundary.
 * 2. DILLA: the kick 3 pulses late, the snare and the clap 1 early, the hats on the beat straight and the off-beat
 *    hats 3 late, a bass sound 10 pulses behind, another melodic track on the grid (a pulse: 1/96 of a beat).
 * 3. RUSH: the snare 10 pulses early (Charnas's 65 ms), fired ahead of its step.
 * 4. The project wire keeps the groove (the tail byte); the TEMPO page's fourth knob is GROOVE; the id is not reserved.
 * 5. A live take on the DRUM track (hits recorded off the grid): with OFF it plays as played; with a FEEL on, each hit
 *    plays once, from the grid, shifted (no flam of the recording's copy and the grid's). */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1                             /* (the project wire, as tests/project_test.c) */
#include "../firmware/src/project.c"

static int check(const char *what, int ok)
{
    printf("groove: %-80s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

typedef struct { uint8_t trk, note; uint32_t at; } ev_t;
static uint32_t step_at[16];                            /* the sample where track 1's step k began */

/* play from the transport's start, CTL samples a block: every voice that starts, with its sample */
static uint32_t play(ev_t *ev, uint32_t max, uint32_t samples)
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

/* the sample at which track t's note first started at or after the step's boundary minus slack; 0xFFFFFFFF: none */
static uint32_t first_at(const ev_t *ev, uint32_t n, uint32_t t, uint32_t note, uint32_t from, uint32_t to)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        if (ev[i].trk == t && ev[i].note == note && ev[i].at + 1u > from && ev[i].at < to)
            return ev[i].at;
    return 0xFFFFFFFFu;
}

static uint32_t count_in(const ev_t *ev, uint32_t n, uint32_t t, uint32_t note, uint32_t from, uint32_t to)
{
    uint32_t i, c = 0;
    for (i = 0; i < n; i++)
        if (ev[i].trk == t && ev[i].note == note && ev[i].at + 1u > from && ev[i].at < to) c++;
    return c;
}

/* a take: track 1 armed, note1 played at sample at1 and note2 at at2 (multiples of CTL), the transport then stopped */
static void play_rec(uint32_t samples, uint32_t note1, uint32_t at1, uint32_t note2, uint32_t at2)
{
    uint32_t f;
    int32_t o[2 * CTL];
    rec_set(&trk[0], 1);
    transport_req = 1;
    for (f = 0; f < samples; f += CTL) {
        mix_block(o, CTL);
        if (f == at1) input_on(&trk[0], note1, 100); else if (f == at1 + CTL) input_off(&trk[0], note1);
        if (f == at2) input_on(&trk[0], note2, 100); else if (f == at2 + CTL) input_off(&trk[0], note2);
    }
    transport_req = 2;
    mix_block(o, CTL);
    rec_set(&trk[0], 0);
}

static int near(uint32_t at, int32_t want, int32_t tol)
{
    return at != 0xFFFFFFFFu && (int32_t)at >= want - tol && (int32_t)at <= want + tol;
}

static void setup(uint32_t groove)
{
    uint32_t k;
    host_tracks_init();
    song.g[G_BPM] = 90;
    song.g[G_GROOVE] = (int16_t)groove;
    host_preset(&trk[0], ENGI_DRUM, 0);                 /* the 808 kit on track 1 */
    host_power_on_sound(&trk[1], 0);                    /* a bass sound on track 2, a pad on track 3 */
    host_power_on_sound(&trk[2], 1);
    groove_bass = 1u << 1;                              /* (the UI's role scan does not run here) */
    for (k = 0; k < 3u; k++) {
        trk[k].p[P_SLEN] = 16;
        trk[k].p[P_SDIV] = 2;                           /* 1/16 */
        trk[k].p[P_SSWING] = 0;
    }
    song.g[G_SWING] = 0;
    for (k = 0; k < 16u; k += 2) {                      /* hats on every 8th; kick 1 and 3, snare 2, clap 4 */
        step_t *s = &trk[0].step[k];
        s->time = ST_NOTE; s->vel = 100; s->n = 0;
        s->hit = (uint8_t)(1u << 3);
        if (k == 0 || k == 8) s->hit |= 1u << 0;
        if (k == 4) s->hit |= 1u << 1;
        if (k == 12) s->hit |= 1u << 2;
    }
    trk[1].step[0].time = ST_NOTE; trk[1].step[0].n = 1; trk[1].step[0].note[0] = 36; trk[1].step[0].vel = 100;
    trk[2].step[0].time = ST_NOTE; trk[2].step[0].n = 1; trk[2].step[0].note[0] = 60; trk[2].step[0].vel = 100;
}

int main(void)
{
    static ev_t ev[256];
    int bad = 0;
    uint32_t n, pulse, tol = 2u * CTL;
    const uint32_t KICK = 36, SNARE = 38, CLAP = 39, HAT = 42;
    pulse = (uint32_t)FS * 60u / 90u / 96u;             /* 306 samples at 90 BPM */

    setup(GROOVE_OFF);
    n = play(ev, 256, FS * 3u);
    bad += check("OFF: the kick, the hats, the snare, the bass and the pad start at their steps' boundaries",
                 near(first_at(ev, n, 0, KICK, 0, 2000), 0, tol) && near(first_at(ev, n, 0, HAT, 0, 2000), 0, tol) &&
                 near(first_at(ev, n, 0, HAT, step_at[2] - 100u, step_at[2] + 3000u), (int32_t)step_at[2], tol) &&
                 near(first_at(ev, n, 0, SNARE, step_at[4] - 4000u, step_at[4] + 3000u), (int32_t)step_at[4], tol) &&
                 near(first_at(ev, n, 1, 36, 0, 5000), 0, tol) && near(first_at(ev, n, 2, 60, 0, 5000), 0, tol));

    setup(GROOVE_DILLA);
    n = play(ev, 256, FS * 3u);
    bad += check("DILLA: the kick 3 pulses after its step (steps 1 and 9)",
                 near(first_at(ev, n, 0, KICK, 0, 4000), (int32_t)(3u * pulse), tol) &&
                 near(first_at(ev, n, 0, KICK, step_at[8], step_at[8] + 4000u), (int32_t)(step_at[8] + 3u * pulse), tol));
    bad += check("DILLA: the hats on the beat straight, the off-beat hats 3 pulses late",
                 near(first_at(ev, n, 0, HAT, 0, 2000), 0, tol) &&
                 near(first_at(ev, n, 0, HAT, step_at[4] - 100u, step_at[4] + 2000u), (int32_t)step_at[4], tol) &&
                 near(first_at(ev, n, 0, HAT, step_at[2] - 100u, step_at[2] + 3000u), (int32_t)(step_at[2] + 3u * pulse), tol) &&
                 near(first_at(ev, n, 0, HAT, step_at[6] - 100u, step_at[6] + 3000u), (int32_t)(step_at[6] + 3u * pulse), tol));
    bad += check("DILLA: the snare and the clap a pulse ahead of their steps",
                 near(first_at(ev, n, 0, SNARE, step_at[4] - 4000u, step_at[4] + 3000u), (int32_t)(step_at[4] - pulse), tol) &&
                 near(first_at(ev, n, 0, CLAP, step_at[12] - 4000u, step_at[12] + 3000u), (int32_t)(step_at[12] - pulse), tol));
    bad += check("DILLA: the bass sound 10 pulses behind its step, the other melodic track on the grid",
                 near(first_at(ev, n, 1, 36, 0, 8000), (int32_t)(10u * pulse), tol) && near(first_at(ev, n, 2, 60, 0, 5000), 0, tol));
    bad += check("DILLA: nothing plays twice (one kick, one snare, one clap, eight hats in the bar)",
                 first_at(ev, n, 0, KICK, 3u * pulse + tol + 1u, step_at[8]) == 0xFFFFFFFFu &&
                 first_at(ev, n, 0, SNARE, step_at[4] + tol, step_at[8]) == 0xFFFFFFFFu);

    setup(GROOVE_RUSH);
    n = play(ev, 256, FS * 3u);
    bad += check("RUSH: the snare 10 pulses ahead of its step, the kick 3 behind",
                 near(first_at(ev, n, 0, SNARE, step_at[4] - 6000u, step_at[4] + 3000u), (int32_t)(step_at[4] - 10u * pulse), tol) &&
                 near(first_at(ev, n, 0, KICK, 0, 4000), (int32_t)(3u * pulse), tol));

    {                                                   /* a live take: a hat 2500 samples into step 3, a snare into step 13 */
        const uint32_t step = (uint32_t)FS * 60u / 90u / 4u, hat_at = (2u * step + 2500u) / CTL * CTL, sn_at = (12u * step + 2500u) / CTL * CTL;
        const step_t *s2 = &trk[0].step[2], *s12 = &trk[0].step[12];
        uint32_t k;
        setup(GROOVE_OFF);
        for (k = 0; k < 16u; k++) trk[0].step[k].hit &= (uint8_t)~(1u << 3);   /* (no hats on the grid, no clap) */
        trk[0].step[12].hit = 0;
        play_rec(FS * 3u, HAT, hat_at, SNARE, sn_at);
        bad += check("a live take: the hat into step 3's cell and the snare into step 13's, as recorded steps",
                     (s2->flags & SF_RECORDED) && (s2->hit & 8u) && (s12->flags & SF_RECORDED) && (s12->hit & 2u));
        n = play(ev, 256, FS * 3u);
        bad += check("OFF: the take plays as played (the hat 2500 samples into its step, once; the snare too)",
                     count_in(ev, n, 0, HAT, 0, step_at[4]) == 1u && near(first_at(ev, n, 0, HAT, step_at[2], step_at[3]), (int32_t)(hat_at + CTL), (int32_t)tol + 8) &&
                     count_in(ev, n, 0, SNARE, step_at[8], step_at[8] + 8u * step) == 1u &&
                     near(first_at(ev, n, 0, SNARE, step_at[12], step_at[13]), (int32_t)(sn_at + CTL), (int32_t)tol + 8));
        song.g[G_GROOVE] = GROOVE_DILLA;
        n = play(ev, 256, FS * 3u);
        bad += check("DILLA: the take's hat plays once, from the grid, 3 pulses after its step; the snare once, a pulse early",
                     count_in(ev, n, 0, HAT, 0, step_at[4]) == 1u && near(first_at(ev, n, 0, HAT, step_at[2] - 100u, step_at[3]), (int32_t)(step_at[2] + 3u * pulse), tol) &&
                     count_in(ev, n, 0, SNARE, step_at[8], step_at[8] + 8u * step) == 1u &&
                     near(first_at(ev, n, 0, SNARE, step_at[12] - 4000u, step_at[13]), (int32_t)(step_at[12] - pulse), tol));
        song.g[G_GROOVE] = GROOVE_RUSH;
        n = play(ev, 256, FS * 3u);
        bad += check("RUSH: the take's snare plays once, 10 pulses ahead of its step",
                     count_in(ev, n, 0, SNARE, step_at[8], step_at[8] + 8u * step) == 1u &&
                     near(first_at(ev, n, 0, SNARE, step_at[12] - 6000u, step_at[13]), (int32_t)(step_at[12] - 10u * pulse), tol));
    }

    {
        static project_t q, q2;
        static project_store_t st;
        const page_t *pg = 0;
        uint32_t i;
        int ok;
        memset(&q, 0, sizeof q);                        /* a blank project, as tests/project_test.c makes one */
        for (i = 0; i < NPART; i++) {
            p5_patch_init(&q.p5[i]);
            cz_patch_init(q.cz[i].raw);
            memcpy(q.fm6[i], FM6_INIT, FM6_PACKED);
        }
        q.magic = PROJ_MAGIC; q.size = sizeof q; q.parts = NPART; q.phys = PROJ_PHYS;
        chain_legacy_defaults(&q.chain);
        q.g[G_GROOVE] = GROOVE_DRAG;
        q.sum = proj_sum(&q);
        ok = proj_pack(&st, &q) && proj_import(&q2, &st, sizeof st) && q2.g[G_GROOVE] == GROOVE_DRAG;
        q.g[G_GROOVE] = GROOVE_OFF;
        q.sum = proj_sum(&q);
        ok = ok && proj_pack(&st, &q) && proj_import(&q2, &st, sizeof st) && q2.g[G_GROOVE] == GROOVE_OFF;
        bad += check("the project wire keeps the groove (DRAG, then OFF)", ok);
        for (i = 0; i < NELEM(PAGES); i++)
            if (str_eq(PAGES[i].title, "TEMPO")) pg = &PAGES[i];
        bad += check("TEMPO's fourth knob is GROOVE (OFF DILLA DRAG RUSH), on an id that is not reserved",
                     pg && pg->id[3] == G_GROOVE && !global_reserved(G_GROOVE) && GP[G_GROOVE].max == GROOVE_N - 1 &&
                     str_eq(GP[G_GROOVE].names[GROOVE_DILLA], "DILLA") && str_eq(GP[G_GROOVE].names[GROOVE_RUSH], "RUSH"));
    }
    printf(bad ? "groove: %d FAILED\n" : "groove: all passed\n", bad);
    return bad != 0;
}
