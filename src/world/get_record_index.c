/* Record index getter — 0x080512B0-0x080512BB
 *
 * Return gRecordIndex directly. The symbol is declared extern u32 in
 * src/misc/record_table.c; use the same type (TYPES-001).
 *
 * Found with tools/find_accessors.py.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/get_record_index.c
 */

#include "gba_types.h"

extern u32 gRecordIndex;

/* 0x080512B0 */
u32 GetRecordIndex(void) { return gRecordIndex; }
