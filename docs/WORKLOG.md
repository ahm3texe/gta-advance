# Work log

## 2026-09-04 — Closing the 53 boundary findings

- Each finding was examined with recursive Thumb flow and ROM disassembly; the
  decision evidence was written into `data/boundary_review.csv`.
- 52 of the findings were false splits created by the `split_at_calls.py` tool
  mistaking literal pools and intra-function shared blocks for linear `bl`
  targets.
- `0x08053FF2` was a false start inside the upper half of the literal
  `0x1C03FFFF`; `0x08053FF4`, which has three real `bl` calls and a
  `push {r4-r7,lr}` prologue, was kept as a separate function.
- The short-boundary debt went from 53 to 0. The 18 records in the ARM range and
  the 4 records exceeding the 4096-byte threshold were kept open as separate
  review classes.
- `ScanAllEntries` was renamed `ProcessFirstEntry` to reflect the ROM's real
  behavior of processing only entry 0.
- Conflicting extern types for `gRam02000F10`, `gRam02001140`, and
  `gRam02025810` were unified into shared raw-storage declarations. The byte
  output of nine matching functions did not change; the consistency gate now
  rejects new type conflicts automatically.
- 57 unused UI scaffold/hook files and eight unnecessary runtime dependencies
  were removed from the dashboard. The full-repository dashboard lint and
  production build now pass; the milestone gate runs the full lint instead of a
  narrowed one.
- Four excessive-growth records were closed with ARM/Thumb disassembly. The
  missing tails of three large functions were extended; the 8,320 bytes between
  `0x0802BDF0` and `0x0802DE70` were verified as a single stack frame. A
  negative-knowledge table of 57 addresses was added to prevent false entries
  from being rediscovered.
- All 18 records in the ARM overlay were reviewed: 14 stack-frame functions and 4
  local BL routines that use the parent's register/frame state. Four missing
  sizes were corrected; the `0x08067E04–0x0806B84C` range was verified gap-free.
  Every open/skipped class in the boundary baseline went to zero.

## 2026-09-02 — First major reverse-engineering pass

### Environment and protections

- The Europe ROM was verified by SHA-1; the ROM and save/state outputs were kept
  out of Git.
- Ghidra 12.1.3, OpenJDK 21, mGBA 0.10.5, and the ARM GNU 16.2 toolchain were
  prepared.
- A Ghidra ARMv4T project, an automatic function map, decompile export, and CSV
  synchronization were set up.

### Modules mapped

- The ARM entry point, IRQ dispatcher, VBlank/VCount, and the interrupt table.
- The `GameInit` startup / main loop skeleton.
- The `CRAWSAVE` metadata layer and the low-/high-level save functions using
  Nintendo's `EEPROM_V124`.
- Three game save slots, the marker/complement checksum scheme, and the
  little-endian serialization helpers.
- The initial UI menu item filtering, drawing, screen initialization, and
  graphics loading helpers.
- The large `RunMenuScreen` entry/submenu flow was documented for the first time;
  not yet matching.

### Verified measurements

- Ghidra function candidates: 1497.
- Byte-matching functions: 42.
- Matching function bodies: 4660 / 290837 bytes (`1.60%`).
- Matching ROM area including literals/padding: 5340 unique bytes.
- Largest contiguous matching range: `0x080000C0–0x08001457`, 5016 bytes.
- Second matching UI range: `0x08001DC0–0x08001F03`, 324 bytes.

### Automated verification

- `make progress`: shows the function and ROM-region metrics.
- `make matching`: rebuilds 21 binary fragments, compares each against the ROM,
  and produces a merged range report.
- `make doctor`: checks the ROM hash and that the required tools are installed.
- `make dashboard-dev`: shows the size, state, and module of 1497 functions on an
  interactive treemap, with search, filtering, and a detail panel.
- `make dashboard-build`: generates the current CSV data and validates the
  dashboard's distribution build.

### Next technical goal

1. Pin down `RunMenuScreen`'s (`0x08001458`) control flow and its input/action
   tables.
2. Classify the UI helpers after `0x08001F04`.
3. Dynamically verify the menu variables with an mGBA breakpoint/watchpoint
   session.
4. Determine the compiler fingerprint and move suitable matching assembly
   fragments to readable C.

## 2026-09-03 — Compiler identity resolved, the move to C begins

### Finding

It was proven at byte level that the ROM was compiled with **`old_agbcc`**. This
means the project is not forced into semantic reconstruction and can target
byte-matching from C.

The evidence chain:

1. **Code patterns.** Across the 42 verified functions, register copying appears
   84 times as `adds rX, rY, #0` (the agbcc pattern) and 0 times as
   `movs rX, rY` (the modern pattern); function returns appear 28 times as
   `pop {rN}` + `bx rN` and 0 times as `pop {..., pc}`.
2. **Byte verification.** `src/save/save_helpers.c` was written; 5 of its 6
   functions matched the ROM exactly. All 28 bytes of `WriteU32LE` matched,
   including a distinctive choice: building the mask with two `mov`+`lsl`
   instructions rather than reading it from the literal pool.
3. **Variant discrimination.** Six combinations were tried: `agbcc -O2/-O1` 3/6,
   **`old_agbcc -O2/-O1` 5/6**, both at `-O0` 0/6. Same C source, without a
   single line changed.

Flags: `old_agbcc -mthumb-interwork -O2 -fhex-asm`.

### Left open

`WriteU16LE` (0x08001124): on entry the ROM performs a semantically unnecessary
16-bit truncation that `old_agbcc` eliminates. Nine different C forms were tried,
none held; the attempts are listed in the source file. The assembly source
remains valid.

