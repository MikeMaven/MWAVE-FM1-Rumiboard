#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Mike Maven and Rumiboard contributors
"""Rumiboard's own Prophet programs (the RUMI bank) -> firmware/src/prophet_rumi.h, 133 raw bytes each.

  tools/gen_prophet_rumi.py [firmware/src/prophet_rumi.h]

The programs follow Sequential's 200 in the Prophet's preset list (eng_prophet_test.c p5_factory: k >= P5_SEQUENTIAL_N
is P5_RUMI[k - P5_SEQUENTIAL_N]) and are written here as panel settings on top of INIT (prophet_patch.c p5_patch_init;
the raw layout: prophet_patch.c, the panel: prophet_edit.h). The recipes: reports/Lofi house and chillhop synth
recipes.md (Attack Magazine's Juno templates, Roland's Juno basses, Attack's Diva portamento lead), docs/VISION.md.
A knob is 0..127; FREQ A / B are stored (semitones + 12) * 2; BEND is stored semitones - 1."""
import sys

(FREQ_A, FREQ_B, FINE_B, SAW_A, PULSE_A, SAW_B, TRI_B, PULSE_B, PW_A, PW_B, SYNC, LOW_B, KEY_B, GLIDE, LEVEL_A, LEVEL_B,
 NOISE, CUTOFF, RESONANCE, KEY_FILTER, FILTER_REV, LFO_RATE, LFO_INITIAL, LFO_SAW, LFO_TRI, LFO_PULSE, WHEEL_MIX,
 WHEEL_FREQ_A, WHEEL_FREQ_B, WHEEL_PW_A, WHEEL_PW_B, WHEEL_FILTER, POLY_ENV, POLY_B, POLY_FREQ, POLY_PW, POLY_FILTER,
 VINTAGE, PRESS_FILTER, PRESS_LFO, ENV_FILTER, VEL_FILTER, VEL_AMP, ATTACK_FILTER, ATTACK_AMP, DECAY_FILTER, DECAY_AMP,
 SUSTAIN_FILTER, SUSTAIN_AMP, RELEASE_FILTER, RELEASE_AMP, RELEASE_ON, UNISON, UNISON_COUNT, UNISON_DETUNE) = range(55)
NAME, NAME_LEN, BEND, RETRIGGER = 65, 20, 86, 87


def semi(st):
    return (st + 12) * 2


def init():
    """as prophet_patch.c p5_patch_init"""
    r = [0] * 133
    r[FREQ_A] = r[FREQ_B] = semi(0)
    r[SAW_A] = r[KEY_B] = 1
    r[PW_A] = r[PW_B] = 64
    r[LEVEL_A] = r[CUTOFF] = 127
    r[FILTER_REV] = 1
    r[DECAY_FILTER] = r[DECAY_AMP] = 60
    r[SUSTAIN_FILTER] = r[SUSTAIN_AMP] = 127
    r[RELEASE_FILTER] = r[RELEASE_AMP] = 35
    r[RELEASE_ON] = 1
    r[UNISON_COUNT] = 5
    return r


def program(name, fenv=None, aenv=None, **fields):
    """a program: INIT, the filter and amp envelopes (A D S R), then fields by their index name"""
    assert len(name) <= NAME_LEN and name == name.upper()
    r = init()
    if fenv:
        r[ATTACK_FILTER], r[DECAY_FILTER], r[SUSTAIN_FILTER], r[RELEASE_FILTER] = fenv
    if aenv:
        r[ATTACK_AMP], r[DECAY_AMP], r[SUSTAIN_AMP], r[RELEASE_AMP] = aenv
    for k, v in fields.items():
        r[globals()[k]] = v
    for i, c in enumerate(name.ljust(NAME_LEN)):
        r[NAME + i] = ord(c)
    assert all(0 <= x < 256 for x in r)
    return name, r


