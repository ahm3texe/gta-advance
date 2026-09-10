# Current project status

This file is not edited by hand. `make status-update` generates it from
`data/*.csv`, the boundary baseline and the toolchain lock. For a live terminal
summary, run `make status`.

## Measurements

| Measurement | Value |
|---|---:|
| Function map | 1981 functions / 457032 bytes |
| Human review (`documented+`) | 859 / 1981 |
| Byte-matching | 795 functions / 64520 bytes (14.12%) |
| C sources | 836 total / 782 matching |
| ROM verified from source | 46084 bytes |
| libc verification | 448 bytes |
| Total verified ROM area | 46532 bytes |
| Open boundary debt | 0 short + 0 ARM review + 0 oversized |

## The single active task

**PHASE-001 — Phase 1: finish the twin and fresh-neighbor harvest completely**

## Open work queue

| ID | Priority | Status | Task | Acceptance criteria |
|---|---|---|---|---|
| BACKUP-001 | P1 | blocked | Create a private remote backup | The full commit history is pushed to a private remote chosen by the user |
| TOOL-010 | P1 | todo | Write a structural gate for new map entries | A candidate may be added to the map only if (a) it starts with a push prologue AND (b) it is preceded by a function-ending instruction (pop{..,pc} / bx lr / unconditional b); this test caught 57 of the 57 false entries produced by split_at_calls |
| MAP-010 | P2 | todo | Apply ONLY the 16 zero-risk ARM entries from the gap analysis | The ARM region was measured independently (the cond!=0xF ratio is exactly 1.0000); these 16 entries are added after passing the TOOL-010 gate. The 123 Thumb leaves without prologues are DELIBERATELY left out |
| ARM-001 | P1 | todo | Match the 18 functions in the ARM region with C | The ARM-mode build chain is working; each candidate is measured against the ROM, and if it matches its region is recorded |
| MATCH-019 | P1 | todo | Resolve the SIO driver's remaining instruction differences | FUN_080657d8 passes the make c-match gate with natural C; the RX address relation and the window/exit blocks are examined separately against the current ROM diff |
| PHASE-001 | P1 | in_progress | Phase 1: finish the twin and fresh-neighbor harvest completely | The fixed initial targets and the twins opened up by new matches are fully measured; at most four attempts per candidate; every result is either a full match or a park with source and measurement evidence; make check-full passes |

## Toolchain lock

- Compatible pret/agbcc revision: `da598c1d918402c42c0c0d7128ba14567f3175e9`
- Fixed representative C-corpus fingerprint: `9cd640a2a570228f958ec9cdd5574c4d267778f68938484158d2b4515bf76b3f`
- The ROM output is a hybrid integration test; unknown bytes are copied from the base ROM.
