/* A record's base plus a one-based offset — 0x08032290-0x080322AF
 *
 * Answers -1 for a zero offset. Otherwise the current record's base, looked up
 * through GetRecordIndex and LookupRomByte, plus the offset minus one; the
 * offset is one-based so that zero can mean "none".
 *
 * GetRecordIndex's answer goes straight into LookupRomByte: the ROM does not
 * touch r0 between the two calls.
 *
 * Rule 49: the -1 is the rare answer and stands at the end.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/status/record_offset.c
 */

#include "gba_types.h"

extern s32 GetRecordIndex(void);
extern s32 LookupRomByte(s32 index);

/* 0x08032290 */
s32 FUN_08032290(s32 offset)
{
    if (offset == 0)
        return -1;
    return LookupRomByte(GetRecordIndex()) + offset - 1;
}
