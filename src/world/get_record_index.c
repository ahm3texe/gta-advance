/* Kayit indisi getirici — 0x080512B0-0x080512BB
 *
 * gRecordIndex'i dogrudan donduruyor. Sembol src/misc/record_table.c'de
 * `extern u32` olarak bildirili; ayni tur kullaniliyor (TYPES-001).
 *
 * tools/find_accessors.py ile bulundu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/get_record_index.c
 */

#include "gba_types.h"

extern u32 gRecordIndex;

/* 0x080512B0 */
u32 GetRecordIndex(void) { return gRecordIndex; }
