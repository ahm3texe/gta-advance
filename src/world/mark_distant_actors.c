/* Marking distant actors — 0x080611CC-0x0806149B
 *
 * TWO FUNCTIONS, ONE BODY.  tools/find_twins.py reported 100%; the ROM bodies
 * are identical instruction by instruction, only the pool addresses are
 * shifted -- the same source compiled twice (docs/WORKFLOW.md section 10).
 *
 * It walks the list THREE TIMES.  An actor is eligible when 0x40 is NOT set
 * at +0x0C, the +0x30 mode of the record at +0x18 equals 2 and, if +0x2C
 * exists, 0x2000000 is not set at its +0x18.  If 0x400000 is set on any
 * eligible actor the function returns immediately.
 *   pass 1: counts the eligible actors; returns if there are fewer than six.
 *   pass 2: does an INSERTION SORT into a six-slot distance array.
 *   pass 3: sets flag 0x400 at +0x0C on those whose distance is greater than
 *      the sixth value.
 *
 * FOUR MEASUREMENTS:
 *   - The insertion loop must be a do-while; written as a `for`, the increment
 *     block goes BEFORE the body (the ROM puts it at the end, 10
 *     instructions).
 *   - The inner shifting loop must use EXPLICIT POINTER WALKING (`q[1]=q[0]`);
 *     `prev[j+1]=prev[j]` produces `adds r1,r0,r6` while the ROM wants
 *     `adds r1,r6,r0` (base first).
 *   - The 0x7FFFFFFF fill constant must be taken into a SEPARATE LOCAL;
 *     written directly, the address computation is emitted before the
 *     constant.
 *   - The fill loop's bound comparison must be SIGNED (the ROM has `bge`); a
 *     pointer comparison produces `bcs`.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/mark_distant_actors.c
 */

#include "gba_types.h"

#define KEEP_COUNT   6
#define FLAG_SKIP    0x40
#define FLAG_ABORT   0x400000
#define FLAG_FAR     0x400
#define OTHER_BUSY   0x2000000
#define MODE_READY   2
#define POS_FLAG     0x30
#define DIST_MAX     0x7FFFFFFF

typedef struct Detail {
    u8  pad00[0x30];
    u8  mode;                   /* +0x30 */
} Detail;

typedef struct Other {
    u8  pad00[24];
    u32 flags;                  /* +0x18 */
} Other;

typedef struct Actor {
    struct Actor *next;         /* +0x00 */
    u8      pad04[4];
    u8      posFlags;           /* +0x08 */
    u8      pad09[3];
    u32     flags;              /* +0x0C */
    u8      pad10[8];
    Detail *detail;             /* +0x18 */
    u8      pad1c[4];
    u8     *posAlt;             /* +0x20 */
    u8      pad24[8];
    Other  *other;              /* +0x2C */
} Actor;

extern Actor *GetUnk0202F310(void);
extern Actor *GetUnk0202F2C0(void);
extern s32    FUN_0803fae8(void *pos);

/* 0x080611CC */
void MarkDistantActors(void)
{
    Actor *actor;
    Other *other;
    u32    flags;
    s32    count;
    s32    dist[KEEP_COUNT];
    s32   *fill;
    s32   *slot;
    s32   *prev;
    s32   *q;
    s32    far;
    s32    d;
    s32    i;
    s32    j;
    void   *pos;

    count = 0;
    actor = GetUnk0202F310();
    while (actor != 0) {
        other = actor->other;
        flags = actor->flags;
        if (!(FLAG_SKIP & flags)
         && actor->detail->mode == MODE_READY
         && (other == 0 || !(other->flags & OTHER_BUSY))) {
            if (flags & FLAG_ABORT)
                return;
            count++;
        }
        actor = actor->next;
    }

    if (count <= KEEP_COUNT - 1)
        return;

    far = DIST_MAX;
    fill = &dist[KEEP_COUNT - 1];
    do {
        *fill = far;
        fill--;
    } while ((s32)fill >= (s32)dist);

    actor = GetUnk0202F310();
    while (actor != 0) {
        other = actor->other;
        flags = actor->flags;
        if (!(FLAG_SKIP & flags)
         && actor->detail->mode == MODE_READY
         && (other == 0 || !(other->flags & OTHER_BUSY))) {
            if (flags & FLAG_ABORT)
                return;
            if (POS_FLAG & actor->posFlags)
                pos = actor->posAlt + 4;
            else
                pos = actor->detail;
            d = FUN_0803fae8(pos);
            i = 0;
            slot = dist;
            prev = dist - 1;
            do {
                if (d < *slot) {
                    j = i + 1;
                    if (j <= KEEP_COUNT - 1) {
                        q = &prev[j];
                        do {
                            q[1] = q[0];
                            q++;
                            j++;
                        } while (j <= KEEP_COUNT - 1);
                    }
                    *slot = d;
                    break;
                }
                slot++;
                i++;
            } while (i <= KEEP_COUNT - 1);
        }
        actor = actor->next;
    }

    if (dist[KEEP_COUNT - 1] == DIST_MAX)
        return;

    actor = GetUnk0202F310();
    if (actor == 0)
        return;
    do {
        other = actor->other;
        flags = actor->flags;
        if (!(FLAG_SKIP & flags)
         && actor->detail->mode == MODE_READY
         && (other == 0 || !(other->flags & OTHER_BUSY))) {
            if (flags & FLAG_ABORT)
                return;
            if (POS_FLAG & actor->posFlags)
                pos = actor->posAlt + 4;
            else
                pos = actor->detail;
            if (FUN_0803fae8(pos) > dist[KEEP_COUNT - 1])
                actor->flags |= FLAG_FAR;
        }
        actor = actor->next;
    } while (actor != 0);
}

