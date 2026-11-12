#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Mike Maven and Rumiboard contributors
"""Rumiboard's groove presets as Ableton Live groove files (.agr), one per element, for Live's Groove Pool.

  ableton_grooves.py [--out ableton/grooves]

A Live groove is a MIDI clip whose notes sit off the grid; applied to a clip (MIDI, or a warped audio clip) it
moves each note or transient by the deviation of the groove's note at that place in the bar. One groove moves
every note of a clip the same way whatever its pitch, so each drum (or the bass) takes its own: a bar of 16ths,
each moved by the element's MPC pulses (1/96 of a beat, as seq.c GROOVES), Timing 100 %, Velocity and Random 0,
Quantize 0. An early element wraps: its first 16th is written at the bar's end, just before the loop, which Live
reads as the first grid line early. The files are plain XML, as Live's own Core Library grooves are."""
import argparse
import os

# Rumiboard's presets, in pulses (firmware/src/seq.c GROOVES; docs/GROOVE.md): kick, snare / clap, hats on the beat,
# off-beat hats, bass
PRESETS = {
    "Dilla": {"Kick": 3, "Snare": -1, "Hats": (0, 3), "Bass": 10},
    "Dilla Drag": {"Kick": 6, "Snare": -1, "Hats": (0, 3), "Bass": 12},
    "Dilla Rush": {"Kick": 3, "Snare": -10, "Hats": (0, 3), "Bass": 10},
}
PULSE = 1 / 96                                       # a beat in Live is 1.0


