# `gRam02025810` (0x02025810) — an evidence-based structure layout proposal

This document is derived from the ROM. No file in the repository was modified;
`data/ram_map.csv` and `include/ram_symbols.h` were left as they are. Applying
the proposal is a human decision.

## 0. Method and scope

The whole ROM was scanned — not just the 0x080308EC–0x08031D24 band.

**Narrowing:** rather than disassembling each of the 1934 functions in
`data/functions.csv` one by one, **every 4-byte-aligned word** in `baserom.gba`
was scanned, and those whose value falls in `[0x02025810, 0x02025810+0x1500)`
were taken as literal pool entries. Because Thumb code can only reach an EWRAM
address through a literal pool, this scan is **complete** (nothing escapes it).
Such a pool word was found in the bodies of 120 functions.

Those 120 functions were disassembled with `arm-none-eabi-objdump` and subjected
to abstract interpretation over registers: the base register loaded from the pool
was tracked and propagated across `adds/subs #imm`, `movs #imm`, `lsls #n`,
`add rD, rN`, high-register `mov`s, and branch targets; the block-relative offset
was recorded at every `ldr/ldrh/ldrb/ldrsb/ldrsh/str/strh/strb` access.

**508 accesses, 72 distinct offsets, and 101 functions** derived from the exact
base (0x02025810) were found. State distribution: 73 `candidate`, 19
`discovered`, 8 `matching`, 1 `decompiled`.

### Two systematic errors corrected during the analysis

They are written here because anyone examining this by hand will make the same
mistakes:

1. **`ldrsb`/`ldrsh` with a constant index is NOT an array.** Thumb has no
   immediate-offset encoding for `ldrsb`/`ldrsh`; the compiler emits
   `movs r0, #10` + `ldrsb r0, [r7, r0]`. In the first scan 47 of these appeared
   to be "an array at offset 0". In reality they are **signed scalar fields**,
   and they are the only source of signedness information within the block.

2. **`lsls #5` is constant generation, not an array stride.**
   `movs r2, #155; lsls r2, r2, #5` = 155 × 32 = 4960 = **+0x1360**. This idiom
   occurs in 34 places and was mistaken for "an array with a stride of 32" in the
   first scan. All of them are scalar accesses at a constant offset.

In addition, `0x5810` — the lower half of the word 0x02025810 — is a valid Thumb
opcode (`ldr r0, [r2, r0]`). Because objdump decoded pool data as code, it
produced six spurious "indexed accesses"; those have been eliminated.

---

## 1. Measured minimum size: **0x1390 = 5008 bytes**

The recorded value of 5006 is **two bytes short**, but the reasoning in the
record was correct.

**Firm evidence — the call that zeroes the block in one piece** (`FUN_080296A0`,
0x080296B0):

```
80296b0:  4f85      ldr  r7, [pc, #532]  @ (0x80298c8)  = 0x02025810   <- base
80296b2:  4a86      ldr  r2, [pc, #536]  @ (0x80298cc)  = 0x00001390   <- length
80296b4:  1c38      adds r0, r7, #0
80296b6:  2100      movs r1, #0
80296b8:  f044 fb02 bl   0x806dcc0  <Memset>
```

The `Memset(dest=r0, value=r1, count=r2)` signature is documented in
`data/functions.csv` (0x0806DCC0). So this is
**`Memset(gRam02025810, 0, 0x1390)`**.

Two points strengthen this evidence:

- The constant `0x00001390` occurs **exactly once in the entire ROM**
  (at 0x080298CC), and that occurrence is this call's length argument. There is
  no chance of confusing it with another object.
- An independent lower bound: the highest byte touched is **+0x138D**, in
  `FUN_0802DF18` at 0x0802DFF4 via `strb r0, [r1, #0]` (r1 = base + 0x138D).
  That alone gives ≥ 0x138E, which rounds up to 0x1390 at 4-byte alignment. The
  two pieces of evidence confirm each other.

The neighboring symbol also supports the bound not being exceeded: the next named
symbol is `gRam02026CD0` = base + 0x14C0, so there is a 0x130-byte unnamed gap
after 0x1390 — no overlap.

> **Note:** this is a *measured* size, not a *minimum*. Memset zeroes the entire
> block, so 0x1390 is both a lower and an upper bound.

---

## 2. Access table

