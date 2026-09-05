/* Aktif dugum listesini OAM'a yazma — 0x08012B9C-0x08012C0B
 *
 * Aktif listeyi bastan gezip her dugumun uc OAM ozniteligini yaziyor,
 * ardindan GECEN KAREDEN kalan fazla yuvalari devre disi birakip yeni
 * sayiyi geri sakliyor.
 *
 * OAM tabani `movs r3,#224 / lsls r3,#19` ile kuruluyor = 0x07000000.
 * Bos yuva kalibi: attr0=512 (0x200, OBJ devre disi), attr1=0, attr2=0.
 *
 * Isaretci ilerleyisi 2/2/4: OAM girisi 8 bayt ama dorduncu yarim-kelime
 * (donusum verisi) yazilmiyor, atlaniyor.
 *
 * Havuz duzeni bu fonksiyonla GENISLEDI:
 *     +0x800  bos liste basi
 *     +0x804  aktif liste basi
 *     +0x808  onceki kare sayaci  <- BURADA bulundu; ram_map boyutu
 *                                    2056 -> 2060 olarak duzeltildi
 *
 * Dugum ve havuzun ortak yerlesimi include/sprite_pool.h icindedir.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * ESLESME: 112/112 bayt. Kalan 9 baytlik komut-sirasi farkini
 * `for (i = count; i < left; i++)` kapatti. agbcc artan induksiyon
 * degiskenini azalan sayaca cevirirken `left - count` cikarmasini
 * dongu sabitlerinden SONRA yerlestiriyor. Elle `left -= count`
 * yazmak ise cikarmayi kaynak deyimi olarak sabitlerden ONCE yayiyor.
 * Ayri sabit yerelleri ve `while (--left != count)` denemeleri eslesmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/flush_sprite_list.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

#define OBJ_HIDDEN 512

/* 0x08012B9C */
void FlushSpriteList(void)
{
    Node *node;
    vu16 *oam;
    s32   count;
    s32   left;
    s32   i;

    count = 0;
    oam = (vu16 *)OAM_BASE;
    node = gNodePool.activeHead;
    if (node != 0) {
        do {
            *oam = node->attr0;
            oam++;
            *oam = node->attr1;
            oam++;
            *oam = node->attr2;
            oam += 2;
            count++;
            node = node->next;
        } while (node != 0);
    }

    left = gNodePool.prevCount;
    if (count < left) {
        for (i = count; i < left; i++) {
            *oam = OBJ_HIDDEN;
            oam++;
            *oam = 0;
            oam++;
            *oam = 0;
            oam += 2;
        }
    }
    gNodePool.prevCount = count;
}
