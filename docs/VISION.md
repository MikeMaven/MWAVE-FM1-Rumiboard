# Rumiboard — vision and sound design brief

Custom firmware for the M-VAVE FM-1: a standalone synth, sampler and groovebox for
chillhop, lofi house and lofi ambient. It has a hands-on, playful feel, but songs are
built from patterns with undo, not a tape recorder.

## 1. What it's for

- A complete instrument: synth voices, a drum rack and a sampler in one box.
- Songs made from patterns and an arrangement you can edit and undo, not a
  one-way tape.
- Sounds that are warm, soft and slightly worn by default. Little harsh high end.

## 2. Sound palette

Each entry is a target sound and the synthesis approach planned for it.

| # | Sound | Character | Approach |
|---|-------|-----------|----------|
| 1 | **Sub bass** | Clean sine sub; optional "bite" | Sine (+ optional octave-up triangle). Bite = soft saturation / gentle wavefold mixed in, plus a short filtered-pulse transient. Mono, legato glide. |
| 2 | **Sine family** (harmony/melody) | Soft, round, chill | 2-operator FM with a low modulation index (sine to soft bell or electric-piano tones), lightly detuned sine pairs, slow chorus. |
| 3 | **Chillhop pluck / bendy lead** | Plucky, rubbery, expressive | Sine or triangle with a fast pitch envelope (drops into the note), portamento, wide bend range, delayed vibrato, and a short lowpass envelope for the pluck. Optional Karplus-Strong string pluck. |
| 4 | **Felted upright piano** | Muffled, intimate, hammer-soft | Short multi-zone samples (ADPCM) with a strong lowpass and soft attack. The "felt" comes from filtering, a quieter hammer transient and slight detune. Fallback: a modal/physical piano model. |
| 5 | **Warm evolving pad** | Gritty but dark; slow attack; **gets brighter the longer a chord is held** | 2–3 detuned saw/pulse oscillators per voice, then a resonant lowpass, then gentle drive. A slow "bloom" envelope keeps opening the filter during sustain, so a held chord slowly brightens. Long attack; voices glide between chord changes. |
| 6 | **Lofi house stabs/leads** | Classic analog, chill but punchy, over soft pads | A classic polysynth voice (saw + sub-square, chorus) for chord stabs. Snappy filter envelope, short decay. |
| 7 | **Ambient (sine-based)** | Same sine family, more space | Patches built on #2 and #3, with long delay and reverb sends. |
| 8 | **Drums** | Punchy but soft | Synthesized kick, snare, clap, hats and percussion, plus user drum samples in a drum rack. |

**Global "lofi" master bus** (cheap DSP that fits every style): tape-style saturation,
wow/flutter, lowpass "warmth" tilt, optional sample-rate/bit reduction, vinyl noise, and a
sidechain-style pump for house.

## 3. Sequencing (instead of tape)

- Per-track step sequencer with live recording; patterns per track.
- **Song mode**: arrange scenes/patterns in a chain that can be edited at any time.
- **Undo/redo** for edits (history depth limited by RAM; see §4).
- Per-step parameter locks, probability, swing (essential for chillhop), and chord entry.

## 4. Hardware reality (from studying Felucca and Melodee sources)

These numbers come from the open firmware projects' code and engineering notes. Confirm
them on our own unit before relying on them.

- **CPU**: JieLi AC79 (WL82) family, two cores running at 360 MHz. Using the second core
  roughly doubles the voices available. Melodee caps its analog-style poly engine at
  8 voices with both cores online.
- **RAM is the main constraint**: about 60 KB of guaranteed audio working memory,
  plus about 28 KB of optional cache. Every delay line, reverb tank and undo history has to fit here.
  For comparison, Melodee's reverb uses 4–32 KB depending on quality.
- **Flash**: about 1 MB in total, shared between the firmware (about 540 KB) and user data.
  Felucca stores user samples as 3 slots of 80 KB, IMA ADPCM. That's a few seconds per slot.
- **Sampling**: neither project records from a live audio input. Samples are loaded
  over USB from a web editor. Our "sampler" will be a short-sample instrument (drum hits,
  one-shots, a few piano zones), not an OP-1-style long-sample recorder.
- **Audio**: 44.1 kHz output; USB audio and USB/TRS MIDI are available.

What this means for the design:
- Lofi suits the hardware. Lower-rate reverb and delay, ADPCM samples and darker filtering
  save CPU and RAM *and* fit the aesthetic.
- Pads (#5) are the most expensive sound and set the polyphony budget.
- The felted piano (#4) has to work with small samples, so most of the felt character
  comes from DSP.

## 5. Development approach

The hardest and riskiest part is the hardware layer: an under-documented chip, the boot
and update loader, flash layout, and recovery from a bricked unit. Felucca and Melodee
have already solved it. Plan:

1. Start from an existing open firmware for hardware support, the installer and recovery
   (license obligations below).
2. Build and flash an unmodified copy first to prove the toolchain and install/recovery work.
3. Add our own engines one at a time, starting with **sub bass** and the **sine family**
   (cheap, high value), then pluck/lead, pad, piano, then the lofi master bus.
4. Reshape the sequencer toward song mode and undo.

Licensing: Felucca's application is GPL-3.0-only, its hardware layer (felucca-bsp) is
MPL-2.0 and its engines are MIT. Melodee is a GPL-3.0 derivative of Felucca. A fork of
either must remain GPL-3.0 and keep the original copyright notices.

## 6. Decisions

- Base project: **Melodee 1.6.1** (forked with its full history; GPL-3.0-only).
- UI direction: see [UI_DIRECTION.md](UI_DIRECTION.md).
- **Keep Melodee's engines and factory sounds** (the Prophet-5 model and its 200 programs, such as
  "It's a Prophet", the CZ, FM6, SID, PHYS and drum kits). The RUMI and 1984 families are added beside
  them, not in their place; a sound found by tweaking a factory program is still a sound of ours.
- Drum kits: see [SAMPLES.md](SAMPLES.md). USB stems and Ableton: see [USB_DAW.md](USB_DAW.md).
- Still open: test that this unit can go back to the stock firmware and recover before the
  first custom flash.
