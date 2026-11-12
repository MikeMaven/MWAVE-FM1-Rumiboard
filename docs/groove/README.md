# Groove measurements

Made with `tools/groove_measure.py` on full mixes (128 kbps sources), each over three 50 s windows; the figures
below are the windows' agreement (they differ by 1–3 ms). Offsets are against a straight grid anchored on the
hi-hats on the beat, negative = early, in milliseconds and MPC3000 pulses (96 a quarter note). The JSON files
are the 70–120 s windows. The bass is not measured (it shares the kick's band; it needs a stem).

| Track | BPM | pulse | kick on beat | kick off-8th | snare / clap on 2 & 4 | hats off-8th | hats swing |
|---|---|---|---|---|---|---|---|
| Slum Village, Get Dis Money (Dilla) | 96.5 | 6.5 ms | +13 ms (+2.0) | +16 ms (+2.5) | −9 ms (−1.5) | +14 ms (+2.2) | 52.2 % |
| Slum Village, Untitled / Fantastic (Dilla) | 91.1 | 6.9 ms | +13..+18 ms (+2..+2.7) | +17..+20 ms (+2.5..+3) | −3..+1 ms (≈ 0, two populations) | +29 ms (+4.2) | 54.4 % |

Reading, against Charnas's description: hats straight on the beat (the anchor), the off-beat hats late (Hein's
"actually late" on Get Dis Money: 2 pulses there, 4 on Untitled); the kick lags by 2–3 pulses in both; the clap
on Get Dis Money is early by 1.5 pulses (Hein: "a tiny bit early"), the snare on Untitled sits on the grid. The
65 ms early snare Charnas measured is on another record: these two are the subtle end of his range. More
staggered tracks to follow.

Caveats: a kick's slower rise reads 3–4 ms late in the self test, so the kick figures may be a shade high; the
snare band also catches the sampled chords' transients (the strongest 60 % of hits are used); the last 50 s of
Untitled / Fantastic (the outro, drums thinning) are not usable and are left out.
