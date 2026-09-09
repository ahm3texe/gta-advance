# Compiler identity: agbcc

## Conclusion

GTA Advance (Europe) was compiled with **`old_agbcc`** — the *old* variant of the
GCC 2.8.1-based compiler that Nintendo shipped with the official GBA SDK. The
same compiler family is used by the pokeruby and pokeemerald decompilations.

What this means: **byte-matching output from C is possible.** The project is not
forced into semantic reconstruction.

## Evidence 1 — code patterns

Compilers consistently pick the same one of several ways to do the same job.
Across the first 42 verified functions (2874 lines of assembly at that stage):

| Pattern | agbcc | modern GCC/Clang | In the ROM |
|---|---|---|---|
| Register copy | `adds rX, rY, #0` | `movs rX, rY` | **84 / 0** |
| Function return | `pop {rN}` + `bx rN` | `pop {..., pc}` | **28 / 0** |

The reason for the return pattern: on ARMv4T, `pop {pc}` does not switch between
Thumb and ARM mode, whereas `bx` does. Older compilers always took the safe long
route.

## Evidence 2 — byte-level verification

The C in `src/save/save_helpers.c` was compiled with agbcc and compared against
the ROM:

```
ReadU8        4 bytes   BYTE-MATCHING  (0x080010F8)
ReadU16LE    12 bytes   BYTE-MATCHING  (0x080010FC)
ReadU32LE    24 bytes   BYTE-MATCHING  (0x08001108)
WriteU8       4 bytes   BYTE-MATCHING  (0x08001120)
WriteU16LE   12 bytes   BYTE-MATCHING  (0x08001124)
WriteU32LE   28 bytes   BYTE-MATCHING  (0x08001130)
```

**All eight functions are byte-matching** (`WriteU16LE` was solved later; the
"Left open" section below was removed).

`WriteU32LE` is the decisive one: all 28 bytes exact, and with a distinctive
choice — rebuilding the mask with two `mov #0xff` + `lsl` instructions rather
than reading it from the literal pool. The wrong compiler cannot produce that.

## Evidence 3 — discriminating the compiler variant

The same C source was tried with six compiler/optimization combinations:

| Compiler | `-O2` | `-O1` | `-O0` |
|---|---|---|---|
| `agbcc` | 3/6 | 3/6 | 0/6 |
| **`old_agbcc`** | **5/6** | 5/6 | 0/6 |

`old_agbcc -O2` also gets `ReadU16LE` and `ReadU32LE` right without any change to
the C. The extra pointer copy `agbcc` generates in those two comes from the
register allocation difference between the two variants.

`-O0` gives zero with both (it adds a stack frame), so the ROM was compiled
optimized.

## Flags

```
old_agbcc -mthumb-interwork -O2 -fhex-asm
```

Some translation units may use a different compiler or level; the other variant
can be tried with `make c-match FILE=... --cc=agbcc`.

## C writing rules (found by measurement)

Each of these turned at least one function from non-matching to matching:

