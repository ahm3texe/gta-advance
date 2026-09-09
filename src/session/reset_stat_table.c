/* Clear the stat table unless something is pending — 0x080621B8-0x080621D7
 *
 * gRam02026D1C guards the clear: while it is non-zero the table is left alone.
 *
 * The table's type comes from include/phase1_types.h, which src/world/p1_6245c.c
 * and its sibling already use; the consistency check requires one extern type
 * per symbol. Its 36 bytes are exactly the twelve u16 ids and twelve u8 values
 * recorded there, which is what the Memset size confirms.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/session/reset_stat_table.c
 */

#include "gba_types.h"
#include "phase1_types.h"

extern u32 gRam02026D1C;
extern Phase1StatTable gRam02035ED0;

extern void *Memset(void *dest, int value, u32 count);

/* 0x080621B8 */
void FUN_080621b8(void)
{
    if (gRam02026D1C == 0)
        Memset(&gRam02035ED0, 0, sizeof(Phase1StatTable));
}