/* 0x08061334 — THE SAME BODY; THE ONLY DIFFERENCE is the list access:
 * GetUnk0202F2C0 (the 0x0202F2C0 list) instead of GetUnk0202F310. */
void MarkDistantActorsB(void)
{
    Actor *actor;
    Other *other;
    u32    flags;
    s32    count;
    s32    dist[KEEP_COUNT];
    s32   *fill;
    s32   *slot;
    s32   *prev;
    s32   *q;
    s32    far;
    s32    d;
    s32    i;
    s32    j;
    void   *pos;

    count = 0;
    actor = GetUnk0202F2C0();
    while (actor != 0) {
        other = actor->other;
        flags = actor->flags;
        if (!(FLAG_SKIP & flags)
         && actor->detail->mode == MODE_READY
         && (other == 0 || !(other->flags & OTHER_BUSY))) {
            if (flags & FLAG_ABORT)
                return;
            count++;
        }
        actor = actor->next;
    }

    if (count <= KEEP_COUNT - 1)
        return;

    far = DIST_MAX;
    fill = &dist[KEEP_COUNT - 1];
    do {
        *fill = far;
        fill--;
    } while ((s32)fill >= (s32)dist);

    actor = GetUnk0202F2C0();
    while (actor != 0) {
        other = actor->other;
        flags = actor->flags;
        if (!(FLAG_SKIP & flags)
         && actor->detail->mode == MODE_READY
         && (other == 0 || !(other->flags & OTHER_BUSY))) {
            if (flags & FLAG_ABORT)
                return;
            if (POS_FLAG & actor->posFlags)
                pos = actor->posAlt + 4;
            else
                pos = actor->detail;
            d = FUN_0803fae8(pos);
            i = 0;
            slot = dist;
            prev = dist - 1;
            do {
                if (d < *slot) {
                    j = i + 1;
                    if (j <= KEEP_COUNT - 1) {
                        q = &prev[j];
                        do {
                            q[1] = q[0];
                            q++;
                            j++;
                        } while (j <= KEEP_COUNT - 1);
                    }
                    *slot = d;
                    break;
                }
                slot++;
                i++;
            } while (i <= KEEP_COUNT - 1);
        }
        actor = actor->next;
    }

    if (dist[KEEP_COUNT - 1] == DIST_MAX)
        return;

    actor = GetUnk0202F2C0();
    if (actor == 0)
        return;
    do {
        other = actor->other;
        flags = actor->flags;
        if (!(FLAG_SKIP & flags)
         && actor->detail->mode == MODE_READY
         && (other == 0 || !(other->flags & OTHER_BUSY))) {
            if (flags & FLAG_ABORT)
                return;
            if (POS_FLAG & actor->posFlags)
                pos = actor->posAlt + 4;
            else
                pos = actor->detail;
            if (FUN_0803fae8(pos) > dist[KEEP_COUNT - 1])
                actor->flags |= FLAG_FAR;
        }
        actor = actor->next;
    } while (actor != 0);
}
