# Open issues — handover document

This document was written so that someone from outside can take over the work I
could not finish. Each item comes with its measured state, root cause, and the
attempts that were eliminated. No guesses; every number is output from
`make c-match` and `tools/diff_function.py`.

Date: 2026-09-07 · Project state: **8.61%** (455/1934 functions, 39,100/454,072
bytes)

---

## 0. What you need to know

**Goal.** A byte-matching decompilation of the GTA Advance (Europe) GBA ROM. The
C that is written must, when compiled with
`old_agbcc -mthumb-interwork -O2 -fhex-asm`, produce the ROM's bytes **exactly**.
"Same behavior" is not enough.

**Measurement.**
```
make c-match FILE=<source.c>                      # the only real criterion
python3 tools/diff_function.py <source.c> <Name>  # side-by-side instruction diff against the ROM
python3 tools/disasm_function.py <address>        # the ROM itself
python3 tools/dump_cfg.py <Name>                  # basic block map
python3 tools/dump_alloc.py <source.c> --function <Name>   # agbcc register allocation
```

**The most important rule: do not look at the byte count, look at the instruction
sequence.** It was misleading twice in this session. In `ServiceLinkFrame`, a fix
that did not change the size at all produced the correct form; in `sio_driver`,
the size was within 2 bytes while half the body was wrong. The
`N/M instructions identical` line at the end of `diff_function.py` is the real
score.

**Rule library.** `docs/COMPILER.md`, 64 measured rules. The items below refer
frequently to rules 45–63. If you find a new rule, write it there **with its
measurement**.

**Prohibitions.** Do not use `asm(".equ ...")` — `agbcc_build.py` resolves symbols
from `data/functions.csv` and `data/ram_map.csv` and emits the `.equ` itself. If a
symbol is missing, record it in the data table. `%` and `/` generate a helper
call; do not use them if that `bl` does not exist in the ROM.

**Acceptance criterion.** Indefensible source is rejected even if it improves the
score: reusing an unrelated variable for a second purpose, `unsigned long long`
for a u16 counter, repeating a call with side effects, a `do{...}while(0)`
wrapper, or invented constant variables such as `bit1 = 2`. The permuter produced
three such candidates this session, and all three were rejected.

---

## 1. `FUN_080657d8` — the SIO driver · register conflict resolved, match still open

| | |
|---|---|
| address / size | 0x080657D8 · **2374 bytes** |
| source | `src/world/sio_driver.c` |
| current state | **669/1174 instructions identical**, 505 differ; C size 2352 |
| previous measurement | 575/1174 instructions identical; C size 2356 |

**Result after the 2026-09-07 handover.** The r4 conflict in the second half of TX
was removed. The new allocation matches the ROM: `k=r4`, `tx2=r5`, `cur=r6`, and
the key ring's base copy in `r7`. The hypothesis that this would close most of the
remaining differences was **not confirmed**: the gain was 94 instructions, and
there is still no full byte match.

**Correction to the old root-cause note.** In the initial build it was the address
`gRam020003C0` that held r4; `gRam02000E80` was in r3. The `.lreg/.greg`
measurement: p1012, L79, 6 references / 20-instruction lifetime → r4.

**The retained solution.** `PackLocalLinkTag` packs the bytes of the relevant
record from two parallel rings. The pointer is first set to the ring base and then
advanced to the record with `entry += index`. This makes the base two separate
short-lived pseudos (p1005 and p1032; 8 references / 8-instruction lifetime each →
r1). The helper inlined at two use sites generates no new BL. Writing it in one
step as `entry = base + index` drops back to 573/1174. The mechanism and its
measurement are in **COMPILER rule 64**.

A diagnostic candidate using only a base alias gave 677/1174; it was not taken
because it does not meet the source acceptance criterion. The retained `entry`
pointer advances to the real record and is used only to read that record's byte.

**Behavior.** `switch (gVBlankEnabled)` over IDLE, READY, LIVE, and SETTLING. LIVE
records the partner's previous and current windows into 32-entry rings; it walks
forward from the ACK and backward for a stable band; it sets up and sends the TX
record. If `mode == 2` it returns; if the local slot does not fill, it continues up
to a 240-frame limit.

**Retained earlier findings.** Making the TX/RX fields structure members fixed the
alias class (553→568). A separate `tx2` pointer for the second half of TX and
separate locals for the two windows brought 568→575; both are retained.

**Remaining differences.** The old stack copy at 0x08065834 now matches. The first
instruction difference is at 0x0806584C, where the inner branch's target address
differs because the epilogue moved. The RX address relation is `(block+constant)+i*16`
in the ROM and `(block+i*16)+constant` in the C. Other allocation/expression
differences persist in the window and exit blocks. The TX tag block's two base
loads are now separate, but against the ROM's `r3→r1` and `r0→r1` address
additions the C advances r1 in place. The block-match list taken from the old
linear diff is no longer current evidence.