# the bank; each with its browser category letter (browse.c CAT_LETTER: B bass L lead P pad R brass / stab)
PROGRAMS = [
    # 1. the lofi house chord stab: Attack's Juno house-bass oscillator mix (saw + square on A, a square sub on B) under
    #    its polysynth-chord envelope (filter all the way, decay, no sustain); snappy, for Mall Grab-style punch
    ("R", program("RUMI HOUSE STAB", fenv=(0, 100, 0, 30), aenv=(0, 64, 32, 40),
                  SAW_A=1, PULSE_A=1, PW_A=64, LEVEL_A=127, PULSE_B=1, FREQ_B=semi(-12), PW_B=64, LEVEL_B=57,
                  CUTOFF=64, RESONANCE=24, KEY_FILTER=1, ENV_FILTER=110, VEL_FILTER=1, VINTAGE=40)),
    # 2. the same stab dark, after Attack's "Winona" analysis: the filter low, the envelope all the way, a longer decay
    ("R", program("RUMI WINONA STAB", fenv=(0, 90, 0, 40), aenv=(0, 80, 40, 50),
                  SAW_A=1, PULSE_A=1, PW_A=64, LEVEL_A=127, PULSE_B=1, FREQ_B=semi(-12), PW_B=64, LEVEL_B=57,
                  CUTOFF=28, RESONANCE=30, KEY_FILTER=1, ENV_FILTER=127, VEL_FILTER=1, VINTAGE=45)),
    # 3. the warm evolving pad (VISION 5): two saws a few cents apart with a slow PWM square, noise for texture, the
    #    filter envelope slow with a high sustain so a held chord keeps brightening; Attack's Overmono pad, Aulart's
    ("P", program("RUMI BLOOM PAD", fenv=(105, 127, 95, 110), aenv=(100, 127, 64, 110),
                  SAW_A=1, LEVEL_A=127, SAW_B=1, PULSE_B=1, FREQ_B=semi(0), FINE_B=10, PW_B=64, LEVEL_B=110, NOISE=10,
                  CUTOFF=45, RESONANCE=22, KEY_FILTER=1, ENV_FILTER=40, VINTAGE=40,
                  LFO_TRI=1, LFO_RATE=30, LFO_INITIAL=20, WHEEL_PW_B=1)),
    # 4. Roland's "Tech House" Juno-60 bass: one square, a square sub, the filter low with a short envelope; mono
    ("B", program("RUMI JUNO BASS", fenv=(0, 30, 0, 30), aenv=(0, 60, 127, 24),
                  SAW_A=0, PULSE_A=1, PW_A=64, LEVEL_A=127, PULSE_B=1, FREQ_B=semi(-12), PW_B=64, LEVEL_B=54,
                  CUTOFF=40, RESONANCE=10, KEY_FILTER=1, ENV_FILTER=45, VINTAGE=30,
                  UNISON=1, UNISON_COUNT=1, UNISON_DETUNE=0, RETRIGGER=2)),
    # 5. the chillhop pluck / bendy lead (VISION 3): Attack's Diva portamento lead, the poly-mod dropping the pitch into
    #    the note, glide, a 2-semitone bend, vibrato on the wheel and aftertouch; mono
    ("L", program("RUMI BEND LEAD", fenv=(0, 50, 18, 40), aenv=(5, 80, 76, 60),
                  SAW_A=1, LEVEL_A=127, TRI_B=1, FREQ_B=semi(0), LEVEL_B=60,
                  CUTOFF=64, RESONANCE=40, KEY_FILTER=1, ENV_FILTER=64, VINTAGE=30,
                  UNISON=1, UNISON_COUNT=1, UNISON_DETUNE=0, GLIDE=28, BEND=1, RETRIGGER=2,
                  POLY_FREQ=1, POLY_ENV=12, LFO_TRI=1, LFO_RATE=70, LFO_INITIAL=0, WHEEL_FREQ_A=1, WHEEL_FREQ_B=1,
                  PRESS_LFO=1)),
    # 6. Roland's Juno "Original Reese": one pulse under a slow LFO on its width, a saw a few cents off beside it; mono
    ("B", program("RUMI PWM REESE", fenv=(0, 60, 127, 40), aenv=(0, 60, 127, 40),
                  SAW_A=0, PULSE_A=1, PW_A=64, LEVEL_A=127, SAW_B=1, FREQ_B=semi(0), FINE_B=12, LEVEL_B=90,
                  CUTOFF=65, RESONANCE=18, KEY_FILTER=1, ENV_FILTER=0, VINTAGE=30,
                  LFO_TRI=1, LFO_RATE=25, LFO_INITIAL=70, WHEEL_PW_A=1,
                  UNISON=1, UNISON_COUNT=1, UNISON_DETUNE=0, RETRIGGER=2)),
]


def main(path):
    rows = lambda v: ",\n".join("     " + ", ".join(f"{x:3d}" for x in v[i:i + 19]) for i in range(0, len(v), 19))
    L = ["/* SPDX-License-Identifier: GPL-3.0-only",
         " * Copyright (C) 2026 Mike Maven and Rumiboard contributors */",
         "/* generated by tools/gen_prophet_rumi.py: the RUMI bank's Prophet programs, 133 raw bytes each, after Sequential's",
         " * 200 in the preset list (p5_factory); their browser categories (browse.c PRESET_CAT) */",
         "#pragma once",
         f"#define P5_RUMI_N {len(PROGRAMS)}u",
         f"#define P5_RUMI_CAT \"{''.join(c for c, _ in PROGRAMS)}\"",
         "#define P5_FACTORY_N (P5_SEQUENTIAL_N + P5_RUMI_N)   /* the Prophet's factory programs: Sequential's, then these */",
         "_Static_assert(P5_FACTORY_N <= 207u, \"the Prophet favorites bitmap (prophet_user.c) holds 208 programs\");",
         "#ifndef MELODEE_RES", "#define MELODEE_RES  /* melodee.c: the resource region (res.c) */", "#endif",
         f"static const uint8_t P5_RUMI[{len(PROGRAMS)}][133] MELODEE_RES = {{"]
    for _, (name, r) in PROGRAMS:
        L += [f"    {{/* {name} */", rows(r) + "},"]
    L += ["};", ""]
    open(path, "w").write("\n".join(L))
    print(f"prophet rumi: {len(PROGRAMS)} programs -> {path}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "firmware/src/prophet_rumi.h")
