/* The record's +0x10 word, adjusted in one range — 0x08063BA4-0x08063BC9
 *
 * Reads the +0x10 word of the 28-byte record at gRom08852A2C[index] and adds
 * 111 to it when it falls in 244..395, leaving it alone otherwise.
 *
 * The range is one UNSIGNED comparison in the ROM (`subs r0,#244 / cmp r0,#151
 * / bls`), which is the two-sided test folded into one; docs/COMPILER.md rule
 * 60 records the two-`if` form that produces it.
 *
 * The 28-byte stride is `lsls r1,r0,#3 / subs r1,r1,r0 / lsls r1,r1,#2`, that
 * is (index * 8 - index) * 4, and the field's +0x10 is added to the table base
 * rather than folded into the pool constant (rule 65).
 *
 * NOT BYTE-MATCHING. 12 of 19 instructions. The block order, the stride, the
 * range test and both answers are reproduced; what is left is the +0x10.
 *
 * The ROM loads the table base FIRST, before the stride is computed, and adds
 * the field offset to it at run time:
 *
 *     ldr r2,=0x08852A2C / lsls r1,r0,#3 / subs r1,r1,r0 / lsls r1,r1,#2
 *     adds r2,#16 / adds r1,r1,r2
 *
 * agbcc folds the 16 into the pool constant instead, putting 0x08852A3C there
 * and loading it after the stride. Six spellings were measured and all six
 * fold the same way:
 *
 *   base = TABLE; base += 0x10; value = *(u32 *)(base + index * 28);
 *   base = TABLE; field = base + 0x10; value = *(u32 *)(field + index * 28);
 *   base = TABLE; offset = index * 28; base += 0x10; value = *(u32 *)(base + offset);
 *   table = (const Record *)TABLE; value = table[index].field10;   (both orders)
 *   field = (const u32 *)((const u8 *)TABLE + 0x10); value = *(u32 *)((u8 *)field + index * 28);
 *   the same with the base `const volatile u8 *`, and with the load volatile too
 *   the same with the stride computed after the +0x10 rather than before
 *
 * Neither rule 65 nor rule 74 reaches it. Rule 65's base is a SYMBOL and its
 * offset a variable; rule 74's `volatile` defeats the fold only when the
 * ACCESS goes through the volatile pointer, and here the pointer is only
 * arithmetic and the fold happens before it becomes a value. Both were tried. src/session/lookup_type_word.c is parked on the same
 * class. Left in place as a record; functions.csv keeps it as decompiled.
 *
 * Rule 73 IS reproduced: the plain answer falls through and the adjusted one
 * is at the end, which is why the test is spelled `>` and not `<=`.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/boot/record_field_adjusted.c
 */

#include "gba_types.h"

#define RANGE_LOW    244
#define RANGE_SPAN   151
#define ADJUSTMENT   111
#define RECORD_TABLE ((const u8 *)0x08852A2C)

/* 0x08063BA4 */
u32 FUN_08063ba4(u32 index)
{
    const u8 *base = RECORD_TABLE;
    u32 offset = index * 28;
    u32 value;

    base += 0x10;
    value = *(const u32 *)(base + offset);
    if (value - RANGE_LOW > RANGE_SPAN) goto plain;
    return value + ADJUSTMENT;
plain:
    return value;
}
