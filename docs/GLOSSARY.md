# Glossary

Terminology used throughout the project. The documentation uses these terms
consistently.

## Workflow terms

**Park / parked**
Leave an unresolved function with its best available C implementation, matching
byte count, rejected approaches, and reasons for rejection documented at the top
of the file. This preserves the work without repeating the same dead ends months
later. The default guidance is one focused attempt after getting stuck, then park;
a task may define its own explicit attempt budget.

**Candidate**
A function selected for work that does not yet match.

**Harvest / matching pass**
A round in which candidates are selected and attempted in sequence. A single
harvest session covers several functions.

**Band**
A grouping by function size. The “60–127 band” contains functions of 60 through
127 bytes. Smaller bands generally offer easier, faster targets than larger ones.

## Matching terms

**Byte matching / byte-matching**
Compiled C reproduces exactly the original ROM bytes. This is the project's
matching criterion, not merely similar behavior.

**`differ: 97/120 bytes`**
The compiled output contains 120 bytes, 97 of which differ from the ROM. Zero
differences indicates a match only when the complete intended range is covered.

**`baserom.gba`**
The user's own ROM copy, kept outside version control and used for local comparison.

**Hybrid ROM**
The output of `make rom`: verified regions are built from project sources and
remaining bytes are copied from the base ROM. Matching the original SHA-1 verifies
the rebuilt bytes and their placement; it does not establish a full source build.

**SHA-1**
A file fingerprint used for ROM identity checks. A byte change normally changes
the digest. The expected ROM SHA-1 is
`06230842626da504f92396074f7c655e100f5d44`.

## Compiler and assembly terms

**agbcc**
The historical compiler family identified for this game. Modern compilers do not
reproduce the same instructions. This project uses a compatible locked revision,
with `old_agbcc` for matching Thumb code.

**Register**
A small, fast storage location in the CPU, such as r0–r7. Most Thumb instructions
operate on these eight low registers; high registers have more restricted uses.

**Register allocation**
The compiler's assignment of values to registers. Many mismatches arise here:
the C logic is correct, but the compiler chooses different registers.

**Spill**
Temporarily storing a value on the stack when it cannot remain in a register.
This adds memory accesses, so the compiler generally avoids it when possible.

**Literal pool**
A table of constants emitted near code, often at the end of a function. Constants
that cannot be encoded in instructions are loaded from it. Pools are **data**,
not instructions; disassembling them as code produces misleading results, a trap
encountered during this project.

**Prologue / epilogue**
Function-entry and function-exit sequences, often involving `push` and `pop`.

**Loop invariant**
A value unchanged across loop iterations. A compiler may compute it before the
loop and retain it in a register. Such transformations can affect matching.

**MMIO (memory-mapped I/O)**
Special memory addresses controlling hardware. For example, writing to
`0x04000208` enables/disables interrupt handling; it is not an ordinary variable.

## Tooling terms

**CFG (control-flow graph)**
A map of a function's basic blocks and branches. `tools/dump_cfg.py` produces it,
allowing the control-flow structure to be inspected before writing C.

**Basic block**
A straight-line instruction sequence with no internal branch; a branch ends it.

**Join point**
A block reached from multiple paths, such as after `if/else` or at a loop header.
Join points can strongly influence register allocation.

**Extern**
A declaration of a symbol defined elsewhere. An incorrect declaration, or one
left unchanged after a rename, can break linking or type consistency.

**Consistency checker**
`tools/check_consistency.py`, which detects issues such as conflicting types and
broken extern references and reports unsupported placeholder naming. It caught
three real mistakes in the session that originally introduced this glossary.

**`check-full`**
Full implementation validation: rebuild sources, construct the hybrid ROM,
compare its SHA-1, check consistency, verify the compiler corpus, and validate the
dashboard. Exit code 0 means all checks passed. Documentation-only changes use
the focused checks in [PROJECT_SYSTEM.md](PROJECT_SYSTEM.md).
