/* Notify every actor on the chain whose +0xB1 byte is set — 0x08029130-0x0802915B
 *
 * Walks the +0x00 chain from GetUnk0202F310 and, for each actor whose +0x1C
 * record has a +0xB1 byte other than 0xFF, calls FUN_08027AE8 with the record
 * and the actor.
 *
 * The record is read once per turn and used twice, so it goes into a local; the
 * +0xB1 read goes through a pointer add because Thumb's ldrb immediate reaches
 * only 31.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/notify_chain_unless_unset.c
 */

#include "gba_types.h"

#define BYTE_OFFSET  0xB1
#define UNSET        0xFF

typedef struct ChainActor {
    struct ChainActor *next;    /* +0x00 */
    u8                 pad04[0x18];
    u8                *record;  /* +0x1C */
} ChainActor;

extern ChainActor *GetUnk0202F310(void);

extern void FUN_08027ae8(u8 *record, ChainActor *actor);

/* 0x08029130 */
void FUN_08029130(void)
{
    ChainActor *actor = GetUnk0202F310();
    u8 *record;

    if (actor == 0)
        return;
    do {
        record = actor->record;
        if (record[BYTE_OFFSET] != UNSET)
            FUN_08027ae8(record, actor);
        actor = actor->next;
    } while (actor != 0);
}
