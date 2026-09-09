/* Link an object to its primary root and update state bits on both sides.
 *
 * Use the second object's +0x30 link as the target if present. Clear or set
 * movement/interaction bits according to the first object's control-block
 * flags, then store the link at +0x30.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm
 * Verification: make c-match FILE=src/world/link_objects.c
 */

#include "gba_types.h"

typedef struct LinkControl {
    u8  pad00[10];
    u16 flags0A;
    u32 flags0C;
} LinkControl;

typedef struct LinkObject {
    u8                 pad00[24];
    u32                flags18;
    u8                 pad1C[12];
    LinkControl       *control28;
    u8                 pad2C[4];
    struct LinkObject *link30;
} LinkObject;

/* 0x0805AC94 */
void LinkObjectPair(LinkObject *self, LinkObject *other, s32 enabled)
{
    LinkControl *control;

    if (other->link30 != 0)
        other = other->link30;

    control = self->control28;
    if (control != 0) {
        u32 one;

        one = 1;
        if ((control->flags0A & one) != 0)
            enabled = 0;
        if ((control->flags0C & one) != 0) {
            self->flags18 |= 0x800000;
            if ((control->flags0C & 0x80) != 0)
                control->flags0C &= ~0x80;
            if ((control->flags0C & one) != 0)
                control->flags0C = (control->flags0C & ~0x100) | 0x40;
        }
    }

    control = other->control28;
    if (control != 0 && enabled != 0)
        control->flags0C |= 0x200;
    self->link30 = other;
}