Width: `B`=1 byte, `H`=2 bytes, `W`=4 bytes. Sign: `S` = an `ldrsb`/`ldrsh` was
seen, `U` = unsigned accesses only. "Fn" = the number of distinct functions
accessing that offset.

### Region A — 0x0000–0x003B: dense player state (scalars)

| Offset | Width | Sign | Array? | Fn | Accesses | Example functions | Confidence |
|---|---|---|---|---|---|---|---|
| +0x00 | B | **S** | no | 10 | 19 | FUN_080296a0, FUN_0802a3f0, FUN_080309d4, ReleaseSlotHeld | High |
| +0x02 | H | **S** | no | 3 | 4 | FUN_080296a0, FUN_0802a480, FUN_08030a3c | High |
| +0x04 | B (+H) | **S** | no | 13 | 26 | FUN_08030114, FUN_08030dd8, HalvesEqual | High (conflict: §5.1) |
| +0x05 | B | **S** | no | 11 | 18 | FUN_08029e44, FUN_0802fe44, FUN_08030b88 | High |
| +0x06 | B (+H) | **S** | no | 4 | 8 | FUN_08029e44, FUN_0802ab18, FUN_0805ee84 | Medium (conflict: §5.1) |
| +0x07 | B | **S** | no | 3 | 8 | FUN_08029e44, FUN_0802ab18, FUN_08030114 | Medium |
| +0x08 | B | **S** | no | 6 | 12 | FUN_08029518, FUN_0802ac40, FUN_0802fe44 | High |
| +0x09 | B | **S** | no | 6 | 12 | FUN_08029518, FUN_0802ac40, FUN_08030114 | High |
| +0x0A | B | **S** | no | 8 | 10 | FUN_0802a5b4, FUN_08031004, FUN_08031054 | High |
| +0x0C | W | U | no | 9 | 17 | CopySrcToDest, FUN_08029e44, FUN_0802a858 | High |
| +0x10 | H | **S** | no | 5 | 13 | FUN_080296a0, FUN_0802f55c, FUN_08031134 | High |
| +0x12 | H | **S** | no | 5 | 12 | FUN_080296a0, FUN_08030114, FUN_080310c0 | High |
| +0x14 | W | U | no | 10 | 15 | RunMenuScreen, CaptureSessionSnapshot, FUN_08030ae4 | High |
| +0x18 | H | **S** | no | 5 | 10 | FUN_08004f74, FUN_08029e44, FUN_08030a60 | High |
| +0x1A | H | **S** | no | 4 | 8 | FUN_080296a0, FUN_08030114, FUN_08030a9c | High |
| +0x1C | B (+H) | U | no | 10 | 12 | **SetHudTime**, HalvesEqual, FUN_08030dd8 | High (conflict: §5.1) |
| +0x1D | B | U | no | 9 | 11 | **SetHudTime**, FUN_0802fe44, FUN_0805e700 | High |
| +0x20 | W | U | no | 3 | 6 | FUN_080296a0, FUN_08029e44, FUN_0802ac40 | Medium |
| +0x24 | B | U | no | 1 | 2 | FUN_08029e44 | Low |
| +0x28 | W | U | no | 1 | 1 | FUN_080296a0 | Low |
| +0x2C | B | U | no | 3 | 5 | FUN_080296a0, FUN_08029e44, FUN_0802a5b4 | Medium |
| +0x2D | B | **S** | no | 5 | 7 | FUN_0802f55c, FUN_08031054, FUN_0802a5b4 | Medium |
| +0x2E | B | U | no | 3 | 3 | FUN_0802a5b4, FUN_0802f55c, FUN_08031054 | Medium |
| +0x30 | W | U | no | 4 | 7 | FUN_08029a88, FUN_08029b3c, FUN_080313f0 | Medium |
| +0x34 | W | U | no | 2 | 2 | FUN_080296a0, FUN_0802bdf0 | Low |
| +0x38 | H | U | no | 2 | 3 | FUN_080296a0, FUN_0802bdf0 | Low |
| +0x3A | H | U | no | 2 | 3 | FUN_080296a0, FUN_0802bdf0 | Low |

### Region B — 0x003C–0x111B: **a 24-entry, 180-byte slot array** (§3)

The offsets below are the fields of the array's **element 0**; all of them are
reached with a register index of the form `base + 180*i + K`.

