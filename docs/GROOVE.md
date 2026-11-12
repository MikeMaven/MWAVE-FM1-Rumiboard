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

## The mode, as built

Melodee's sequencer has SWING (per track and global: the even steps longer, the odd ones shorter, 0..100 =
50..70 %) and fires every element of a step at the step boundary. The Dilla feel needs **shift**: an element
moved off the grid by a fixed amount, early or late, in pulses so it scales with the tempo. Rumiboard adds it
as **GROOVE**, the fourth knob of SEQ > TEMPO (`song.g[G_GROOVE]`, kept in the project; `seq.c GROOVES`):

| GROOVE | kick | snare, clap | hats on the beat | off-beat hats | bass sound | other tracks | from |
|---|---|---|---|---|---|---|---|
| OFF | 0 | 0 | 0 | 0 | 0 | 0 | |
| DILLA | +3 | −1 | 0 | +3 | +10 | 0 | the measurements: the kick 2–3 pulses behind, the clap a shade early, 52–54 % hats; the bass Hein's 32nd |
| DRAG | +6 | −1 | 0 | +3 | +12 | 0 | The Light's kick |
| RUSH | +3 | −10 | 0 | +3 | +10 | 0 | Charnas's rushed snare, 65 ms |

In MPC pulses, 1/96 of a beat (256 of the sequencer's units; 6.9 ms at 90 BPM). The drum lanes are KICK,
SNARE, CLAP, the two hats (on the beat or on the second 8th of it, by the step's place), and TOM / RIM / BELL
as percussion (0). A track whose sound is a BASS by its category (browse.c `groove_roles`, every UI frame) is
the bass; the other melodic tracks stay on the grid. SWING still applies on top.

How it plays (`seq.c groove_step`, `groove_peek`, `groove_tick`): at a step's boundary the late lanes are
queued, grouped by their shift, and played when their time comes (`gr_q`, six events a track); a melodic
track's late step is the whole step, queued and played by `seq_step` later, so its gate and slides are as
on the grid. An early lane is played ahead of its step from the next step's hits, when its time comes within
the block, and skipped when the step arrives (`gr_early`); not across a bank change or a chain's rest, where
the step waits. The transport's start and stop clear both. The clock, the click, MIDI clock out and recording
stay on the grid: a part played along to the heard drums records where it was played against the grid, as on
the MPC. `tests/groove_test.c` plays a bar and measures each element against its step.

A DRUM track's live take (hits recorded with QNT OFF keep their played timing and play from the recording,
their grid cells only an overview) takes the feel too: with a FEEL on, its lane hits play from the grid, on
their steps and shifted like entered hits, and not from the recording, which would play them unshifted a few
milliseconds apart (a flam); OFF plays the take as played again. A take's other GM drums (a crash, a low tom)
stay the recording's. `tests/groove_test.c` records a take and checks each hit plays once.

The firmware costs: 416 bytes of RAM (the queues), about 1 KB of flash.

## Ableton Live: the same feel on your own tracks

`tools/ableton_grooves.py` writes the shifts as Live groove files (`ableton/grooves/*.agr`, plain XML Live
reads): one groove per element, a bar of 16ths each moved by its pulses, Timing 100 %, no velocity or
random. Drop a groove on a clip (MIDI, or a warped audio clip: Live moves its transients) through the Groove
Pool; since a groove moves notes by their position and not by their pitch, give each drum its own clip or
track (a Drum Rack: Extract Chains) and its own groove. For an audio track that should simply sit behind,
Live's Track Delay in the mixer does the same in milliseconds: the table in `ableton/grooves/README.md`
gives the pulses in milliseconds at common tempos.
