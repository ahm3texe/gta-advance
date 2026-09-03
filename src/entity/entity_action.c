/* Varlik eylem denemesi — 0x08020A9C-0x08020B6D
 *
 * Adi henuz bilinmiyor. Varligin 0x64'teki alt nesnesi uzerinde iki kez
 * FUN_080208a8 deneniyor; sonuca ve bayraklara gore 0x114'teki durum
 * baytina 2 veya 8 yaziliyor.
 *
 * ROM'un kuyrugunda ucu de ayni depolamaya varan uc yol var: bayrak testi
 * ile durum testi hicbir seyi degistirmiyor, yalnizca adres hesabi bir
 * yolda tekrarlaniyor. Bicim ROM'daki gibi birakildi.
 *
 * HENUZ ESLESMIYOR: 98 komutun 85'i birebir tutuyor. Prolog ve ilk cagriya
 * kadar (0x08020A9C-0x08020ACE) tam eslesiyor. Iki fark kaldi:
 *   - `return 0` blogunun yeri: ROM onu erken koyup (0x8020AD2) dort yerden
 *     oraya dalliyor; biz sona koyup ileri dalliyoruz.
 *   - Kuyruktaki dalin yonu: ROM `bls` ile depolamaya, biz `bhi` ile else'e.
 *
 * Denenenler: `==0 return` / `!` / govdeyi if icine almak (151/151/140 bayt;
 * sonuncusu secildi), uc yollu kuyruk (150-151), ters kosul (140), tek if
 * (140), kosulsuz depolama (133 bayt ama yalnizca 76/98 komut -- bayt sayisi
 * dal ofsetleri yuzunden yaniltici, hizalama olcutu daha guvenilir).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/entity/entity_action.c
 */

#include "gba_types.h"

#define FLAG_SKIP_RETRY   0x00440000
#define FLAG_ALLOW_BLOCK  0x00100000
#define FLAG_FORCE_STATE  0x00040000

#define STATE_BLOCKED     2
#define STATE_DONE        8
#define STATE_BLOCK_MAX   1
#define STATE_DONE_MAX    3

#define SUB_OFFSET        0x13C

typedef struct Entity {
    u8   pad00[0x64];
    u32  sub;               /* +0x64 */
    u8   pad68[0xAC];
    u8   state;             /* +0x114 */
} Entity;

extern u32 FUN_0803c400(u32 sub);
extern u32 FUN_08023660(u32 sub);
extern u32 FUN_080208a8(Entity *e, void *b, u32 flags, int *a,
                        int *c, int *d, int *f, void *owner);

u32 FUN_08020a9c(Entity *e, void *b, int unused, u32 flags)
{
    int slot0;
    int slot1;
    int slot2;
    int slot3;

    FUN_0803c400(e->sub);
    if (FUN_080208a8(e, b, flags, &slot0, &slot1, &slot2, &slot3, e) != 0) {

        if ((flags & FLAG_SKIP_RETRY) == 0) {
            FUN_0803c400(e->sub);
            if (FUN_080208a8(e, b, flags, &slot0, &slot1, &slot2, &slot3,
                             (u8 *)e + SUB_OFFSET) != 0) {
                FUN_0803c400(e->sub);
                return 0;
            }
        }

        if (FUN_0803c400(e->sub) != 0 && FUN_08023660(e->sub) != 0
            && (flags & FLAG_ALLOW_BLOCK) != 0) {
            if (e->state > STATE_BLOCK_MAX)
                return 0;
            e->state = STATE_BLOCKED;
            return 0;
        }

        if ((flags & FLAG_FORCE_STATE) != 0 || e->state <= STATE_DONE_MAX)
            e->state = STATE_DONE;
        else
            e->state = STATE_DONE;

        return 1;
    }

    return 0;
}
