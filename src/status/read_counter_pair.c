/* Two counters, chosen by kind — 0x08031EEC-0x08031F17 and 0x08031F18-0x08031F47
 *
 * Two adjacent getters of the same shape. Kind 1 reads the save buffer, kind 2
 * the status record, and everything else answers the fallback. They differ in
 * which pair of fields they read and in what the fallback is: the first answers
 * 0 when the save buffer is inactive, the second answers 100.
 *
 * The second one tests the active flag the other way round, `bne` into the body
 * rather than `beq` out of it, which is why its 100 stands first and the first
 * function's 0 stands with the other failures at the end.
 *
 * That difference also forces two different spellings of the LAST test. The
 * first function's `return 0` is reached by falling through, so its `goto`
 * label sits above the status read and the ROM's block order follows. The
 * second has nothing to fall through from, and a plain `if (kind !=
 * KIND_STATUS) return 0;` comes out inverted; only rule 71's two-armed if with
 * a result variable, zero arm first, reproduces it.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/status/read_counter_pair.c
 */

#include "gba_types.h"
#include "../status/status_types.h"

#define KIND_SAVE    1
#define KIND_STATUS  2
#define NO_LIMIT     100

extern MenuCtx      gSaveBuffer;
extern StatusRecord gRam02026CD0;

/* 0x08031EEC */
u32 FUN_08031eec(u32 kind)
{
    if (gSaveBuffer.active == 0) goto zero;
    if (kind == KIND_SAVE)
        return gSaveBuffer.w0C;
    if (kind == KIND_STATUS) goto status;
zero:
    return 0;
status:
    return gRam02026CD0.h00;
}

/* 0x08031F18 */
u32 FUN_08031f18(u32 kind)
{
    u32 result;

    if (gSaveBuffer.active == 0)
        return NO_LIMIT;
    if (kind == KIND_SAVE)
        return gSaveBuffer.w0E;
    if (kind != KIND_STATUS)
        result = 0;
    else
        result = gRam02026CD0.h02;
    return result;
}
