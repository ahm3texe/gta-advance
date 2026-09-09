# Technical roadmap and decision history

The numbers in this file are analysis snapshots dated 2026-09-03; they are not
live status or a work queue. Current, automated measurements live in
[STATUS.md](STATUS.md), the single active task in `data/work_queue.csv`, and the
binding process in [PROJECT_SYSTEM.md](PROJECT_SYSTEM.md). The long-term goal is
in [ROADMAP.md](ROADMAP.md), and measured compiler behavior in
[COMPILER.md](COMPILER.md).

## 2026-09-03 snapshot (historical)

```
Function map:         1,974 functions / 431,116 bytes of code
Byte-matching:        206 functions / 9,886 bytes   (2.29%)
Verified ROM:         11,288 bytes (80 regions) + 448 bytes libc = 11,736
Map gap:              ~35 KB (7.5%) — unclassified
Parked (written,
no match):            17 functions — the Phase 2 test corpus
Consistency:          `make consistency` CLEAN
make rom:             SHA-1 exact, verified on every commit
```

### Why the percentage dropped three times

| Stage | Denominator | Apparent ratio |
|---|---|---|
| Start | 291,535 | 2.34% |
| After boundary audit | 338,163 | 2.05% |
| After discovery | 413,699 | 2.38% |
| After tail-call splitting | 433,403 | 2.28% |
| After ghost/duplicate cleanup | 431,116 | 2.29% |

Coverage never fell; **the denominator moved closer to reality**. Ghidra had
either never seen or mis-bounded about 33% of the code. The lesson: the mapping
work must finish before the coverage work — which is why Phase 0 was expanded as
below.

---

## Phase 0 — Function map (ANALYSES COMPLETE, APPLICATION PARTIAL)

Done: boundary audit (647 corrections, 43 deletions) → three-method discovery
(`bl` target certain, function pointer strong, prologue pattern probable; +460
functions) → tail-call splitting (32 records → 54 functions). Discovery now
yields **0 new** functions, and the "call target landing inside a body" warning
is at **0**.

**Data integrity was also repaired** (audit findings): 2 duplicate records, 2
ghost records (no caller in the ROM + mid-body), 2 over-extended boundaries, a
size in `ram_map` exceeding EWRAM by 9.2 MB, and 3 symbols giving two names to
the same address. `tools/check_consistency.py` now catches all of these classes
in `make check`.

### Phase 0 analyses — all three measured

**1. Jump table scan — DONE.** The code region holds 101 `mov pc, rN`
instructions, 93 of which are real switch tables (2,393 entries, 734 unique
targets). For 89 of the 93 tables the dispatch instruction is inside a known
function; 4 are unowned. Table entries landing in no known function: 20.
**5 new function candidates** emerged, all three with the same profile: no `push`
prologue (leaf), and no caller anywhere in the ROM — they were structurally
invisible because all three of our discovery methods rest on exactly those
conditions.
*Note: they are certainly code, but the EXACT entry point is uncertain to ±2
bytes; each must be disassembled individually before being added.*

**2. The ARM region — MEASURED AND CORRECTED.** The `ARM_RANGES` constant was
wrong: it is not four separate ranges but **one contiguous region**,
`[0x08067E04, 0x0806B84C)` = **14,920 bytes**, 1.78 times the documented 8,384.
The old constant contained no false positives but was missing 6,524 bytes (44%).

Evidence (measured directly): in **all** 3,730 words of the region the condition
field is `!= 0xF`; random data or Thumb would be expected to show `0xF` in about
1 word in 16. Immediately before it the ratio is 0.9533 and after it 0.9747, so
the boundaries are sharp. The upper bound is the start of the BIOS `swi` thunks.

At `0x08CA4514` there is an **IWRAM overlay table** structured as triples
*(IWRAM destination, ARM start, ARM end)*, with consecutive entries chained (one
entry's end is the next one's start). This is independent evidence that the
region really is executed code.

**14 ARM functions** with an `stmfd sp!` prologue were measured and added to the
map (6,792 B). The remaining ~8.1 KB are ARM leaves that save no registers — the
same leaf problem as in Thumb.

**The contents are NOT an audio driver** (the hypothesis in the hint was wrong):
there is no m4a/sappy signature in the ROM, and not a single audio register
access in the region. The code is a **rasterizer**: affine texture mapping,
Cohen-Sutherland clipping, and 8bpp tiled framebuffer addressing.

