# Hand-edited creature frames

Drop a hand-edited copy of a creature's preview here, under the same name (`master_snail.png`,
`bass_anglerfish.png` ...), and it becomes the truth for that creature: the build reads it instead of
cutting the frames out of the concept sheet, and the sheet's patches no longer apply.

How to edit one:

1. Copy `docs/creatures/sprites/<name>.png` into this folder. It is the four frames side by side at 4x
   (480 x 88 px frames, 1920 x 352 px in all), drawn in the creature's own colours.
2. Edit it in any pixel editor (Aseprite, Pixelmator, Photoshop, Piskel ...). Keep the image size (or an
   exact whole multiple of it), and use only the colours already in it: the background colour is
   "transparent", the others are the creature's body, accents and dark. Every pixel is mapped back to the
   nearest of those colours, so anti-aliasing is harmless but gradients are not.
3. Rebuild (`./build.sh`, or `tools/gen_creatures.py --header build/gen/creature_sprites.h`). The tool
   prints `from the edited ...` for the creature it took from here.

A frame is 120 x 88 device pixels (shown 4x); the creature stands on row 86.