| # | Rule | Why |
|---|---|---|
| 1 | **RAM addresses must be `extern` symbols**, not `#define ((T*)0xADDR)` | As a constant, agbcc folds `base+offset` into a separate literal; the ROM keeps the base in a register |
| 2 | When accessing a member of an array element, **both forms generate different code**; pick by looking at the ROM | `array[i].field` can make agbcc fold the field offset into the base literal (`.word base+0x3c`); `p = &array[i]; p->field` leaves the offset in the load instruction. Which is correct varies by function — `entity_flags.c` needed the second, other measurements the first. Rule 23 was the second half of this rule and was merged in |
| 3 | **A temporary buffer on the stack must be `volatile`** | Otherwise agbcc reorders the address-taking against the constant load |
| 4 | **`volatile` is tried separately for each access** | It is an ordering knob, not a semantic one: it had to be removed for `gBiosIrqFlags` and added for `REG_IF` — in the same `x \|= constant` form |
| 5 | External symbols are given to the assembler with `.equ` | Left to the linker, an interworking veneer is inserted |
| 6 | The section address is fixed in the link script (`SUBALIGN(1)`) | agbcc aligns `.text` to 8; if the base is not a multiple of 8 every measurement shifts |
| 7 | `.align 2, 0` is added at the end of the generated assembly | `as` pads the Thumb section with NOP, the ROM with zero |
| 8 | An array-clearing loop is written **forward** (`i = 0; i < N; i++`) | agbcc turns it into a backward pointer walk, which is the form in the ROM. Writing it backward by hand produces different code |
| 9 | The loop index must be **signed** (`int`) | A pointer comparison generates an unsigned branch (`bcs`); the ROM uses a signed one (`bge`) |
| 10 | A chained assignment (`a = b = c`) generates different code from separate lines | The ROM's form is chained; written separately, the address computation flips order |
| 11 | An address that lives across calls is taken into a local **at the top** | The ROM keeps it in a callee-saved register; read at the point of use, the compiler does not hoist it |
| 12 | A register with a save/restore pair (`REG_IME`) must be `volatile` | Otherwise the compiler merges the saves of the two critical sections |
| 13 | Length/size parameters may be **unsigned** | In `save_slots` the only difference was `ble` ↔ `bls`: it matched once `length` became `u32` |
| 14 | Repeated byte copies are **written out explicitly**, not wrapped in a loop | `-funroll-loops` produces different code from the ROM's (136B/127 differences → 256B/239); writing the eight copies by hand gave an exact match |
| 15 | A narrow parameter's **signedness** determines the entry normalization | An `s16` parameter generates the `lsls #16`/`lsrs #16` pair, a `u16` does not. `WriteU16LE` is the proof |
| 16 | In rule 11, the **assignment site** matters, not the declaration site | If `payload = &g...` is initialized at the top, the lifetime grows too long and the allocation shifts (220/240 difference); moving the assignment just before the use makes it match |
| 17 | A single address may need **two separate locals, each used once** | Using one pointer in two places makes agbcc hoist the computation to the top of the function; the ROM's `adds r3, r4, #0` only appears with two variables |
| 18 | Reading an argument into a local in a **separate statement** flips the argument setup order | `f(480, gSlotCount)` sets up the constant first; an intermediate `count = gSlotCount;` line gives the ROM's order |
| 19 | The **source order** of pre-loop assignments is preserved | agbcc emits source assignments in source order and its own counter initializations adjacent to the loop head; changing the order leaves a 13-14 byte difference |
| 20 | For a stack slot that is 4-byte aligned but written as a halfword, use a **`u16 x[2]` array** | Because an array is BLKmode it is placed in declaration order and aligned to 4; `x[0]=0` still generates `strh`, and `(u32)x` gives the address in one instruction. A scalar `u16` drops the frame to 12 bytes, and `u32` makes the write a word |
| 21 | Constant assignments used inside a loop are written **inside the loop** | Written before the loop, agbcc emits it as a source statement *before* the preheader copies; taken inside, the loop-invariant hoister puts it at the end of the preheader and the order matches the ROM's |
| 22 | Write a **reassignment from the constant**, not a copy of the same base | With `b = a;` agbcc merges the two variables into one pointer; `b = (T *)ADDRESS;` gives a separate lifetime |
| 24 | For a seven-bit field write a **bitfield**, not a mask | Instead of `x & 0x7F`, use `u8 f : 7` — the ROM generates the `lsls #25`/`lsrs #25` pair |
| 25 | A global read can be done in a **separate statement** rather than at the point of use | `if (g[26] != 0)` and `v = g[26]; if (v != 0)` produce different ordering |
| 26 | A narrow **struct field's** signedness determines the mask width | With `s8 flags`, `flags &= ~4` keeps the mask at 32 bits (`movs #5`/`negs`); with `u8` it narrows to a byte (`movs #251`). The field version of rule 15. Second measurement: in `InitActor`, making the 0x8A/0xA8 fields `s8` reduced the difference from 40 to 14 bytes |
| 27 | A raw value and its derivative can be held **in one variable** | `index = id; index = (u16)(index - 1);` gives a different register allocation from two separate variables |
| 30 | If sparse `case` values are spread over a wide range, write an **`\|\|` comparison chain**, not a `switch` | A `switch` (6 values between case 21..57) makes agbcc generate a 37-entry jump table: 212 bytes instead of 64. The ROM's `cmp`/`beq` chain only appears with `if (k == 57 \|\| k == 25 \|\| ...)`, and **the source order must match the ROM's comparison order**. The inverse of rule 19 (a dense `switch` → a jump table) |
| 32 | For consecutive word copies write a **struct assignment**, not `*dest++ = *src++;` | agbcc generates three separate `ldr/str` pairs for a `*p++ = *q++;` triple (12 instructions). Declaring `typedef struct { u32 a,b,c; } Triple;` and writing `*(Triple*)dest = *(Triple*)src;` triggers the `ldmia/stmia {r0,r1,r2}` pair (2 instructions). At `0x08031FB4` the difference went 54 → 27 bytes (measured; the file was later unparked and deleted). It will not produce a `memcpy` call — a struct assignment is required |
| 31 | The loop counter's signedness determines the `bls` (unsigned) vs `ble` (signed) branch choice | `for (u32 i = 0; i <= N; i++)` → `bls`; `for (s32 i = 0; i <= N; i++)` → `ble`. The ROM uses both — the counter's type decides which appears. In `slot_scan.c` this alone took a 1-byte difference to 0. This was the reason `MaybeAdvance` was parked; `int counter` could be tried there instead of `u16 counter` |
| 29 | If two branches do the same work, write an **early `return` + a shared tail**, not an assignment to a common variable | `if (k) { p->h = A; return; } ... p->h = B;` produces two separate copies as in the ROM; `handler = A else B; p->h = handler;` makes agbcc merge the branches (cross-jumping) and leaves a 39-byte difference |
| 28 | When adding two terms to a scaled base, **pointer arithmetic** and **array indexing** produce different code | `*(t + x + (y << s))` scales each term separately (`lsl` + `lsl` + two additions); `t[x + (y << s)]` adds first and scales once. In `IsTileTypeInRange` the array form gave a 33-byte difference **and** an unnecessary `push {r4,lr}`; the pointer form reduced it to 3 and made the function a leaf |
| 33 | To mask a variable with a constant, put the constant in a **separate result local** and use `&=` in place | `return (flags & 3) << 8` keeps the result in `flags`'s register; `mask = 3; mask &= flags; return mask << 8` keeps it in the constant's register. `QueryEntity` went from a 2-byte difference to an exact match |
| 34 | Where necessary, write null checks that lead to the same zero return as **explicit early returns** | Despite equivalent semantics, nested `if`s placed the shared zero block after the value block. An `if (!p) return 0;` chain produced the ROM's block order and literal pool placement in `ProbeObject` and `GetInnerId` |
| 35 | If `pop {r0}; bx r0` follows a call, the wrapper's return type is most likely **`void`** | With a `u32` return, r0 stays live, so agbcc takes the return address into r1. Making `CallWithOffset`'s signature `void` closed a 10-byte register/epilogue difference entirely |
| 36 | If a memory address must be set up before a constant write, **take the target field's pointer into a separate local first** | The order `tail = &actor->unk90; i = 0; *tail = i` made agbcc set up the address first and the zero constant second. It closed `InitActor`'s last instruction-order difference |
| 37 | In parallel walks derived from the same base, **keep the base as a separate local too** | Writing `cur = g; kind = cur + 100` directly merged the base with `cur`. `base = g; kind = base + 100; cur = base` produced the ROM's separate r0/r1/r2 lifetimes and literal pool placement; `HasWantedEntry` went from a 25-byte difference to a match |
| 38 | Even if an early comparison and the final store carry the same value, if the ROM wants a separate branch, split the value and the **base copy before the comparison** | The form `current = h->current; h2 = h; if (value == current) return;` prevented agbcc from merging the equality path with the final store and closed a 31-byte difference in `PushHistory` |
| 39 | If the ROM re-reads memory only after a particular write, apply `volatile` **to that access, not the whole field** | `*(volatile u16 *)&gSaveBuffer.distance` forced only the second `ldrh` in the overflow check. Making the field volatile in its entirety increased register pressure and broke `AddDistance` by 53 bytes, while the narrow use made the function match |
| 40 | If the ROM loads a separate base in two branches and shares one store, **set up the control flow explicitly with labels** | agbcc inverted the structured `if/else` and merged the bases. The labels `reset:`, `increment:`, and `store:` produced `BumpOrReset`'s two `ldr` + shared `strb` layout; this is an acceptable low-level CFG expression within clean C |
| 41 | If a scaled offset will be used with several bases, compute the product in a **single assignment** and set up the field bases in separate locals | `scaled = index; scaled *= 180` raised the pseudo's priority and inverted r3/r4. With `scaled = index * 180` plus a `heldBase`/`extraBase` split, `ReleaseSlot` produced the ROM's single-r3-offset + two-base pattern |
| 42 | Even when the ROM uses a decreasing loop counter, an **increasing indexed `for`** should be tried in C | In `FlushSpriteList`, `for (i = count; i < left; i++)` is converted by agbcc into a loop decreasing by `left - count`. The subtraction the compiler generates comes after the constant setups; a hand-written `left -= count` came before. The remaining 9-byte difference closed: 112/112 |
| 43 | If a counter and a pointer advance together, express **both in the `for` increment, in the ROM's order** | In `InitSpritePool`, the `node++` in the body came before the counter decrement. The form `for (...; ...; i++, node++)` produced the ROM's counter-first order: a 4-byte difference closed, 124/124 |
| 44 | Take the comparison constant **into a local variable**: `hi = 15; if (x < hi)` | agbcc canonicalizes a literal comparison (`< 15` → `<= 14`, `bls`); if the ROM shows `cmp #15 / bcc`, no variation of the literal spelling will hold. With the constant in a variable, canonicalization is skipped and the constant is still emitted as an immediate. `EitherInRange`: 8/44 → 44/44. Found by the permuter's lifting, plus the remaining bound lifted by hand. |

## The mechanism of register allocation

Most of the rules (11, 16, 17, 20, 23, 27) are faces of the same single
mechanism. agbcc's (GCC 2.8.1's) global register allocator sorts virtual
registers by the following priority and gives each, in turn, the first suitable
hardware register:

```
priority = floor_log2(reference_count) × reference_count / lifetime_length
```

So **adding or removing a reference** to a variable can change which register it
lands in, and therefore flip the *entire* allocation. Measured in
`ClearTextArea`:

| Variable | references / lifetime | priority |
|---|---|---|
| `dma` | 9 / 52 | 5192 |
| `control` | 5 / 21 | 4761 |

In that order `dma` is allocated first and takes `r3`, the reverse of the ROM's.
Once the dead read in block 2 is assigned to the `control` variable, `control`
rises to 6 references (6/22 → 5454 > 5192), is allocated first, and takes `r3` —
with `dma` landing in `r4`, `dest` in `r5`, the IME base in `r6`, and the stride
in `r7`: **exactly the ROM's allocation.** A 13-byte difference drops to 1.

This is also the answer to *why* rule 17 ("two separate locals for one address")
works: the second local splits the lifetime and changes the priorities.

**Practical consequence:** on a register mismatch, instead of tinkering with the
C at random, count the reference count and lifetime of the variables involved;
compute which one needs to be allocated first; and flip the order by adding or
removing references.

One caveat: while the whole function is a single extended basic block, **you
cannot add a free reference**. Every reg-reg copy is propagated by CSE and
deleted by combine; `x = x`, a dead `x = 0`, `x |= 0`, `x + 0`, and `x ^ x` are
all eliminated. The only thing that gains a reference is a `volatile` access —
and that changes an operand's register.

For measurement, `old_agbcc -dg` (global) and `-dl` (local) produce an allocation
dump; in this repository's invocation the files came out empty, and the formula
was confirmed by indirect measurement.

The mechanism behind rule 20 generalizes: **what determines stack layout is not
declaration order but whether the type is BLKmode.** One agent tried 24
declaration-order permutations and measured that the layout never changed.

**Do not forget that the rules are interdependent.** Rule 18 changed nothing when
tried on its own; it only became decisive after 16 and 17 were applied — because
the ordering difference was not an independent knob but a consequence of register
pressure. So a "tried, did not hold" record must not be evaluated in isolation:
a change that came out ineffective may work when combined with another.