| Block offset | Element offset | Width | Array? | Fn | Example functions | Confidence |
|---|---|---|---|---|---|---|
| +0x3C | +0x00 | B | **YES** | 4 | FUN_080296a0, FUN_0802b394, FUN_0802b484 | High |
| +0x40 | +0x04 | W | **YES** | 3 | FUN_080296a0, FUN_0802b394, FUN_0803095c | High |
| +0x44 | +0x08 | W | **YES** | 3 | FUN_080296a0, FUN_0802b394, FUN_0803095c | High |
| +0x48 | +0x0C | W | **YES** | 2 | FUN_080296a0, FUN_0802b394 | High |
| +0x4C | +0x10 | W (ptr) | **YES** | 3 | **ReleaseSlot**, FUN_080296a0, FUN_0802b394 | High |
| +0x50 | +0x14 | W (+B) | **YES** | 7 | ReleaseSlot, FUN_0802b4e8, FUN_0803095c | High (conflict: §5.2) |
| +0x54 | +0x18 | W | **YES** | 3 | FUN_080296a0, FUN_0802b394, FUN_0802b4e8 | High |
| +0x58 | +0x1C | address taken | **YES** | 4 | FUN_080299b0, FUN_0802b484, ReleaseSlotHeld | Medium |
| +0xA0 | +0x64 | address taken | **YES** | 1 | FUN_080299b0 | Low |
| +0xE8 | +0xAC | W | **YES** | 3 | FUN_080296a0, FUN_0802b394, FUN_0802b4e8 | High |
| +0xEC | +0xB0 | B | **YES** | 5 | FUN_080296a0, FUN_080299b0, FUN_0802b4e8 | High |
| +0xED | +0xB1 | B | **YES** | 3 | FUN_080296a0, FUN_080299b0, FUN_0802b484 | High |
| +0xEE | +0xB2 | H | **YES** | 2 | FUN_0802b394, **FUN_08030d0c** | Medium |

### Region C — 0x111C–0x114B: 12 consecutive u32s

Twelve words, fully consecutive with no gaps. All accessed only via `ldr`/`str`.

| Offset | Fn | Accesses | Example functions | Confidence |
|---|---|---|---|---|
| +0x111C | 2 | 8 | FUN_08029c20, FUN_0802df18 | High |
| +0x1120 | 2 | 3 | FUN_08029c20, FUN_0802df18 | Medium |
| +0x1124 | 2 | 3 | FUN_0802df18, FUN_0802e9e4 | Medium |
| +0x1128 | 6 | 9 | FUN_0802e048, FUN_0802f55c, **FUN_08031034** | High |
| +0x112C | 4 | 6 | FUN_0802b01c, FUN_0802ead4, **ResetMapView** | High |
| +0x1130 | 5 | 5 | FUN_080296a0, FUN_0802e048, **ResetMapView** | High |
| +0x1134 | 1 | 3 | FUN_0802b01c | Low |
| +0x1138 | 2 | 3 | FUN_0802af40, **FUN_08030d4c** | Medium |
| +0x113C | 3 | 4 | FUN_0802af40, FUN_0802e9e4, **FUN_08031204** | Medium |
| +0x1140 | 2 | 2 | FUN_0802e9e4, **FUN_08031204** | Medium |
| +0x1144 | 2 | 4 | FUN_08029d44, FUN_0802af40 | Medium |
| +0x1148 | 3 | 8 | FUN_080296a0, FUN_08029d44, FUN_0802af40 | High |

### Region D — 0x114C–0x134B: a 512-byte buffer (§3)

No scalar accesses. In `FUN_08029E44`, `base+0x114C` (= 0x0202695C) is passed
**as an address** to a graphics routine. 0x114C–0x134C is exactly
**0x200 = 512 bytes**.

### Region E — 0x134C–0x138F: counters and flags

