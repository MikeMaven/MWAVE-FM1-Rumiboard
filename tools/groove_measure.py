#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Mike Maven and Rumiboard contributors
"""Measure a beat's microtiming: how early or late the kick, snare and hats sit against the hi-hat grid.

  groove_measure.py TRACK.wav [--bpm 90] [--start 30 --end 60] [--json out.json]
  groove_measure.py --selftest

The drums are split by band (kick: 40-120 Hz, snare / clap: 150-600 Hz with 1.5-5 kHz, hats: above 7 kHz), each
band's onsets are detected, the tempo is found (or given) and the grid is anchored on the hi-hats, which Charnas (Dilla
Time) describes as Dilla's straight element. Each hit's offset to its nearest 16th-note grid line is reported in
milliseconds and in MPC3000 pulses (96 a quarter note), as medians with their spread, and the hats' odd / even
16th offsets give a swing figure. A whole mix works for the drums; the bass needs its own stem (it shares the
kick's band). Any format ffmpeg reads. Needs numpy and scipy (the repo's .venv has them)."""
import argparse
import json
import subprocess
import sys

import numpy as np
from scipy import signal

SR = 48000                                            # (48 samples a millisecond: the onset functions run at 1 ms steps)


def load(path, start, end):
    cmd = ["ffmpeg", "-v", "error", "-i", path]
    if start: cmd += ["-ss", str(start)]
    if end: cmd += ["-to", str(end)]
    cmd += ["-ac", "1", "-ar", str(SR), "-f", "f32le", "-"]
    raw = subprocess.run(cmd, check=True, capture_output=True).stdout
    return np.frombuffer(raw, dtype=np.float32).astype(np.float64)


def band(x, lo, hi):
    sos = signal.butter(4, [lo, hi] if lo else hi, btype="band" if lo else "low", fs=SR, output="sos") if hi else \
          signal.butter(4, lo, btype="high", fs=SR, output="sos")
    return signal.sosfiltfilt(sos, x)


