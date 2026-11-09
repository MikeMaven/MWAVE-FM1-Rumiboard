# Presets and sounds from other firmwares

What other FM-1 firmwares offer that we can use, what it would take, and what is really there. All of them are
GPL-3.0 like us, so code and presets can be brought over with their notices kept (LICENSING.md lists every
origin).

## SPAMPY (Spampy-Droid/SPAMPY-0.0-r949)

A Felucca/Sloop-based firmware with a synthwave slant (and a Melodee port of some parts: its Prophet engine is
ours). Checked against its r949 source.

| What | Where in SPAMPY | For us |
|---|---|---|
| **Prophet bank** | `p5_selected.h`, 44 slots, names in `eng_prophet.c` | A curated pick of Sequential's factory programs ("IT'S A PROPHET 5", "FANTASTIC FIVE", "STABBY BRASS", "MIDNIGHT PWM", "80S HORROR STRINGS" ...). We already carry all 200; the pick is worth copying as a **shortlist** for auditioning, not as new sounds. Same `p5_patch_t`, so the bytes load as they are. |
| **CZ tones** | `cz_selected.h`, 4 tones (144 B) | Importable through our CZ SysEx path if any differ from our 64 factory tones. |
| **ANALOG engine presets** | `eng_analog.c`, 57 presets: ACID 303, REESE, TRAP PLUCK, SUPERSAW, JUNO LEAD, JUNO STRINGS, SUB BASS, WARM PAD, VHS PAD, MIAMI BASS, NIGHT DRIVE, VICE SUB, NEON PULSE, CASSETTE KEYS, POLY SUNSET, NOCTURNE PAD, RAINY CITY ... | **The real prize for lofi house**, but they are presets *of an engine we do not have*: a two-oscillator analog synth with a state-variable filter (Felucca's, GPL-3.0, ~12 KB of source). Using them means porting the engine. |
| **OBERVAVE engine presets** | `eng_obervave.c`, 15: NEON PAD, DREAM ARP, BIG BRASS, WARM PAD, DARK PAD, AIR CHOIR, GHOST PAD, TAPE KEYS, PWM LEAD ... | Same: an Oberheim-style engine to port (~6.5 KB source). |
| **WAVETABLE presets** | `eng_wavetable.c`, 14: NEON SWEEP, WAVE RUNNER, CYBER ... | Same (~4 KB source). |
| **LOFI chip voice** | `eng_lofi.c`, 13 presets | A chip voice (pulse / tri / saw / wave RAM, quantised); fits our lofi aims; an engine to port (~12 KB). |
| **Synth drum kits** | `drum_synth.c` + Felucca's `gen_drumkits.py` kits | Felucca's modelled drum engine (layers: tone, click, noise, filter, drive, crush). Different from our 808 / 909 / CR-78 models; its kits only play on it. Porting the engine (~20 KB source) would add a flexible designer's kit beside the fixed classics. |
| Startup art | `synthwave_miami.h` | Theirs; not for us. |

### The catch: flash

Our firmware sits at ~563 KB of a 581.5 KB code area, so ~19 KB is free. Each engine above compiles to tens
of KB. Bringing SPAMPY's engines in means **giving something up or re-planning the flash map** (STORAGE_PLAN.md
has the candidates: the user-data area is 400 KB; the Rubik font and some engines we may not want are the
usual trades). Decide what we want to lose before porting anything.

### Order

1. Copy the Prophet shortlist as a browser list ("SPAMPY picks") for auditioning: a few hundred bytes.
2. Decide the flash trade (which Melodee engines or data to drop).
3. Port ANALOG first (its 57 presets are the lofi-house material), then the chip voice; OBERVAVE and
   WAVETABLE after, if room remains.
4. The drum designer engine last, weighed against the sampled kits (SAMPLES.md).