| Offset | Width | Sign | Fn | Accesses | Example functions | Confidence |
|---|---|---|---|---|---|---|
| +0x134C | W | U | 4 | 6 | FUN_080296a0, FUN_08029918, FUN_08029e44 | High |
| +0x1358 | W | U | **30** | 32 | **StepThenCheck**, FUN_08030d84, ResetMapView | **Very high** |
| +0x135C | W | U | 2 | 2 | FUN_080296a0, FUN_0802b01c | Low |
| +0x1360 | W | U | **28** | 38 | FUN_08030458, **FUN_08030d4c**, **FUN_08030db4** | **Very high** |
| +0x1364 | W | U | 7 | 8 | FUN_08029918, FUN_0802e048, **ResetMapView** | High |
| +0x1368 | W | U | 5 | 6 | FUN_0802df18, FUN_0802ead4, **ResetMapView** | High |
| +0x136C | W | U | 5 | 5 | FUN_0802b01c, FUN_0802e048, **ResetMapView** | High |
| +0x1370 | H | U | 1 | 3 | FUN_08029e44 | Low |
| +0x1373 | B | U | 2 | 6 | FUN_080296a0, FUN_0802eb50 | Medium |
| +0x1374 | B | U | 2 | 2 | FUN_0802f55c, **FUN_080310c0** | Medium |
| +0x1375 | B | U | 1 | 1 | FUN_08029e44 | Low |
| +0x1376 | B | U | 2 | 4 | FUN_080296a0, FUN_08029e44 | Medium |
| +0x1377 | B | U | 2 | 3 | FUN_08029e44, FUN_08032318 | Medium |
| +0x1378 | W | U | 3 | 3 | FUN_080296a0, FUN_0802e3fc, FUN_0802f55c | Medium |
| +0x137C | B | U | 4 | 9 | FUN_0802de70, FUN_0802ead4, **ResetMapView** | High |
| +0x137D | B | U | 2 | 8 | **BumpStepCounter**, FUN_08029e44 | High |
| +0x137E | B | U | 2 | 5 | **CleanupAreaTiles**, ShowLevelBadge | High |
| +0x1380 | H | U | 4 | 6 | FUN_08029c20, FUN_0802af40, FUN_0802df18 | High |
| +0x1382 | H | U | 2 | 8 | FUN_08029c20, FUN_08029d44 | Medium |
| +0x1384 | H | U | 4 | 6 | FUN_08029c20, FUN_0802af40, FUN_0802df18 | High |
| +0x1388 | W | U | 1 | 2 | FUN_0802bdf0 | Low |
| +0x138C | B | U | 1 | 1 | FUN_0802af40 | Low |
| +0x138D | B | U | 1 | 1 | FUN_0802df18 (0x0802DFF4) | High (size evidence) |

---

## 3. Array and buffer geometry (register-indexed offsets)

The task description asks about the rule `lsls #1` → u16, `lsls #2` → u32.
**That pattern does not occur at all in this block.** The only genuine array is
the 180-byte slot array addressed with `muls`; all 34 apparent `lsls #5` sites
are constant generation (§0).

### The slot array: base+0x3C, stride 180 (0xB4), 24 elements

The stride and element count are read directly from the code (`ReleaseSlot`,
0x080308AC):

```
80308b0:  2917      cmp  r1, #23          <- index upper bound 23  => 24 elements
80308b4:  4d0b      ldr  r5, [pc, #44]    = 0x02025810
80308b6:  20b4      movs r0, #180  @ 0xb4 <- stride 180
80308ba:  4343      muls r3, r0
80308be:  304c      adds r0, #76   @ 0x4c <- element + 0x10
80308c0:  181c      adds r4, r3, r0
```

`FUN_080296A0` walks the same array with
`base + 180*i + {60,64,68,72,76,80,84,88,232,236,237}`. Because 236 and 237
exceed 180, the **element base cannot be 0x4C**; the only consistent solution is
an element base of **0x3C**:

- 236 − 60 = 176 = element+0xB0 ✓ (within 180)
- 237 − 60 = 177 = element+0xB1 ✓
- 238 − 60 = 178 = element+0xB2, `strh` → element+0xB2..0xB3 ⇒ **the element
  closes exactly at 0xB4 = 180** ✓

Both ends of the array close independently — the strongest evidence for the
layout:

```
0x3C + 24 × 180 = 0x3C + 0x10E0 = 0x111C
```

and **+0x111C** is the first byte accessed at a constant offset after the array
ends. So the "4142-byte gap between +0xEE and +0x111C" is not a gap at all; it is
elements 1 through 23 of the array.

### The 512-byte buffer: base+0x114C

`FUN_08029E44` (0x0802A25C) loads `0x0202695C` = base+0x114C from the pool and
passes it as an address. 0x114C + 0x200 = **0x134C**, which is Region E's first
scalar. This region also closes from both ends.

### Other register-indexed accesses

None. Every access in Regions A, C, and E is at a constant offset.

---

## 4. Proposed C structure

Every unknown region is **padding**; there are no invented fields. Field names
are deliberately neutral (`unkNN`), except for the three fields whose meaning is
known. Naming is a separate task; this document only fixes the layout.

