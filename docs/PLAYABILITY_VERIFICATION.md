# Playability verification — v0.19

This document records the regressions reported after v0.18 and the automated
checks that prevent them from returning.

## Board/render boundary

The exact playfield begins at nametable tile X=12 and is 10×20 tiles. The old
statistics cleanup covered X=4..12, so it restored the first playfield column
with background tiles after the board and active piece had been drawn.

v0.19 limits that cleanup to X=6..9. `playability_regression_tests` asserts that
the cleanup end is never greater than the playfield start.

## Normal level selection

The original visible selector contains levels 0–9. v0.18 could persist 10–19
while drawing only the low digit, so a saved level 10 looked like level 0.

v0.19 migrates old values by their displayed digit (`10 → 0`, `19 → 9`) and
clamps normal keyboard/controller selection to 0–9. Replay decoding still
accepts historical start levels above 9.

## Type/music menu

The translated menu input now follows the original sequence:

- Left/right: A-Type or B-Type.
- Down: Music 1 → Music 2 → Music 3 → Off.
- Up: reverse order.
- Start/Enter: continue to level selection.

The same helper is used for keyboard, controller and touch-generated keys.

## Audio callback margin

The live ROM APU executes the original 6502 sound driver. Older builds asked
SDL for 512 samples, about 10.7 ms at 48 kHz. v0.19 requests 2048 samples on PC
and 4096 on Android to reduce underruns while the APU path is still being moved
toward a producer/ring-buffer design.

Advanced users can set `TETRIS_AUDIO_BUFFER_SAMPLES` to a power of two from 256
to 8192. This is a stability repair, not a claim of cycle-perfect audio.

## Original PPU addresses asserted by tests

- Lines: `$2073`
- Top score: `$20B8`
- Score: `$2118`
- Level: `$22BA`

The regression test also checks level-zero gravity (48 NTSC frames per drop).