### Tools added

- `make agbcc` — builds the compiler locally (`tools/setup_agbcc.sh`). Because
  agbcc is 1998-era C source, a modern clang compatibility wrapper is needed; the
  script sets it up.
- `make c-match FILE=...` — compares every function in a C file against the ROM.
- `make diff FILE=... FUNC=...` — shows a single function's ROM form beside its
  compiled form, instruction by instruction. The answer to "where does it
  diverge" when fixing a non-matching function.
- `make doctor` now also checks the compiler.

objdiff was evaluated but not installed: it compares two *object files*, which
would require a splat/dtk pipeline producing target `.o` files on the ROM side as
well. Until that is set up, `make diff` does the same job within our data model.

### Repository hygiene

The first commit was made (154 files). Kept out of Git: the ROM, the Ghidra
project, the Ghidra decompiler output (`analysis/decompiler/`), the generated
dashboard data, and the agbcc binaries. Because the decompiler output is material
derived from the ROM, it was left local in line with the repository's own
publication policy.

### Next technical goal

1. Move the remaining `EraseSaveSlot`, `GetSaveSlotHeader`, and `WriteU16LE` in
   the `save_helpers` block to C; once the block is complete, switch
   `matching_regions.csv` to the `.c` build and remove `save_helpers.s`.
2. Continue from the leaf functions and move the save and ui modules to C.
3. An mGBA patch/run loop — behavioral verification for non-byte-matching
   functions.
4. `RunMenuScreen`'s control flow.

## 2026-09-03 (continued) — The linking pipeline and the save_helpers block

### Critical infrastructure: linking

No non-leaf function can be verified without linking — `bl` instructions do not
produce the correct bytes until the target address is resolved.
`tools/agbcc_build.py` was added: it compiles the C source, links it at the
block's ROM base address, and resolves external symbols from
`data/functions.csv`. `verify_c_function.py` and `diff_function.py` now use this
shared layer.

**A trap caught:** agbcc aligns the `.text` section to 8. If the base address is
not a multiple of 8 (like 0x08001094), the linker pushes the section 4 bytes
forward and *every measurement shifts, including previously matching functions*.
The section address is now explicitly fixed in the link script (`SUBALIGN(1)`).
This failure is silent: everything appears "not matching" and the cause is
assumed to be in the code.

### The save_helpers block: 5 of 8 functions byte-matching from C

Added: `EraseSaveSlot`, `GetSaveSlotHeader`. Neither matched, but their structure
is correct — all the instructions are there, the difference is in the ordering.

Both show the **same systematic difference**: the ROM loads the base address
before the index computation, agbcc after. For `GetSaveSlotHeader`, five
different local variable arrangements, pointer arithmetic, an inverted condition,
a `void*` return, an extern array symbol, and two compiler variants were tried —
**all five produced byte-for-byte identical output.** agbcc is insensitive to the
C form in this function, so it is not a problem that can be solved by tinkering
with the C. The remaining possibilities: a difference between pret/agbcc's
rebuilt version and the original SDK version, or a compiler flag not yet found.

`WriteU16LE` is also open (nine C forms tried, recorded).

### Memset named

`FUN_0806dcc0` was examined and confirmed as `Memset`: it takes (dest, value,
count), spreads the byte into a 4-byte pattern, writes 16 bytes at a time with
`stmia`, finishes the remainder byte by byte, and returns `dest`. Recorded in
`function_overrides.csv` as `documented`/`sdk`.

### Added: make disasm

`make disasm FUNC=...` produces a function's disassembly directly from the ROM.
It will be deleted as assembly sources move to C; access to the original code is
preserved by this tool instead, in a form that needs no maintenance and cannot go
stale.

### Status

`make matching` 21/21, unbroken. `save_helpers.s` is still a valid build source;
it stays that way until the block is 8/8.

## 2026-09-03 (continued 2) — The first assembly file retired

### The real finding: RAM addresses must be extern symbols

I had assumed the reason `EraseSaveSlot` and `GetSaveSlotHeader` did not match
was the compiler version. **That was wrong.** The cause was on the C side:

```c
#define gSaveSlotHeaders ((SaveSlotHeader *)0x02000460)   /* gets folded */
extern SaveSlotHeader gSaveSlotHeaders[3];                /* correct     */
```

When the address is a compile-time constant, agbcc turns `base + 16` into a
separate literal and does not keep the base in a register; the ROM, however,
loads the base once and keeps it. Converted to an extern symbol, `EraseSaveSlot`
matched instantly, and `GetSaveSlotHeader` matched once it was switched to direct
member access.

The conclusion recorded in the previous session — "the compiler hypothesis is
exhausted, these three functions will not close" — was therefore mistaken;
`docs/COMPILER.md` was corrected. The flag sweep and the `release` version
measurements remain on record — both really were ineffective, but the decisive
variable was elsewhere.

### save_wrappers: the first complete block

`IsSaveSlotValid`, `ReadSaveMetadata`, `WriteSaveMetadata` — 3/3 byte-matching,
and the whole 68-byte region exact. `src/save/save_wrappers.s` and
`config/save_wrappers.ld` were deleted; the region is now produced from C.
`make matching` still passes 21/21.

save_helpers is at 7/8: only `WriteU16LE` is open.

### Three traps fixed along the way

1. **External symbols are supplied with `.equ`.** Left to the linker, an absolute
   symbol is not treated as a Thumb function and an interworking veneer is
   inserted, giving the wrong `bl` target.
2. **Section-end padding.** `as` pads Thumb sections with NOP (`0x46C0`), the ROM
   with zero. `.align 2, 0` was added at the end of the generated assembly.
