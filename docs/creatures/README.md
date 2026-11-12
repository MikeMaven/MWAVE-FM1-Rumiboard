# The creatures

Every sound family has a creature that lives on the play page (`firmware/src/ui_play.c`) and, small, in
Stage's band. The designs are in `refs/` (concept sheets made with Gemini from our brief; each shows
four key poses in two colour pairs). This file maps each creature to its sound, its motion and the way
it is drawn on the device.

## How a sheet becomes a creature on the device

- **Size:** 64 px tall at rest inside a 120 × 88 px canvas (the play page); 22 px tall in the band.
  Features at least 3 px at 64 px: eyes are 9 px dots, teeth 6 px.
- **The art is the sheet's.** `tools/gen_creatures.py` cuts the four poses out of each sheet, scales them
  to the canvas and stores them as run-length pixel art in colour roles: body, accent, a second accent
  (the sheet's three most frequent colours), dark, and light / cut-outs (the background). Each creature
  keeps **its own colours** from the sheet (`CR_SPR_COLS`), as designed; the track's colour stays on the
  chrome around it. In the tests' greyscale palette the colours are drawn as greys. All 32 poses: ~25 KB,
  in the flash resource region (read in place, no RAM).
- **Posed by code** (`ui_creature.c`): a note shows the attack pose, the sustain pose while the sound
  holds, the release pose, then rest; the cloud follows its envelope (against its recent peak), the
  octopus the drum lanes, the snail the master bus; a bob, lunge or jump is added in code. A creature's
  state is ~20 bytes (RAM has ~1 KB to spare), stepped on the clock every 30 ms.
- **What drives it:** the track's latest note (pitch, velocity), its amp envelope (`m_env`), whether
  notes are held (`live_held`), the drum lanes' hits, the clock for idle sway.
- Better sheets make better creatures: flat colours, strong silhouettes, poses that read at 64 px; a
  pose that faces the other way (the chameleon's fourth) is not used.

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
| **Lofi master bus** | Tape-spool snail (`refs/master_snail.jpg`) | idle / drive → warm up (saturation) → age / drop out (crinkle) → tape tear | the reel on its back warms to a gold glow while the mix is loud (pushing the bus); its oxide-tape trail crinkles, in and out, while a TAPE or SPIT insert on the master ages the mix; the tape tears (with a recoil) when the limiter catches a peak |
| *(any sound without its own)* | The blob (`ui_creature.c` CR_BLOB) | squash on a note, spring back, breathe | the first creature, now the fallback |

## Status

All nine are drawn and moving (`firmware/src/ui_creature.c`), a first pass for review. Wired by Melodee's
sound categories: BASS → anglerfish, BRASS → bulldog (stabs, until our presets name their creature),
PLUCK / LEAD → chameleon, PAD / STRING → cloud, DRUM → octopus, KEYS / ORGAN / BELL → axolotl, the
rest → blob. The hedgehog waits for the felted-piano preset and the snail for the lofi bus (both show in
the review renders through the test-only `cr_force`). Review renders: `creature_*` scenes in
`tests/ui_render.c`; a filmstrip of each creature's action is made by the film tool in the scratchpad
(to be added to tools/).

**A sound names its creature** (`ui_creature.c creature_of_entry`): a word of its name decides first (PIANO,
FELT → hedgehog; EP, TINE, RHODES, SINE, BELL, KEYS → axolotl; STAB, BRASS, CHORD, HOUSE → bulldog; SUB,
BASS, REESE → fish; PLUCK, LEAD, BEND, GLIDE, ARP → chameleon; PAD, CLOUD, SWELL, STRING, CHOIR → cloud;
TAPE, REEL, LOFI, VINYL, CASSETTE → snail; KIT, DRUM, PERC → octopus), then a per-factory-preset letter table
(`PRESET_CREATURE`, empty so far), then the category. Our own banks will carry an id.

**Sheet corrections** live in the tool (`PATCHES`): boxes on the role map where pixels change role, applied
to every pose (the chameleon's eyes in its in-between frames). A sheet whose poses touch (the snail's tape trail
runs on under the next pose) is cut at fixed panels (`PANELS`), its four poses at one scale and anchored at the left
(`ANCHOR_LEFT`), so the creature keeps its size and place while only its extras (the trail, the crinkle, the torn
end) change.

Next: in-between frames from the sheets' first and last poses (the tool takes any four frames in order;
the firmware will then play them as a sequence and back), the hop transition when ALGORITHM changes track.
