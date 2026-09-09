# VBlank interrupt analysis

## `VBlankIntr` — `0x08000220`

This Thumb function runs on each vertical blank interrupt. Ghidra measured its
body as 312 bytes; the reconstructed continuous ROM range is 364 bytes including
interleaved literal pools.

Verified high-level flow:

1. Increments the frame counters at `0x02035CA8` and IWRAM `0x03000004`.
2. Increments the interrupt-depth/reentrancy counter at `0x02000EB8`.
3. Skips expensive updates and takes the exit path unless the counter is `1`.
4. Calls at least two fixed per-frame subsystems and a third optional subsystem when `0x02000D08` is enabled.
5. Selects one of two graphics/transfer update paths based on `REG_VCOUNT` (`0x04000006`) and the state flag at `0x02000130`.
6. Caps the IWRAM frame/delay value at `5` under certain conditions.
7. Calls the shared finalization function, decrements the reentrancy counter, and sets the BIOS VBlank IRQ flag through `0x03007FF8`.

The original names of its 10 distinct callees are unknown. Functional names can
be assigned using hardware-register accesses and mGBA observations.

## Matching status

The current readable C source is `src/interrupt/vblank_intr.c`, replacing the
earlier Thumb assembly source `src/interrupt/vblank_intr.s`. `make vblank-match`
verifies **364/364 matching bytes** at ROM offsets `0x000220–0x00038B`.
