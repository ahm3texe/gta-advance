/* Script command: subtract the operand from a progress field — 0x0805A024-0x0805A03F
 *
 * Reads the +0x0C word of the player progress block (data/ram_map.csv,
 * docs/GRAM02025810_LAYOUT.md) and hands the difference to FUN_08030AE4 with a
 * constant 1 in the second place, where the sibling at 0x0805A0AC passes the
 * operand itself. Always reports success.
 *
 * The block is declared `u8 []` here because that is the type every other user
 * of the symbol gives it (include/ram_symbols.h; the consistency check requires
 * one type per symbol). The word at +0x0C is therefore reached through a local
 * u32 view. That local is also what keeps the ROM's split of base and
 * displacement: the pool holds 0x02025810 and the load carries the 12, whereas
 * a cast applied to the address itself folds the two into 0x0202581C.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/script/cmd_progress_minus.c
 */

#include "gba_types.h"

#define PROGRESS_VALUE  3       /* +0x0C, as a u32 index */

extern u8 gRam02025810[];

extern void FUN_08030ae4(u32 value, u32 slot);

/* 0x0805A024 */
u32 FUN_0805a024(u32 a, u16 amount)
{
    u32 *progress = (u32 *)gRam02025810;

    FUN_08030ae4(progress[PROGRESS_VALUE] - amount, 1);
    return 1;
}
