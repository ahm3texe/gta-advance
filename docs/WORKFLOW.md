# Function matching workflow

The binding process contract is [PROJECT_SYSTEM.md](PROJECT_SYSTEM.md), current
state is [STATUS.md](STATUS.md), and compiler behavior is [COMPILER.md](COMPILER.md).
This document describes how to reverse engineer a single function or block.
PROJECT_SYSTEM prevails if the two documents conflict.

## 1. Verification commands

Before an implementation commit:

```sh
make check
```

This daily check covers matching sources, toolchain identity, data/source
consistency, the boundary baseline, every C source, the work queue, and the
generated status document. Use `make check-full` before an implementation
milestone to bypass the build cache and validate the corpus and dashboard.
Documentation-only changes use the focused checks in PROJECT_SYSTEM instead.

**Rule:** implementation commits require a passing `make check`. A hybrid ROM
hash proves only the placement of verified source regions, not that the entire
ROM has been rebuilt from source.

## 2. Target selection

```sh
python3 tools/find_leaf_candidates.py --limit=20 --max-size=200
```

Leaf functions (those without a `bl`) are the least expensive targets. However,
**one C file produces a contiguous ROM region**, so the unit of work is a *block
of adjacent candidates*, not just an isolated function.

Priority order:

1. Adjacent leaf blocks — the most predictable.
2. Functions whose callees are known — work upward through the call tree.
3. Blocks extending a continuous range — a better health indicator than a coverage percentage alone.
4. Large functions — these move the byte percentage most.

### Select candidates with tools, not ad hoc lists

Two read-only tools generate candidate lists and can write CSV output with `--out`:

- `tools/find_neighbour_dense.py` — functions with matching neighbors but no C source yet; nearby types are already understood.
- `tools/find_twins.py` — structurally identical function pairs/clusters (see section 10).

**Every reported count must include its generating command.** Do not select
candidates using one-off Python fragments: on 2026-09-07, a plan claimed “123 fresh
neighbor-dense candidates,” but nothing in the repository could reproduce it.
Counts are also snapshots: each new match increases its neighbors' counters, so
the candidate count can *increase*. Validate a count at the commit where it was
measured. For example, with `<commit>` replaced and the temporary directories prepared:

```sh
mkdir -p /tmp/at/tools
git archive <commit> data/functions.csv src | tar -x -C /tmp/at
cp tools/find_neighbour_dense.py /tmp/at/tools/ && cd /tmp/at
python3 tools/find_neighbour_dense.py
```

## 3. Function iteration

1. Read the ROM with `tools/disasm_function.py <name>` and **understand** the behavior.
2. Write clean C; do not copy Ghidra output.
3. Measure with `make c-match FILE=...`.
4. If it does not match, inspect differences with `make diff FILE=... FUNC=...`.
5. Change the **C**, not the assembly; consult [COMPILER.md](COMPILER.md).
6. Once matching, register the region, run `make check`, and commit.

## 4. Files and symbols

| Information | Location |
|---|---|
| Base types (`u8`, `s16`, `vu32`) | `include/gba_types.h` |
| Hardware registers, `DmaChannel`, memory bases | `include/gba_io.h` |
| RAM/ROM data symbols | `data/ram_map.csv` |
| Reviewed function name, state, and module overrides | `data/function_overrides.csv` |
| Verified source regions | `data/matching_regions.csv` (through tools) |
| libc regions | `data/libc_regions.csv` |

The existing convention keeps shared type definitions and `REG_...` definitions
in headers. Add new hardware registers to `gba_io.h`.

Use the data-update tools rather than manually rewriting `data/*.csv`:
`add_c_region.py`, `retire_asm.py`, `audit_boundaries.py`, and
`discover_functions.py`. The old linear scan in `split_at_calls.py` is permanently
disabled in write mode because it produced 52 false boundaries.

**The primary source is `data/functions.csv` itself.** `sync_function_map.py`
*rebuilds* the map from an outdated Ghidra export. An accidental run deleted
479 records, 148 names, and 133 `matching` states. A loss-prevention check now
rejects such changes; use the tool only for an intentional reconstruction.

### Renaming symbols

Renaming a function in `functions.csv` breaks **every source** still declaring
its previous name with `extern`. This happened seven times in this project and
was only detected at the end of `make rom` each time.

**Rule:** run `make consistency` after a symbol rename. It identifies broken
references immediately. Update all references before committing.

## 5. Evidence standards

These are correctness requirements. Each addresses a mistake already encountered
in this project.

- **Do not invent names.** If behavior cannot be established, retain `FUN_...`. The `.equ` labels in earlier assembly were hypotheses, and many were wrong.
- **Mark uncertainty.** When two symbols have identical bodies, do not guess which belongs where. Record `discovered` and note the alternative.
- **Provisional stays provisional.** Unverified `ram_map.csv` entries keep the `provisional` state.
- **Do not call a non-match a match.** Partial results are useful; unsupported success claims are not.
- **Record failed approaches** at the top of the source file. Repeating the same dead end is expensive.
- **Correct stale notes.** A comment saying “unresolved” after the issue has been solved is misinformation.

## 6. Fixed-register bindings are prohibited

Explicit bindings such as `register T *p asm("r4")` are in the same category as
inline assembly: they may reproduce bytes while hiding **why** the natural C
would generate those instructions.

Almost any register mismatch could be forced this way. Allowing it would remove
the purpose of discovering the compiler rules in `docs/COMPILER.md` and reduce
matching to an artificial exercise. There is also no evidence that the original
2004 source fixed these registers explicitly.

A C function counts as matching only when it matches using **natural C**.
`tools/review_c_source.py` detects prohibited bindings and fails `make check`.
The same restriction applies to inline assembly.

## 7. Isolate incomplete work

If one function in a block resists matching, move the matching portion into a
separate file and register its region. Keep the unresolved function in its own
file with attempted approaches documented in comments.

## 8. Negative results are evidence too

Document exhausted hypotheses. However, **a change that has no effect alone may
be decisive in combination with another**; this has happened in the project.
A failed experiment is not proof that the approach can never help.

## 9. Parallel work

When multiple agents are working:

- Do not disrupt shared directories such as `build/`; never run `rm -rf build`.
- Keep one writer for `data/*.csv`.
- Each agent edits only its assigned source files.
- Assembly retirement and region-registration decisions stay with the coordinating process.
- Agent reports are not verification; measure results against the ROM again.

## 10. Compare the sibling's ROM body first

If a function does not match and a **similarly sized sibling already matches**,
compare their ROM bodies before investigating register allocation:

```sh
python3 tools/disasm_function.py 0x08031844 | sed -E 's/^ *[0-9a-f]+:\t[0-9a-f ]+\t//' > a
python3 tools/disasm_function.py 0x08031A1C | sed -E 's/^ *[0-9a-f]+:\t[0-9a-f ]+\t//' > b
diff a b
```

Measured example: `0x08031844` (472 bytes) consumed 341,000 agent tokens and stalled
at 226/235 instructions. The report claimed a cyclic permutation of three
registers with no source-level way to change it. Its sibling `0x08031A1C` already
matched. Comparing the bodies showed **235/235 corresponding instructions**, with
only branch targets and the polarity of a column test differing (`blt` ↔ `bge`).
In C, the change was `if (col++ >= 0)` → `if (col++ < 0)`. Copying the matching
sibling and reversing that test produced a full match on the first attempt.

The same approach matched `0x080316B0` on its first attempt: the same family,
without column clipping. **If a sibling exists, diff it first; use `dump_alloc.py`
last.**
