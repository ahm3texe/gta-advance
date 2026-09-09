# band_a — reasons the remaining functions were not written

Agent measurements. The symbols have since been added to data/ram_map.csv, so
these functions are now writable.

---

## 0x080308EC

```
/* 0x080308EC -- 72 bytes -- NOT WRITTEN: gRam02025810 was off limits.
 *
 * The literal pool holds a single literal: 0x0202585C = gRam02025810 + 0x4C,
 * which is exactly the 24 x 180-byte slot array handled by
 * src/world/release_slot.c (0x080308AC, matching). This function walks that
 * ENTIRE array, clears the 0x02000000 flag on every occupied slot's object,
 * zeroes the slot and its +4 field, and returns 1 -- the "all slots" version
 * of ReleaseSlot. 0x0202585C is NOT a separate symbol in data/ram_map.csv;
 * the only way in is through gRam02025810, which this task explicitly
 * forbade. (The task note said "none of the nine touch 0x02025810"; against
 * the ROM that is NOT TRUE, since the single pool word is precisely that
 * block's +0x4C.)
 *
 * The one interesting detail to resolve if it is written: the in-loop
 * `cmp r2,r5 / bhi` (unsigned) and the end-of-loop `cmp r2,ip / ble`
 * (signed) test the SAME bound with two different signednesses, and the
 * bound is held in two separate registers (r5 and ip) -- meaning the source
 * writes the same expression twice, at two different types. */


```

---

## 0x08031294

```
/* 0x08031294 -- 132 bytes -- NOT WRITTEN: THREE symbols are missing from
 * data/ram_map.csv.
 *     0x02026D90  (4 bytes, u32; if zero, the function returns immediately)
 *     0x02000008  (2 bytes, u16; last written X value)
 *     0x0200000A  (2 bytes, u16; last written Y value)
 * Two of the symbols it uses DO exist, either in ram_map (gSlotSelector
 * 0x02000D40) or in functions.csv (SelectSlotAB 0x0803C49C, FUN_0802A734,
 * FUN_080625D0).
 *
 * Structure: after calling FUN_080625D0, if 0x02026D90 is set, it reads two
 * fixed-point coordinates (>>22, narrowed to u16) from the +0x18 pointer of
 * the record selected by SelectSlotAB(gSlotSelector + 1). When the parameter
 * is zero and both coordinates equal the cached ones, it does nothing;
 * otherwise it refreshes the cache and redraws the HUD fields with
 * FUN_0802A734(x, 18, 10) and FUN_0802A734(y, 22, 10). */


```

---

## 0x08031388

```
/* 0x08031388 -- 88 bytes -- NOT WRITTEN: 0x02026DA0 is missing from
 * data/ram_map.csv (the nearest records are 0x02026CD0 and 0x02026DF0, and
 * neither range covers this address).
 *
 * Structure: it sets up the object at 0x02026DA0 with
 * FUN_08014FFC(obj,0,0,0), loads its data with
 * FUN_08014040(obj, 0x083444E8, 512, 32, 32, 0), binds the palette/second
 * resource with FUN_08014EE4(obj, 0x08CA635C), then writes 200 to obj+8 and
 * 8 to obj+10 (both u16) and submits it with FUN_08015038(obj).
 * `cmp r4,#0 / beq` -> the two u16 writes sit under an `if (obj != 0)`
 * guard, so the object is held as a pointer in the source. Both ROM
 * addresses (0x083444E8, 0x08CA635C) also need data symbols. */


```

---

## 0x08030F84

```
/* 0x08030F84, 0x08030B40 and similar are the other candidates in this band;
 * they are out of scope for this task. */

```
