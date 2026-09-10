# Roadmap and progress measurement

## Definition of success

The long-term technical goal is to produce a working, verifiable ROM from the
user's local `baserom.gba` input. "Complete" is not measured by the amount of
readable C alone: the recompiled code must be behaviorally correct and, ideally,
byte-matching against the original machine code.

## Phases

| Phase | Output | Measurement | State |
|---|---|---|---|
| 0. Foundation | Hash, Git protections, tool report | ROM verifies | Complete |
| 1. Mapping | ARM/Thumb function and data boundaries | Number of discovered functions | In progress; 1934 functions |
| 2. Skeleton | Linker script, assembly sources, recompilation | ROM size/layout | Complete |
| 3. Module analysis | Graphics, input, world, mission, audio, save subsystems | Documented functions | In progress; IRQ and save mapped |
| 4. Matching decomp | C/assembly sources and compiler flags | Matching/total functions and bytes | In progress; **428/1934 functions, 34,816/454,072 bytes (7.67%)** |
| 5. Verification | Automated ROM diff + mGBA tests | Hash/behavior tests | `make rom` places source regions into a hybrid image and verifies the SHA-1; a full source build and mGBA behavior tests are still missing |

## First working session

1. Prepare the local ROM and verify its hash with `make prepare-rom ...`.
2. Install Ghidra + Java 21, mGBA, and the devkitPro `gba-dev` tools.
3. Load into Ghidra as a raw binary: ARM little-endian, ARMv4T/ARM7TDMI, base
   address `0x08000000`.
4. Verify the `0x080000C0` entry point; extract the ARM/Thumb transitions and
   the initial call graph.
5. Add every verified function to `data/functions.csv`.
6. Pick the first small module; establish the assembly-output, C-equivalent, and
   diff loop.

## Measurements

- **Discovery coverage:** number of discovered functions. No percentage is given
  until an initial function map exists.
- **Documentation coverage:** `documented + decompiled + matching` / total
  discovered.
- **Decomp coverage:** `decompiled + matching` / total discovered.
- **Matching coverage:** `matching` / total discovered.
- **Byte coverage:** matching bytes / total code bytes, once function sizes are
  reliable. This is the primary metric, and more meaningful than function count.

Functions of unknown size are excluded from the byte metric, so that estimated
percentages are not presented as firm progress.

`data/matching_regions.csv` holds the real ROM ranges, including literal pools
and padding. `make matching` checks every generated fragment against the ROM;
`make progress` reports both the function-body metric and the unique ROM-region
metric.

## Where we stand (2026-09-06)

**7.67%** — 428/1934 functions, 34,816/454,072 bytes. The remaining work splits
into three bands, and their costs differ sharply:

| Band | Functions | Bytes | Share of ROM | Note |
|---|---|---|---|---|
| < 120 bytes | 756 | 44,858 | 9.9% | mostly stubs/wrappers, cheap but small |
| 120–560 bytes | 558 | 141,418 | 31.1% | the genuinely productive band |
| ≥ 560 bytes | 183 | 219,322 | **48.3%** | the real wall |

**The sibling band is closed.** The earlier text's "1.6 KB / 18,818 bytes" figure
could not be reproduced. The real selection behind that number is 120–560-byte
Thumb game functions within **200 bytes** of a matching range: **84 functions /
19,794 bytes** at the `393b82e` baseline. That fixed list and each target's
outcome live in `data/sibling_band.csv`; `make sibling-check` validates both the
selection and the results. 9 targets / 1,262 bytes became new byte-matching C;
the remaining 75 were parked with a measured C difference or with ROM
call/branch/literal triage.

**This pace will not hold.** Once the sibling band is finished, work moves to the
rest of the 120–560 band, where siblings are not adjacent and the per-function
cost rises.

## Next steps

**1. Finish the sibling band. — COMPLETE (2026-09-06).** All 84 targets ended in
either a match or an evidenced park; the data view was added to the CI gate.

**2. Take the input / link / statistics region.** 34 of the 54 functions between
0x08065000 and 0x08067400 are unwritten (~8.6 KB). This region is now a subsystem
whose **purpose is known**: input polling (KEYINPUT), serial communication
(SIOCNT), and statistics counters. Details in
[EXTERNAL_FINDINGS.md](EXTERNAL_FINDINGS.md).

**3. Find the script interpreter.** Missions are compiled bytecode in the ROM, so
the interpreter that executes them is most likely one of the functions in the
≥560-byte band. Finding it is the first concrete target that would open that
band.

**4. Clear the naming debt.** 100 matching functions still carry a `FUN_`
placeholder name, and 139 of 172 RAM symbols are `provisional`. Some are
deliberately unnamed (empty stubs, blind wrappers), but not all. This improves
documentation quality rather than the percentage, and `check_consistency` prints
the count on every run.

**5. Systematize the register-allocation class.** `dump_alloc.py --rom` now
answers "is this an allocation problem or not?" at a glance (COMPILER.md
rule 50). Three files were worked on blindly for days before that distinction
was available.

**What we will NOT do:** play the game in mGBA looking for matches. It was
measured — every blockage is a compiler pattern, not semantics. mGBA's place is
item 4: working out the meaning of RAM fields and statistics offsets.

## Tools

- **Ghidra:** static analysis, ARM/Thumb disassembly, decompiler, symbols, and
  call graph.
- **mGBA:** running the game, breakpoints/watchpoints, GDB remote, and behavior
  testing.
- **devkitARM (`gba-dev`):** ARM assembler/linker/objdump and a modern GBA build
  toolchain.
- **agbcc / old_agbcc:** byte-matching C compilation. The fingerprint was
  verified, and 5 of 6 functions in `src/save/save_helpers.c` were reproduced
  exactly from C; details in [COMPILER.md](COMPILER.md).
- **Git:** tracking every discovery, symbol name, and matching conversion
  reversibly.
- **Python tools:** ROM parsing, asset extraction, table generation, and
  automated diffing.

## Legal and distribution boundary

Only a legally obtained ROM may be used. The ROM, extracted art/audio/data
assets, and any producer material unsuitable for publication must not be
committed to the repository. The shareable project consists of original
analysis, rewritten code, tools, and scripts that extract data locally from the
user's own ROM.