**Re-eliminated.** On the new 669 baseline, a u16 read through the RX field address
gives 654; LinkSlot array/cast forms give 602. On the initial baseline, operand
reversals gave 567–569; tag locals at most 580; index locals at most 586.
Splitting `step` in two has no effect. The volatile sweep produced accesses that
do not exist in the ROM, so those candidates were not taken.

**2026-09-07 second round — the score did not change (669), three negative
results:**

1. **The first real difference is at 0x08065A52, and it is a CONSEQUENCE, not a
   cause.** There the ROM jumps into the body with `b.n 0x8065A6A`; the timeout
   test (0x08065A5C) is placed **before** the body. The reason is the Thumb
   conditional branch range (±256 bytes): the LIVE body is ~1232 bytes, and if the
   test came after the body the backward branch would be out of range. The layout
   is a consequence forced by the size of the body's contents — **this block will
   not align until the body is fixed.** The top-down method breaks here; the next
   round of work must target content differences independent of placement within
   the body.
2. **The outer loop spelling is not a lever.** Four spellings produced BYTE-FOR-BYTE
   identical output (2352 bytes, 669/1174): `do {...} while (cond)`,
   `for (;;) { ... if (!cond) break; }`, `while (1) { ... break; }`, and an
   inverted-condition `do/while`. agbcc reduces them all to the same internal form.
3. **The RX association does not close by rewriting.** For each field the ROM sets
   up a combined constant (0x190 + field offset) and computes `base + constant`,
   then adds `+ i*16` (measured at 0x08065A8E: `movs #207 / lsls #1 / adds /
   adds`). We produce `(base + i*16) + constant`. Four new spellings were
   eliminated: `FRAME(b)->rx` array decay → 669 (byte-for-byte identical),
   `(&rx[0])[i]` → 602, `rx[0 + i]` → 669 (identical), current → 669. Together
   with the previous round, **eight spellings** have been tried. The structure
   aliasing requirement conflicts with the ROM's association, and the structure
   form wins.

**Reproduction:** `python3 tools/probe_sio_tx.py`; the real gate is
`make c-match FILE=src/world/sio_driver.c`. The probe compares the earlier direct
spelling with the one-step and two-step helpers without modifying the source.

---

## 2. `FUN_08067014` — the percentage score

| | |
|---|---|
| address / size | 0x08067014 · 368 bytes |
| source | `src/world/link_score.c` |
| state | **85/183 instructions identical**, 98 differ |

**What it does.** It counts the thresholds in the link report and returns the
percentage `(100 * (head + count)) / (headLow + 23)`, clamped at 99, or 100
directly if the threshold has already been exceeded. The division must be written
as a `__divsi3` call.

**Remaining difference.** The ROM keeps the `report` pointer **in `ip` (r12)** and
copies it into a low register before each of the 15 accesses (`mov rN, ip`). We
keep it in a low register directly. The ROM also **re-reads** the +0x06 and +0x07
bytes at every use, whereas our build reads them once and caches them.

**Why it did not close (measured).** From `dump_alloc`: in the `lap` triple we keep
the `<<` intermediate results live and re-derive the `lsrs` — the same as the ROM.
But in the `rank` triple, CSE folds the whole `(x<<k)>>27` expression and reuses
the extracted value, and it also reduces the two sum tests to a single test. The
difference is the **container width**: `lap` is in a `u16` container, where HImode
conversions break CSE; `rank` is in a `u32` container. Because `rankB` spans bits
13–17, the container **must** be u32 (rule 61; the ROM does `ldr r0,[r1,#32]`).
With the low registers left free, `report` stays in r3, and the ROM's `ip` form
never arises.

**Eliminated.** Applying rule 55's local-copy branch to the `queryB` quadruple
**drops the score to 59/183** — the ROM re-reads the bytes there, so direct member
access is required (the inverse of rule 55 was confirmed). Removing the `(s32)`
casts in the sums changes nothing.

**Status: OPEN.** The container width requirement conflicts with CSE behavior;
there may be a source-side route, but it was not found.

---

## 3. `WaitForPartner` — the link waiting screen

| | |
|---|---|
| address / size | 0x0806620C · 320 bytes |
| source | `src/world/input_extra.c` |
| state | **134/139 instructions identical**, 5 differ (12 bytes) |

