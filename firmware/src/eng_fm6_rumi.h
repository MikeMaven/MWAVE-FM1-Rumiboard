/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Mike Maven and Rumiboard contributors */
/* The RUMI bank's FM6 voices (Rumiboard's own, no instrument's ROM data), after INIT VOICE in the preset list
 * (eng_fm6.c fm6_factory, FM6_PRESETS). The macros as eng_fm6_rom.h: OP(EG rates 1-4, EG levels 1-4, key level
 * scaling break point, left / right depth, left / right curve, key rate scaling, AMS, velocity, output level, coarse
 * (0 = 0.5), fine, detune -7..7); OPX: a fixed frequency (coarse 0..3 = 1 Hz .. 1 kHz, x (1 + fine / 100)).
 * Algorithm 5: three pairs (1<-2, 3<-4, 5<-6, the feedback on 6); 32: six carriers.
 * Their browser categories: browse.c PRESET_CAT (KEYS KEYS BELL PAD BASS KEYS).
 * The recipes: reports/Lofi house and chillhop synth recipes.md (the DX7 tine's x14 modulator, Plugin Boutique's FM
 * cookbook, Yamaha's fixed-frequency hammer), docs/VISION.md (the sine family, the sub, the felt piano). */
#define OP(r1, r2, r3, r4, l1, l2, l3, l4, bp, ld, rd, lc, rc, rs, am, kv, ol, cr, fi, dt) \
    {r1, r2, r3, r4, l1, l2, l3, l4, bp, ld, rd, lc, rc, rs, am, kv, ol, 0, cr, fi, (dt) + 7}
#define OPX(r1, r2, r3, r4, l1, l2, l3, l4, bp, ld, rd, lc, rc, rs, am, kv, ol, cr, fi, dt) \
    {r1, r2, r3, r4, l1, l2, l3, l4, bp, ld, rd, lc, rc, rs, am, kv, ol, 1, cr, fi, (dt) + 7}
#define OP_OFF OP(99, 99, 99, 99, 99, 99, 99, 0, 39, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0)
#define PEG(r1, r2, r3, r4, l1, l2, l3, l4) {r1, r2, r3, r4}, {l1, l2, l3, l4}
#define PEG_FLAT PEG(99, 99, 99, 99, 50, 50, 50, 50)
#define VOICE(o1, o2, o3, o4, o5, o6, peg, alg, fb, oks, lfo, name) \
    {{o1, o2, o3, o4, o5, o6}, peg, (alg) - 1, fb, oks, lfo, 24, name}
#define LFO(spd, dly, pmd, amd, sync, wave, pms) spd, dly, pmd, amd, sync, wave, pms
#define NO_LFO LFO(35, 0, 0, 0, 1, 0, 0)