In agbcc, `volatile` is an **instruction ordering knob**, not a semantic marker.
For the same `x |= constant` idiom it had to be removed for `gBiosIrqFlags` and
added for `REG_IF`. It cannot be memorized — try both directions for each access.

The same applies to address form: **rule 1 is not universal.** RAM symbols must
be `extern`, but addresses agbcc can produce by shifting (such as
`0x03000000` = `0xc0 << 18`) are written in the ROM as constant casts. Whether an
address is read from a literal pool or computed in the ROM — the diff tells you.

## RAM addresses must be extern symbols — the most important rule

`EraseSaveSlot` and `GetSaveSlotHeader` did not match for a long time. The cause
was assumed to be the compiler version; **it was not.** The real cause was on the
C side:

```c
#define gSaveSlotHeaders ((SaveSlotHeader *)0x02000460)   /* WRONG   */
extern SaveSlotHeader gSaveSlotHeaders[3];                /* CORRECT */
```

When the address is a compile-time constant, agbcc folds it: it turns the
expression `base + 16` into a separate literal (`0x02000EE0`) and does not keep
the base in a register. The ROM, by contrast, loads the base once and keeps it in
a register. With the address as an extern symbol the compiler cannot fold it, and
it produces the code the ROM has.

With that single change `EraseSaveSlot` matched instantly; `GetSaveSlotHeader`
matched once it was switched to direct member access:

```c
if (gSaveSlotHeaders[slot].marker == 0)   /* not an intermediate pointer variable */
    return 0;
return &gSaveSlotHeaders[slot];
```

**Rule: every RAM address is recorded in `data/ram_map.csv` and declared `extern`
in C.** `tools/agbcc_build.py` resolves the symbol from there.

## Measured but found ineffective

Before the cause above was found, two hypotheses were tested exhaustively. Both
came out ineffective; they stand on record so that they are not retried:

**Flag sweep** — 15 candidate flags across two compilers (`-fforce-addr`,
`-fforce-mem`, `-fno-strength-reduce`, `-fomit-frame-pointer`, `-fno-peephole`,
`-fcaller-saves`, `-fno-cse-follow-jumps`, `-fno-expensive-optimizations`,
`-fno-defer-pop`, `-fno-function-cse`, and others). Not one changed a single
byte.

**Compiler version** — pret/agbcc's `release` tag was also built. Its binaries
differ from `master`'s, but its output is byte-for-byte identical.

So the problem was never in the compiler. This is an example of how reading
negative results as "that road is closed" can mislead: the real variable was
elsewhere.

## The `agbcc` variant was tried on 18 parked functions — none matched

2026-09-05. All 44 rules measured since the project began had been derived with
`old_agbcc`; the `--cc=agbcc` option in `tools/agbcc_build.py` was documented but
had never been used. All 18 functions on the parked list were re-measured with
both compilers.

**`agbcc` matched none of them.** The difference decreased in four and increased
in three:

| function | old_agbcc | agbcc |
|---|---|---|
| FUN_0800cb08 | 291 | 146 |
| ClearHudFieldA | 34 | 14 |
| ClearHudFieldB | 28 | 15 |
| TryEngageTarget | 211 | 208 |
| IsTargetNear | 12 | **40** |
| RunMenuScreen | 949 | **955** |
| StepEntryTimer | 55 | **64** |

The improvement in the two VRAM fillers is **misleading**. `agbcc` gets the size
to 40 instead of 36, but not for the reason the ROM does: the body is identical
to `old_agbcc`'s, with an unnecessary `push {lr}` / `pop {r0}; bx r0` wrapper on
top (+4 bytes). The +4 in the ROM is an extra register copy plus alignment
padding. A coincidence that arrives at the same size by another route, not
progress.

**Conclusion: a per-file compiler selection marker was NOT ADDED to
`agbcc_build.py`.** A `COMPILER: agbcc` marker analogous to `MODE: ARM` would
have been necessary only if a match were possible with it; there is no such
match. Adding it would have introduced another branch into the build layer while
gaining nothing.

`FUN_0800cb08` dropping from 291 to 146 (348 bytes, still not matching) may be
worth another look on its own; for the others this road is closed.

## Register copies: a class that cannot be produced at the source level

`ClearHudFieldA` / `ClearHudFieldB` (both 36/40, four bytes short) are the clean
example of this class. `old_agbcc` produces the ROM's body instruction for
instruction; the only thing missing is the ROM's extra `adds r2, r0, #0`. The ROM
loads the tile constant into r0 first and moves it into r2 because the allocator
leaves r0 to the loop counter; ours gives the constant straight to r3 and avoids
the copy.

Everything swept and eliminated in an attempt to produce that copy:

- **144 declaration/assignment orders** (4! declarations × 3! assignments) — all
  36 bytes
- **13 flag sets** — `-O0/-O1/-O2/-O3/-Os`, `-fno-omit-frame-pointer`,
  `-fforce-mem`, `-fforce-addr`, `-fno-strength-reduce`, `-fno-defer-pop`,
  `-fcaller-saves`, `-fno-cse-follow-jumps` — none changed the 36
- **Structural families** — `for` / `while` / `do-while`, `*p++` vs `*p=t; p++`,
  tile type `u16` / `s32` / `int` / `vu16`, an intermediate copy variable, a
  counter variable, `q = p + 32` — all 36 bytes

So this is the inverse of rule 44 (comparison canonicalization): there, a lever
existed in the source; here there is none. The "load/store register allocation"
class named in `tools/sweep_variants.py`'s scope-limit note is exactly this. Do
not return to these functions until a new mechanism is found.

## Rule 45 — separate locals per branch prevent block merging

In long `if/else if` chains, when several branches have identical bodies, agbcc
merges them by cross-jumping and one branch's code disappears entirely. If those
branches exist as separate physical copies in the ROM, then in the original
source **each branch had its own local variables**.

Measured in `FUN_080260a8` (1518 bytes, a 12-branch chain). Giving all branches a
shared `bias`/`shift` pair merged `case 8`'s body entirely with `0x1f`/`0x28`:
**4 bytes** between `cmp #8` and `cmp #30` in ours, **156** in the ROM. Opening
three separate locals per branch took it from 1312 to 1428 bytes.

**How to spot it:** look at the ROM's stack slots. If two branches doing the same
work use different `sp` offsets (here `case 8` → `sp+8/12/16`, `case 0x1f` →
`sp+32/36/40`), the locals are separate. Ghidra already shows this correctly
(separate names such as `local_48/44/40` and `local_30/2c/28`); do not dismiss
the local names in its output as "noise" — **they carry structural information**.

A second indicator: count how many times a repeated pool constant occurs in the
ROM. Here `0x03FFFFFF` sits in six separate pool words, meaning there are six
physical blocks using the mask. Ours coming out at five was direct evidence that
a block had merged.

It cannot be solved with flags: `-fno-thread-jumps`, `-fno-cse-follow-jumps`,
`-fno-expensive-optimizations`, and `-O1` were tried, and none removed the
merging. The lever is in the source, in the separation of local variables.

**Where it applies — `tools/scan_dispatch.py`.** The rule only helps in long
`if/else` chains branching on a state variable, so candidates are searched for
directly in the ROM with that signature: consecutive `cmp rX,#imm` links against
the same register, containing enough **distinct values greater than zero**.
`cmp rX,#0` links are null checks and are filtered out.

The tool validates itself: `FUN_080260a8` appears in the list with exactly the
values that were matched (35, 33, 8, 30, 31, 40, 50, 32, 7, 6). With the
threshold at 6 links / 5 distinct values, **13 candidates, 18,788 bytes** remain;
the largest is `FUN_08017628` (1536 bytes, 26 distinct values, 35 links).

**One route was tried and abandoned:** the same scan was first done over Ghidra's
output by counting "consecutive identical expression blocks". It gives false
positives — in `FUN_080108f4` it counted 139 repetitions, but that function does
not contain repeated branch bodies; it contains a repeated global access pattern
(40 distinct `DAT_` globals, nested loops). Textual similarity is the wrong
criterion; the signature is in the machine code.

**The warnings that indicate Ghidra's output is incomplete** are essential when
filtering candidates: `Could not recover jumptable` (in 19 of 33 outputs),
`Removing unreachable block` (this one is insidious — the output looks tidy but
blocks have been dropped, as with `FUN_0802e3fc`), `Bad instruction`, and
`truncated`.

