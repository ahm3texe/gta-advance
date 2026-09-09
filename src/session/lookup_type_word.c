/* One word from a ROM table, for one id only — 0x08061F34-0x08061F53
 *
 * Answers 0 for every id but 65, and for that one indexes a table at
 * 0x08EC79D4 with the RAM block's +0x08 type byte.
 *
 * The table is in the ROM, so a constant cast is used rather than an extern
 * (rule 1 applies to RAM only).
 *
 * There is no prologue: the function calls nothing and returns through `bx lr`.
 *
 * NOT BYTE-MATCHING. 12 of 16 instructions. Rule 71 gives the block order, and
 * everything else is reproduced; what is left is the ORDER OF THE TWO POOL
 * LOADS. The ROM materialises both addresses before it touches the block:
 *
 *     ldr r1,=0x08EC79D4 / ldr r0,=0x02035EA0 / ldrb r0,[r0,#8] / lsls r0,#2
 *
 * agbcc puts the table's load at its point of use, after the shift, and swaps
 * the two pool words. Rule 70 does not reach it: seven spellings were measured
 * and every one of them scheduled the table load late.
 *
 *   table = TYPE_TABLE; result = table[gRam02035EA0.type];
 *   table = TYPE_TABLE; block = &gRam02035EA0; result = table[block->type];
 *   table = TYPE_TABLE; index = block->type; result = table[index];   (u32, u8)
 *   table = TYPE_TABLE; block = ...;         result = *(table + block->type);
 *   entry = TYPE_TABLE + gRam02035EA0.type;  result = *entry;
 *   block = &gRam02035EA0; result = TYPE_TABLE[block->type];
 *   const u32 *table = TYPE_TABLE;  (initialised at declaration)
 *
 * This is the scheduling half of the rule 44 class. Left in place as a record
 * of the behaviour and of what has been tried; functions.csv keeps it as
 * decompiled, not matching, so it does not count towards the figures.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/lookup_type_word.c
 */

#include "gba_types.h"

#define WANTED_ID   65
#define TYPE_TABLE  ((const u32 *)0x08EC79D4)

typedef struct RamBlock {
    u32 unk00;                  /* +0x00 */
    u8  pad04[4];
    u8  type;                   /* +0x08 */
    u8  mode;                   /* +0x09 */
    u8  pad0A[10];
    u8  byte14;                 /* +0x14 */
    u8  pad15[1];
    u16 word16;                 /* +0x16 */
    u8  pad18[4];
    u16 word1C;                 /* +0x1C */
    u16 word1E;                 /* +0x1E */
    u8  pad20[4];               /* out to the 36 bytes 0x08061D34 clears */
} RamBlock;

extern RamBlock gRam02035EA0;

/* 0x08061F34 */
u32 FUN_08061f34(u32 id)
{
    const u32 *table;
    RamBlock *block;
    u32 result;

    if (id != WANTED_ID) {
        result = 0;
    } else {
        table = TYPE_TABLE;
        block = &gRam02035EA0;
        result = table[block->type];
    }
    return result;
}
