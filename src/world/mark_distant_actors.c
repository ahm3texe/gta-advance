/* Uzaktaki aktorleri isaretleme — 0x080611CC-0x0806149B
 *
 * IKI FONKSIYON, AYNI GOVDE.  tools/find_twins.py %100 verdi; ROM
 * govdeleri komut komut ayni, yalnizca havuz adresleri kayik -- ayni
 * kaynak iki kez derlenmis (docs/WORKFLOW.md §10).
 *
 * Listeyi UC KEZ geziyor.  Elverisli aktor: +0x0C'de 0x40 kurulu DEGIL,
 * +0x18'deki kaydin +0x30 kipi 2, ve +0x2C varsa onun +0x18'inde
 * 0x2000000 kurulu degil.  Herhangi bir elverisli aktorde 0x400000
 * kuruluysa fonksiyon hemen doner.
 *   1. gecis: elverisli aktorleri sayiyor; alti taneden az ise cikiyor.
 *   2. gecis: alti gozlu bir uzaklik dizisine SIRALI EKLEME yapiyor.
 *   3. gecis: uzakligi altinci degerden buyuk olanlara +0x0C'de 0x400
 *      bayragini kuruyor.
 *
 * DORT OLCUM:
 *   - Sirali ekleme dongusu do-while yazilmali; `for` yazimi ilerletme
 *     blogunu govdenin ONUNE koyuyor (ROM sonuna koyuyor, 10 komut).
 *   - Ic kaydirma dongusu ACIK ISARETCI YURUTMESI olmali (`q[1]=q[0]`);
 *     `prev[j+1]=prev[j]` yazimi `adds r1,r0,r6` uretiyor, ROM
 *     `adds r1,r6,r0` istiyor (taban once).
 *   - 0x7FFFFFFF doldurma sabiti AYRI YERELE alinmali; dogrudan
 *     yazilirsa adres hesabi sabitten once uretiliyor.
 *   - Doldurma dongusunun sinir karsilastirmasi ISARETLI olmali
 *     (ROM `bge`); isaretci karsilastirmasi `bcs` uretiyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/mark_distant_actors.c
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

/* 0x08061334 — AYNI GOVDE; TEK FARK liste erisimi
 * GetUnk0202F310 yerine GetUnk0202F2C0 (0x0202F2C0 listesi). */
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
