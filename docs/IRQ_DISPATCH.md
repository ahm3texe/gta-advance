# Interrupt dispatcher map

## `IntrMain` — `0x08000104`

`AgbMain` writes this ARM function's address to the BIOS user IRQ vector at
`0x03007FFC`. The dispatcher contains 276 bytes of code and an 8-byte literal pool.

Execution order:

1. Reads `REG_IE` and `REG_IF` together through `0x04000200`.
2. Saves `REG_IME` (`0x04000208`) and temporarily restricts interrupt processing.
3. Computes enabled, pending interrupts as `IE & IF`.
4. Selects the first source in the priority order below.
5. Acknowledges the selected IF bit.
6. Records the handler offset at `0x02000284`.
7. Calls the ARM/Thumb handler pointer in the table at `0x02000170 + handler_offset`.
8. Restores CPU, IE/IF/IME, register, and SPSR state.

## Priority and table offsets

| Priority | IF bit | Source | Handler offset |
|---:|---:|---|---:|
| 1 | `0x0001` | VBlank | `0x04` |
| 2 | `0x0004` | VCount | `0x0C` |
| 3 | `0x0002` | HBlank | `0x10` |
| 4 | `0x0008` | Timer 0 | `0x14` |
| 5 | `0x0100` | DMA 0 | `0x18` |
| 6 | `0x0200` | DMA 1 | `0x1C` |
| 7 | `0x0400` | DMA 2 | `0x20` |
| 8 | `0x0800` | DMA 3 | `0x24` |
| 9 | `0x1000` | Keypad | `0x28` |
| 10 | `0x2000` | GamePak | `0x2C`; special case |

A GamePak interrupt clears `SOUNDCNT_X` (`0x04000084`) and enters an infinite loop;
no normal handler is called.

Timer 1–3 and Serial bits are not selected directly by this scan chain. The effect
of mask `0x20C0` on IE writeback needs further handler-table analysis.

## `InitInterrupts` — `0x0800038C`

Initializes the handler table and hardware as follows:

- Disables `REG_IME`.
- Fills the 13-entry table at `0x02000170` with the default Thumb handler `0x08000735`.
- Installs `VBlankIntr` (`0x08000221`) in the VBlank slot.
- Installs Thumb handler `0x0800079D` in the Timer 0 slot.
- Uses DMA3 to copy `IntrMain` from ROM address `0x08000104` to EWRAM address `0x020004D0`.
- Points the BIOS IRQ vector at `0x03007FFC` to the EWRAM copy.
- Sets `REG_DISPSTAT = 0x3228`, `REG_IE = 0x0005`, and finally `REG_IME = 1`.

The current source is `src/bootstrap/init_interrupts.c` (formerly
`src/bootstrap/init_interrupts.s`). `make init-interrupts-match` verifies
**164/164 matching bytes** at ROM offsets `0x00038C–0x00042F`.

## Helper handlers — `0x08000730–0x080007B3`

- `NoOpVBlankFinalize` and `DummyIntr`: two-byte return functions.
- `RunVBlankTransfers`: runs the graphics/transfer callees seen during VBlank through a shared path and caps the IWRAM frame/delay counter.
- `NoOpInterruptHelper`: a second two-byte empty helper.
- `VCountIntr`: calls subroutine `0x080327C8` and acknowledges the VCount bit in `REG_IF`.

This block matches **132/132 bytes**. Its current source is
`src/interrupt/irq_helpers.c`, replacing the earlier `.s` implementation.

## `ResetDisplayAndInterrupts` — `0x080007B4`

Clears VRAM and OAM with DMA3, prepares VBlank/display state, updates the BIOS IRQ
flag, and calls `InitInterrupts` again. `src/bootstrap/reset_display_interrupts.c`
replaces the earlier `.s` implementation and matches **120/120 bytes**, including
the body and literal pool.

## Matching status

The readable assembly source is `src/bootstrap/intr_main.s`. `make intr-match`
compares the body and literal pool against ROM offsets `0x000104–0x00021F`:
**284/284 matching bytes**.