```c
/* gRam02025810 — 0x02025810, the player progress block.
 *
 * MEASURED SIZE: 0x1390 = 5008 bytes.
 *   Evidence: FUN_080296A0 @0x080296B8 -> Memset(gRam02025810, 0, 0x1390).
 *   The constant 0x00001390 occurs only there in the entire ROM.
 *   Independent lower bound: FUN_0802DF18 @0x0802DFF4 strb -> base+0x138D.
 *
 * Signedness is claimed only where an ldrsb/ldrsh was observed. A field marked
 * "u" does NOT mean "proven unsigned"; it means "no signed access was seen".
 */

/* Slot array element — 180 bytes. Stride measured from the muls in ReleaseSlot. */
typedef struct ProgressSlot {
/* +0x00 */ u8   unk00;
/* +0x01 */ u8   pad01[3];
/* +0x04 */ u32  unk04;
/* +0x08 */ u32  unk08;
/* +0x0C */ u32  unk0C;
/* +0x10 */ void *held;        /* ReleaseSlot: object pointer, its +0x0C flag is cleared */
/* +0x14 */ u32  unk14;        /* CONFLICT: u32 via str/ldr, ldrb in two places — see §5.2 */
/* +0x18 */ u32  unk18;
/* +0x1C */ u8   unk1C[0x90];  /* +0x1C and +0x64 are address-taken; contents UNKNOWN */
/* +0xAC */ u32  unkAC;
/* +0xB0 */ u8   unkB0;
/* +0xB1 */ u8   unkB1;
/* +0xB2 */ u16  unkB2;
} ProgressSlot;                /* sizeof == 0xB4 == 180 */

typedef struct Progress {
    /* ---- Region A: dense player state ---- */
/* +0x0000 */ s8   unk00;
/* +0x0001 */ u8   pad01;
/* +0x0002 */ s16  unk02;
/* +0x0004 */ s8   unk04;      /* the +0x04/+0x05 pair is read as u16 in places — §5.1 */
/* +0x0005 */ s8   unk05;
/* +0x0006 */ s8   unk06;      /* the +0x06/+0x07 pair is read as u16 in places — §5.1 */
/* +0x0007 */ s8   unk07;
/* +0x0008 */ s8   unk08;
/* +0x0009 */ s8   unk09;
/* +0x000A */ s8   unk0A;
/* +0x000B */ u8   pad0B;
/* +0x000C */ u32  unk0C;
/* +0x0010 */ s16  unk10;
/* +0x0012 */ s16  unk12;
/* +0x0014 */ u32  cash;       /* menu_screen.c: compared against the mission bail-out fee */
/* +0x0018 */ s16  unk18;
/* +0x001A */ s16  unk1A;
/* +0x001C */ u8   hudMinutes; /* SetHudTime 0x08030B60: clamped to 0..99 */
/* +0x001D */ u8   hudSeconds; /* SetHudTime 0x08030B60: clamped to 0..59 */
/* +0x001E */ u8   pad1E[2];
/* +0x0020 */ u32  unk20;
/* +0x0024 */ u8   unk24;
/* +0x0025 */ u8   pad25[3];
/* +0x0028 */ u32  unk28;
/* +0x002C */ u8   unk2C;
/* +0x002D */ s8   unk2D;
/* +0x002E */ u8   unk2E;
/* +0x002F */ u8   pad2F;
/* +0x0030 */ u32  unk30;
/* +0x0034 */ u32  unk34;
/* +0x0038 */ u16  unk38;
/* +0x003A */ u16  unk3A;

    /* ---- Region B: the slot array, 0x003C..0x111C ---- */
/* +0x003C */ ProgressSlot slots[24];      /* 24 * 180 = 4320 = 0x10E0 */

    /* ---- Region C: 12 consecutive u32s, 0x111C..0x114C ---- */
/* +0x111C */ u32  unk111C;
/* +0x1120 */ u32  unk1120;
/* +0x1124 */ u32  unk1124;
/* +0x1128 */ u32  unk1128;
/* +0x112C */ u32  unk112C;
/* +0x1130 */ u32  unk1130;
/* +0x1134 */ u32  unk1134;
/* +0x1138 */ u32  unk1138;
/* +0x113C */ u32  unk113C;
/* +0x1140 */ u32  unk1140;
/* +0x1144 */ u32  unk1144;
/* +0x1148 */ u32  unk1148;

    /* ---- Region D: a 512-byte buffer, passed as an address ---- */
/* +0x114C */ u8   buffer114C[0x200];      /* FUN_08029E44 @0x0802A25C */

    /* ---- Region E: counters and flags ---- */
/* +0x134C */ u32  unk134C;
/* +0x1350 */ u8   pad1350[8];             /* UNTOUCHED */
/* +0x1358 */ u32  pending;                /* step_then_check.c; read by 30 distinct functions */
/* +0x135C */ u32  unk135C;
/* +0x1360 */ u32  unk1360;                /* read by 28 distinct functions */
/* +0x1364 */ u32  unk1364;
/* +0x1368 */ u32  unk1368;
/* +0x136C */ u32  unk136C;
/* +0x1370 */ u16  unk1370;
/* +0x1372 */ u8   pad1372;                /* UNTOUCHED */
/* +0x1373 */ u8   unk1373;
/* +0x1374 */ u8   unk1374;
/* +0x1375 */ u8   unk1375;
/* +0x1376 */ u8   unk1376;
/* +0x1377 */ u8   unk1377;
/* +0x1378 */ u32  unk1378;
/* +0x137C */ u8   unk137C;
/* +0x137D */ u8   stepWarnFlag;            /* link_state_step.c / BumpStepCounter */
/* +0x137E */ u8   unk137E;                 /* CleanupAreaTiles */
/* +0x137F */ u8   pad137F;                 /* UNTOUCHED */
/* +0x1380 */ u16  unk1380;
/* +0x1382 */ u16  unk1382;
/* +0x1384 */ u16  unk1384;
/* +0x1386 */ u8   pad1386[2];              /* UNTOUCHED */
/* +0x1388 */ u32  unk1388;
/* +0x138C */ u8   unk138C;
/* +0x138D */ u8   unk138D;                 /* highest byte touched */
/* +0x138E */ u8   pad138E[2];              /* padding up to the Memset length */
} Progress;                                 /* sizeof == 0x1390 == 5008 */
```