## Rule 46 — a switch's range determines the choice between a tree and a jump table

agbcc turns a `switch` into either a **comparison tree** or a **jump table**, and
decides based on the density of the case set. If there is no `mov pc,rX` in the
ROM, a tree was chosen, and our source must produce a tree too, or the size will
not hold.

Measured in `FUN_08017628` (1536 bytes, ~40 cases). Written with cases between 1
and 0x97, the set stayed dense and agbcc generated a jump table: **1814 bytes**
(296 too many), 45 comparisons, one `mov pc,r0`. Adding the four large values
Ghidra had shown as pool constants (0x4005, 0x4026, 0x4027, 0x4028) as cases
opened the range to 1..0x4028, forcing gcc to generate a tree: **1510 bytes**, 92
comparisons (exactly matching the ROM), zero `mov pc`. One change, 304 bytes.

**Diagnosis:** count `mov pc,rX` (0x4687/0x468F/0x4697) in the ROM and in your
own output, then compare the `cmp rX,#imm` counts. If the counts agree, the tree
shape is correct; if you have a `mov pc` and the ROM does not, your case set is
too dense and you have missed the distant cases.

**A trap when reading the tree:** when a range has one case left, gcc separates
it with `<` rather than equality, so that case does not appear as a `cmp` in the
ROM. In `FUN_08017628`, 0x0E is such a case: there is no `cmp #14` in the ROM but
the case exists, separated by `< 0x0F` because it was the only value left in the
(0x0D, 0x0F) range. Do not remove it thinking the case is missing.

## Ghidra: the ARM mode trap and a false "jump table" label

Ghidra's automatic analysis tries to decode some Thumb entry points **in ARM
mode**, gives up with `bad instruction data`, and does not even define a function
at that address. Five of the 7 entry points found by our boundary scanner were
like this (including `FUN_08017628`). The fix: set the `TMode` register to 1,
clear the region, and re-disassemble — `tools/ghidra/ExportDecompileBatch.java`
does this unconditionally when a size is given. **It must be unconditional:** a
broken function may be left over from a previous run, and "create if absent" is
not enough. This repair made five functions totaling 8,586 bytes readable.

Ghidra can also mistake the `bx r0` at the end of the sequence
`pop {r4,r5,r6}; pop {r0}; bx r0` for an "unrecoverable jump table". That is what
happened in `FUN_08017628`; there is no table there, only the interworking form
of a void return.

## Rule 47 — a byte field's signedness determines how the mask is built

`x &= ~15` written on a **`u8`** field makes agbcc reduce the constant to `0xF0`
(`movs r1,#240`). The same line on an **`s8`** field promotes the field to `int`
and builds the mask as `-16` (`movs r1,#16 / negs r1,r1`) — one more instruction,
two more bytes.

Measured in `FUN_08016768`: leaving the fields as `u8` left the function two
bytes short and also shifted the instruction ordering; making them `s8` fixed
both at once and the function **matched**.

**The rule is narrow, do not generalize it (2026-09-06 correction).** What is
decisive is not the field's signedness but the width at which the AND result is
consumed. In `FUN_080526b8`, the compound assignment `node->kind &= ~12` produces
`-13` **even on a `u8` field** and matches, because the operation is done at int
width. In `FUN_08016768` the result was consumed narrowed to a narrow type, so on
a `u8` field the constant was folded to `0xF0`. So seeing `negs` in the ROM does
not mean "the field is signed"; it means "the constant was built at int width".
Change the field type only when there is other evidence.

**A separate trap — do not take it into an intermediate local.** Writing the same
work as `s32 k = field & ~12; field = k;` pushes agbcc into inserting an
`lsls #24 / asrs #24` normalization before the store: two extra instructions that
are not in the ROM. Write the compound assignment directly.

Taking the mask into a wide-typed local (`s32 m = ~15; x &= m;`) brings `negs`
back, but because it emits the constant BEFORE the expression, its order clashes
with the address computation. The correct solution is to fix the field's type,
not to move the mask.

## Rule 48 — materializing a condition's result in a variable

If you see the sequence `movs r0,#0 / ... / movs r0,#1 / cmp r0,#0 / beq` in the
ROM, the source is not branching on the condition directly — it is **writing the
result into a variable and testing that**:

```c
ok = 0;
if (ent->kind == 4) ok = 1;
if (ok) GetOwnerSlot(ent);
```

The short-circuiting form `if (ent != 0 && ent->kind == 4)` produces a direct
branch and is six bytes shorter. The pattern had been seen before in the project,
in `target_follow.c`.

## The control register is read back after DMA setup

After writing the DMA3 control word, the ROM **reads it back** once
(`ldr rX,[rY,#8]`). It does not look functional, but omitting it makes every DMA
block two bytes short. It is present in both `cutscene_frame.c` (matched) and
`flush_palette_queue.c`:

```c
REG_DMA3.control = TILE_CTRL;
REG_DMA3.control;          /* read-back; two bytes short if omitted */
REG_IME = ime;
```

## Rule 49 — the ROM keeps rare bodies at the END of the function

agbcc emits `if` bodies in source order. If a branch in the ROM jumps **forward**
(`beq` far, `bne` far), that body is at the end of the function; writing the same
body inline moves the block forward and inverts the branch.

Applied three times consecutively in `FUN_080543D0` (126 bytes), gaining each
time:

| moved | before | after |
|---|---|---|
| the "found" body from the loop to the end | 128 B | difference 87 |
| `return 0` to the end | difference 87 | **difference 56** |

The form: a `goto` to the end instead of an early return, with the body labeled
after the `return`. The project already uses this form (`target_follow.c`,
`menu_screen.c`).

```c
    if (spare->id != SPARE_ID) goto none;   /* NOT `return 0;` */
    ...
    return spare;
found:                                       /* rare bodies at the end */
    ...
    return cur;
none:
    return 0;
```

**How to spot it:** look at the DIRECTION of the conditional branch in the ROM.
If it jumps forward, the target body is ahead; if you have a `bne` where the ROM
has a `beq` (or vice versa), your block order is inverted.

**A side finding — loop rotation.** Using `break` in the same function pushed
agbcc into rotating the loop (jumping to the test at the bottom with a `b`). For
the ROM's entry-guard + bottom-returning form, write an explicit `goto` instead
of `break`.

**FUNCTIONS IN THE SAME FAMILY MAY NOT USE THE SAME LOOP FORM.** `FUN_080543D0`
and `FindOrInitAreaNode` are near twins (both search a sorted list for an id and
set up a spare node), yet they were compiled differently in the ROM:

| | the ROM's form | correct spelling |
|---|---|---|
| FUN_080543D0 | entry guard + a do/while returning from the bottom | `if (cur == 0) goto ...` + `goto scan` |
| FindOrInitAreaNode | a rotated `for` (jumping to the test with `b`) | `goto test;` + `step:` / `test:` |

Writing a plain `for` in `FindOrInitAreaNode` made agbcc **peel** the first
iteration (the id comparison appears twice in the output); 176/164 bytes, 12 too
many. Writing the ROM's form with explicit jumps made the size hold and reduced
the difference from 162 to 107.

The lesson: DO NOT COPY a sibling file's loop form; read each function's form
from the ROM. Structs and called signatures can be shared, control flow cannot.

## Rule 50 — the only way to flip register priority from source: SPLIT the variable

agbcc's allocation order was measured and confirmed (2026-09-06, three functions,
39 allocnos re-derived by hand):

```
priority = floor_log2(refs) * refs / lifetime        (on a tie, the smaller pseudo first)
```

When its turn comes, `find_reg` gives the **smallest free** register in the
conflict graph; if the allocno crosses a call, it only considers callee-saved
(r4+) candidates.

**The lever is that `floor_log2` is a step function.** Splitting a variable in
two roughly halves both refs and lifetime — the ratio stays about the same, but
`floor_log2(refs)` drops a whole step and the priority falls to about a third.

Measured in `FUN_08052DDC`: with two outer loops sharing a single `p` pointer,
`3*14/29 = 1.448` was beating the counter's `1.185` and taking r4. Given a
separate pointer for the second loop it fell to `2*7/14 = 1.000`, the counter was
allocated first and took r4: **difference 17 → 0, byte-matching.**

