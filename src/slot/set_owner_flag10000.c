/* Set flag 0x10000 on the owner — 0x0805564C-0x0805565D
 *
 * 0x10000 is `movs r1,#128 / lsls r1,#9`. A missing owner at +0x2C does
 * nothing.
 *
 * No prologue: nothing is called and the function returns through `bx lr`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/slot/set_owner_flag10000.c
 */

#include "gba_types.h"

#define OWNER_FLAG  (128 << 9)  /* 0x10000 */

typedef struct FlagOwner {
    u8  pad00[0x18];
    u32 flags;                  /* +0x18 */
} FlagOwner;

typedef struct FlagHolder {
    u8         pad00[0x2C];
    FlagOwner *owner;           /* +0x2C */
} FlagHolder;

/* 0x0805564C */
void FUN_0805564c(FlagHolder *holder)
{
    FlagOwner *owner = holder->owner;

    if (owner == 0)
        return;
    owner->flags |= OWNER_FLAG;
}
