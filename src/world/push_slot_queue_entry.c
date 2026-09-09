/* Pushing an entry onto the slot queue — 0x08031C98-0x08031D23
 *
 * gFlagsB[slot] is the number of entries in that slot; if the count has passed
 * 3 the queue is first drained with FUN_0802ebdc and the count is re-read.
 * The entry is then written into six parallel tables at [count][slot] (five
 * u16, one u32) and the counter is incremented by one.
 *
 * The tables are in COLUMN order: the row stride is 8 entries, i.e. 16 bytes
 * in the u16 tables and 32 in the u32 table.  The number of rows (N) is
 * UNKNOWN, so the symbols are declared as arrays with the first dimension left
 * open.
 *
 * RULE 1 WAS MEASURED HERE: when the tables are written as casts such as
 * `((u16 (*)[8])0x02026BA0)`, agbcc puts the pool load AFTER the address
 * computation (the ROM puts it BEFORE); the reduced register pressure does not
 * move `n` into a callee-saved register and one register fewer is saved ->
 * 132 bytes (the ROM has 140), 25/69 instructions.  With extern ARRAY symbols
 * the difference is zero.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/push_slot_queue_entry.c
 */

#include "gba_types.h"

#define QUEUE_MAX 3
#define SLOTS     8

extern u8  gFlagsB[];               /* 0x02026EF0 — the entry counter per slot */
extern u16 gRam02026BA0[][SLOTS];
extern u16 gRam02027230[][SLOTS];
extern u32 gRam02026C10[][SLOTS];
extern u16 gRam02026F80[][SLOTS];
extern u16 gRam02026C90[][SLOTS];
extern u16 gRam02026F40[][SLOTS];

extern void FUN_0802ebdc(u32 slot);

/* 0x08031C98 */
void PushSlotQueueEntry(u32 slot, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f)
{
    u8 *countp;
    s32 n;

    countp = &gFlagsB[slot];
    n = *countp;
    if (n > QUEUE_MAX) {
        FUN_0802ebdc(slot);
        n = *countp;
    }

    gRam02026BA0[n][slot] = a;
    gRam02027230[n][slot] = b;
    gRam02026C10[n][slot] = c;
    gRam02026F80[n][slot] = d;
    gRam02026C90[n][slot] = e;
    gRam02026F40[n][slot] = f;

    *countp = *countp + 1;
}
