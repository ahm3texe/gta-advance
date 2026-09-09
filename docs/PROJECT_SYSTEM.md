# Project working conventions

This document defines the project's process contract. Current figures live in
[STATUS.md](STATUS.md), active and queued work in `data/work_queue.csv`, function
techniques in [WORKFLOW.md](WORKFLOW.md), and compiler behavior in
[COMPILER.md](COMPILER.md). Do not maintain a second manual copy of the same facts.

## 1. Data ownership

| Information | Source of truth | Derived view |
|---|---|---|
| Function address, size, state, and module | `data/functions.csv` | Dashboard, `make status` |
| Reviewed name/state overrides | `data/function_overrides.csv` | `functions.csv` |
| C implementation and matching result | `src/**/*.c` + `data/c_sources.csv` | Dashboard |
| Verified source regions | `data/matching_regions.csv` | Hybrid ROM, dashboard |
| RAM/ROM data symbols | `data/ram_map.csv` | Linker scripts |
| Open technical work | `data/work_queue.csv` | `docs/STATUS.md`, dashboard |
| Accepted boundary debt | `data/boundary_baseline.json` | `make boundary-check` |
| Rejected false entries | `data/non_function_entries.csv` | `make consistency` |
| ARM overlay boundaries | `data/arm_boundary_review.csv` | `make consistency` |
| Toolchain identity | `config/toolchain.lock.json` | `make toolchain-check` |
| Current project summary | The data above | Generated `docs/STATUS.md` |

`README.md`, `PLAN.md`, and `WORKLOG.md` are not sources of current counters.
README is the entry point, PLAN records decisions/direction, and WORKLOG is history.

## 2. Session protocol

At the start:

1. Use `git status --short` to identify existing changes made by others.
2. Use `make status` to inspect the recorded state and the single active task.
3. Keep at most one task `in_progress` in `data/work_queue.csv`.
4. Its acceptance criteria define the session's scope; additional work gets a new record.

At the end:

1. Produce evidence; a comment or agent report alone is not proof.
2. Update the task state and `evidence` field when task progress changes.
3. Run `make status-update` when source or progress data needs refreshing.
4. For implementation changes, run `make check`; use `make check-full` for milestones. Documentation-only changes follow the focused checks below.
5. Do not claim `done`, `matching`, or completion before the applicable checks pass.

## 3. Validation levels

### Documentation-only changes

For prose translations, document renames, and corresponding path references,
check technical meaning, Markdown, links/anchors, old-name references, and the
scope of the diff. Confirm that executable code and measurement values remain
unchanged. Do not rebuild the game or dashboard solely for documentation changes.

Generated-document text must be changed in its producer. Check that producer's
syntax and generated output with the existing recorded data. A change to build
behavior, executable code, data schema, or matching results is outside this
exception and requires the implementation checks below.

### `make check` — implementation commits

- Checks installed toolchain artifact identities.
- Verifies all registered matching regions against the ROM.
- Checks source-region placement in the hybrid ROM.
- Checks CSV/source consistency and the work-queue schema.
- Audits boundaries against the baseline and rejects new or changed debt.
- Scans every C source; any compilation failure fails the check and preserves the old `c_sources.csv`.
- Checks for prohibited inline assembly and fixed-register bindings.
- Generates dashboard data and checks that `docs/STATUS.md` is up to date.

### `make check-full` — implementation milestones and merges

Reproduces everything in `make check` without using the build cache. Also checks
the fixed 23-function reference corpus fingerprint, dashboard lint, and the
production build.

### Boundary baseline policy

The baseline is not a declaration that everything is correct. The short-boundary
list was reduced to zero on 2026-09-04 by reviewing 53 records individually. A new
finding, changed reachable size, or reopened finding fails `make check`.
Review decisions are stored in `data/boundary_review.csv`.

Run `make boundary-baseline` only after reviewing every difference. Never refresh
the baseline merely to silence a counter.

## 4. Work queue

Every task needs a stable ID, P0–P3 priority, state, deliverable, measurable
acceptance criteria, and evidence when completed. Valid states are `todo`,
`in_progress`, `blocked`, and `done`.

Only one task may be `in_progress` at a time in the current queue. If another
problem is discovered, add a task rather than expanding the active objective.

- **P0:** a check protecting correctness or reproducibility.
- **P1:** work blocking the next technical step.
- **P2:** architectural/quality debt whose cost grows with scale.
- **P3:** cosmetic or optional improvements.

## 5. States and evidence

| State | Minimum evidence |
|---|---|
| `candidate` | An automatically mapped entry; no correctness claim |
| `discovered` | BL target, pointer, prologue, or split evidence for the entry; this does not mean the body has been reviewed |
| `documented` | Disassembly read, behavior/calls documented, name supported by evidence |
| `decompiled` | Readable, natural C exists but is not yet byte-matching |
| `matching` | A registered source region reproduces the ROM exactly in a clean build |

The reviewed metric is only `documented + decompiled + matching`. Function count
is a secondary metric; matching code bytes are the main progress metric.

Source kind is independent of state: `c`, `asm`, or `none`. Non-matching C is still
C; a candidate without source is not assembly.

## 6. Evidence standards

- Original C names and comments are not recovered from this ROM; do not invent unsupported names.
- Do not present partial or behavioral equivalence as byte matching.
- A hybrid ROM is not a complete source build: unknown bytes are copied from `baserom.gba`.
- Support a claim with numbers, hashes, disassembly, or a clean build.
- Record unsuccessful approaches at the source-file header or in WORKLOG.
- Correct stale comments and counters in the same relevant change.
- Do not force C matching with `register ... asm(...)` or inline assembly.
- An unresolved function must not hold up matching neighbors; isolate it in its own translation unit.

## 7. Classification strategy

The strategy originally covered 1,988 functions; that is a historical count.
Do not assign all names in advance. Classify main loop, input, UI, entity,
world/collision, rasterizer, mission/script, audio, and save clusters as the call
graph reveals them. Keep `FUN_...` when evidence is insufficient.

Prioritize adjacent leaf blocks, clusters with known calls, blocks extending a
continuous verified range, and large functions that reveal a subsystem.

## 8. Multiple agents and publication

- Each agent must know the active task ID; only one writer updates `data/*.csv` at a time.
- Do not delete the shared `build/` directory. Region/baseline decisions remain with the coordinating process.
- An agent report is not validation; the coordinating process measures the result against the ROM again.
- ROMs, saves, Ghidra projects, decompiler dumps, and generated ROMs stay out of Git.
- Remote repositories default to private; do not send work externally before the user selects a destination.
