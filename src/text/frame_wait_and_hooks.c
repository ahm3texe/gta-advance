/* Frame wait and two hooks — 0x08063E58-0x08063ECF
 *
 * Three adjacent ROM functions:
 * 0x08063E58 waits for VBlank until a condition holds, clears a flag and returns 1.
 * 0x08063E7C and 0x08063EAC are near-twins: when gRam02025800 is nonzero, use
 * it to index a table of 28-BYTE records. If the first field is 6, invoke
 * the hooks: two calls at 0x08063E7C, one at 0x08063EAC.
 *
 * The ROM forms 28 = 7*4 with lsls #3 / subs / lsls #2 (x*8-x, then <<2),
 * agbcc's constant multiplication pattern. Rule 35: pop {r0}; bx r0 means
 * void; 0x08063E58 instead uses pop {r1}; bx r1 and returns VALUE 1 in r0.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/text/frame_wait_and_hooks.c
 */

#include "gba_types.h"

#define RECORD_MATCH 6

typedef struct Record {
    u32 kind;                   /* +0x00 */
    u8  pad04[24];
} Record;                       /* 28 bytes */

extern void VBlankIntrWait(void);
extern void FUN_08006210(void);
extern u32  FUN_08004100(void);
extern void ClearMapWindowDma(void);
extern void FUN_080125b4(void);
extern void FUN_08012618(void);

extern u32    gRam02025800;
extern u8     gRam02035B1C;
extern Record gRom08852A2C[];

/* 0x08063E58 */
u32 WaitForFrameFlag(void)
{
    do {
        VBlankIntrWait();
        FUN_08006210();
    } while (FUN_08004100() == 0);

    gRam02035B1C = 0;
    return 1;
}

/* 0x08063E7C */
void RunKind6HooksA(void)
{
    u32 index;

    index = gRam02025800;
    if (index != 0 && gRom08852A2C[index].kind == RECORD_MATCH) {
        ClearMapWindowDma();
        FUN_080125b4();
    }
}

/* 0x08063EAC */
void RunKind6HooksB(void)
{
    u32 index;

    index = gRam02025800;
    if (index != 0 && gRom08852A2C[index].kind == RECORD_MATCH)
        FUN_08012618();
}