3. **Section alignment** (from the previous session): agbcc aligns `.text` to 8,
   and if the base is not a multiple of 8 every measurement shifts.

### Additions

- `tools/build_c.py` — produces a `.bin` linked at a ROM address from a C source;
  the Makefile region rules can now use it.
- `data/ram_map.csv` is now read by `agbcc_build`; `gSaveMetadata` (`0x02000ED0`)
  and `gSaveSlotHeaders` (`0x02000460`) were added as verified.

### Next

`WriteU16LE`; then moving small blocks such as `menu_helpers` (112 bytes) and
`reset_display_interrupts` (63 lines) to C. `agb_main.s` and `intr_main.s` remain
permanently assembly.

## 2026-09-03 (continued 3) — The second block moved to C

`ResetDisplayAndInterrupts` (`0x080007B4`, a 120-byte region) is byte-matching
from C. `src/bootstrap/reset_display_interrupts.s` and its link script were
deleted. `make matching` 21/21; two regions are now produced from C.

### Two new rules

**A temporary buffer on the stack must be `volatile`.** Without making the stack
variable used as a DMA source `volatile`, agbcc emitted `mov r0, sp` and
`movs r2, #0` in the reverse order. The difference dropped from 57 bytes to 7.

**A hardware/BIOS variable must NOT be `volatile`.** The remaining 7 bytes were
in the `gBiosIrqFlags` access; removing `volatile` produced an exact match. The
two look contradictory, but here `volatile` is not a semantic switch — it is an
ordering knob.

All rules are tabulated in `docs/COMPILER.md`.

### Unverified names were not adopted

The assembly source had labelled three external functions `WaitForDma3`,
`InitSubsystem`, and `WaitForVBlank`. The disassembly does not support these:
`FUN_08063b74` is not a DMA loop but writes constants to four hardware registers;
`FUN_0800cae4` does not wait for VBlank but calls two functions. The Ghidra names
were used in the C file and the rationale written in a comment.

### RAM map

`gVBlankState` (0x02000130), `gDisplayState` (0x020004BC, provisional), and
`gBiosIrqFlags` (0x03007FF8) were added.

## 2026-09-03 (continued 4) — The C metric and the third block

### The measurement was separated

Assembly transcription and byte-matching from C were appearing in the same
metric. `make c-status` (`tools/scan_c_sources.py`) now compiles the C sources
under `src/`, compares them against the ROM, and produces `data/c_sources.csv`.
`make progress` reports two new lines; the dashboard gained a separate display
state (**matching from C**, in a brighter green), a counter on the summary card,
an option in the status filter, and the source file path in the detail panel.

### menu_helpers moved to C

`ResetMenuState`, `IsMenuFlagSet`, `FinalizeMenuLayout` — 3/3, the whole 112-byte
region. `src/ui/menu_helpers.s` and its link script were deleted. The third
retired assembly file.

**Two new rules:** an array-clearing loop must be written *forward* (agbcc turns
it into a backward pointer walk; writing it backward by hand produces different
code), and the loop index must be *signed* (a pointer comparison generates an
unsigned branch, whereas the ROM uses a signed one).

### Status

- `make matching` 21/21; three regions produced from C
- C source: 15 functions, 14 byte-matching
- 8.45% of the matching bytes now come from C (394/4660)
- RAM map: menu symbols added

`FUN_080512b0` and `FUN_08004280` were not named; no name is given without
verification.

## 2026-09-03 (continued 5) — irq_helpers moved to C

The fourth retired assembly file. `NoOpVBlankFinalize`, `DummyIntr`,
`RunVBlankTransfers`, `NoOpInterruptHelper`, `VCountIntr` — 5/5, the whole
132-byte region.

### Two rules corrected

**Rule 1 is not universal.** RAM symbols must be `extern`, but addresses that
agbcc can produce by shifting are written in the ROM as constant casts:
`0x03000000` is computed in the ROM with `movs #0xc0` + `lsls #18`, not read from
a literal pool. Making it an extern symbol gave a 55-byte divergence; as a
constant cast it dropped to 8. The diff tells you which form is correct.

**Rule 4 was over-generalized.** In the previous session I wrote "a hardware
variable must not be volatile" — from a single example. `REG_IF` (`0x04000202`)
demands exactly the opposite: a 7-byte divergence without `volatile`, an exact
match with it. The same `x |= constant` idiom, with opposite requirements. The
rule was corrected to "tried separately for each access."

### A third finding

`gFrameDelay = counter = gIwramFrameCounter;` — a chained assignment. Written as
two separate lines, agbcc put the address computation in the reverse order (an
8-byte difference); written chained, it matched exactly.

### Status

- `make matching` 21/21; four regions produced from C
- C source: 20 functions, 19 byte-matching
- **10.73%** of the matching bytes come from C (500/4660)

`FUN_08012b9c`, `FUN_080133a8`, `FUN_080130f4`, `FUN_08013900`, `FUN_080101d8`,
and `FUN_080327c8` were not named.

## 2026-09-03 (continued 6) — init_menu_screen moved to C

The fifth retired assembly file. The whole 172-byte region is byte-matching. The
most complex block so far: two DMA transfers, two IME critical sections, six
consecutive calls, and a conditional tail.

### Three new rules

**A register with a save/restore pair must be `volatile`.** With `REG_IME` not
`volatile`, agbcc merged the two critical sections' saves and destroyed the
ordering entirely (a 136-byte divergence). Making it `volatile` together with
DMA3 brought it down to 46.

