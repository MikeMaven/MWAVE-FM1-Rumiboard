# Drum kits and samples

## 1. The decision

- **Keep the synthesized kits** the base firmware has (808, 909, CR-78, FM, SID). House leans on
  808 and 909 sounds, they are fully synthesized, and they cost no flash beyond code already there.
- **Add sampled lofi kits**, preloaded in the firmware package, and a handful of hand-picked house
  one-shots. The FM-1 has one 1 MB flash chip and no expansion (see STORAGE_PLAN.md), so this is a
  curated bank, not a library.
- Sample playback returns from Felucca's engines (GPL-3.0, same family; Melodee removed them), reading
  IMA ADPCM straight from flash. The drum rack then mixes synthesized lanes and sampled lanes per kit.

## 2. The kits (brief)

Four lofi kits, six to eight sounds each (kick, snare, clap, closed hat, open hat, one or two
percussion; a rim or shaker where it suits), plus a small house extras set.

| Kit | Character | Notes for picking sounds |
|---|---|---|
| **SLEEPY** | very soft, chill, late-night | muffled, round kick; brushed or felt-soft snare; hats like tape hiss; everything dark, slow decays |
| **STUDY** | the "beats to study to" kit: steady, warm, unobtrusive | dusty but tight kick; mellow snare with a little crack; soft closed hat with a short open hat; a woody perc |
| **JAZZ** | brushed kit, room, swing | brushed snare and swish; low floor tom; ride-like hat; rim click; a kick with a real-room thud |
| **CHILLHOP** | snappier, more head-nod | punchier kick with a click; snappy snare and a layered clap; crisp hats; a snap or bongo perc |
| **HOUSE EXTRAS** | a few one-shots to sit beside the 808/909 synth kits | a vinyl-worn clap, a shaker, a ride, a tambourine, a deep sub kick, an open-hat loop slice or two |

All of them run through the lofi master bus (tape saturation, wow/flutter, tilt, vinyl noise), so the
raw samples can be cleaner than the final sound; the bus supplies the dust.

## 3. Budget

Rates: 22.05 kHz is the default (the classic sampler sound, right for this music); 44.1 kHz only for
a few sounds that need the top (a bright hat, a clean clap). IMA ADPCM, 4 bits a sample.

| Item | Estimate |
|---|---:|
| one 7-sound lofi kit at 22 kHz (kick 0.4 s, snare 0.3, clap 0.3, CH 0.15, OH 0.5, 2 perc 0.2) | ~22 KB |
| four kits | ~90 KB |
| house extras (8 sounds, a few at 44.1 kHz) | ~35 KB |
| **sample bank** | **~125 KB**, round to a 160 KB partition for headroom and swaps |

That fits in the 400 KB user-data area, taken mostly from the project pool (four slots of up to
~42 KB); the exact split is decided once we know how much project space is used in practice.

## 4. Pipeline

1. Curate: pick the sounds (the user's go-to drums), trim and normalise, choose the rate per sound.
2. Convert: `tools/` gets a `gen_kits.py` that writes the ADPCM bank and a kit table (lane, name,
   root, decay) from a folder of WAVs, as Felucca's `gen_samples.py` does.
3. Package: the bank goes into the firmware package so installing Rumiboard writes the kits.
4. Swap: the web editor's sample upload (Felucca's protocol, `tools/fm1_sample_upload.py`) replaces a
   kit without reflashing.
5. Licensing: only sounds we have the right to ship. The user's own kits are fine; third-party
   sample packs need a licence that allows redistribution in firmware (CC0 or the pack's terms).

## 5. Order

1. Bring Felucca's ADPCM playback back as sampled drum lanes (engine work).
2. The bank format, the generator and the flash partition.
3. The four kits and the house extras, curated with the user.