**Remaining difference.** Only the **order of the invariant hoists** in the loop
preheader:
```
0x08066238  ROM:  movs r6,#2 / ldr r7,=gBiosIrqFlags / movs r4,#1
            ours: movs r6,#2 / movs r4,#1 / ldr r7,=gBiosIrqFlags
0x080662C4  ROM:  ldr r7,=gVBlankEnabled / ldr r6,=gBiosIrqFlags
            ours: the reverse (the pool words are swapped too)
```

**Why it did not close (rule 58).** The hoist order follows the use order in the
source, but the two requirements **are mutually exclusive**:

| spelling | result |
|---|---|
| `if (armed == 0) {B} else {A}` | the ROM's block layout is correct, the hoist order is reversed → 12 bytes |
| `if (armed != 0) {A} else {B}` | the hoist order is correct, but the compiler lifts B above the loop and adds an entry jump → 316 bytes |

Manual hoisting (`bit1 = 2; irq = &gBiosIrqFlags;`) **fully closes the first loop**
(8/320) and proves that source-level initialization is emitted before `loop.c`'s
hoists — but in the second loop it frees a register, causes the constant to be
hoisted as well, and moves it into r8 (336/344). Besides, `bit1 = 2` is not
defensible source.

**Eliminated.** Splitting `armed` into two locals per rule 54 → **17 bytes** (both
loops want r5, so a single variable is correct). A nested-`if` exit condition → 12,
unchanged. Making `gBiosIrqFlags` volatile → **24 bytes**. Making `gVBlankEnabled`
volatile → 12, unchanged. The permuter went 80 → 20 in 1222 iterations; the best
candidate was rejected because it called `VBlankIntrWait` twice and used the `held`
variable for an unrelated purpose.

**Status: OPEN but narrow.** A five-instruction ordering issue. It closes if rule
58's exclusion can be worked around.

---

## 4. `PollInput` — key polling

| | |
|---|---|
| address / size | 0x080656F4 · 228 bytes |
| source | `src/world/poll_input.c` |
| state | **65/112 instructions identical** (4 bytes) |

**Remaining difference.** In the shoulder mask test the ROM copies **both operands**
into fresh registers:
```
ROM:  adds r1,r0,#0 / adds r0,r4,#0 / ands r0,r1
ours: ands r0,r4
```

**Why it did not close — proven at the RTL level.** From the `old_agbcc -da` dump:
after `combine`, the instruction is `(set (reg 37) (and (reg/v 22) (reg 36)))` and
reg 36 (the 0x300 constant) carries **`REG_DEAD`**. Because of that, `regmove`'s
`fixup_match_1` pass renames the destination over the dead constant and reduces
three instructions to one. The ROM's form can only arise if **neither operand
dies**, or if `reg_is_remote_constant_p` fires (the constant being set up in
another basic block). `keys` already does not die; the only remaining route would
be a second use of 0x300, and **no such use exists in the ROM**.

**Rule 59 does not apply here**, and this is that rule's limit: because 0x300 is
not in the low byte, a `u8` local truncates the mask — semantically wrong, and
measured at 216 bytes.

**Status: CLOSED.** There is no source-side lever, and this is proven. No more time
should be spent here.

---

## 5. `BuildLinkPacket` — packet construction

| | |
|---|---|
| address / size | 0x0806673C · 92 bytes |
| source | `src/world/link_packet.c` |
| state | **43/45 instructions identical**, 2 differ (20 bytes) |

**Remaining difference.** After the loop the ROM **keeps** the `gRam02036338`
address in r4 (`ldr r2,[r4,#0]`), whereas we reload it from the pool
(`ldr r0,[pc]; ldr r2,[r0,#0]`).

**Why it did not close — measured.** agbcc generates **two separate pseudos** for
the same address constant:
```
27  mem[.LC0]  refs 5  lifetime 46  priority 0.217  -> r4
74  mem[.LC0]  refs 2  lifetime  4  priority 0.500  -> r0
```
The two `.LC0` pseudos live in blocks **L0** (before the loop) and **L2** (after
the loop). CSE's extended basic block path **is cut at L1** because of the loop's
back edge; L2's only predecessor is L1, and L1 has two predecessors. **No source
expression can make L2 reuse L0's pseudo.**

The only known route is an address local (`CommBlock **slot = &gRam02036338;`) —
the invented-variable class, rejected.

**Eliminated.** s32/u32 counters, do-while/while/for, indexed access (a u32 counter
brought 29 → 20 bytes, the rest had no effect); moving the tail block into a local;
changing the order of the two tail assignments (worsened to 36 bytes); making
`total` a u32; capturing a `LinkPacket *pkt` before the loop and using it in the
tail (**32 bytes** — the ROM re-reads `ldr r1,[r2,#28]` after the loop).

**Status: CLOSED.** The absence of a lever is explained by the internal compiler
mechanism.

---

