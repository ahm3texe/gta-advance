/* Notify the active slot when the +0x1360 handle is set — 0x08031E18-0x08031E3F
 *
 * The same shape as src/session/notify_when_pending.c over a different record
 * and a different id: 355 here, from the literal pool.
 *
 * The base goes through a local because `gRam02025810 + 0x1360` folds into the
 * pool constant otherwise, and the ROM keeps 0x02025810 there and adds the
 * offset at run time (rule 65).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/progress/notify_when_handle_b.c
 */

#include "gba_types.h"

#define HANDLE_B   (155 << 5)   /* 0x1360 */
#define NOTIFY_ID  355

extern u8 gRam02025810[];

extern u32  GetActiveSlot(void);
extern void FUN_08035058(u32 slot, u32 id);

/* 0x08031E18 */
void FUN_08031e18(void)
{
    u8 *base = gRam02025810;

    if (*(u32 *)(base + HANDLE_B) != 0)
        FUN_08035058(GetActiveSlot(), NOTIFY_ID);
}