**3. Classification of the 35 KB gap — DONE.** Of the 35,146 bytes:
- **55.1% (19,352 B) is code** — 200 new function entries (14,702 B) + 4,650 B of
  "code tail", i.e. the overflowing bodies of 29 functions whose sizes were
  recorded short.
- **42.3% (14,902 B)** is structural appendage to code: literal pools, jump
  tables, alignment padding.
- **only 2.5% (892 B)** is unclassifiable data.

All 7 gaps larger than 512 B (12,354 B) were resolved. **There is NO
graphics/text/data table** — not a single printable text block or piece of
graphics data appears in the gaps within `0x080000C0-0x08071E16`; the ROM's data
portion lies outside that range. This confirms that the assets are gathered in a
separate region.

### The work list produced by Phase 0 (awaiting application)

The following were **measured but not written into the map** — each needs
individual verification, since bulk insertion has already cost this project data
twice:

- 200 gap functions (61 Thumb with an agbcc prologue = low risk; 123 leaves
  without a prologue = medium risk; 16 ARM = negligible risk)
- the short-recorded sizes of 29 functions (4,650 B of tail)
- 5 jump table functions (entry point must be verified to ±2 bytes)
- ~8.1 KB of ARM leaves without prologues

Applying these would take the map to **~2,190 functions**. The current 1,988 is a
**lower bound**, and the code denominator (431,116 B) must be remeasured
accordingly.

---

## Phase 1 — Small function harvest (ONGOING, productive)

The pool (unmatched, game code):

| Size | Count | Bytes | Share of remainder |
|---|---|---|---|
| ≤64 | 625 | 19,552 | 4.6% |
| 65–256 | 716 | 94,284 | 22.3% |
| 257–512 | 223 | 79,931 | 18.9% |
| 513–1024 | 142 | 99,758 | 23.6% |
| 1025+ | 67 | 130,014 | 30.7% |

The method is proven: adjacent leaf clusters + a **library of proven idioms**.
In this session most of about 30 regions matched exactly on the first attempt.
Reused idioms: the overflow-guarded counter, the doubly linked list, the
IME-wrapped DMA block, tile pointer arithmetic, the 148/180-byte table entry, and
`ldmia` struct assignment.

Expectation: 60–70% of the ≤64 pool falls to the current method (~12–14 KB, i.e.
about 5.5–6% in total). What comes after depends on Phase 2.

---

## Phase 2 — Systemic obstacles (THE REAL BET)

Medium and large functions reach 85–95% instruction alignment and then get stuck
on the same handful of obstacles. The 16-file parked corpus exists precisely for
this — **the right solution opens the whole class, not just one file.**

### Test corpus (current differences, 2026-09-03)

| File | Difference | Obstacle class |
|---|---|---|
| clear_text_area | 1/132 | B1 register allocation |
| maybe_advance | 0/42 (matched) | Solved: the previous condition read the ROM semantics backwards |
| entity_query | 0/60 (matched) | Solved: a separate mask local + in-place `&=` |
| scan_all | 0/62 (matched) | Solved: integer addition with the product written first |
| is_ram_mode | 0/28 (matched) | Solved: explicit early return for the first condition |
| object_query:GetInnerId | 0/20 (matched) | Solved: explicit early return for the first null check |
| object_query:ProbeObject | 0/58 (matched) | Solved: explicit early returns for the null checks |
| offset_helpers | 0/24 (matched) | Solved: `void` instead of an incorrect `u32` return type |
| actor_init | 0/192 (matched) | Solved: zero from the counter + a `tail` local that sets up the address first |
| kind_scan | 0/48 (matched) | Solved: separate lifetimes for `base`, `kind`, `cur` |
| history_push | 0/48 (matched) | Solved: a separate `current` + a base copy before the comparison |
| distance_accum | 0/68 (matched) | Solved: an early `accum` pointer + a narrow volatile re-read |
| bump_or_reset | 0/56 (matched) | Solved: explicit `reset/increment/store` CFG labels |
| release_slot | 0/64 (matched) | Solved: a single product expression + separate field bases |
| area_cleanup | 56/84 | B1/B4 |
| entity_action | 140/210 | B5 block layout |
| menu_screen | 946/1456 | B1 (the whole allocation is shifted) |

### Obstacle classes

- **B1 Register allocation order.** The mechanism is documented
  (`priority = floor_log2(refs) × refs / lifetime`), but applying it by hand is
  trial and error. → `alloc_advisor.py`: given a diff, it should compute which
  variable's reference/lifetime balance needs to change.