> **Compatibility warning with existing sources:** `src/world/step_then_check.c`
> and `src/world/area_cleanup.c` currently use the name `Progress` with **their
> own local, short** definitions (`->pending`, `->pendingCleanup`, `->unk14`,
> `->cash`). If the full structure above is placed in a shared header, those
> local definitions will conflict and must be removed. It must also be verified
> whether `pendingCleanup` in `area_cleanup.c` and `pending` in
> `step_then_check.c` are **at the same offset** (both +0x1358) — this document
> treats them as one field.

---

## 5. Conflicts and uncertainties I could not resolve

### 5.1 Two u8s read as a u16 — in three places

This is **not a real conflict but a real overlap**; how to write it in C is a
human decision (two `u8`s, a `union`, or a `u16`).

| Offset | Byte accesses | Half-word accesses |
|---|---|---|
| +0x04 / +0x05 | 17 `strb`, 3 `ldrb`, 3 `ldrsb` (13 functions) | 3 `ldrh` — `HalvesEqual` (0x08030BEE), FUN_0805EFF0, FUN_080625A0 |
| +0x06 / +0x07 | 4 `strb`, 1 `ldrb`, 3 `ldrsb` | 1 `ldrh` — FUN_0805EE84 (0x0805EF0E) |
| +0x1C / +0x1D | 10 `strb`, 1 `ldrb` (`SetHudTime` writes both) | 1 `ldrh` — `HalvesEqual` (0x08030BF2) |

`SetHudTime` writes the minutes to +0x1C and the seconds to +0x1D with separate
`strb` instructions, so **they are certainly two separate u8s**. `HalvesEqual`
masks the pair with 0xFFFF and compares it with a single `ldrh` — most likely the
compiler's merged form of an "are both zero" test.

I wrote them as **two separate `s8`/`u8`s** in the structure above, because the
write side is unambiguously byte-based. But `src/world/halves_equal.c` already
reads a `u16`; a `union` may be needed there for byte-matching. **I am leaving
this to a human decision.**

### 5.2 `ProgressSlot.unk14` (block +0x50): u32 or u8?

The most uncomfortable conflict. The same offset:

```
ReleaseSlot   0x080308D8  str  r1, [r0, #0]     <- u32 written (zero)
FUN_0802B394  0x0802B456  str  r2, [r1, #0]     <- u32
FUN_0802B4E8  0x0802B6E8  ldr  r1, [r0, #0]     <- u32
FUN_080299B0  0x08029A10  ldrb r0, [r0, #0]     <- u8 !
FUN_0802B484  0x0802B496  ldrb r5, [r0, #0]     <- u8 !
```