**An address that lives across calls is taken into a local at the top.** The ROM
loads the palette source into `r5` in the function's first instruction and keeps
it there across six calls. Read at the point of use, the compiler does not hoist
it:

```c
const u8 *palette = gMenuPaletteSource;   /* at the top */
...
REG_DMA3.src = palette;                   /* later      */
```

That single change took a 46-byte divergence to zero.

**Chained assignment** (from the previous block): `a = b = c` produces different
code from separate lines.

The rule count rose to 12.

### Status

- `make matching` 21/21; five regions produced from C
- C source: 21 functions, 20 byte-matching
- **13.69%** of the matching bytes come from C (638/4660)
- 16 assembly files remain, two of them permanent

## 2026-09-03 (continued 7) — Four blocks attempted, three finished

`make matching` still holds at 21/21.

### The three finished blocks

**menu_graphics** (212 B, 4 functions) — **4/4 on the first attempt.** The
accumulated rules (IME `volatile`, DMA3 `volatile`, constant cast) worked
directly.

**save_slots** (228 B, 2 functions) — 119 of 120 bytes held on the first attempt.
The only difference was `ble` ↔ `bls`: it matched once the `length` parameter was
made `u32`.

**read_eeprom_bytes** (208 B) — the ROM's unrolled eight-byte copy could not be
produced with `-funroll-loops` (136B/127 differences → 256B/239, worse). Writing
the eight copies out explicitly with a `COPY_EEPROM_BYTE` macro produced an exact
match.

### Left unfinished: init_save_system

197 of 240 bytes hold and the structure is correct. Two clusters of differences
resist:

1. In the slot flag clearing loop, the ROM walks the pointer downward from +31,
   ours upward from +16. The counter is the same. Six different loop forms were
   tried; the best gives a 41-byte difference.
2. The ROM takes `&gSavePayloadSize` into a callee-saved register before the
   division call. A local pointer was tried: 220 at the top of the function, 80
   at the point of use.

The attempts were written into the comment at the top of the source file.
`init_save_system.s` remains a valid build source.

### Status

- Retired assembly files: 8
- C source: 28 functions, 27 byte-matching
- **29.06%** of the matching bytes come from C

## 2026-09-03 (night) — Verified ROM area grew

For the first time this session, **new ROM area was verified** — until now the
work had been moving already-matching regions from assembly to C, without
increasing coverage.

### libc regions wired into the build

That the ROM links against agbcc's newlib had been established earlier, but that
was only a scan result. Now, with `data/libc_regions.csv` + `make libc-verify`,
each entry is extracted from `libc.a` and compared against the ROM, and
`make matching` runs it automatically.

**9/9 fragments, 448 bytes.** Total verified ROM area went from 5340 to
**5788 bytes**.

These are not a reverse-engineering result; they are proof that library code
whose source we have is identical to the bytes in the ROM.

### The layout argument

Because the bodies of `_exit` and `_kill` are identical, byte comparison could
not say which was where. The solution was not in the bytes but in the layout:
that body occurs **exactly twice** in the ROM, 32 bytes apart — the same distance
as in `syscalls.o` (`_exit` at offset 892, `_kill` at 924). The pair can only be
laid out in that order.

A generalizable method for symbol pairs with identical bodies. It could not be
applied to the `toupper` / `_toupper` pair: both contain relocations, so a plain
byte search in the ROM returns nothing. That pair remains ambiguous.

### The game_init draft

The largest block (768 bytes). The control flow was fully extracted and the head
of the function matches exactly; about 508 of 772 bytes hold.

Measured: the stack variable type makes a large difference — `u16` sources give
673 differences, `volatile u16` 590, **`u32` 264**, a `u16` array 304, a union
`.half` 739, and a `u32` slot with cast spelling 739. The ROM's 16-byte stack
frame was captured with `u32`s.

Known remaining divergence: the ROM writes a halfword (`strh`) to the DMA source,
ours a word (`str`). The slots must be 4 bytes apart while the write is 16 bits —
none of the five forms tried gave both at once.

### Parallel work infrastructure

A 12-agent workflow was launched with Ultracode. Beforehand, three race
conditions were closed: `diff_function.py`'s shared temporary file was made
call-specific, the 20 RAM addresses appearing in the remaining assembly were
added to `ram_map.csv` in one pass (the agents do not write to that file), and
`make` was forbidden to the agents entirely. Retirement decisions and final
verification stay in the main process.

## 2026-09-03 (night, workflow) — Ten blocks moved to C at once

The 12-agent parallel workflow completed (0 errors, ~33 minutes). Every result
was independently re-verified against the ROM in the main process; the agent
reports were not trusted.

### Result

**The remaining portable assembly is done.** Only three `.s` files remain under
`src/`: `agb_main.s` and `intr_main.s` (in ARM mode, permanently assembly) and
`game_init.s` (a backup of the not-yet-matching draft).

- `make matching` 21/21 + libc 9/9
- C source: 40 functions, **39 byte-matching**
- **78.97%** of the matching bytes now come from readable C (at the start of the
  session: 0%)
- 19/19 C files pass the readability check

### Both resisting functions were also solved

**`WriteU16LE`** — the answer was that the parameter is a **signed narrow type**
(`s16`). Throughout the session I had been trying *wider* types; the direction was
backwards. The `lsls #16`/`lsrs #16` pair in the ROM is semantically unnecessary,
so it is never generated with `u16`: an unsigned HImode parameter already arrives
zero-extended from the caller. Written as `s16`, the value is treated as
sign-extended and agbcc is forced to clear the upper half. **So those four bytes
are proof that the parameter was signed in the original source.**

