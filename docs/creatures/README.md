# The creatures

Every sound family has a creature that lives on the play page (`firmware/src/ui_play.c`) and, small, in
Stage's band. The designs are in `refs/` (concept sheets made with Gemini from our brief; each shows
four key poses in two colour pairs). This file maps each creature to its sound, its motion and the way
it is drawn on the device.

## How a sheet becomes a creature on the device

- **Size:** 64 px tall at rest inside a 120 × 88 px canvas (the play page); 22 px tall in the band.
  Features at least 3 px at 64 px: eyes are 9 px dots, teeth 6 px.
- **Colours are roles**, so every creature works in all ten palettes and the greyscale test:
  body = the track's colour (`T_THEME`); accent (gloves, fins, record, hammer) = the track's first
  companion shade (`T_KNOB(1)`, the colour turned 32° round the hue circle); cut-outs (eyes, mouths,
  seams) = the background (`T_BG`); a dark detail (pupils, a mouth's inside) = the text colour. No
  gradients, outlines or shading.
- **Built from parts, moved by code** (`ui_creature.c`): discs, capsules, rounded rectangles and
  column-by-column spans for ellipses, fins and arcs. A creature's state is a few bytes (RAM has ~1 KB
  to spare); its motion is a small spring or a short timed action, stepped on the clock every 30 ms.
- **What drives it:** the track's latest note (pitch, velocity), its amp envelope (`m_env`), whether
  notes are held (`live_held`), the knobs' values where the sheet says so (filter, decay), and the
  tempo for idle sway.
- Drawn frames (sprites) are a last resort: flash has ~20 KB spare.

## The roster

| Sound family | Creature | Signature action (the sheet's poses) | Sound → motion |
|---|---|---|---|
| **Sub bass** | Anglerfish (`refs/bass_anglerfish.jpg`) | ready → inhaling → sound blast → return | note-on opens the mouth wide (velocity = how wide) and the lure flares with sound rings; held = mouth stays open; release closes it; low notes sit the fish lower; BITE bares the teeth |
| **Sine keys** | Axolotl in a bubble (`refs/keys_axolotl.jpg`) | float → initiate flip → flip & trail → settle | a note starts a slow flip, the tail trails; chords = more bubbles; vibrato/FM depth = gill wiggle; idle floats |
| **Pluck / bendy lead** | Chameleon (`refs/pluck_chameleon.jpg`) | perch → snap! → bend (wave) → retract | note-on snaps the tongue out (velocity = reach), pitch bend / glide bends the tongue in a wave, release retracts; the eye follows the last note's pitch |
| **Felted piano** | Hedgehog pincushion (`refs/piano_hedgehog.jpg`) | at guard → the strike → the vibration → damp / release | note-on swings the mallet and strikes the string line, the string vibrates while the note rings (envelope), release damps it with a "pff"; velocity = swing size |
| **Warm pad** | Cloud (`refs/pad_cloud.jpg`) | resting → churning / expanding → full bloom → dissipating | the cloud grows and brightens with the filter "bloom" while a chord holds (eyes open at full bloom), dissipates on release; the LFO churns it |
| **Lofi house stab** | Bulldog (`refs/stab_bulldog.jpg`) | idle groove → the squash → chord stab! (bark) → tail-wag delay | each stab: squash then bark with a jump (velocity = jump); the tail wags with the echo's repeats; idle nods on the beat |
| **Drums** | Octopus at the pads (`refs/drums_octopus.jpg`) | swing left → pad slam → knob twiddle → swing right | kick and snare = a tentacle slams a pad (which lights); hats = a tentacle twiddles a knob; it sways left-right with the bar; the pads light per lane |
| **Lofi master bus** | Sloth with a record (`refs/master_sloth.jpg`) | lazy wobble → adjust weight → needle skip! → return to groove | the record spins at the tempo, wobbles with wow/flutter, the needle skips on a peak (limiter) or the bit-crusher; the sloth blinks slowly |
| *(any sound without its own)* | The blob (`ui_creature.c` CR_BLOB) | squash on a note, spring back, breathe | the first creature, now the fallback |

## Order of building

1. Anglerfish (sub bass): the first sheet-based creature, replacing the blob for BASS sounds; the blob
   becomes the fallback for every other sound.
2. Bulldog (stabs) and chameleon (plucks): action-on-note creatures, the same pattern.
3. Cloud (pads): envelope- and knob-driven growth.
4. Octopus (drums): per-lane triggers.
5. Hedgehog (piano) and axolotl (keys).
6. Sloth: the master bus, once the lofi bus exists.
