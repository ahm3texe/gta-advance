/* The record field query — 0x08066D54-0x08066ECD
 *
 * The second parameter is a pointer to a record structure, the first a field
 * id between 0 and 35.  According to the id it reads and returns a single
 * field of that record; 0 for an out-of-range id.
 *
 * The 36 branches are turned into a jump table (0x08066D6C, the function's own
 * literal pool): `cmp r0,#35 / bls` + `lsls r0,#2 / ldr a pc-relative base /
 * ldr / mov pc,r0`.  Because the range 0..35 is contiguous, agbcc produces a
 * table rather than a comparison tree; the case bodies are laid out in SOURCE
 * ORDER in the ROM, so the cases are written in the ROM's block order rather
 * than the table's (cases 2..7, then 1, then 8..20, 22..25, 21, 26..28,
 * 29..34, 0, 35).
 *
 * The structure is the record starting at +0x50 of gSaveBuffer in
 * bump_rank_counter.c / link_state_step.c; the fields there correspond as
 * follows:
 *   the gSaveBuffer +0x70 word      == RecordData +0x20  (stepA/B/C, 5 bits each)
 *   the gSaveBuffer +0x7E half word == RecordData +0x2E  (rankA/B/C, 5 bits each)
 *
 * IMPORTANT: the container at +0x2C must be u32, NOT u16.  The `unk2C_11`
 * field is at bits 11-16, i.e. it crosses the 0x2D/0x2E byte boundary; neither
 * the half word at 0x2C nor the one at 0x2E covers it, so agbcc reads a full
 * word (the ROM: "ldr r0,[r2,#44] / lsls #15 / lsrs #26").  rankA/B/C are bits
 * 17-21, 22-26 and 27-31 of that container; for each, agbcc picks the
 * NARROWEST access that COVERS the field:
 *   17-21 -> the 0x2E byte            (ldrb, lsls #26 / lsrs #27)
 *   22-26 -> the 0x2E half word       (ldrh, it crosses the byte boundary)
 *   27-31 -> the top of the 0x2F byte (ldrb, only lsrs #3)
 * bump_rank_counter.c writes the same bits as a u16 container at +0x7E; the
 * two descriptions give the same bit layout, and u16 was enough there because
 * the 6-bit field is not read.
 *
 * The 8- and 4-bit fields at 0x20 and 0x2C come out in a single instruction:
 *   unk20_00 (bits 0-7)  -> a plain ldrb, no shift
 *   unk2C_00 (bits 0-3)  -> ldrb + lsls #28 / lsrs #28
 *
 * In byte loads, offsets of 0x20 and above take the form
 * `adds r0,r2,#0 / adds r0,#N / ldrb r0,[r0,#0]`: Thumb's ldrb imm5 only
 * reaches 31.  Because ldrh is imm5*2, offsets such as 0x32/0x2E are loaded
 * directly.  So these three-instruction patterns are not a quirk of the source
 * but a consequence of the offsets being right.
 *
 * The switch variable is UNSIGNED (the ROM has bls).
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/rank_query.c
 */

#include "gba_types.h"

typedef struct RecordData {
    /* 0x00 */ u16 unk00;
    /* 0x02 */ u16 unk02;
    /* 0x04 */ u8  unk04;
    /* 0x05 */ u8  unk05;
    /* 0x06 */ u8  unk06;
    /* 0x07 */ u8  unk07;
    /* 0x08 */ u8  slots[8];
    /* 0x10 */ u8  unk10[4];
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ u16 unk1C;
    /* 0x1E */ u16 unk1E;
    /* 0x20 */ u32 unk20_00 : 8;    /* bit  0-7  */
               u32 stepA    : 5;    /* bit  8-12 */
               u32 stepB    : 5;    /* bit 13-17 */
               u32 stepC    : 5;    /* bit 18-22 */
               u32 unk20_23 : 9;    /* bit 23-31 */
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u16 unk26;
    /* 0x28 */ u16 unk28;
    /* 0x2A */ u16 unk2A;
    /* 0x2C */ u32 unk2C_00 : 4;    /* bit  0-3  */
               u32 unk2C_04 : 7;    /* bit  4-10 */
               u32 unk2C_11 : 6;    /* bit 11-16 */
               u32 rankA    : 5;    /* bit 17-21 */
               u32 rankB    : 5;    /* bit 22-26 */
               u32 rankC    : 5;    /* bit 27-31 */
    /* 0x30 */ u8  unk30;
    /* 0x31 */ u8  unk31;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u8  unk34;
} RecordData;

/* 0x08066D54 */
u32 GetRecordField(u32 id, RecordData *rec)
{
    switch (id) {
    case 2:  return rec->unk00;
    case 3:  return rec->unk02;
    case 4:  return rec->unk04;
    case 5:  return rec->unk05;
    case 6:  return rec->unk06;
    case 7:  return rec->unk07;
    case 1:  return rec->unk32;
    case 8:  return rec->unk2A;
    case 9:  return rec->unk18;
    case 10: return rec->unk1A;
    case 11: return rec->unk14;
    case 12: return rec->unk16;
    case 13: return rec->unk1E;
    case 14: return rec->unk1C;
    case 15: return rec->unk34;
    case 16: return rec->unk24;
    case 17: return rec->unk26;
    case 18: return rec->unk2C_11;
    case 19: return rec->unk20_00;
    case 20: return rec->unk28;
    case 22: return rec->unk30;
    case 23: return rec->stepA;
    case 24: return rec->stepB;
    case 25: return rec->stepC;
    case 21: return rec->unk2C_00;
    case 26: return rec->rankA;
    case 27: return rec->rankB;
    case 28: return rec->rankC;
    case 29: return rec->slots[0];
    case 30: return rec->slots[1];
    case 31: return rec->slots[2];
    case 32: return rec->slots[3];
    case 33: return rec->slots[4];
    case 34: return rec->slots[5];
    case 0:  return rec->slots[6];
    case 35: return rec->slots[7];
    }
    return 0;
}