- **B2 Branch direction / comparison normalization.** agbcc chooses the direction
  of some branches independently of the C expression. Five spellings produced the
  same output (maybe_advance). A systematic variant sweep is needed, and the
  criterion is NOT bytes but **instruction alignment** (the byte count is
  misleading because of branch offsets).
- **B3 Base copying / two bases.** The ROM loads a base and copies it, or
  computes `index*N` once and adds it to two separate constant bases. Rules 17
  and 22 were not enough; a new C form must be discovered.
- **B4 Pool placement.** The point at which the literal pool is embedded in the
  body changes the instruction flow, and it cannot be controlled directly from C.
- **B5 Block layout.** Placing the `return 0` block early or late
  (entity_action). An interaction between cross-jumping and branch cost.
- **B6 Disjoint classes.** The LibraryAssert cluster compiled at `-O0` (needs
  per-file flag support — a small addition to `agbcc_build.py`); and product
  factorization (`x*28`, tune_adjust).

### Work list (in order)

1. `tools/sweep_variants.py` — apply mechanical transformations automatically
   (flip a branch, split out an early return, u8↔s8, local↔inline,
   pointer↔array, declaration order), score them by **instruction alignment**,
   and run over the corpus. A generalization of the sweep scripts written by hand
   so far.
2. Per-file flags in `agbcc_build.py` → open up the LibraryAssert (-O0) cluster.
   Cheap, independent, ~200 B.
3. `tools/alloc_advisor.py` — the reference/lifetime computation for B1. Risky
   (the `-dl/-dg` dumps come out empty in this repository, so it must be measured
   indirectly).
4. Targeted research for B3: release_slot (47/48!) is the clearest example — one
   function, one pattern.

**Decision gate:** if at least 8 of the 16 corpus files do not close after these
four tasks, the Phase 3-4 expectation is lowered and the emphasis shifts to
behavioral verification (below).

---

## Phase 3 — Medium functions (65–512 B, 174 KB)

Depends on the Phase 2 output. With the sweep tool plus the idiom library,
**15–20% total coverage** can be targeted from here.

## Phase 4 — Large functions (513+ B, 230 KB, 54% of the remainder)

Realistic only if Phase 2 is fully solved. `menu_screen.c` (503/694 instructions)
is the indicator for the class: the structure is right, but the whole register
assignment is shifted by one.

---

## Horizontal work (parallel to the phases)

- **Behavioral verification (mGBA).** ROADMAP Phase 5. The first concrete step: a
  script that runs `out/gtaadvance.gba` for N frames under headless mGBA and
  compares frame hashes. It looks unimportant today because we are byte-exact,
  but if Phase 2 fails it becomes the only verification path for
  *semantically-correct-but-not-matching* C.
- **Asset pipeline.** 97.2% of the ROM is data (15.5 MB). It is not decompiled
  but extracted: the scripts live in the repository and the extracted assets stay
  local via `.gitignore` (the ROADMAP legal boundary). The formats are already
  being worked out from the code as it progresses; low priority.
- **Naming coverage + call graph.** 1,773 functions are still `FUN_xxxxxxxx`.
  It does not affect byte-matching; it is readability work. (The user deferred
  it.)
- **`tools/rename_symbol.py`.** Renaming broke another file SEVEN times in this
  session. A tool that updates every reference with one command.
- **Data hygiene.** `make check` now also refreshes the dashboard JSON; parked
  files carry `decompiled` status; region records are audited on every commit by
  `make matching`.

---

## Honest time estimate

At a harvest rate of ~2,500–3,000 bytes per session (this session's measurement):

| Scenario | Outcome |
|---|---|
| Phase 2 succeeds (≥8/16 corpus closes) | Medium functions open up; 15–20% within a few months, and what follows depends on the large ones. Full byte-matching: **12–18+ months** with regular work |
| Phase 2 partial (3–7/16) | A slowdown around 8–10%; the large functions stay closed |
| Phase 2 fails (<3/16) | A ceiling around 6–8%; the project shifts from "full byte-matching" to "byte-matching core + behaviorally-correct body" |

The source of the uncertainty is not the schedule but **whether B1–B5 can be
solved**. The decision gate will settle that early, within a few sessions.

## Next concrete step

1. The Phase 0 remainder (the jump table scan is the cheapest) — half a session
2. `sweep_variants.py` + a corpus run — Phase 2's first real test
3. Continue according to the decision gate