static const fm6_rom_t FM6_RUMI[] MELODEE_RES = {
    /* RUMI TINES: the tine EP dulled for lofi; the x14 tine (OP4) at a lower level and gone sooner, the body
     * modulators pulled back, the three carriers spread -3 / 0 / +4, a longer tail, little feedback */
    VOICE(OP(95, 32, 24, 52, 99, 0, 0, 0, 39, 0, 0, 0, 0, 3, 0, 2, 98, 1, 0, -3),
          OP(95, 45, 25, 60, 99, 0, 0, 0, 39, 0, 0, 0, 0, 3, 0, 5, 60, 1, 0, -1),
          OP(95, 30, 24, 50, 99, 0, 0, 0, 39, 0, 0, 0, 0, 3, 0, 2, 92, 1, 0, 0),
          OP(99, 74, 40, 60, 99, 0, 0, 0, 51, 0, 40, 0, 1, 3, 0, 6, 50, 14, 0, 0),
          OP(95, 33, 24, 52, 99, 0, 0, 0, 39, 0, 0, 0, 0, 3, 0, 2, 84, 1, 0, 4),
          OP(95, 50, 25, 60, 99, 0, 0, 0, 39, 0, 0, 0, 0, 3, 0, 5, 52, 1, 0, 2),
          PEG_FLAT, 5, 1, 1, LFO(30, 0, 0, 0, 1, 0, 0), "RUMI TINES"),
    /* RUMI SINE: the sine family (VISION 2): two sine carriers 6 cents apart, each under a low-index modulator
     * (x1 and x2), decaying like a key; nothing else */
    VOICE(OP(92, 30, 22, 55, 99, 90, 0, 0, 39, 0, 0, 0, 0, 2, 0, 2, 99, 1, 0, -3),
          OP(95, 40, 30, 60, 99, 70, 0, 0, 39, 0, 0, 0, 0, 2, 0, 3, 46, 1, 0, 0),
          OP(92, 32, 22, 55, 99, 90, 0, 0, 39, 0, 0, 0, 0, 2, 0, 2, 96, 1, 0, 3),
          OP(95, 45, 30, 60, 99, 60, 0, 0, 39, 0, 0, 0, 0, 2, 0, 3, 40, 2, 0, 0),
          OP_OFF, OP_OFF,
          PEG_FLAT, 5, 0, 1, NO_LFO, "RUMI SINE "),
    /* RUMI BELL: a soft bell: a 1 : 3.5 pair (the inharmonic partial, its modulator fading first) beside a 1 : 2
     * pair, long carriers, no feedback */
    VOICE(OP(99, 24, 20, 40, 99, 80, 0, 0, 39, 0, 0, 0, 0, 1, 0, 1, 99, 1, 0, 2),
          OP(99, 55, 40, 60, 99, 0, 0, 0, 39, 0, 0, 0, 0, 2, 0, 3, 56, 3, 50, 0),
          OP(99, 26, 20, 40, 99, 75, 0, 0, 39, 0, 0, 0, 0, 1, 0, 1, 90, 1, 0, -3),
          OP(99, 50, 40, 60, 99, 0, 0, 0, 39, 0, 0, 0, 0, 2, 0, 3, 48, 2, 0, 0),
          OP_OFF, OP_OFF,
          PEG_FLAT, 5, 0, 1, NO_LFO, "RUMI BELL "),
    /* RUMI HAZE: the ambient sine texture (VISION 7): the sine pairs at -7 / +7 with a slow rise and a long tail, a
     * half-speed carrier under them, a slow sine LFO on the pitch */
    VOICE(OP(42, 20, 20, 32, 99, 95, 0, 0, 39, 0, 0, 0, 0, 1, 0, 1, 99, 1, 0, -7),
          OP(50, 30, 25, 40, 99, 80, 0, 0, 39, 0, 0, 0, 0, 1, 0, 2, 44, 1, 0, 0),
          OP(42, 22, 20, 32, 99, 95, 0, 0, 39, 0, 0, 0, 0, 1, 0, 1, 96, 1, 0, 7),
          OP(50, 30, 25, 40, 99, 70, 0, 0, 39, 0, 0, 0, 0, 1, 0, 2, 40, 2, 0, 0),
          OP(38, 20, 20, 30, 99, 95, 0, 0, 39, 0, 0, 0, 0, 1, 0, 1, 78, 0, 0, 0),
          OP(50, 30, 25, 40, 99, 70, 0, 0, 39, 0, 0, 0, 0, 1, 0, 2, 30, 1, 0, 0),
          PEG_FLAT, 5, 0, 0, LFO(18, 0, 4, 0, 0, 4, 2), "RUMI HAZE "),
    /* RUMI SUB: the sub (VISION 1): one sine with a soft attack (no click), full sustain, a short release; a quiet
     * octave above it for warmth. Mono (FM6_PRESETS) */
    VOICE(OP(76, 60, 99, 72, 99, 99, 99, 0, 39, 0, 0, 0, 0, 0, 0, 1, 99, 1, 0, 0),
          OP_OFF,
          OP(76, 60, 99, 72, 99, 99, 99, 0, 39, 0, 0, 0, 0, 0, 0, 1, 56, 2, 0, 0),
          OP_OFF, OP_OFF, OP_OFF,
          PEG_FLAT, 32, 0, 1, NO_LFO, "RUMI SUB  "),
    /* RUMI FELT: the felt piano by FM (VISION 4's fallback): a sine body whose x1 modulator is heard only in the
     * attack (the felt: a low-pass on the hammer), an octave carrier with a short x1 modulator, and a hammer thud:
     * a fixed 100 Hz carrier driven hard for 30 ms (Yamaha's piano trick) */
    VOICE(OP(99, 28, 22, 55, 99, 85, 0, 0, 39, 0, 0, 0, 0, 2, 0, 3, 99, 1, 0, 0),
          OP(99, 68, 40, 60, 99, 0, 0, 0, 39, 0, 0, 0, 0, 3, 0, 4, 58, 1, 0, 0),
          OP(99, 34, 24, 55, 99, 70, 0, 0, 39, 0, 0, 0, 0, 2, 0, 3, 68, 2, 0, 4),
          OP(99, 75, 45, 60, 99, 0, 0, 0, 39, 0, 0, 0, 0, 3, 0, 4, 40, 1, 0, 0),
          OPX(99, 86, 99, 90, 99, 0, 0, 0, 39, 0, 0, 0, 0, 0, 0, 2, 66, 2, 0, 0),
          OPX(99, 88, 99, 90, 99, 0, 0, 0, 39, 0, 0, 0, 0, 0, 0, 2, 88, 2, 0, 0),
          PEG_FLAT, 5, 0, 1, NO_LFO, "RUMI FELT "),
};
#define FM6_NRUMI (sizeof FM6_RUMI / sizeof FM6_RUMI[0])

#undef OP
#undef OPX
#undef OP_OFF
#undef PEG
#undef PEG_FLAT
#undef VOICE
#undef LFO
#undef NO_LFO