def onset_env(x, smooth_ms):
    """the band's onset function: the rise of its energy envelope, half-wave rectified, at 1 ms steps"""
    env = np.abs(x)
    k = max(1, int(SR * smooth_ms / 1000))
    env = np.convolve(env, np.ones(k) / k, mode="same")
    hop = SR // 1000
    env = env[: len(env) // hop * hop].reshape(-1, hop).max(axis=1)
    d = np.diff(env, prepend=env[0])
    return np.maximum(d, 0), env


def pick(onset, env, min_gap_ms, thresh_rel):
    """peaks of the onset function at least min_gap apart and above a share of its loudest peaks: times in ms"""
    if not onset.any(): return np.array([])
    top = np.sort(onset)[-max(4, len(onset) // 200):].mean()
    idx, _ = signal.find_peaks(onset, height=top * thresh_rel, distance=min_gap_ms)
    # the hit's time: where its steep rise starts (back from the steepest ms while the rise is still a tenth of it)
    out = []
    for i in idx:
        j = i
        while j > 0 and onset[j - 1] >= onset[i] * 0.1 and i - j < 20: j -= 1
        out.append(j + 0.5)
    return np.array(out, dtype=float)


def estimate_bpm(onset_all, lo=60, hi=140):
    """tempo from the autocorrelation of the summed onset function (1 ms steps)"""
    x = onset_all - onset_all.mean()
    ac = signal.correlate(x, x, mode="full")[len(x) - 1:]
    ac /= ac[0] if ac[0] else 1
    best, bpm = -1, None
    for lag in range(int(60000 / hi), int(60000 / lo) + 1):
        s = ac[lag] + 0.5 * ac[2 * lag] + 0.25 * ac[lag // 2] if 2 * lag < len(ac) else ac[lag]
        if s > best: best, bpm = s, 60000 / lag
    return bpm


def grid_phase(times, period):
    """the phase (ms) of a grid of the given period that most of the times sit on: the phase whose capped distance
    to the hits is least (a swung off-beat hat is far from every beat line whatever the phase, so it does not pull)"""
    best, bp = None, 0
    for ph in np.arange(0, period, 1.0):
        d = (times - ph) / period
        d = np.abs(d - np.round(d)) * period
        loss = np.minimum(d, 30).sum()
        if best is None or loss < best: best, bp = loss, ph
    return bp


def offsets(times, period, phase):
    """each time's offset (ms) to its nearest grid line (negative = early)"""
    rel = (times - phase) / period
    return (rel - np.round(rel)) * period, np.round(rel).astype(int)


def stats(v):
    if len(v) == 0: return None
    q1, q3 = np.percentile(v, [25, 75])
    return {"n": int(len(v)), "median_ms": float(np.median(v)), "iqr_ms": float(q3 - q1), "mean_ms": float(np.mean(v))}


def measure(x, bpm=None):
    bands = {"kick": (band(x, 40, 120), 8, 150, 0.25), "snare": (band(x, 150, 600) + 0.5 * band(x, 1500, 5000), 4, 150, 0.3),
             "hats": (band(x, 7000, None), 2, 60, 0.2)}
    hits, onsets, strength = {}, [], {}
    for name, (b, smooth, gap, th) in bands.items():
        o, e = onset_env(b, smooth)
        hits[name] = pick(o, e, gap, th)
        strength[name] = o
        onsets.append(o / (o.max() or 1))
    # a kick's click reaches the snare band: a weak snare-band hit within 12 ms of a kick is the kick
    if len(hits["snare"]) and len(hits["kick"]):
        o = strength["snare"]
        st = np.array([o[int(t):int(t) + 20].max() for t in hits["snare"]])
        typical = np.percentile(st, 75)
        near = np.array([np.abs(hits["kick"] - t).min() < 12 for t in hits["snare"]])
        hits["snare"] = hits["snare"][~(near & (st < 0.6 * typical))]
    if bpm is None:
        bpm = estimate_bpm(sum(onsets))
    beat = 60000 / bpm
    # the grid: anchored on the hats (8ths: the straight element); hats absent, on the kick's downbeats
    anchor = hits["hats"] if len(hits["hats"]) >= 8 else hits["kick"]
    phase = grid_phase(anchor, beat)
    # the tempo refined: a line through the anchor hits on the beat (their index against their time)
    for _ in range(2):
        rel = (anchor - phase) / beat
        on = np.abs(rel - np.round(rel)) * beat < 30
        if on.sum() >= 4:
            k, tt = np.round(rel[on]), anchor[on]
            slope, icpt = np.polyfit(k, tt, 1)
            beat, phase = float(slope), float(icpt % slope)
            bpm = 60000 / beat
    sixteenth = beat / 4
    # hats play two interleaved beat-grids (on the beat, and the off-8ths); swing delays the off-8ths, so the
    # straight set is the one the other sits late against; without swing, the set nearer the kicks is the beat
    if anchor is hits["hats"] and len(hits["kick"]):
        rel = (anchor - phase - beat / 2) / beat
        other = anchor[np.abs(rel - np.round(rel)) * beat < beat / 4]
        if len(other) >= 4:
            o2 = other - phase - beat / 2
            late = np.median(o2 - np.round(o2 / beat) * beat)       # the off-8th set against the midpoint
            if late < -3 or (abs(late) <= 3 and np.median(np.abs(((hits["kick"] - phase) / beat + 0.5) % 1 - 0.5))
                                               > np.median(np.abs(((hits["kick"] - phase - beat / 2 - late) / beat + 0.5) % 1 - 0.5))):
                phase = (phase + beat / 2 + late) % beat
    res = {"bpm": float(bpm), "pulse_ms": beat / 96, "anchor": "hats" if anchor is hits["hats"] else "kick", "elements": {}}
    for name, t in hits.items():
        unit = beat / 2 if name == "snare" else sixteenth   # (a snare sits on 8ths: a 65 ms early one must not wrap)
        off, line = offsets(t, unit, phase)
        d = {"all": stats(off)}
        pos = (line * (2 if name == "snare" else 1)) % 4   # the 16th within the beat: 0 on the beat, 2 the off-8th
        d["on_beat"] = stats(off[pos == 0]); d["off_8th"] = stats(off[pos == 2]); d["16ths"] = stats(off[(pos == 1) | (pos == 3)])
        if name == "hats" and d["off_8th"] and d["on_beat"]:   # swing: the off-8th's delay as a share of the 8th
            d["swing_pct"] = 50 + 100 * (d["off_8th"]["median_ms"] - d["on_beat"]["median_ms"]) / beat
        res["elements"][name] = d
    return res


def report(res):
    p = res["pulse_ms"]
    print(f"tempo {res['bpm']:.1f} BPM, an MPC pulse {p:.2f} ms, grid anchored on the {res['anchor']}")
    print(f"{'element':8} {'where':8} {'hits':>5} {'median ms':>10} {'pulses':>7} {'spread ms':>10}")
    for name, d in res["elements"].items():
        for where in ("all", "on_beat", "off_8th", "16ths"):
            s = d.get(where)
            if not s: continue
            print(f"{name:8} {where:8} {s['n']:5d} {s['median_ms']:+10.1f} {s['median_ms'] / p:+7.1f} {s['iqr_ms']:10.1f}")
        if "swing_pct" in d:
            print(f"{name:8} swing (off-8th delay) {d['swing_pct']:.1f} %")
    print("negative = early, positive = late, against the straight 16th grid; the bass needs its own stem")


def selftest():
    """a 90 BPM beat with known offsets: snare 65 ms early, kick 25 ms late, hats straight with 56 % swing"""
    bpm, bars = 90, 8
    beat = 60 / bpm
    n = int(SR * beat * 4 * bars + SR)
    x = np.zeros(n)
    t = np.arange(SR // 2) / SR

    def hit(at, wave):
        i = int(at * SR)
        x[i:i + len(wave)] += wave[: n - i]

    kick = np.sin(2 * np.pi * (55 * t + 40 / 30 * (1 - np.exp(-t * 30)))) * np.exp(-t * 18)   # 95 Hz dropping to 55
    snare = (np.random.default_rng(1).standard_normal(len(t)) * 0.4 + np.sin(2 * np.pi * 190 * t)) * np.exp(-t * 25)
    snare = band(np.concatenate([snare, np.zeros(10)]), 150, 6000)[: len(t)]
    hat = np.random.default_rng(2).standard_normal(len(t)) * np.exp(-t * 90)
    hat = band(np.concatenate([hat, np.zeros(10)]), 7000, None)[: len(t)] * 0.5
    for bar in range(bars):
        b0 = bar * 4 * beat
        hit(b0 + 0.025, kick); hit(b0 + 2 * beat + 0.025, kick); hit(b0 + 2.5 * beat + 0.025, kick * 0.6)
        hit(b0 + beat - 0.065, snare); hit(b0 + 3 * beat - 0.065, snare)
        for e in range(8):
            hit(b0 + e * beat / 2 + (0.06 * beat if e & 1 else 0), hat * (1 if e % 2 == 0 else 0.7))
    res = measure(x, None)
    report(res)
    e = res["elements"]
    ok = abs(res["bpm"] - 90) < 0.5 and abs(e["kick"]["all"]["median_ms"] - 25) < 6 and \
         abs(e["snare"]["all"]["median_ms"] + 65) < 6 and abs(e["hats"]["on_beat"]["median_ms"]) < 4 and \
         abs(e["hats"]["swing_pct"] - 56) < 1.5
    print("selftest", "ok" if ok else "FAILED")
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("track", nargs="?")
    ap.add_argument("--bpm", type=float)
    ap.add_argument("--start", type=float)
    ap.add_argument("--end", type=float)
    ap.add_argument("--json")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        sys.exit(selftest())
    if not a.track:
        ap.error("a track, or --selftest")
    res = measure(load(a.track, a.start, a.end), a.bpm)
    report(res)
    if a.json:
        with open(a.json, "w") as f:
            json.dump(res, f, indent=1)


if __name__ == "__main__":
    main()
