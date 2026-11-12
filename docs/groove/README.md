# Groove measurements

Made with `tools/groove_measure.py` on full mixes (128 kbps sources), each over three 50 s windows; the figures
below are the windows' agreement (they differ by 1–3 ms). Offsets are against a straight grid anchored on the
hi-hats on the beat (or, where the hats cannot be found cleanly under the sample, on the snare: then the hats
are read against the snare), negative = early, in milliseconds and MPC3000 pulses (96 a quarter note). The
anchor-free figure, the kick after the snare, does not depend on that choice. The JSON files are the 70–120 s
windows. The bass is not measured (it shares the kick's band; it needs a stem).

| Track | BPM | pulse | anchor | kick on beat | kick off-8th | snare / clap on 2 & 4 | hats off-8th | hats swing | kick after snare |
|---|---|---|---|---|---|---|---|---|---|
| Slum Village, Get Dis Money (Dilla) | 96.5 | 6.5 ms | hats | +13 ms (+2.0) | +16 ms (+2.5) | −9 ms (−1.5) | +14 ms (+2.2) | 52.2 % | +23 ms (+3.6) |
| Slum Village, Untitled / Fantastic (Dilla) | 91.1 | 6.9 ms | hats | +13..+18 ms (+2..+2.7) | +17..+20 ms (+2.5..+3) | −3..+1 ms (≈ 0, two populations) | +29 ms (+4.2) | 54.4 % | +18 ms (+2.6) |
| Common, The Light (Dilla) | 96.9 | 6.5 ms | snare | +40 ms (+6.2) | +29 ms (+4.4) | 0 (the anchor) | hats not cleanly found (the sample's guitar and voice fill the top band); on-beat hats sit with the snare | – | +38 ms (+5.9) |
| The Pharcyde, Runnin (Dilla) | 94.1 | 6.6 ms | hats | +4..+12 ms (+0.5..+1.8, wide: the bass guitar shares the band) | +9..+13 ms (+1.4..+1.9) | −4..−8 ms (−0.6..−1.2) | 0 (straight) | 50 % | +10..+20 ms (+1.4..+3.1) |

Reading, against Charnas's description: hats straight on the beat (the anchor), the off-beat hats late (Hein's
"actually late" on Get Dis Money: 2 pulses there, 4 on Untitled); the kick lags in every track, by 2–3 pulses on
the Slum Village records and 6 pulses (38 ms) on The Light, where the kick is the dragged element; the clap on
Get Dis Money is early by 1.5 pulses (Hein: "a tiny bit early"), the snares on Untitled and The Light sit with
the hats. The 65 ms early snare Charnas measured is on another record: these are the kick-lag end of his range.
Runnin is nearly straight in its drums (hats straight, the snare a pulse early, the kick one or two late): its
stagger is the sample's playing (the guitar and flute of "Saudade Vem Correndo"), not drum offsets.

## The DILLA preset from these four

In MPC pulses (1/96 of a beat): **hats 0**; **snare / clap −1** (0 to −1.5 measured; Charnas's 65 ms snare,
about −10, is not in these four and would be a second, "rushed snare" preset); **kick +3** (the four tracks:
+1.5, +2.5, +3.5, +6; The Light's +6 as the "dragged kick" variant); **off-beat hats +3 when swung** (52–54 %;
Runnin and The Light straight); **bass +8..+12** (Hein's 32nd-late notes on Get Dis Money, unmeasured here: a
stem or an instrumental would settle it).

Caveats: a kick's slower rise reads 3–4 ms late in the self test, so the kick figures may be a shade high; the
snare band also catches the sampled chords' transients (the strongest 60 % of hits are used); the last 50 s of
Untitled / Fantastic (the outro, drums thinning) are not usable and are left out.