**`InitSaveSystem`** — two agents independently found 240/240. The tail section
held only when three spelling choices were applied *together*; none is sufficient
alone (rules 16, 17, 18).

One of the agents also demonstrated with byte evidence that my comment in the
source file was stale: the first cluster, which I had marked as an "unsolvable
loop", was in fact already matching — the first differing byte was *after* the
loop. Rule 8 was thereby confirmed by measurement.

### A tool bug fixed

`diff_function.py` was truncating the ROM side using the size in `functions.csv`.
That size is Ghidra's body estimate and can leave the literal pool out; as a
result, spurious "ROM da YOK" lines appeared **even for a fully matching
function**. This had misled me too. Now the larger of the two sides is used.

### The rule set grew to 19

Five new rules (15-19) and one important meta-rule: **the rules are
interdependent.** Rule 18 was ineffective on its own but became decisive once 16
and 17 were applied. A "tried, did not hold" record must not be evaluated in
isolation.

### Process note

While the agents were working I ran `rm -rf build` and deleted four variant files
under `build/variants/`. One agent had noticed and left a backup; the
`InitSaveSystem` solution was recovered from there, and the `WriteU16LE` solution
was rewritten from the agent's report text. In parallel work, shared directories
must not be touched.

## 2026-09-03 (night, 2nd workflow) — GameInit matched: portable assembly is done

The last block. Six agents, six different angles. **Nine independently reached 0
differences** (some agents produced more than one solution) — strong
cross-validation. The most explanatory one was chosen: written from scratch from
the ROM, 39% comment ratio, 768/768 bytes.

### Solving the real knot

The point I had been stuck on all session was this: the ROM uses a 16-byte stack
frame with slots 4 bytes apart, yet writes a **halfword** to sp+4 and sp+8. A
`u16` scalar dropped the frame to 12 bytes (673 differences); `u32` made the
write a word (264).

The answer: making the slots a **`u16 x[2]` array**. Because an array is BLKmode,
agbcc places it in declaration order and aligned to 4; `x[0] = 0` still generates
`strh`, and the `(u32)x` address comes out in a single instruction. The word slot
must be an array too — left as a scalar, it is placed after the arrays and loses
`sp+0`.

Tried and did not hold: `struct{u16 h; u16 pad;}` (agbcc treats it as SImode and
generates `ldr`/`and`/`str`), a single large struct, a union, and casts.

### Three new rules (20-22)

Plus a measured mechanism explanation: **what determines stack layout is not
declaration order but whether the type is BLKmode.** One agent tried 24
declaration-order permutations and showed that the layout never changed — which
supplies the *reason* for the observation I had previously recorded as
"declaration order has no effect".

### Final status

```
make matching        21/21 + libc 9/9
C source             40 functions, 40 byte-matching
matching bytes from C  4332/4660  (92.96%)
remaining assembly   agb_main.s (52 B) + intr_main.s (276 B) = 328 B
```

**The remaining 7.04% is exactly those two files** (4660 - 4332 = 328). So
everything portable has been moved: what remains is only the ARM-mode startup
code and the IRQ dispatcher, which were assembly in the original source too and
will stay that way.

19/19 C files pass the readability check.

---

## 2026-09-03 — Map revision, harvest, 32 rules

### The map: 1,466 → 1,978 functions (+35%)

Three pieces of work together rebuilt the function map from scratch:

1. **Boundary audit** (`audit_boundaries.py`): 647 boundaries were corrected with
   recursive descent, and 43 false records deleted. The first approach (linear
   disassembly) was "growing" 998 functions to 8 KB — caught in report-only mode.
2. **Missing function discovery** (`discover_functions.py`, three methods):
   `bl` call targets (certain, 51+49+3), function pointers in ROM data (46 with a
   prologue requirement; the raw scan gave 1,538 candidates, most of them
   coincidences in graphics data), and prologue patterns (358). Repeated until it
   converged.
3. **Tail call splitting** (`split_at_calls.py`): the walker was treating an
   unconditional `b` as intra-function flow, but GCC also uses it for tail calls.
   52 `bl` targets fell INSIDE known records — 32 records were split, separating
   54 functions. Discovery yields 0 afterwards.

