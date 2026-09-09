/* Place a record, and settle if nothing is held — 0x0805A824-0x0805A87F
 *
 * NOT BYTE-MATCHING. 27 of 42 instructions, from ONE root cause: the ROM
 * normalises the first operand IN PLACE and then copies it
 * (`lsrs r1,r1,#16 / adds r5,r1,#0`), and agbcc narrows straight into the
 * callee-saved register. Everything after that is the same instructions with
 * r4 and r5 exchanged and every branch two bytes earlier.
 *
 * Rule 75's mixed-type lever was tried both ways round and does not reach this
 * shape; it only works when the ROM narrows into one register and copies to
 * another. src/script/cmd_chain_end_field1c.c and src/session/is_special_id.c
 * are parked on the same thing, and rule 75 records the boundary.
 *
 * Runs only once gFrameCounterEwram is going and the operand resolves to a
 * record. FUN_0802B394 then places it, and when NEITHER of the two handles at
 * +0x1358 and +0x1360 of the progress block is set, FUN_08029918 runs as well.
 *
 * The second offset is DERIVED from the first with `adds r2,#8` rather than
 * built again, so the two are one offset variable (rule 65).
 * src/progress/either_handle_set.c reads the same pair from the other end and
 * derives its second with a `subs`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_place_record.c
 */

#include "gba_types.h"

#define HANDLE_A  0x1358

extern u32 gFrameCounterEwram;
extern u8  gRam02025810[];

extern void *GetOrCreateRecordNode(u16 id);
extern void  FUN_0802b394(u16 id, u16 value, u32 mode);
extern void  FUN_08029918(void);

/* 0x0805A824 */
u32 FUN_0805a824(u32 a, u16 id, u16 value)
{
    u8 *base;
    u32 offset;

    if (gFrameCounterEwram != 0) {
        if (GetOrCreateRecordNode(id) != 0) goto have;
    }
    return 0;
have:
    FUN_0802b394(id, value, 1);
    base = gRam02025810;
    offset = HANDLE_A;
    if (*(u32 *)(base + offset) == 0) {
        offset += 8;
        if (*(u32 *)(base + offset) == 0)
            FUN_08029918();
    }
    return 1;
}
