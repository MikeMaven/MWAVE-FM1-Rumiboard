# Rumiboard's Dilla grooves for Ableton Live

The same shifts the FM-1's GROOVE knob plays (docs/GROOVE.md, measured on Dilla's records in docs/groove/),
as Live groove files, one per element. Made by `tools/ableton_grooves.py`; Live 11 and 12 read them.

| File | moves every 16th by |
|---|---|
| Rumi Dilla Kick | +3 pulses (late) |
| Rumi Dilla Snare | −1 pulse (early); use it on claps too |
| Rumi Dilla Hats | 0 on the beat, +3 on the off-beat 8ths (a 52–54 % swing) |
| Rumi Dilla Bass | +10 pulses |
| Rumi Dilla Drag Kick / Snare / Hats / Bass | the kick +6 and the bass +12 (The Light) |
| Rumi Dilla Rush Kick / Snare / Hats / Bass | the snare −10 (Charnas's 65 ms rushed snare) |

A pulse is 1/96 of a beat (the MPC3000's), so the grooves follow the Set's tempo.

## Using them

1. Drag the `.agr` files into Live's browser (User Library > Grooves), or drop one straight onto a clip.
2. A groove moves every note of a clip by its place in the bar, not by its pitch, so each element needs its
   own clip: for a Drum Rack, right-click a pad > Extract Chains (one track per drum), or draw the kick, the
   snare and the hats as separate clips. Samples dropped on the timeline are already their own clips.
3. In the Groove Pool, keep Timing at 100 % (less is a lighter feel), Velocity and Random at 0. An audio clip
   must be warped for a groove to move its transients.
4. The Snare and Bass grooves move notes early or far: the first hit of a clip that starts exactly on the bar
   cannot play before the clip starts, so start such clips a bar early, or Commit the groove and nudge.

## Track Delay instead

For a whole audio track that should sit behind (a bass, a sampled loop), Live's Track Delay in the mixer
(Options > Track Delay, in ms) does the same without a groove, at one tempo. The shifts in milliseconds:

| pulses | 80 BPM | 85 | 90 | 95 | 100 | 120 |
|---|---|---|---|---|---|---|
| 1 | 7.8 | 7.4 | 6.9 | 6.6 | 6.3 | 5.2 |
| 3 (kick) | 23.4 | 22.1 | 20.8 | 19.7 | 18.8 | 15.6 |
| 6 (Drag kick) | 46.9 | 44.1 | 41.7 | 39.5 | 37.5 | 31.3 |
| 10 (bass, Rush snare early) | 78.1 | 73.5 | 69.4 | 65.8 | 62.5 | 52.1 |
| 12 (Drag bass) | 93.8 | 88.2 | 83.3 | 78.9 | 75.0 | 62.5 |

An early element is a negative Track Delay (the snare at −7 ms, or −69 ms for Rush); Live allows it.
