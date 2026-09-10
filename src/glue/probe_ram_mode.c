/* Ask IsRamModeWanted and answer 0 anyway — 0x08051148-0x08051153
 *
 * The callee's answer is discarded; only its side effects, if it has any, can
 * matter. Rule 35: `pop {r1}; bx r1` means r0 carries a return value, and the
 * ROM sets it to 0 after the call, so the discard is deliberate rather than a
 * void wrapper.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/probe_ram_mode.c
 */

#include "gba_types.h"

extern u32 IsRamModeWanted(void);

/* 0x08051148 */
u32 FUN_08051148(void)
{
    IsRamModeWanted();
    return 0;
}