**The metric was revised downward three times** (denominator 291K → 338K → 414K →
433K; the ratio's appearance 2.34% → 2.28%). Coverage never fell; the denominator
moved closer to reality. The lesson: the mapping work should have finished BEFORE
the coverage work.

### Harvest: 7,268 → 11,264 verified ROM bytes

About 30 new regions, mostly leaf clusters of 2-6 functions. Reusing the proven
idioms (the overflow-guarded counter, the doubly linked list, the DMA block, tile
pointer arithmetic, and the 148/180-byte table entries) gave an exact match on the
FIRST attempt in most clusters.

### Rules: 27 → 32

- 28: pointer arithmetic != array indexing (scaling order)
- 29: two identical branches → early return + shared tail (blocking cross-jump)
- 30: sparse cases → an `||` chain (the jump table disaster: 212 bytes instead of
  64)
- 31: the loop counter's signedness determines the `bls`/`ble` choice
- 32: consecutive word copies via struct assignment (the `ldmia/stmia` trigger)
- Register allocation priority was documented as a mechanism section
  (`priority = floor_log2(refs) × refs / lifetime`).

### The parked corpus: 16 files

Seven are within ≤5 bytes (ClearTextArea 1, MaybeAdvance 1, QueryEntity 2,
ProcessFirstEntry 2, GetInnerId 4, IsRamModeWanted 4, ProbeObject 5). The
obstacle classes were defined: register allocation order, branch direction
normalization, base copying / two bases, pool placement, the -O0 class, and
product factorization. These form Phase 2's test corpus.

### Other

- The BIOS surface is closed: the game's entire `swi` contact is 10 thunks, all
  matched.
- libc: `findslot`/`remap_handle` were identified by masked matching (not counted
  as regions because they are not byte-exact); `identify_libc_at.py`.
- Dashboard: stale JSON (make check now refreshes it), palette tone separation,
  and the panel now shows our source rather than Ghidra's.
- Renaming broke another file SEVEN times → a rename tool is needed.

## 2026-09-04 — ProcessFirstEntry byte match

- The last two-byte difference in `src/world/scan_all.c` was closed. Expressing
  the pointer computation in the order `(u32)i * sizeof(Entry) + (u32)tbl` made
  old_agbcc produce the ROM's `adds r1, r0, r5` encoding.
- The function matches 62/62 bytes; together with the two bytes of alignment
  padding, the range `0x08029014–0x08029054` was added to the permanent matching
  chain.
- The `scan_all` obstacle in the parked corpus is closed; this expression order is
  a reusable candidate for similar register-allocation differences.

## 2026-09-04 — MaybeAdvance semantic correction

- The single-byte `bls`/`bhi` difference was proven not to be a compiler
  preference: the previous C and its comment were reading the ROM's branch target
  backwards.
- The real behavior: 0 only when `gVBlankEnabled == 2` and the counter is `> 1`;
  1 in every other case.
- The corrected natural C matched 42/42 bytes. With two bytes of alignment, the
  region `0x080664F0–0x0806651C` was added to the permanent matching chain.

## 2026-09-04 — QueryEntity byte match

- Although both operands of the `flags & 3` expression give the same value, agbcc
  was holding the result in a different register from the ROM.
- The form `mask = 3; mask &= flags` kept the result in the constant's register
  and closed the remaining two opcode bytes. The function matches 60/60 bytes.
- `0x08055AF8–0x08055B34` was added to the permanent matching chain; the compiler
  behavior was recorded as `COMPILER.md` rule 33.

## 2026-09-04 — The object query pair byte-matched

- `ProbeObject` (58/58) and `GetInnerId` (20/20) fell into the ROM's block order
  once the nested null checks were turned into explicit early `return 0` checks.
- Because another verified function sits between them, the file was split into two
  real ROM regions: `object_query.c` and `get_inner_id.c`.
- `0x080381F8–0x08038234` and `0x0803824C–0x08038260` were added to the permanent
  matching chain; the pattern was recorded as `COMPILER.md` rule 34.

## 2026-09-04 — IsRamModeWanted byte match

- The nested first condition was turned into an explicit `if (!active) return 0;`.
  That placed the shared zero block before the literal pool and closed the
  remaining four-byte control-flow difference.
- The function matched 28/28 bytes and `0x08062530–0x0806254C` was added to the
  permanent matching chain. Rule 34 was thereby confirmed in a third function.

## 2026-09-04 — CallWithOffset signature correction

- The ROM's epilogue was clobbering the call result's r0 with `pop {r0}`; that
  evidence showed the wrapper's previous `u32` signature was wrong.
- With the return type changed to `void`, the function matched 24/24 bytes,
  including register allocation and the epilogue. `0x080509C4–0x080509DC` was
  added to the permanent matching chain; the inference was recorded as
  `COMPILER.md` rule 35.

## 2026-09-04 — InitActor byte match

- Reusing the loop counter in the final tail zero writes reduced the difference
  from 14 bytes to a single moved instruction.
- Explicitly computing the address `tail = &actor->unk90` before the zero
  assignment produced the ROM's instruction order; the function matched 192/192
  bytes.
- `0x080154D8–0x08015598` was added to the permanent matching chain and the
  pattern recorded as `COMPILER.md` rule 36.

## 2026-09-04 — HasWantedEntry byte match

- Expressing the `base`, `kind`, and `cur` pointers as separate lifetimes restored
  the ROM's r0/r1/r2 register allocation.
- Once the missing copy instruction returned, the literal pool and the loop target
  also fell into the right positions; the function matched 48/48 bytes.
- `0x08028E3C–0x08028E6C` was added to the permanent matching chain; the pattern
  was recorded as `COMPILER.md` rule 37.

## 2026-09-04 — PushHistory byte match

- The global base, the `current` value read, and the write base were split into
  separate lifetimes before the comparison.
- This form prevented agbcc from merging the equality path with the final store;
  together with the register allocation and the loop target, the function matched
  48/48.
- `0x08008064–0x08008094` was added to the permanent matching chain; the pattern
  was recorded as `COMPILER.md` rule 38.

## 2026-09-04 — AddDistance byte match

- Taking the `gDistanceAccum` address into a separate pointer produced the ROM's
  early r4 base load and its single-base usage.
- Giving only the `distance` read in the final comparison a narrow volatile view
  preserved the ROM's post-store `ldrh` re-read.
- The function matched 68/68 bytes; `0x08067274–0x080672B8` was added to the
  permanent matching chain and the pattern recorded as `COMPILER.md` rule 39.

## 2026-09-04 — BumpOrReset byte match

- Instead of a structured `if/else`, the ROM's three blocks were expressed
  explicitly with the C labels `reset`, `increment`, and a shared `store`.
- agbcc then loaded a separate counter address in the two branches and shared a
  single `strb`; the function matched 56/56 bytes.
- `0x0805AC50–0x0805AC88` was added to the permanent matching chain and the
  pattern recorded as `COMPILER.md` rule 40.

## 2026-09-04 — ReleaseSlot byte match

- Struct indexing was removed and the `heldBase` and `extraBase` field bases were
  set up in separate locals from a `scaled` offset computed once.
- Reducing the multiplication from two statements to a single
  `scaled = index * 180` assignment fixed the last r3/r4 swap; the function
  matched 64/64 bytes.
- `0x080308AC–0x080308EC` was added to the permanent matching chain and the
  pattern recorded as `COMPILER.md` rule 41.

## 2026-09-05 — The sprite pool: five additional C matches

| Function | Address | Matching function bytes |
|---|---|---:|
| SortSpriteList | 0x08012A00 | 152 |
| InitSpritePool | 0x08012B20 | 124 |
| FlushSpriteList | 0x08012B9C | 112 |
| SortActiveSprites | 0x08012C54 | 32 |
| InsertSpriteSorted | 0x08012C74 | 98 |

- `FlushSpriteList`'s 9-byte timing difference was closed by the forward-counting
  `for (i = count; i < left; i++)`. The decreasing counter agbcc generates places
  the subtraction after the constant setups. In `InitSpritePool`, the
  `i++, node++` order also closed the remaining 4-byte difference. Both
  experiments were recorded as `COMPILER.md` rules 42–43.
- The same natural C body for sorted insertion reproduces the ROM exactly both as
  a standalone function and when inlined into the `SortSpriteList` loop. The
  shared body lives in `include/sprite_sort.h`. A labelled loop with separately
  masked locals left a 30/98 difference; a structured `for` with direct masks
  together produced the match.
- `include/sprite_pool.h` collects the 128 × 16-byte nodes and the three pool
  fields into one type. The `+6` byte is the sort's secondary key; `+7` is not yet
  known. The initializer zeroes OAM's 1024 bytes with DMA3.
- `AllocNode` (72 bytes), which had already matched in the previous session, was
  not in the recorded ROM regions. It too was taken into the permanent build
  chain. Six new regions total **592 bytes**: the 518 function bytes matched this
  session, 2 bytes of alignment, and `AllocNode`'s 72 bytes. The new contiguous
  sprite range is `0x08012B20–0x08012CD8` (end exclusive), 440 bytes.
- `UnlinkToFree` was moved to the same shared type; its 53/150 difference did not
  change. Splitting the two branches' `prev` local had no effect, and using a
  separate `next` for the free list came out worse. The function was not counted
  as matching.
- The number of functions matching from C went 279 → 284; recorded matching code
  went 13,848 → 14,366 bytes (3.05% → 3.16%). No function boundaries were
  changed; the sources and names of existing records were opened up without
  adding new entries or boundaries.
- Verification: `make check-full` passed. The cacheless matching build, the
  23-function toolchain corpus, the boundary audit, the dashboard lint, and the
  production build are all clean. The hybrid ROM's SHA-1 did not change:
  `06230842626da504f92396074f7c655e100f5d44`.

## 2026-09-05 — Runtime tracing, the language system, ARM mode

A long session with three separate work streams. Result: 298 → 356 functions,
3.21% → 4.27%.

### 1. The mGBA tracer and game sessions
`tools/make_trace_script.py` -> `tools/trace.lua`: reads 322 RAM symbols every
frame and logs the changes along with the key state. The user played eight
sessions (startup, driving, standing still, damage/death, the language screen, a
mission, arrest). Analysis: `tools/analyze_trace.py --list / --session N`.

Firm conclusions (all `verified` in ram_map.csv):
- `gSessionPtr` -> `gRam02000F10`; its `+0x10` field is health in 16.16 form
  (100.0 on a new game, -4.0 per punch, 0 on death). `player_health` is its copy.
- `gLanguage`: 0=English, 1=Spanish, 2=French, 3=Italian, 4=German. `SetLanguage`
  is called on cursor movement (live preview).
- `gRam02030330` = the wanted/police block, with its field map extracted;
  `wanted_level_true` is its +0x10 field (not an overlap — it was a recording
  error).
- `mission_timer` really is a timer — my "distance" claim was REFUTED.
- The text table: 618 strings x 5 languages, slice k = language k, encoding
  LATIN-1.

Two defects in the tool were found and fixed: startup validation (with `emu` not
ready, the whole list was being emptied) and double loading (two instances were
writing to the same log; an incrementing-counter guard was added).

### 2. Harvest
Two workflow rounds (10 + 13 agents): 21 matches. Solo: 12 matches. During the
merge, four problems invisible in the agent reports were caught: an
`asm(".equ")` workaround, a type conflict (solved with the shared header
`node_list.h`), a bare address, and a status drift. `tools/rename_symbol.py` was
written (naming had broken externs four times in one session).

Measurement: the parked "hard class" is 14 functions / 3708 bytes / 0.82% — NOT
the bottleneck. The 220 functions in the 512+ band hold 54% of the ROM; the
percentage will come from there.

### 3. ARM mode
The build chain was tied to Thumb only; `agbcc_arm` had been sitting idle from the
start. It was wired in (the `KIP: ARM` marker, `.align 4`). A flag discovery:
`-fomit-frame-pointer -fno-schedule-insns -fno-schedule-insns2` took the first
candidate from 244 to 188 bytes. Barrel-shifter fusions are generated from C
(measured), so the region is reachable.

A STRUCTURAL LIMIT: agbcc_arm always pushes 8 registers ({r4-r9,sl,lr}) and never
brings fp/ip into allocation; the ROM pushes 11. Nine flags were tried, none
widened the set. First candidate 0x0806A77C: 171/196, parked. The ARM region
(14,920 bytes) appears closed to matching with this configuration.

### Dashboard
Unit (source file) grouping was added; unknown regions are shown with their
address range. Inferring the module from adjacency was tried and RETRACTED: in a
decomp, the module is not guessed — it is the file that was decompiled.

## 2026-09-06 — The sibling function band closed

- For the first time, the roadmap's 84 targets were tied to a fixed, reproducible
  data view: `data/sibling_band.csv`. `make sibling-check` re-selects the targets
  from the `9cbaf27` baseline using the rules 120–560 bytes, at most a 200-byte
  gap, and a Thumb game module. The real total is 19,794 bytes; the old "1.6 KB"
  and "18,818 bytes" figures did not produce the same list and were corrected.
- 9 functions / 1,262 function bytes matched the ROM exactly: buffer setup,
  palette lerp, nibble replacement, object linking, zeroing the runtime globals,
  area transition position, two frame-dispatch chains, and a four-way position
  test.
- Six additional functions were converted to clean C, measured against the ROM,
  and parked with in-file evidence. The closest is `FUN_080515d0`: 164/164 in
  size with only a 20-byte difference; the remaining obstacle is the load order of
  two global literals. The other targets were statically parked with exact ROM
  call/branch/literal counts and a dependency class.
- The matched regions were added to the hybrid ROM chain. The overall matching
  measurement rose from 419/33,554 bytes to 428/34,816 bytes (7.67%).
- The nine new matches were bound to real names without increasing the naming debt
  (`InitWorkBuffers`, `BlendPaletteBlock`, `ReplaceNibbleField`, `LinkObjectPair`,
  `ResetRuntimeGlobals`, `AdjustAreaPosition`, `ProbeNearbyPosition`,
  `RunFrameStageOne/Two`). The number of matching functions carrying a placeholder
  `FUN_` name fell from 109 to 100.
- The first version of `make sibling-check` compared targets against the
  **names** in the baseline, and it broke as soon as the naming was done. Because
  what needs to be frozen is the target set, the comparison was reduced to address
  + size; the name column is current information, not identity.


## 2026-09-07 — The SIO TX register conflict resolved (MATCH-018)

- `FUN_080657d8` started at 575/1174 instructions, 2356 bytes; the retained source
  is at 669/1174 instructions, 2352 bytes. **There is no full byte match**; the
  505-instruction difference is open under MATCH-019. The matching percentage did
  not increase with this change.
- `PackLocalLinkTag` packs the tag of a record from two parallel rings. Advancing
  the ring pointer in two steps prevented the local allocator from keeping a base
  constant in r4 throughout TX. The allocation k=r4, tx2=r5, cur=r6, and key ring
  base=r7 is aligned with the ROM. The 0x08065834 triple at the entry also
  matched. No new external call or volatile access was introduced.
- The r4 symbol in the handover note was corrected: in the old build, r4 held
  `gRam020003C0`; `gRam02000E80` was in r3. Resolving the conflict did not close
  most of the remaining differences; that expectation is not presented as a
  confirmed result. The RX address relation and other block differences persist.
- `tools/probe_sio_tx.py` rebuilds three controlled candidates in a temporary
  directory: direct expressions 575, the one-step helper 573, and the retained
  two-step helper 669. The measurement is in COMPILER rule 64. A diagnostic
  candidate giving 677 with a pure base alias was not taken, because of the source
  acceptance criterion.
- `make c-match FILE=src/world/sio_driver.c` reports a short output as expected
  (2352/2374); the source was not marked as matching. `make check` passed:
  toolchain, recorded regions, hybrid ROM hash, C scan, consistency, work queue,
  boundary audit, and generated views are all clean. The stale 2372-byte
  measurement in `c_sources.csv` was also refreshed by the scan.

## 2026-09-07 — The game's real frame rate measured: ~15 fps

- The question was answered from the code, then confirmed by play.
  `vblank_intr.c` (0x08000220, byte-matching) increments `gIwramFrameCounter` on
  every VBlank; when a logic frame finishes, it copies the value to `gFrameDelay`
  (0x03000000), clamps it at 5, and zeroes the counter.
- `gFrameDelay` is a **time step multiplier**, a usage rather than a measurement:
  per the note in `nodelist_c8.c`, the counters at 0x08053AD8 decrease by
  `gFrameDelay` every frame.
- **Play measurement: ~15 fps**, i.e. four hardware frames per logic frame,
  `gFrameDelay ≈ 4`. Much worse in cutscenes (0-1 fps observed).
- Code evidence that independently confirms this: **`FRAME_DELAY_MAX = 5`**. In a
  game running at 60 fps the compensation value would never exceed 1; clamping at
  5 shows that values of 4-5 were considered normal operation. So the low frame
  rate is not a performance accident but a range the design accepted.
- A second consequence of the clamp: if a logic frame takes longer than 5 hardware
  frames, the compensation hits the ceiling and **the simulation goes into slow
  motion** (movement advances slower than real time). The "frozen" feel in
  cutscenes is probably this — not a frame rate drop but time slowing down.
- In link/two-player mode (`gGameState[12]` being 1 or 2), `gFrameDelay` is fixed
  at 5 REGARDLESS of the frames actually elapsed. The reason is not written in the
  code; the plausible guess is keeping the two devices deterministic, but that is
  an inference.
- `tools/fps_watch.lua` was added: it catches the moment the counter is zeroed in
  mGBA and prints the step distribution. The FPS in mGBA's title bar is the
  emulator's speed and does not show the game's logic step.

CORRECTION: before this finding I had said "the game does not lock to a fixed FPS,
it uses a variable time step." The mechanism was right, but I had assumed its
consequence; it gave the impression that the game ran close to 60 fps. The
measurement refuted that.
