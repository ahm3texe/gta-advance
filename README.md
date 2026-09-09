# GTA Advance

A work-in-progress matching decompilation of **Grand Theft Auto Advance** for the
Game Boy Advance, targeting the European release.

The goal is to reconstruct readable C source that compiles to the same machine
code as the original game. The project combines reverse engineering, documented
compiler behavior, and byte-level verification against a locally supplied ROM.

Maintained by **ahm3texe** and open to outside contributions.

[Project status](docs/STATUS.md) · [Getting started](#getting-started) ·
[Contributing](#contributing) · [Documentation](#documentation)

## Project status

Matching decompilation is in progress. Current measurements and active work are
recorded in the generated [status report](docs/STATUS.md) and
[work queue](data/work_queue.csv). Progress figures are maintained there so this
README does not become a second, outdated source of numbers.

Public progress reporting on **decomp.dev** is planned. The repository's local
reports remain the current way to track progress.

This project was originally written in Turkish and has since been translated
into English: source comments, tool messages, and research documents are now
English throughout. The commit history is preserved as it stands, so its
messages remain in Turkish.

## What this project reconstructs

The ROM contains compiled machine code and game data. This project reconstructs
C from that machine code and uses a compatible compiler to reproduce the same
bytes. It does not recover the developers' original C files, variable names,
or comments.

The deliverable is therefore behavior plus, where achievable, a byte-for-byte
match — not the developers' original files. Symbol names and comments in this
repository are interpretations derived from evidence, and they stay deliberately
uncertain until the ROM justifies something more specific.

## Target version

| Property | Value |
|---|---|
| Platform | Game Boy Advance |
| Region | Europe |
| Languages | English, French, German, Spanish, Italian |
| ROM title | `GTA ADVANCE` |
| Game code | `BGTP` |
| Revision | `0` |
| ROM size | 16 MiB |
| SHA-1 | `06230842626da504f92396074f7c655e100f5d44` |

Provide your own ROM as a local build input. ROM images, save files, extracted
game assets, and generated ROMs are excluded from version control. The project
does not provide ROM downloads or original developer source code.

## Getting started

The current tooling is developed around a **macOS / Clang** environment. Setup on
other hosts still needs validation, particularly the compiler identity checks.

### Requirements

For the core build and verification workflow:

- Git, Make, and Python **3.10 or newer**.
- Clang, Clang++, and a C preprocessor available as `cpp`.
- ARM GNU binutils on `PATH`, including `arm-none-eabi-as`, `arm-none-eabi-ar`,
  `arm-none-eabi-ld`, `arm-none-eabi-nm`, `arm-none-eabi-objcopy`, and
  `arm-none-eabi-objdump`.
- `shasum` for ROM verification, plus `unzip` if importing a ZIP archive.
- A ROM matching the SHA-1 above.

The core Python tools rely only on the standard library. Ghidra with Java 21 and
mGBA are additional analysis tools; they are not required to verify an existing
C match. Node.js and npm are needed for the remaining dashboard checks described
below.

### 1. Prepare the ROM

Run commands from the repository root. Place your ROM at `baserom.gba`, then check
its identity:

```sh
make verify-rom
```

Alternatively, import it from a local ZIP archive:

```sh
make prepare-rom ROM_ZIP="/path/to/Grand Theft Auto Advance (Europe).zip"
```

### 2. Build the matching compiler

```sh
make agbcc
```

This fetches and builds the agbcc revision recorded in
[the toolchain lock](config/toolchain.lock.json), installs it locally under
`tools/agbcc/`, and checks its output against a fixed set of reference functions.
The matching workflow uses `old_agbcc` for Thumb code and `agbcc_arm` for ARM code.

The fast toolchain check compares the installed binaries against the maintainer's
reference hashes. A build on another host can have different binary hashes even
when its generated code matches. If this happens, run `make toolchain-corpus` and
include both results in a setup report; a passing corpus check does not bypass
the current `make check` identity requirement.

### 3. Inspect the environment and verify the project

```sh
make doctor
make status
make check
```

`make doctor` reports the available build and analysis tools. Because it also
lists optional ones, a missing entry does not always block the core workflow.

`make check` validates the toolchain, registered matching regions, hybrid ROM
integration, C sources, metadata consistency, function boundaries, work queue,
and generated reports. These are local commands; automatic GitHub checks have
not been configured yet.

## Working on a function

Read the original instructions, compile the C implementation, and inspect any
differences:

```sh
make disasm FUNC=WriteU16LE
make c-match FILE=src/save/save_helpers.c
make diff FILE=src/save/save_helpers.c FUNC=WriteU16LE
```

These commands use an existing function as an example. For your own work,
substitute the source path and function name. The diff shows ROM instructions on
the left and compiled C instructions on the right.

Use readable C and verify changes against the ROM. Preserve uncertainty in symbol
names and comments until there is evidence for a more specific interpretation.
Inline assembly and fixed-register tricks are excluded from the C matching
workflow. Shared types and hardware definitions belong in `include/`.

Once a region matches from C, its assembly implementation can be retired from the
build. The ARM entry point, interrupt dispatcher, and BIOS syscall wrappers retain
their assembly implementations. Reference disassembly can be regenerated from the ROM.

See the [function workflow](docs/WORKFLOW.md) for the complete process and the
[compiler notes](docs/COMPILER.md) for observed matching patterns.

## Understanding progress

```sh
make status       # Summarize recorded progress and active work
make progress     # Show function, byte, and ROM-region measurements
make c-status     # Recompile C sources and refresh their matching results
```

`make status` and `make progress` read recorded data; they do not perform a fresh
build. `make c-status` updates [the C matching table](data/c_sources.csv).

| Measurement | Meaning |
|---|---|
| Function coverage | Counts of mapped functions at each research stage |
| Matching code bytes | Bytes belonging to functions marked as matching, relative to mapped function bytes |
| C matching | Functions whose compiled C output matches the ROM, tracked separately from assembly |
| Verified ROM regions | Registered address ranges, including the literal pools or padding they cover |

The function map is still being refined, so its totals can change. A percentage
of mapped code is not a percentage of the entire ROM. Assets, function bodies,
and literal pools have different roles in these measurements.

Function states are `candidate`, `discovered`, `documented`, `decompiled`, and
`matching`. They distinguish unverified candidates, verified entries and
boundaries, documented behavior, C implementations awaiting a match, and
byte-matching implementations.

The current [function map](data/functions.csv) is the primary record of function
names, boundaries, and states. Additional overrides are recorded in
[function overrides](data/function_overrides.csv).
Address ranges that have been rebuilt and verified against the ROM, including
the literal pools and padding they cover, are recorded in
[matching regions](data/matching_regions.csv).

### Hybrid ROM verification

```sh
make rom
```

This builds verified regions from project sources, copies the remaining bytes
from `baserom.gba`, and compares the resulting image's SHA-1 with the original.
It verifies that rebuilt regions integrate at their expected locations. The
output is a **hybrid integration test**, not a complete ROM rebuilt from source.
Generated images stay under `out/` and are ignored by Git.

## Contributing

Contributions are welcome in matching C implementations, reverse engineering,
compiler research, tooling, documentation, and setup testing.

1. Check the [work queue](data/work_queue.csv) and existing GitHub Issues. Discuss
   substantial work in an issue before starting so efforts do not overlap.
2. Fork the repository and make a focused change on a branch.
3. Include evidence for technical claims: affected ROM addresses, matching or
   diff results, and the commands needed to reproduce them.
4. Refresh generated status with `make status-update` when progress changes, and
   run `make check` before submitting. Include any failures or environment
   limitations in the PR description.
5. Submit a pull request for maintainer review. Use English for new descriptions,
   comments, and commit messages.

Keep ROMs, extracted assets, save files, Ghidra databases, and raw decompiler
exports out of contributions. Use the existing data-update tools when changing
the function map or registering matching regions. Coordinate updates to shared
metadata with the maintainer.

GitHub Issues are the place for reproducible problems and proposed work; pull
requests are the place for code and documentation changes. The current
[project rules](docs/PROJECT_SYSTEM.md) describe validation and data ownership.

## Documentation

Research logs and roadmap snapshots contain historical measurements; use the
status report for current figures.

| Document | Contents |
|---|---|
| [Status](docs/STATUS.md) | Generated measurements and active work |
| [Project rules](docs/PROJECT_SYSTEM.md) | Data ownership, validation, and working conventions |
| [Function workflow](docs/WORKFLOW.md) | Target selection, matching, and verification |
| [Compiler notes](docs/COMPILER.md) | Toolchain findings and matching patterns |
| [Roadmap](docs/ROADMAP.md) | Technical direction and milestone history |
| [Boot sequence](docs/BOOT_SEQUENCE.md) | Startup analysis |
| [Save system](docs/SAVE_SYSTEM.md) | EEPROM and save-system research |
| [Prior research](docs/PRIOR_ART.md) | Related work and reference material |
| [Work log](docs/WORKLOG.md) | Historical experiments and findings |

## Repository layout

| Path | Purpose |
|---|---|
| `src/` | Reconstructed C and assembly, organized by subsystem |
| `include/` | Shared types, memory layouts, and hardware definitions |
| `data/` | [Function map](data/functions.csv), RAM map, [matching regions](data/matching_regions.csv), [work queue](data/work_queue.csv), and evidence |
| `config/` | ROM identity, linker scripts, and toolchain lock |
| `tools/` | Build, analysis, matching, and validation utilities |
| `docs/` | Technical documentation and research history |
| `analysis/` | Tracked function-map export; local analysis databases are ignored |
| `dashboard/` | Existing local progress viewer, pending the decomp.dev transition |
| `build/`, `out/` | Local generated files, ignored by Git |

## Full validation and the local viewer

Before a merge or milestone, the project uses `make check-full`. It rebuilds with
Make's cache bypassed, checks the compiler reference corpus, and runs the existing
dashboard lint and production build.

Until the dashboard is retired, this requires **Node.js 22.13.0 or newer** and npm:

```sh
npm --prefix dashboard ci
make check-full
```

The existing viewer can still be opened with `make dashboard-dev`. Run
`make dashboard-watch` in another terminal to refresh its data as the CSV files
change. Dashboard retirement and decomp.dev integration are planned separately;
the local verification tools remain part of the project.

## License

The project's own work — reconstructed C and assembly, headers, tools, build
files, data tables and documentation — is released under the
[MIT License](LICENSE).

That covers only what this project wrote. It does not grant any right to the
game itself: the ROM, its code and its extracted assets remain the property of
their rights holders and are not distributed here. Supply your own ROM as a
local build input.
