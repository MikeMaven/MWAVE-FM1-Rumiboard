# The RUMI bank: Rumiboard's own sounds

The core palette of docs/VISION.md §2, built on the engines this firmware ships (the Prophet, FM6, SID; TRIO and
PHYS are retired in Melodee 1.6, `core.h MELODEE_LEGACY_EXTRAS 0`), from the recipes in
`reports/Lofi house and chillhop synth recipes.md`. They lead each engine's list in the browser (the PRESETS knob, LIST, the ENGINE knob's landing sound), then
the engine's INIT, then Melodee's factory sounds in their order; their stored numbers come after the factory
sounds, so saved projects and favorites keep meaning. They are named `RUMI ...`, so LIST finds them by category
and by name too. Each is dry: the shared rack
makes the lofi of it (the report's chain: slot 1 CHORUS MIX 50 DEPTH 60–70 at a slow RATE for the Juno feel,
slot 2 TAPE CHARACTER 32–40 MIX 60, slot 3 ROOM MIX 20–30 SIZE 50 DECAY 50 TONE −8; CRUSH BITS 12 when crunch
matters more than chorus).

| Sound | Engine, preset | VISION | What it is |
|---|---|---|---|
| RUMI HOUSE STAB | Prophet 201 | 6 | the lofi house chord stab: Attack Magazine's Juno house-bass oscillator mix (saw + square on A, a square sub on B) under its polysynth-chord envelope (the filter all the way, decay, no sustain); snappy |
| RUMI WINONA STAB | Prophet 202 | 6 | the same stab dark, after Attack's "Winona" analysis: the cutoff low, the envelope all the way, a longer decay |
| RUMI BLOOM PAD | Prophet 203 | 5 | the warm evolving pad: two saws a few cents apart with a slow PWM square, noise for texture; the filter envelope slow with a high sustain, so a held chord keeps brightening |
| RUMI JUNO BASS | Prophet 204 | — | Roland's "Tech House" Juno-60 bass: one square, a square sub, the filter low with a short envelope; mono |
| RUMI BEND LEAD | Prophet 205 | 3 | the chillhop pluck / bendy lead: Attack's Diva portamento lead; the poly-mod drops the pitch into the note, glide, a 2-semitone bend, vibrato on the wheel and aftertouch; mono |
| RUMI PWM REESE | Prophet 206 | — | Roland's Juno "Original Reese": a pulse under a slow LFO on its width, a saw a few cents off beside it; mono |
| RUMI TINES | FM6 26 | 2 | the tine electric piano dulled for lofi: the x14 tine lower and gone sooner, the carriers spread, a longer tail, little feedback |
| RUMI SINE | FM6 27 | 2 | the sine family: two sine carriers 6 cents apart, each under a low-index modulator, decaying like a key |
| RUMI BELL | FM6 28 | 2 | a soft bell: a 1 : 3.5 pair beside a 1 : 2 pair, long carriers, no feedback |
| RUMI HAZE | FM6 29 | 7 | the ambient sine texture: the pairs at −7 / +7 with a slow rise and a long tail, a half-speed carrier under them, a slow sine LFO on the pitch; for the long ROOM and a NOTE ECHO |
| RUMI SUB | FM6 30 | 1 | the sub: one sine with a soft attack (about 20 ms, no click), full sustain, a short release; a quiet octave above it; mono |
| RUMI FELT | FM6 31 | 4 | the felt piano by FM (the sampled one is the plan): a sine body whose modulator is heard only in the attack, an octave carrier, and a 30 ms hammer thud from a fixed 100 Hz operator |
| RUMI ACID | SID 33 | — | the "Winona" acid line: a saw, a short squelch of the 6581's filter, a little drive; mono |
| RUMI SUB BITE | SID 34 | 1 | the sub with bite: a triangle and its sub, a 25 ms attack, the 8580's drive; mono |

Preset numbers are the stored ones (the engine's own count). The Prophet's RUMI programs follow Sequential's 200
(`tools/gen_prophet_rumi.py` → `firmware/src/prophet_rumi.h`, raw 133-byte programs; `p5_factory` serves them,
`P5_FACTORY_N` counts both banks, at most 207 for the favorites bitmap); FM6's follow INIT VOICE
(`firmware/src/eng_fm6_rumi.h`, the ROM voice macros); SID's are rows of `eng_sid.c SID_PRESETS`. Their browser
categories: `browse.c PRESET_CAT` (the stab is BRASS, as Melodee files stabs: the bulldog); a bass category
puts the track behind the beat under the Dilla FEEL (docs/GROOVE.md).

## Hearing them on the host

`build/host/hostsim ENGINE PRESET MONO OUT.wav` with `PRESET=1` loads the whole preset (the Prophet's program,
FM6's patch); `CHORD=4 KEYS=5,8,12,15` holds a minor-7 voicing (key 7 = C4), `STEPS=36,0,0,36,...` plays a line.
The engines: Prophet 19, FM6 12, SID 3; the first renders are in the session's scratch folder, not the repo.

## Not here yet

The CZ M1-style stab (the CZ bank is Casio's 64, a native patch format of its own), the sampled felt piano (VISION 4:
ADPCM zones), the drums (the kits exist: VISION 8 is about tuning and samples), and the master-bus pump and
wow / flutter that the report lists as missing from the firmware.
