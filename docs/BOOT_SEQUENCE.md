# GBA boot sequence

This evolving boot map was derived from the initial automated analysis. Early
estimates and historical source paths are retained below; see [STATUS.md](STATUS.md)
for current progress.

## `AgbMain` — `0x080000C0`

The ARM branch in the ROM header targets this address. The function:

1. Switches the CPU to IRQ mode (`0x12`) and sets the IRQ stack pointer to `0x03007FA0`.
2. Switches the CPU to System mode (`0x1F`) and sets the main stack pointer to `0x03007E00`.
3. Writes the address of `IntrMain` (`0x08000104`) to the GBA BIOS user IRQ pointer at `0x03007FFC`.
4. Branches through pointer `0x08000431` to the Thumb function `GameInit` (`0x08000430`).
5. Restarts from the entry point if `GameInit` returns.

Reassembling `src/bootstrap/agb_main.s` reproduces **68/68 bytes** at ROM offsets
`0x0000C0–0x000103`, including the function body and adjacent literal pool.
`make bootstrap-match` verifies this automatically.

## `IntrMain` — `0x08000104`

The ARM user interrupt dispatcher reads the GBA interrupt flags and branches to
the appropriate handler. Its body and literal pool are byte-matching. See
[IRQ_DISPATCH.md](IRQ_DISPATCH.md) for the detailed priority order.

## `GameInit` — `0x08000430`

The high-level Thumb initialization function. Ghidra initially estimated a
652-byte boundary, which required manual verification.

The initial decompilation and literal constants indicate that it:

- Configures `REG_WAITCNT` (`0x04000204`).
- Uses the DMA3 register block (`0x040000D4`) to fill/clear EWRAM (`0x02000000`), IWRAM (`0x03000000`), VRAM (`0x06000000`), and OAM (`0x07000000`) during initialization.
- Sets the BIOS VBlank interrupt flag at `0x03007FF8`.
- Establishes a high-level game loop reaching 44 distinct subfunctions.
- Calls subsystems believed to handle input, graphics, audio, entities, and game state each frame. Precise names require dynamic testing and register-access analysis.

The initial raw C-like output is stored locally as `analysis/decompiler/GameInit.c`.
The reviewed, named assembly implementation was originally `src/bootstrap/game_init.s`;
its current C implementation is `src/bootstrap/game_init.c`. The function and
literal pools reproduce **768/768 bytes** at `0x08000430–0x0800072F`.

Together, the boot, IRQ, display reset, save, serialization, and initial UI
functions reproduce a continuous **5016/5016-byte** region at
`0x080000C0–0x08001457`.

## Confidence

- Addresses and ARM/Thumb modes: high confidence.
- `AgbMain` behavior: high confidence, byte-matching.
- `IntrMain` and `GameInit` names: functional/provisional names, not original symbols.
- `IntrMain` boundary and behavior: high confidence, byte-matching.
- `GameInit` boundary and machine code: high confidence, byte-matching; the roles of its unnamed callees remain provisional.