def groove_xml(name, offsets):
    """a Live groove: a one-bar clip of 16ths, offsets[k] (beats) moving 16th k; negative offsets at k = 0 wrap"""
    notes = []
    nid = 1
    for k in range(16):
        t = k * 0.25 + offsets[k]
        if t < 0:
            t += 4.0                                  # (the first 16th, early: written just before the loop's end)
        notes.append((t, nid))
        nid += 1
    notes.sort()
    note_lines = "\n".join(
        f'\t\t\t\t\t\t\t\t\t<MidiNoteEvent Time="{t:.10g}" Duration="0.0625" Velocity="100" VelocityDeviation="0" '
        f'OffVelocity="64" Probability="1" IsEnabled="true" NoteId="{i}" />' for t, i in notes)
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<Ableton MajorVersion="5" MinorVersion="11.0_428" SchemaChangeCount="3" Creator="Rumiboard tools/ableton_grooves.py" Revision="">
\t<Groove>
\t\t<LomId Value="0" />
\t\t<Name Value="{name}" />
\t\t<Clip>
\t\t\t<Value>
\t\t\t\t<MidiClip Id="0" Time="0">
\t\t\t\t\t<LomId Value="0" />
\t\t\t\t\t<LomIdView Value="0" />
\t\t\t\t\t<CurrentStart Value="0" />
\t\t\t\t\t<CurrentEnd Value="4" />
\t\t\t\t\t<Loop>
\t\t\t\t\t\t<LoopStart Value="0" />
\t\t\t\t\t\t<LoopEnd Value="4" />
\t\t\t\t\t\t<StartRelative Value="0" />
\t\t\t\t\t\t<LoopOn Value="true" />
\t\t\t\t\t\t<OutMarker Value="4" />
\t\t\t\t\t\t<HiddenLoopStart Value="0" />
\t\t\t\t\t\t<HiddenLoopEnd Value="4" />
\t\t\t\t\t</Loop>
\t\t\t\t\t<Name Value="{name}" />
\t\t\t\t\t<Annotation Value="" />
\t\t\t\t\t<Color Value="24" />
\t\t\t\t\t<LaunchMode Value="0" />
\t\t\t\t\t<LaunchQuantisation Value="0" />
\t\t\t\t\t<TimeSignature>
\t\t\t\t\t\t<TimeSignatures>
\t\t\t\t\t\t\t<RemoteableTimeSignature Id="0">
\t\t\t\t\t\t\t\t<Numerator Value="4" />
\t\t\t\t\t\t\t\t<Denominator Value="4" />
\t\t\t\t\t\t\t\t<Time Value="0" />
\t\t\t\t\t\t\t</RemoteableTimeSignature>
\t\t\t\t\t\t</TimeSignatures>
\t\t\t\t\t</TimeSignature>
\t\t\t\t\t<Envelopes>
\t\t\t\t\t\t<Envelopes />
\t\t\t\t\t</Envelopes>
\t\t\t\t\t<ScrollerTimePreserver>
\t\t\t\t\t\t<LeftTime Value="0" />
\t\t\t\t\t\t<RightTime Value="4" />
\t\t\t\t\t</ScrollerTimePreserver>
\t\t\t\t\t<TimeSelection>
\t\t\t\t\t\t<AnchorTime Value="0" />
\t\t\t\t\t\t<OtherTime Value="0" />
\t\t\t\t\t</TimeSelection>
\t\t\t\t\t<Legato Value="false" />
\t\t\t\t\t<Ram Value="false" />
\t\t\t\t\t<GrooveSettings>
\t\t\t\t\t\t<GrooveId Value="-1" />
\t\t\t\t\t</GrooveSettings>
\t\t\t\t\t<Disabled Value="false" />
\t\t\t\t\t<VelocityAmount Value="0" />
\t\t\t\t\t<FollowAction>
\t\t\t\t\t\t<FollowTime Value="4" />
\t\t\t\t\t\t<IsLinked Value="true" />
\t\t\t\t\t\t<LoopIterations Value="1" />
\t\t\t\t\t\t<FollowActionA Value="4" />
\t\t\t\t\t\t<FollowActionB Value="0" />
\t\t\t\t\t\t<FollowChanceA Value="100" />
\t\t\t\t\t\t<FollowChanceB Value="0" />
\t\t\t\t\t\t<JumpIndexA Value="0" />
\t\t\t\t\t\t<JumpIndexB Value="0" />
\t\t\t\t\t\t<FollowActionEnabled Value="false" />
\t\t\t\t\t</FollowAction>
\t\t\t\t\t<Grid>
\t\t\t\t\t\t<FixedNumerator Value="1" />
\t\t\t\t\t\t<FixedDenominator Value="16" />
\t\t\t\t\t\t<GridIntervalPixel Value="20" />
\t\t\t\t\t\t<Ntoles Value="2" />
\t\t\t\t\t\t<SnapToGrid Value="true" />
\t\t\t\t\t\t<Fixed Value="true" />
\t\t\t\t\t</Grid>
\t\t\t\t\t<FreezeStart Value="0" />
\t\t\t\t\t<FreezeEnd Value="0" />
\t\t\t\t\t<IsWarped Value="true" />
\t\t\t\t\t<TakeId Value="1" />
\t\t\t\t\t<Notes>
\t\t\t\t\t\t<KeyTracks>
\t\t\t\t\t\t\t<KeyTrack Id="1">
\t\t\t\t\t\t\t\t<Notes>
{note_lines}
\t\t\t\t\t\t\t\t</Notes>
\t\t\t\t\t\t\t\t<MidiKey Value="36" />
\t\t\t\t\t\t\t</KeyTrack>
\t\t\t\t\t\t</KeyTracks>
\t\t\t\t\t\t<PerNoteEventStore>
\t\t\t\t\t\t\t<EventLists />
\t\t\t\t\t\t</PerNoteEventStore>
\t\t\t\t\t\t<NoteIdGenerator>
\t\t\t\t\t\t\t<NextId Value="{nid}" />
\t\t\t\t\t\t</NoteIdGenerator>
\t\t\t\t\t</Notes>
\t\t\t\t\t<BankSelectCoarse Value="-1" />
\t\t\t\t\t<BankSelectFine Value="-1" />
\t\t\t\t\t<ProgramChange Value="-1" />
\t\t\t\t\t<NoteEditorFoldInZoom Value="-1" />
\t\t\t\t\t<NoteEditorFoldInScroll Value="0" />
\t\t\t\t\t<NoteEditorFoldOutZoom Value="3072" />
\t\t\t\t\t<NoteEditorFoldOutScroll Value="-1577" />
\t\t\t\t\t<NoteEditorFoldScaleZoom Value="-1" />
\t\t\t\t\t<NoteEditorFoldScaleScroll Value="0" />
\t\t\t\t\t<ScaleInformation>
\t\t\t\t\t\t<RootNote Value="0" />
\t\t\t\t\t\t<Name Value="Major" />
\t\t\t\t\t</ScaleInformation>
\t\t\t\t\t<IsInKey Value="false" />
\t\t\t\t\t<NoteSpellingPreference Value="3" />
\t\t\t\t\t<PreferFlatRootNote Value="false" />
\t\t\t\t\t<ExpressionGrid>
\t\t\t\t\t\t<FixedNumerator Value="1" />
\t\t\t\t\t\t<FixedDenominator Value="16" />
\t\t\t\t\t\t<GridIntervalPixel Value="20" />
\t\t\t\t\t\t<Ntoles Value="2" />
\t\t\t\t\t\t<SnapToGrid Value="false" />
\t\t\t\t\t\t<Fixed Value="false" />
\t\t\t\t\t</ExpressionGrid>
\t\t\t\t</MidiClip>
\t\t\t</Value>
\t\t</Clip>
\t\t<Grid Value="3" />
\t\t<QuantizationAmount Value="0" />
\t\t<TimingAmount Value="100" />
\t\t<RandomAmount Value="0" />
\t\t<VelocityAmount Value="0" />
\t\t<Annotation Value="" />
\t\t<Selection Value="true" />
\t\t<SourceContext />
\t</Groove>
</Ableton>
'''


def element_offsets(spec):
    """the 16 offsets (beats) of an element: one shift, or (on the beat, off-beat 8th) for the hats"""
    if isinstance(spec, tuple):
        on, off = spec
        return [(off if k % 4 == 2 else on) * PULSE for k in range(16)]
    return [spec * PULSE] * 16


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default="ableton/grooves")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rows = []
    for preset, elements in PRESETS.items():
        for element, spec in elements.items():
            pulses = spec if not isinstance(spec, tuple) else f"{spec[0]} / off-beat {spec[1]:+d}"
            name = f"Rumi {preset} {element}"
            path = os.path.join(a.out, name + ".agr")
            with open(path, "w", encoding="utf-8") as f:
                f.write(groove_xml(name, element_offsets(spec)))
            rows.append((name, pulses))
            print(f"{path}: {pulses} pulses")
    # the pulses in milliseconds at common tempos, for Live's Track Delay
    print("\npulses in ms:  " + "  ".join(f"{bpm} BPM" for bpm in (80, 85, 90, 95, 100, 120)))
    for p in (1, 3, 6, 10, 12):
        print(f"{p:3d} pulses  " + "  ".join(f"{p * 60000 / bpm / 96:6.1f}" for bpm in (80, 85, 90, 95, 100, 120)))


if __name__ == "__main__":
    main()