**SPLITTING ONLY WORKS IF THERE ARE TWO SEPARATE PRODUCTION SITES.** Copy-based
splitting (`lst = list;`) is always eliminated — 16 spellings were tried and none
produced a new allocno. Values genuinely born from two separate sources, such as
`p = entry->ids` / `p2 = entry->slots`, can be split.

**refs are not weighted by loop depth** — references inside a loop are not counted
extra, so the table can be worked out by hand from the source.

**A tooling gap:** `tools/dump_alloc.py` reads only the priority table from the
`.greg` dump. The real answer is in the **`Register dispositions`** section below
it: the pseudo → hardware register map. The one-line answer to "which of my
variables took r4" is there; the tool does not print it.

## A counterexample to rule 47 — signedness SOMETIMES matters

In `FUN_080526B8` (nodelist_b6.c) it made no difference whether the `+0x0B` field
was `u8` or `s8`. In `ReleaseAreaNode` it **does**: on a `u8` field, `kind &= ~1`
folds into a single instruction (`movs r0,#254`), whereas the ROM builds −2 with
`movs r0,#2 / negs r0,r0`. The field must be `s8`. There is no cost in the other
direction: even as `s8`, `kind & 1` and `(kind & 0xF) | 0x10` still generate
`ldrb`, since agbcc does not add a sign-extend for masks below 0x100.

So rule 47's decision procedure is not "the width of the AND result" but
**measuring both directions**. A two-line experiment is cheaper than a guess.

## Is the problem really allocation? — measure that first with `dump_alloc.py --rom`

Before applying rule 50, **measure whether the problem is allocation at all**.
The tool now prints the ROM's `push` list and per-register operand counts side by
side; the distinction is visible at a glance.

**Two opposing examples measured (2026-09-06):**

`FUN_080543D0` — difference 41. The callee-saved traffic is **identical** to the
ROM's: r3 9/9, r4 11/11, r5 8/8, r6 7/7, r7 1/1. The difference is only in
r0/r1/r2 (23/28, 26/23, 7/5). So the allocation is **already correct**; the 41
bytes come from temporary registers and instruction selection. Applying variable
splitting here is **wasted effort** — the reason three files were tinkered with
blindly was precisely this distinction not being made.

`FindOrInitAreaNode` — difference 80. The callee-saved traffic **does diverge**:
r3 ROM 13 / ours 6, r4 ROM 16 / ours 13, r2 ROM 10 / ours 16. The ROM keeps the
weight in r3+r4, we dump it into r2. The culprit was measured: one allocno is the
second densest value at 12 refs / 64 lifetime but **never crosses a call**, so
`find_reg` gives it the smallest free register (r2) and never looks at
callee-saved candidates. This is the file where rule 50's lever is meaningful.

`FUN_08052DDC` (matching) is the calibration reference: its `push` list and
**all** operand counts are identical to the ROM's. Proof that the tool reads
correctly.

**Rule: run `--rom` first.** If the callee-saved counts agree, the allocation is
correct — look elsewhere.

## agbcc pseudo → C variable name: IMPOSSIBLE EXCEPT FOR PARAMETERS

Tried and measured: agbcc has no `-g`, `-gstabs` gives "invalid debug option",
the generated `.s` contains not a single `.stab` line, and the line number field
of RTL insns is −1. The compiler **never writes** local variable names into these
dumps.

Parameters can be extracted: in the prologue, the `(set (reg/v N) (reg H rH))`
pattern before `NOTE_INSN_FUNCTION_BEG` gives the argument order, and the name is
read from the source's **definition** signature. For locals the name column stays
empty and the first defining expression is printed instead (such as
`mem[p22+40]`) — a name is never invented.

Two traps, both caught while writing the tool: if `note` nodes are not scanned,
`NOTE_INSN_FUNCTION_BEG` is never seen and **call return values are mistaken for
parameters**; and looking at the first occurrence in the file for a parameter's
name is wrong — an old signature in the header comment can contradict the real
definition (which is exactly what happened in `nodelist_a1.c`).

## Two other traps

**Section alignment.** agbcc aligns `.text` to 8. If the base address is not a
multiple of 8 (like `0x08001094`), the linker pushes the section forward and
every measurement shifts, *including previously matching functions*. The section
address is explicitly fixed in the link script (`SUBALIGN(1)`).

**Section-end padding.** `as` pads Thumb sections with NOP (`0x46C0`), the ROM
with zero. `.align 2, 0` is added at the end of the generated assembly.

**External symbols are given with `.equ`, not left to the linker.** Because the
linker does not recognize an absolute symbol as a Thumb function, it inserts an
interworking veneer and the `bl` target comes out wrong.

## Closed: WriteU16LE

It was open for a while: the ROM normalized the value to 16 bits on entry
(`lsls #16` / `lsrs #16`) and the code we generated skipped those four bytes. It
closed once rule 15 was found (a narrow parameter's signedness determines the
entry normalization); `src/save/save_helpers.c` is now 8/8 byte-matching.

## Setup

The binaries do not enter the repository (8.8 MB). To build them locally:

```sh
make agbcc
```

`tools/setup_agbcc.sh` pins the pret/agbcc source to the compatible revision in
`config/toolchain.lock.json` and builds it. Because agbcc is 1998-era C source it
does not build with modern clang's defaults; the tracked `tools/agbcc_host_cc.sh`
carries the necessary compatibility flags. At the end of setup, the fingerprint
of the fixed 23-function representative corpus is verified; adding new sources
does not change that lock by itself.

## The verification loop

```sh
make c-match FILE=src/save/save_helpers.c
```

It compiles each function separately and compares it against the ROM at its
address from `data/functions.csv`. The assembly equivalent of a matching function
is then unnecessary.

