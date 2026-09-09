/* Reset the draw queue — 0x080130D4-0x080130EF
 *
 * Clears the queue count, sets the last-index field to -1 and zeroes the
 * separate queue counter at 0x02022ABC.
 *
 * The Queue body is the one shared with src/world/alloc_draw_entry.c (the
 * consistency gate requires one body per symbol). 0x02022ABC sits four bytes
 * past the end of that 12-byte block and is a symbol of its own, which is why
 * the ROM loads a second pool word for it instead of reaching it as an offset.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/reset_draw_queue.c
 */

#include "gba_types.h"

typedef struct Entry { u32 a; u32 b; u16 flags; u8 kindA; u8 kindB; u32 key; u8 pad10[12]; } Entry;
typedef struct Queue { u32 count; Entry *entries; s16 last; } Queue;

extern Queue gRam02022AB0;
extern u32 gRam02022ABC;

/* 0x080130D4 */
void ResetDrawQueue(void)
{
    gRam02022AB0.count = 0;
    gRam02022AB0.last = -1;
    gRam02022ABC = 0;
}
