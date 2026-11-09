# Rumiboard as a DAW-in-a-box: USB audio stems, MIDI and Ableton

Goal: play and arrange on the FM-1, then pull the song into Ableton Live for finishing as
separate stems, including the drums split by sound, over one USB cable.

## 1. What the Melodee base already does (inherited, untested on our unit)

From Melodee's README and `firmware/src/usb_audio*.c`:

- A class-compliant USB audio device (UAC1): the computer plays through the FM-1, and the FM-1
  records into the computer. 44.1 kHz, 16 or 24 bit.
- **USB INPUT = STEREO** (the master) or **4 TRACKS**: four isolated mono stems, taken after each
  track's effects and level, before the shared effects (echo / reverb) and the master.
- USB MIDI in and out: notes go out over USB; MIDI clock master (**INT**) or follower, with Song
  Position and Continue; the sequencer follows a DAW's play button.
- Either direction can be switched off (**USB AUDIO**); stereo capture halves the packing work.

So "record four stems and MIDI into Ableton, locked to its clock" should already work on the base
firmware. First job when the unit arrives: try it.

## 2. What KARMA does, and what we would borrow

[KARMA](https://github.com/eatyourgreensstudio-dotcom/karma-fm1) (Ariel Muchnik, GPL-3.0-only) is a
Felucca-family firmware (via SLOOP). Its **USB AUDIO = STEMS** mode gives **six channels**: the four
tracks plus the shared effects as a stereo pair, at 44.1 or 48 kHz, with MIDI in and out and DAW
clock following. Its stems capture is credited to JIANT FM (Juan Jiant) and lives in its
`fx.c` (`stem_buf`) and `audio.c` (`uac_tap6`). KARMA's own notes say a very heavy STEMS load can
still overload the CPU.

Worth borrowing (same licence family, with credit):

1. **The effects return as channels 5–6**, so the reverb and echo tails land on their own stereo
   track in Live instead of being lost (today the 4 TRACKS mode drops them).
2. **48 kHz**, so a 48 kHz Live session needs no resampling.

Not needed from KARMA: its USB transport (ours is Melodee's, which KARMA does not use) and its
sound engines.

## 3. Drums split by sound (new work; nobody does this yet)

The drum engine renders its eight lanes (BD, SD, CP, CH, OH, TM, RS, CB) into one track. Splitting
them means rendering lanes into separate buffers and sending more USB channels.

**The ceiling is USB bandwidth.** The FM-1's USB is full speed: one isochronous packet per
millisecond of at most 1023 bytes.

| Format | Bytes per channel per ms | Max channels |
|---|---:|---:|
| 44.1 kHz, 16 bit | ~89 | 11 |
| 48 kHz, 16 bit | 96 | 10 |
| 44.1 kHz, 24 bit | ~133 | 7 |

So the full set (3 synth tracks + 8 drum lanes = 11 mono) only just fits, at 16 bit and 44.1 kHz,
with nothing left for the effects pair. The practical design:

- **DRUM STEMS mode**: the drum track as **four group stems** (kick / snare + clap / hats / perc)
  plus the three synth tracks and the effects pair: 9 channels, ~800 B/ms at 16 bit. Fits with
  headroom.
- An **8-lane** variant at 16 bit without the effects pair as an option, if the CPU allows.
- 24 bit stays for the stereo and 4-track modes.

CPU is the other limit: each extra stem is another buffer to collect and pack every block. KARMA's
overload warning applies to us too. Measure on hardware before promising a channel count; the
mixer's existing stem path (`ua_tracks`) is where the work goes.

## 4. Ableton side

- A Live template set: audio tracks pre-armed on the FM-1's input channels (named per stem), a
  MIDI track for the FM-1's notes, and MIDI clock sent to the FM-1 so pressing play in Live starts
  the groovebox (or the FM-1 leads, with Live following).
- Document the round trip: record stems, drop the FM-1's pattern/song MIDI in for editing.

## 5. Caveats

- All of §1 is inherited and unverified on our unit; test before building on it.
- Stock firmware is now V16; the community's return-to-stock tools were built around V15. Check
  the recovery path before the first custom flash.
- Full-speed USB also carries the MIDI and (if on) the computer's playback; heavy stem modes may
  need playback switched off.

## 6. Order

1. Verify 4 TRACKS + MIDI + clock in Live on the real unit.
2. Effects pair (channels 5–6) and 48 kHz, after KARMA / JIANT FM.
3. DRUM STEMS (four groups), then measure; 8 lanes if it holds.
4. Live template and a short guide.