The writing side is always u32, and two of the reading sides are u8. There are
two plausible explanations, and **I cannot distinguish them from ROM evidence**:

- The field is a u32, and two places test only its low byte as a flag/boolean (on
  little-endian, `ldrb` yields the low byte, which is enough for a zero test).
- The field is really a `u8` + 3 bytes of padding, and zeroing it with `str` also
  clears the adjacent flags.

I wrote `u32` because the **write** width is u32 and `ReleaseSlot` is already
verified as byte-matching with `u32`. Anyone changing this must verify that it
does not break `ReleaseSlot`'s match.

### 5.3 The element's +0x1C..0xAB range (144 bytes) is entirely unknown

Only two points are *address-taken* (element+0x1C and element+0x64); they are
neither read nor written, only passed to a function. I cannot say what is inside.
I left it as padding. The evidence for element+0x64 comes from a **single**
function (FUN_080299B0) — low confidence.

### 5.4 It is not certain that Region D is really 512 bytes

I can see that the address `base+0x114C` is passed to a graphics routine; the
figure 512 was derived from the **`movs r2,#128; lsls r2,#2` in the same
instruction sequence** and from the absence of any scalar access up to 0x134C. I
could not verify with certainty that the length belongs to *this* buffer (the
same block also contains a second buffer pointer, 0x02026DA0, which lies outside
this block). The size closes correctly, but this is **medium confidence**.

### 5.5 Fields supported by a single function (low confidence)

+0x24, +0x28, +0x34, +0x38, +0x3A, +0x1134, +0x1370, +0x1375, +0x1388, +0x138C,
+0x138D. Some of these may be part of a larger field (for example +0x38/+0x3A
could be a single u32; +0x1388 and +0x138C could be one contiguous structure).
There is no evidence to distinguish them.

### 5.6 A band count mismatch

The task states there are **24** unwritten functions in the 0x080308EC–0x08031D24
band. I found **26** functions accessing the block in that band, 4 of which are
already `matching` (`SetHudTime`, `HalvesEqual`, `CleanupAreaTiles`,
`CaptureSessionSnapshot`), giving **22 unwritten**. The difference is probably
that the band's lower bound starts at 0x080308AC (`ReleaseSlot`) rather than
0x080308EC, or that it comes from a different list. I could not determine whose
count is correct; whoever makes the decision should compare against their own
list.

---

## 6. One structure, or separate regions that happen to be adjacent?

**One allocation, four logical regions.** It should not be split into separate
symbols.

Reasons not to split it:

1. **A single `Memset` covers the whole block.** `Memset(base, 0, 0x1390)` zeroes
   everything from 0x0000 to 0x138F in one operation. This is direct evidence
   that the block is allocated as a single object. Were they separate objects (as
   in this repository's `gRam020246F0` family), we would see separate DMA/memset
   calls.
2. **The ROM literal pool contains almost nothing but the base.** All 508 accesses
   derive from the base register. There are only five folded constants (+0x24,
   +0x4C, +0x58, +0xA0, +0x114C), and **all of them lie within the measured
   size** — i.e. the compiler folding an offset within the same object, not a
   pointer to a separate object.
3. **Every large gap has been explained.** At first glance there are three huge
   gaps (4142, 516, and 4320 bytes); all three closed as arrays/buffers, and
   **each one fits exactly at both ends**:
   - 0x3C + 24×180 = 0x111C = the next scalar ✓
   - 0x114C + 0x200 = 0x134C = the next scalar ✓
   - the element's last field 0xB2+2 = 0xB4 = the measured stride ✓

   It closes far too well to be coincidence. **Not one unexplained large gap
   remains** in the block; the remaining padding is at most 8 bytes.

That said, the regions are **semantically distinct**: A and E are player/session
state (dense, small, partly signed fields), B is a table of object slots, and D is
a graphics buffer. That is why I separated the sub-regions with comments in the
structure above and lifted B out into its own `ProgressSlot` type — but it should
be declared as a **single `Progress` structure**, under one symbol.

---

## 7. Proposed correction for `data/ram_map.csv` (not applied)

The current row records a size of `5006`. Proposed: **`5008`**, with the Memset
evidence in the rationale field. I did not change it myself — three agents are
writing files concurrently.
