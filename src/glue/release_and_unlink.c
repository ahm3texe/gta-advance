/* Release the slot's object and unlink it — 0x08008C34-0x08008C5B
 *
 * The slot's +0x00 is the object; when there is one, its own +0x00 goes to
 * FUN_0800C804 with the owner's, the word behind its +0x04 is cleared, the
 * +0x04 itself is cleared, and then the pair is handed to FUN_08008934.
 *
 * The zero is written twice from ONE register: the ROM materialises it once and
 * stores it through two different addresses, which is what two assignments of
 * the same constant give.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/release_and_unlink.c
 */

#include "gba_types.h"

typedef struct SlotObject {
    u32  handle;                /* +0x00 */
    u32 *back;                  /* +0x04 */
} SlotObject;

typedef struct SlotOwner {
    u32 *sink;                  /* +0x00 */
} SlotOwner;

typedef struct SlotHolder {
    SlotObject *object;         /* +0x00 */
} SlotHolder;

extern void FUN_0800c804(u32 *dest, u32 value);
extern void FUN_08008934(SlotOwner *owner, SlotObject *object);

/* 0x08008C34 */
void FUN_08008c34(SlotOwner *owner, SlotHolder *holder)
{
    SlotObject *object = holder->object;

    if (object == 0)
        return;
    FUN_0800c804(owner->sink, object->handle);
    *object->back = 0;
    object->back = 0;
    FUN_08008934(owner, object);
}