| # | Rule | Why |
|---|---|---|
| 68 | If all the registers are **one higher** than expected, there is an **extra parameter being forwarded** untouched in the source | In `SubmitPack` the ROM used `{r4,r5,r6}` + r2/r3 while ours generated `{r3,r4,r5}` + r1/r2; the entire difference was a single register shift. There was no instruction SETTING r1, so the value was an incoming parameter. Adding the second parameter to the signature and forwarding it in the call took an 8-byte difference to zero. Earlier, adding an argument to `FUN_08060db4(void)` had already brought it from 16 to 8 |
| 69 | If the ROM keeps short-lived intermediates in **scratch registers** (r0-r3), the source must also use **block-scoped separate temporaries**; a single reused local pushes them into callee-saved | In `ClipBounds` the ROM generated `push {r4,r5,lr}` while ours generated `push {r4,r5,r6,lr}`: a single `cand` variable lived across the whole function and held r2, whereas the ROM reuses that register once `pad` dies. Putting each component into a `{ s32 cand = ...; if (...) ...; }` block produced six independent short-lived temporaries. Repeated memory reads must also be done through a narrow volatile view; otherwise the compiler caches them as a common subexpression and extends the lifetime (96 bytes vs the ROM's 100). Together they took a difference of 26 to 0. WARNING: the technique is NOT FUNCTIONAL but LOCAL — the same move worsened CleanupAreaTiles from 7 to 85 and left SetBg1Enable unchanged |

## Register allocation: measured by controlled experiment

This table is not a guess; it is the result of controlled experiments on the
installed `old_agbcc` binary (probe: each variant was compiled and the prologue's
`push` list read). It was the shared obstacle behind three parked files and a
372-byte scaling attempt.

### Leaf function (NO calls)

| values live simultaneously | prologue |
|---|---|
| 2 | no push |
| 3 | no push |
| 4 | no push |
| 5 | `push {r4, lr}` |
| 6 | `push {r4, r5, lr}` |
| 7 | `push {r4, r5, r6, lr}` |

So **up to four live values fit in `r0`-`r3`**; the fifth goes to `r4`, the sixth
to `r5`, the seventh to `r6`.

### With calls

Because a call clobbers `r0`-`r3`, EVERY value living across a call needs a
callee-saved register:

| live across a call | prologue | note |
|---|---|---|
| 1 | `push {r4, lr}` | |
| 2 | `push {r4, r5, lr}` | |
| 3 | `push {r4, r5, r6, lr}` | |
| 4+ | `push {r4, r5, r6, lr}` | the list DOES NOT GROW, it spills to the stack |

At four, the instruction count jumps from 13 to 20: instead of moving to r7, it
spills. **`r7` only appears at 7+ live values.**

### Things with no effect (measured)

- **The NUMBER of calls**: 3 live values with 1/2/3 calls -> all `{r4,r5,r6}`.
- **Pointer vs scalar**: 4 values, both give `{r4,r5,r6}`.
- **Where a constant is set up**: inside/outside the loop/directly -> the same
  code. agbcc hoists the constant.
- **Copies**: `w = v` NEVER survives. One local, two locals with a copy, and two
  locals both built from the constant -> all three give the SAME code. This is
  rule 37's ("take the base into a separate local") limit: giving it a separate
  name does NOT PRODUCE a copy — it only does when the value has two distinct USE
  SITES.

### How to use it

The ROM's prologue tells you how many callee-saved registers it wants; from that,
work backwards to how many live values the ROM has and bring the source to that
number. One extra `push` means one extra live value.

WARNING 1: you must reduce the NUMBER of values, not CHANGE them. In
CleanupAreaTiles I removed the counter and put an end pointer in its place — the
number stayed at six, the prologue did not change, and the difference went from 7
to 60.

WARNING 2 — THE RULE'S REAL LIMIT: the number of locals in the source is NOT the
allocator's live set. In CleanupAreaTiles I hoisted the `block` local out of the
loop in two different ways (an inline call; an assignment after the loop) and the
prologue did NOT change in EITHER — because agbcc already hoists the constant
address, so `block` was never live across the loop.

So the rule can be measured in an isolated experiment, but applying it to a real
function requires SEEING the allocator's live set; counting locals in the source
misleads.

THIS STEP IS SOLVED: agbcc accepts the `-dg` flag and produces a global
allocation dump (`.greg`). The dump contains the allocator's OWN priority list —
`refs` and `live_length` for each pseudo. `tools/dump_alloc.py` reads it and
prints the table.

THE FORMULA WAS CONFIRMED AGAINST THE DUMP. In one probe function the order the
dump printed was:
    R25 refs=7 lifetime=18 -> 0.778
    R23 refs=7 lifetime=26 -> 0.538
    R22 refs=7 lifetime=28 -> 0.500
    R27 refs=4 lifetime=20 -> 0.400
    R26 refs=3 lifetime=18 -> 0.167
    R24 refs=2 lifetime=24 -> 0.083
The order computed with `floor_log2(refs) * refs / lifetime` is EXACTLY the same.

THE FIRST MEASUREMENT WAS SURPRISING: CleanupAreaTiles has 12 pseudo-registers
and 5 spills; I had been counting six live values. Each local in the source
expands into more than one pseudo, which is why counting by hand was misleading.

SPILL COUNT IS NOT A CRITERION — measured across 285 functions, and the
hypothesis was REFUTED:

    277 matching functions   : spills mean 3.85, MAX 170, 43% with zero spills
    8 non-matching functions : spills mean 19.75, max 99, 12% with zero spills

There are matching functions with up to 170 spills, so a high spill count does
NOT PREVENT matching. The converse holds too: EitherInRange has 3 pseudos and
ZERO spills yet is still 8 bytes off (its obstacle is comparison constant
canonicalization, nothing to do with allocation). HalvesEqual has 1 pseudo / 2
spills and also does not match.

So `dump_alloc.py` is a valid criterion for comparing a function's OWN variants
(how many pseudos/spills form A produces versus form B), but there is no
cross-function threshold. No goal of the form "get spills below N" can be set.

The non-matching set's pseudo average is high (34.4 vs 7.0), but that is
misleading: those eight functions were deliberately chosen as the hardest
examples.

## Rule 70 — a base address in a local loads early; used directly it loads late

`FUN_08038608` (0x08038608) reads the same ROM table twice and the cartridge
places the two literal-pool loads differently:

```
first lookup : ldr r0,[pc,#48] / lsls r1,r4,#2 / adds r1,r1,r4 / lsls r1,r1,#4
tail lookup  : lsls r0,r4,#2 / adds r0,r0,r4 / lsls r0,r0,#4 / ldr r1,[pc,#8]
```

What decides it is whether the base passes through a local. Assigning the symbol
to one materialises the pool load at the assignment, ahead of the index scaling;
using the symbol directly in the expression leaves the load at its point of use,
after the scaling. Measured on that function, against 60 bytes: a local in both
lookups is 1 instruction off, no local in either is 6 off, and a local in the
first with a direct tail matches.

Two consequences worth carrying to other functions. First, an asymmetry like
this is evidence that the original source wrote two otherwise identical
expressions differently — do not "tidy" them into one shape. Second, in the twin
`FUN_08038644` the subscript and pointer forms of the same field read are not
interchangeable: `table[i].field` reintroduces the early load and is 6 off,
while `(table + i)->field` matches.

## Rule 67 — address locals look unnecessary but determine register allocation

When the three independent `u16` globals in `ResetRuntimeGlobals` were zeroed
directly, agbcc produced a leaf function and drifted 22 instructions from the
ROM. Holding the first two addresses in local pointers restored the `r4`/`r3`
lifetimes and the `{r4,lr}` prologue; the function became 204/204 byte-matching.
If the ROM uses a callee-saved register in consecutive global stores, an address
local must not be deleted merely because it is semantically unnecessary.

## Rule 51 — with two equal-priority variables, the shape of the flow decides the register

Two pseudos with the same reference count and the same lifetime fall into an
**exact tie** in rule 50's priority formula (`floor_log2(refs) * refs /
lifetime`). On a tie, agbcc orders by allocno number, so parameter order wins and
you **cannot break** the register assignment you want with lexical spelling
changes (flipping a comparison's operands, a `(u32)` cast, copying into a local,
changing declaration order) — 13 spellings were tried, nine produced the same
table.

What breaks the tie is **the shape of the control flow**: a single `&&` chain
keeps both values live up to the same basic block, whereas an early-exit
`if (...) return` chain extends the lifetime of the first-tested value by one
instruction and lowers its priority.

Measurement — `IsSlotValueInRange` @ 0x08065538:

| spelling | low pseudo | high pseudo | result |
|---|---|---|---|
| `a && b && c && d` as one expression | lifetime 13, priority 0.154 | lifetime 13, priority 0.154 | tie; low takes r4, **4-byte difference** |
| an early-exit `if` chain | lifetime 14, priority 0.143 | lifetime 13, priority 0.154 | high takes r4, **byte-matching** |

The diagnosis is one command: `dump_alloc.py --function <name>`. If the two
pseudos have equal priority, the problem is the shape of the flow, not the
spelling; try splitting the expression rather than sweeping lexical variants.

### Addendum to rule 49 — loop rotation is determined by where the flag is set

Measured on `WaitLinkSettle` @ 0x08066454. The same body with two spellings:

| spelling | layout produced |
|---|---|
| `for (;;) { check; if (!again) break; wait; }` | the check block first and the wait last; agbcc moves the second global's address **out of the loop** as a loop invariant (an extra register + push) |
| `again = 1; while (again) { check; ...; wait; }` | the wait block first, with a `b` jumping into the body on entry; the address is loaded **inside** the block — **byte-matching** |

The diagnostic sign: if the ROM loads a global's address repeatedly inside the
loop, that block is in the *rotated* part of the loop body in the ROM's source.
Setting the flag before the loop and writing `while (flag)` makes the rotation
match the ROM's.

## Rule 52 — a chained assignment keeps two targets live simultaneously

When writing the same value into two separate globals, the spelling **changes the
register allocation**:

```c
a = 0;              /* two separate expressions */
b = 0;
```
The local allocator puts each address into the **same** register immediately
before its own store and uses them in turn:
`ldr r0,=a / movs r1,#0 / strb r1,[r0] / ldr r0,=b / strb r1,[r0]`

```c
b = (a = 0);        /* one chain */
```
A single zero value is produced and **both addresses are live simultaneously**:
`ldr r2,=b / ldr r1,=a / movs r0,#0 / strb r0,[r1] / strb r0,[r2]`

Measurement — `ResetLinkSession` @ 0x08066144: an 8-byte difference with separate
expressions, **byte-matching** with the chain.

### This refutes a judgment I recorded earlier

When `dump_alloc` showed the two address constants as separate pseudos, both in
the local allocator and both in the same register, I parked it saying "they are
not competing globally, rule 50's priority does not apply, there is no source
lever." **That was wrong.** The lever exists; the diagnostic tool does not show
it, because the allocation table shows the *result*, not how many values the
expression produces in RTL. When you see two pseudos meeting in the local
allocator, do not park; first try merging the expression.

### How it was found

The permuter (`build/permuter/ResetLinkSession`, base score 230, score 0 in about
560 iterations). What rule 44 predicted: on small differences that a manual sweep
cannot close, the permuter finds the **mechanism**. The mechanism here was one
line long and generalizable, so it became a rule of its own.

## Rule 53 — the multiplication operand order is reversed in the source

agbcc loads the **second** operand of `a * b` first. Measured on
`LoadBitmapAsset` @ 0x08065574: the ROM generates `ldrh [r4,#14]` (height) then
`ldrh [r4,#12]` (width); the source that gives this is `width * height`, not
`height * width`. The reversed spelling drifts by 4 bytes, and in two places in
the same function.

Diagnosis: look at the ROM's load order and write **the reverse** in the source.

## Rule 54 — split a save/restore variable into blocks

When saving and restoring the same hardware register in two separate blocks, use
**two separate locals**. A single variable makes the live range span both blocks
and increases register pressure: in `LoadBitmapAsset` this pushed `dest` into a
callee-saved high register and added an extra push/pop pair — **12 bytes**. The
ROM keeps the second save in `ip`, i.e. as a separate, short-lived value.

## Rule 55 — take a heavily used `u8` field into a local first

If you use a `u8` struct field in more than one expression in the same block (for
instance both `x >> 1` and a comparison against `x`), direct member access makes
agbcc generate an unnecessary zero-extension pair:

```c
if (r->a >= r->b >> 1) ...      /* ldrb / lsls #24 / lsrs #24 / lsrs #25 */
if (r->a >= r->b) ...
```

Taking the value into a local first does not repeat the extension `ldrb` already
performs and gives the ROM's shape:

```c
altB = r->b;                    /* ldrb */
curA = r->a;                    /* ldrb */
if (curA >= altB >> 1) ...      /* lsrs #1 */
if (curA >= altB) ...
```

Measurement: `FUN_08067014` @ 0x08067014 gained two instructions and its
instruction sequence aligned with the ROM's. Note: the inverse branch also
exists — the ROM sometimes **re-reads** the same byte at every use; in that case
do NOT use a local, write direct member access. Read which case applies from the
ROM's `ldrb` count.

## Rule 56 — bitfield comparisons: masked for equality, unsigned for saturation

Measured on `BumpStepCounter` @ 0x08066B40, with two separate traps:

**The equality test.** agbcc never converts a bitfield equality test into a mask
comparison; because the field is promoted to `int`,
`optimize_bit_field_compare` never runs and `x.f == 19` always generates an
`lsl/lsr` extraction + `cmp`. If you see `ands r0,#MASK / cmp r0,#(VALUE<<SHIFT)`
in the ROM, then **the source itself** wrote the unshifted masked comparison —
i.e. a `union` giving both a bitfield and a raw view of the same address. The
bitfield view and the raw view share the same base register, as in the ROM.

**The saturation test.** `x.f > 20` generates a signed `ble`; if the ROM has
`bls`, the constant must be explicitly unsigned: `> (u32)20`. It is easy to miss
because the field is already declared `unsigned`.

Eliminated spellings: a struct with only bitfields + `field == 19` (316 bytes, 24
short); `> 20` with a plain `int` constant (`ble`, not `bls`).

## Rule 57 — `volatile` is used selectively on SIO registers

Measured on `SerialIrqHandler` @ 0x08066904. Three separate views are needed for
the same address, and which one is `volatile` is read from the ROM:

| access | correct form | cost of the wrong form |
|---|---|---|
| The SIOMLT_SEND write | **not volatile** | a volatile view generates a dead `ldrh` before the write |
| The first SIOCNT read | **not volatile** | volatile adds an extra `ldrh` + `adds` pair, +4 bytes |
| The `gBiosIrqFlags` update | **volatile** (`*(volatile u16 *)&gBiosIrqFlags`) | non-volatile loads the extern's constant from memory first and disrupts that block's register allocation |
| REG_IME | **volatile** | the non-volatile spelling breaks the match |

The SIOMULTI copy must also be **4-aligned**: a plain `u16 data[4]` is 2-aligned
and agbcc generates a `memcpy` call; a `union` containing `u32 word[2]` gives the
ROM's `ldr/ldr/str/str` pair.

Eliminated spellings (no effect): making the counter u16/u32/s32, declaring it
earlier, nested `if`s, initializing at the definition, `|=`, reversed operand
order, taking into a local, `(u16)` narrowing. Adding the constant with `+` and
writing IME without volatile **break** the match.

## Rule 58 — the hoist order in a loop preheader can conflict with block layout

Measured on `WaitForPartner` @ 0x0806620C: 134/139 instructions identical, the
remaining 5 being only **the order in which** loop invariants (constants and
global addresses) are hoisted into the preheader.

The hoist order follows the **use** order in the source. But in this function two
requirements are mutually exclusive:

| spelling | result |
|---|---|
| `if (armed == 0) {B} else {A}` | the ROM's block layout is correct, the hoist order reversed → 12-byte difference |
| `if (armed != 0) {A} else {B}` | the hoist order is correct, but the compiler lifts B above the loop and adds an entry jump → 316 bytes |

Manual hoisting (`bit1 = 2; irq = &gBiosIrqFlags;`) fully closes the first loop
(8/320) — proving that source-level variable initialization is emitted BEFORE
`loop.c`'s hoists — but in the second loop it frees a register, causes the
constant to be hoisted too, and moves it into r8 (336/344). Besides, `bit1 = 2`
is not defensible source, so it was not taken.

## Rule 59 — taking a mask result into a `u8` local prevents merging

Measured on `StepLinkFrame` @ 0x0806660C. If you take a mask result into a `u32`
local, agbcc's `regmove` pass **merges** the constant pseudo with the AND result
and a single instruction comes out:

```c
u32 m = cnt & 0x30;      /* movs r0,#0x30 / ands r0,r6 */
u8  m = cnt & 0x30;      /* movs r1,#0x30 / adds r0,r6,#0 / ands r0,r1  <- ROM */
```

A `u8` local makes the constant a QImode pseudo, merging does not happen, and the
ROM's three-instruction form appears. If the mask is in the low byte, no extra
narrowing is generated either. (Confirmed in the RTL dumps: `combine` gives the
ROM's form, and the naming is broken in `regmove`.)

Three additional points measured in the same function:
- Clearing **a single bit** in a hardware register must be a `bitfield`
  assignment. `&= ~0x40` narrows the mask to 8 bits and generates `movs #0xBF`;
  a bitfield builds it at 32 bits like the ROM (`movs #65 / negs`).
- Taking an if/else's result into an intermediate variable and assigning
  afterwards preserves the ROM's `adds r3,r0,#0` copy at the join point. Assigning
  directly makes agbcc cross-jump the two arms' common final `orr`.
- Use **three separate local pointers** for three separate regions of the same
  struct; one shared variable inflates the reference count and inverts the r4/r5
  allocation.

## Rule 60 — a range guard is written as two separate `if`s, not with `||`

Measured on `GetStepIconId` @ 0x08066ED0 — the project's **first jump-table**
match (36 states, table at 0x08066EF0).

```c
if (n < 0 || n > 35) return X;      /* agbcc folds the two into ONE unsigned
                                       `cmp r0,#35 / bls` and merges it with
                                       the switch's own range check
                                       -> 312 bytes, 12 short */

if (n < 0)  return X;               /* signed `cmp #0 / blt` */
if (n > 35) return X;               /* signed `cmp #35 / ble`; the two
                                       `return`s merge into one body via
                                       cross-jumping, and the switch generates
                                       its own independent `cmp #0x23 / bls`
                                       -> 324/324 BYTE-MATCHING */
```

The parameter must be **signed** (`s32`): the guard's `blt`/`ble` require it. The
switch's own check, as agbcc always generates for jump tables, is unsigned
(`bls`).

Also confirmed: in this ROM, Ghidra's low line density plus a "Could not recover
jumptable" warning means **a jump table**. The table entries are disassembled as
`ldr`/`lsrs` garbage; verify by reading code addresses at 4-byte intervals from
the ROM.

## Rule 61 — a bitfield's container determines the narrowest access that FITS the field

Measured on `GetRecordField` @ 0x08066D54. Declaring the same bit sequence with a
`u16` or a `u32` container generates different instructions, because agbcc picks,
for each field, **the narrowest access that fully contains it**:

- If the field does not cross a byte boundary → `ldrb`
- If it is within a halfword but crosses a byte boundary → `ldrh`
- If it also crosses a halfword boundary → `ldr` (a full word)

So the container's width is determined by *the widest field in that container*.
At `RecordData+0x2C` there is a field spanning bits 11–16; because no halfword
contains it, the container **must be `u32`**. The other fields in the same
container still get their own narrowest accesses (`ldrb` 0x2E, `ldrh` 0x2E,
`ldrb` 0x2F).

The sibling file `bump_rank_counter.c` defines the same bits with a `u16`
container and stays CORRECT there — because that function never reads the 6-bit
overflowing field. So the container width can vary by file; the criterion is the
ROM's load width in that function.

An additional measurement: the Thumb `ldrb` imm5 offset ends at 31, so every byte
field at an offset above 0x20 requires the triple
`adds r0,r2,#0 / adds r0,#N / ldrb` — this is not a source oddity but an
instruction set limit. Because the `ldrh` offset is scaled by 2, halfwords are
loaded directly.

Also: switch bodies are laid out in the ROM in **source order**, not table order.
To capture the ROM's block order, write the `case`s in the ROM's body order.

## Rule 62 — do not use a volatile view when WRITING to a hardware register

Measured on `ShutdownAndReset` @ 0x08065650, the confirmed form of rule 57 for
DMA control:

```c
REG_DMA0.control = REG_DMA0.control & MASK;   /* volatile DmaRegs:
                                                 ldrh / and / ldrh(dead) / strh */
```

Writing through a volatile view generates **a dead `ldrh`** before the `strh`.
Define two views for the address — one volatile for reads (and deliberate empty
reads), one non-volatile for writes:

```c
#define DMA_W(n)  (*(DmaRegs *)(0x040000B0 + (n) * 12))          /* write */
#define REG_DMA(n) (*(volatile DmaRegs *)(0x040000B0 + (n) * 12)) /* read  */
```

8 dead loads across four channels = **16 bytes**; the difference dropped from 139
to 4 bytes.

## Rule 63 — an `a = b = 0` chain produces the constant AFTER the address

The last 4 bytes of the same function. The ROM generates
`ldr r0,=IME / movs r5,#0 / strh`: the address first, then the constant. A
separate `zero = 0;` statement produces the constant **first**.

```c
zero = 0;  REG_IME = zero;    /* constant first -> diverges from the ROM */
REG_IME = zero = 0;           /* the constant as the outer assignment's RHS,
                                 AFTER the LHS address -> the ROM's form */
```

The sibling of rule 52 (`b = (a = 0)` keeps two addresses live simultaneously): a
chained assignment determines not only liveness but also **production order**.
Eliminated: writing all the zeros as plain constants (168 bytes, 137 differences —
zero is no longer kept in r5).

## Rule 64 — advancing a pointer can split a base constant's lifetime

`FUN_080657d8`'s TX tags, 2026-09-07. In the `PackLocalLinkTag` helper, which
packs values at the same index from two parallel byte rings, writing:

```c
entry = gRam020003C0 + index;  /* a single expression */
```

instead as:

```c
entry = gRam020003C0;
entry += index;               /* advance from the base to the relevant record */
```

keeps the base load separate and short-lived in both inline expansions. `entry` is
not merely a base alias: its value changes and the byte of the record it advances
to is read. The helper masks the ring index with 31 and turns two bytes into a
`u16` tag. No new side-effecting call or `volatile` access is introduced.

| source form | size | identical instructions |
|---|---:|---:|
| The earlier two direct packing expressions | 2356 | 575/1174 |
| A one-step pointer inside the helper | 2356 | 573/1174 |
| Two-step advancement inside the helper | 2352 | **669/1174** |

In the initial `.lreg/.greg` dumps, the TX block's `gRam020003C0` base is p1012:
L79, 6 references, a 20-instruction lifetime, **r4**. In the retained spelling its
place is taken by two separate local pseudos, p1005 and p1032, each with 8
references / an 8-instruction lifetime, in **r1**. The `gRam02000E80` base stays
in r3. As a result, k's p44 allocation moves from r5 to **r4**; the TX pointer
becomes r5, cur r6, and the key ring's base copy r7. The old handover document's
identification of the symbol in r4 as `gRam02000E80` was wrong.

This result **does not show that the whole function matches**: a 505-instruction
difference persists, and the two address additions in TX still use different
registers. Getting k=r4 like the ROM was necessary progress, but not sufficient on
its own. Despite the size moving away from the ROM's, there is a net gain of 94
instructions.

Reproduction: `python3 tools/probe_sio_tx.py`. The tool compiles three candidates
in a temporary directory without modifying the source file, computes the same
score with `diff_function.py`, and verifies that the external call targets and
counts have not changed. The full-match gate is still
`make c-match FILE=src/world/sio_driver.c`.

## Rule 65 — hardware addresses: an absolute macro is invisible to CSE, a pointer variable is not

When the same address is written in two different forms, agbcc generates different
code, because **whether the address constant becomes a pseudo-register** changes:

- `#define R (*(vu16 *)0x04000008)` → the address stays a MEM address, common
  subexpression elimination (CSE) does not see it, and it gets its own pool entry.
- `vu16 *p = (vu16 *)0x04000008;` → the address enters a pseudo-register; if
  another nearby constant exists, CSE derives it from that one.

Measurement (`0x080127A8`, SetupBg0Bg1): the function first writes `REG_DISPCNT`
(0x04000000, generated with `movs #128 / lsls #19`), then BG0CNT (0x04000008).
With a pointer variable, agbcc derived the second as `adds r1,#8` and the
`0x04000008` entry disappeared from the pool (80 bytes, ROM 84). With the absolute
macro the difference is zero.

This does not mean **turning CSE off**: the same ROM derives BG1CNT (0x0400000A)
as base+2 by itself. What is decisive is not whether CSE happens but **where it
derives from**.

The converse also holds — see rule 66: if the ROM shows base+displacement
addressing (`strh r0,[r1,#10]`), an absolute macro will not work, because the
macro folds the displacement into the address constant and puts the wrong word in
the pool.

## Rule 66 — writing to a `volatile struct` member generates an extra read

The `REG_DMA1.control` form in `include/gba_io.h` (i.e.
`(*(volatile DmaRegs *)ADDR).control = ...`) generates **an extra volatile read
before every write**. Doing the same job with a `vu16 *` base + index does not.

Measurement (`0x080337A8`, StopAudioDmaOnCartFlag; 2 writes per channel):

| spelling | reads/writes | identical instructions |
|---|---|---:|
| `REG_DMA1.control = M & REG_DMA1.control;` | 5 reads / 2 writes | 48/63 |
| `v = REG_DMA1.control; REG_DMA1.control = M & v;` | 5 reads / 2 writes | 48/63 |
| `REG_DMA1.control &= M;` | 5 reads / 2 writes | 48/63 |
| `p1[5] = M & p1[5];` (`vu16 *p1`) | **3 reads / 2 writes** | **match** |

The ROM has 3 reads / 2 writes. That all three struct spellings give the same
score shows the difference comes not from the source spelling but from **the view
type**.

The **macro** `((vu16 *)0x040000BC)[5]` is not the solution: the macro folds the
`+10` into the address constant and puts `0x040000C6` in the pool, making the
displacement 0; in the ROM the base is `0x040000BC` and the displacement is 10.
So the base must be a **pointer variable**.

With two channels, two separate variables are needed, and **the second must be
assigned at its own point of use**:

- if both are assigned at the top, both pool loads are gathered at the top of the
  function (the ROM loads the second at its point of use),
- if a single variable is assigned twice, agbcc derives the second as the first's
  +12 (`adds r4,#12`) — the same as rule 65.
