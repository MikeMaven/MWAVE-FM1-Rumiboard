# Groove: swing, shift and the Dilla mode

Rumiboard's one groove to imitate is J Dilla's, as Dan Charnas describes it in *Dilla Time* (the authority for
this feature; tutorial sites that reverse his findings are ignored). It is not a swing amount. It is several
feels at once: the hi-hats straight and on the grid, the snare or clap early ("rushed"), the kick lagging, the
bass far behind, each element on its own clock as the MPC3000 allowed (96 pulses a quarter note, about 7 ms
each at his tempos; its SHIFT TIMING moved a whole track by pulses). The published numbers:

| Element | Figure | Source |
|---|---|---|
| snare / clap | early by about 65 ms, "5/192 of a bar" (69 ms at 90 BPM) | Charnas, quoted in the LRB review |
| bass | late, "an entire 32nd note" on some notes (83 ms at 90 BPM) | Ethan Hein, "Get Dis Money" |
| off-beat hats | late, no amount | Hein |
| kick | lags, no amount | Charnas |

Those are two measured points. `tools/groove_measure.py` measures the rest from his records: it splits a
track's drums by band, finds the tempo, anchors a straight grid on the hi-hats and reports every element's
offset in milliseconds and MPC pulses, with the hats' swing (`--selftest` proves it on a synthetic beat to
within about 4 ms). The bass needs its own stem. Measurements go in `docs/groove/` as they are made, and the
DILLA preset below takes its numbers from them.

## What the sequencer has, and what the mode needs

Melodee's sequencer has SWING (per track and global: the even steps longer, the odd ones shorter, 0..100 =
50..70 %) and fires every element of a step at the step boundary. The Dilla feel needs **shift**: a whole
element moved off the grid by a fixed amount, early or late, in pulses so it scales with the tempo.

Design (to build once the measurements are in):

- **SHIFT per track** (−24..+24 pulses; a DRUM track: one per lane), stored as common parameters just before
  P_E0 so projects and presets of older layouts load by count. A late shift is a delayed trigger: the step
  fires into a small per-track queue (lane or notes, due time) that the block tick drains. An early shift
  is the same queue with every track's base delay raised to the largest early shift in use, so the earliest
  element fires on the grid and the rest after it; with no early shift the base is 0 and nothing is delayed.
  The click and the MIDI clock take the base delay too, so what the player hears stays with the grid.
- **Recording** subtracts a track's effective delay from a recorded onset, so a part played along to the
  heard beat lands on the step it was meant for.
- **GROOVE presets** on the song page: STRAIGHT (0), MPC 54 / 58 / 62 (swing only), DILLA: hats 0, snare
  −9 pulses, kick and bass late by the measured figures (until measured: kick +3, bass +12).