## 6. `ReceiveLinkPackets` — packet reception

| | |
|---|---|
| address / size | 0x08066798 · 212 bytes |
| source | `src/world/link_receive.c` |
| state | **58/104 instructions identical** (8 bytes) |

**Remaining difference.** The ROM uses **three** high registers (r8 = the global's
address, r9 = destination, sl = the constant −13), we use **two**. The missing 8
bytes are exactly that third register's push/pop pair. The ROM's register file is
exactly a rotation of ours:
```
ROM: r4=payload  r5=slot  r6=i  r7=i+1  r8=addr  r9=dest  sl=-13
```

**Why it did not close — measured.** Our `slot` allocno (p30, refs 6, lifetime 14,
priority 0.857) **never crosses a call**, so `find_reg` gives it the lowest free
register (r1). In the ROM, `slot` is in callee-saved r5. Every spelling that keeps
`slot` live until the second `CpuSet` **is broken by GCSE** — it lifts the common
`slot + 4` expression out of the `if` into r4 and kills `slot` there.

**A trap (recorded in the file).** Writing the last two statements through `block->`
**does** produce the ROM's `push {r5,r6,r7}`, but the size drops to 200 because the
ROM re-reads the global's address a third time there. So `gRam02036338->` is the
correct form; that is not the pressure being sought.

**Eliminated.** Making the counters u32 fixed the inner loop (29 → 8 bytes). Writing
the payload as a variable/expression, or taking it into a local inside the `if` —
no effect. The permuter went 1095 → 195 in 1519 iterations; the best candidate was
rejected because it used `total` both as the outer loop bound and as the inner sum
(**semantically wrong**).

**Status: OPEN but narrow.** Reduced to a single allocation fact.

---

## 7. `ServiceLinkFrame` — SIO frame service

| | |
|---|---|
| address / size | 0x0806686C · 152 bytes |
| source | `src/world/link_service.c` |
| state | **51/75 instructions identical** (8 bytes) |

**Remaining difference.** The ROM keeps the global's address in **r4**
(callee-saved), we keep it in r3. The missing 8 bytes are that push/pop pair and
its alignment.

**The real fix made this round.** Writing the BIOS flag through
`*(volatile u16 *)&gBiosIrqFlags` **does not change the byte count** but makes the
`else` branch instruction-for-instruction identical to the ROM: the non-volatile
form loads the constant before memory (`mov r0,#0x80 / ldrh r3,[r1] / orr`), while
the volatile form gives the ROM's order (`ldrh r0,[r2] / mov r1,#128 / orrs`) and
also moves `REG_IME` into r3 as the ROM does. **The previous note said "no change" —
that judgement looked only at the byte count and was wrong.**

**Eliminated.** Interleaving the two buffer swaps → 19/152; doing `ready = 0` before
the swaps → 39/152. Both **reach** 152 bytes, but they move the instructions away
from the ROM and shift `block` from r2 to r3, so they cannot converge. Not taken.

**Status: OPEN but narrow.**

---

## 8. Structural debts

**Naming.** 100 matching functions still carry a `FUN_` placeholder name.
`tools/check_consistency.py` prints the count on every run.
`tools/rename_symbol.py` changes the name in the tables, sources, headers, and
documents in one operation — do not use sed by hand, it broke the build three
times.

**RAM map.** Most of the 231 symbols are still `provisional`. Most sizes are
estimates. The measured ones are those derived from `Memset`/`CpuSet` calls; their
notes identify them as measured. Before correcting a symbol's size, look at the ROM's literal
pool: is a base + offset being loaded, or an absolute address? That distinction
misled me on `gRam02025810` (I thought it was 8 bytes; it turned out to be at least
0x137E).

**The ARM region.** 14,920 bytes. `agbcc_arm` is stuck at 8 registers, so it is
closed with this configuration. It should not be retried.

---

## 9. Where to look (suggestion)

The distribution of the remaining work:

| band | functions | bytes | share of ROM |
|---|---|---|---|
| < 120 bytes | ~740 | ~44,000 | 9.7% |
| 120–560 bytes | ~550 | ~142,000 | 31.3% |
| ≥ 560 bytes | ~185 | ~228,000 | **50.2%** |

Items 4 and 5 above are **closed**; no time should be spent on them. Items 3, 6,
and 7 are narrow but all in the same class: agbcc's register allocation, with no
clear source-side lever.

In **item 1** (the SIO driver, 2,374 bytes) the block-79 r4 conflict was resolved;
a 505-instruction difference remains. The next round of work should separate the RX
address relation and the window/exit blocks using the current diff. The hit rate is
much higher in the non-sibling part of the 120–560 band — most of the functions
written from there this session matched on the first attempt.
