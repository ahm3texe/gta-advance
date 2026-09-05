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
 * Node'un ilk alti bayti OAM oznitelikleri; alloc_node.c'deki pad00[8]'in
 * ic yapisi burada adlandirildi (bayt duzeni AYNI kaldi).
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * DURUM: PARK — 9/112 fark.  Boyut DOGRU ve fark yalnizca KOMUT SIRASI;
 * uretilen komutlarin kendisi ROM'la BIREBIR AYNI.
 *
 *   bizim: subs r0,r0,r4 / movs r2,#128 / lsls r2,#2 / adds r1,r2,#0 / movs r2,#0
 *   ROM  : movs r2,#128 / lsls r2,#2 / adds r1,r2,#0 / movs r2,#0 / subs r0,r0,r4
 *
 * ROM once bos-yuva sabitlerini (512 ve 0) kuruyor, `left -= count`
 * cikarmasini dongu on-basligının EN SONUNA birakiyor.  Bizimki cikarmayi
 * basa aliyor.  Fonksiyonun geri kalan 103 bayti tamamen tutuyor.
 *
 * ELENEN IKI YOL:
 *   1. Sabitleri ayri yerel degiskenlere alip (`hidden`, `zero`) sirayi
 *      elle dayatmak -> 108 bayt, DAHA KOTU (degiskenler birlestirildi).
 *   2. Cikarmayi dongu kosuluna gommek (`while (--left != count)`)
 *      -> yine KISA.  agbcc bunu ROM'daki on-baslik cikarmasina cevirmiyor.
 *
 * Bu, dagitim degil ZAMANLAMA (scheduling) sinifi: agbcc'nin dongu
 * on-basligini nasil siraladigi.  Kaynak duzeyinden dayatilamadi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/flush_sprite_list.c
 */

#include "gba_types.h"

#define OAM_BASE   ((vu16 *)0x07000000)
#define OBJ_HIDDEN 512
#define NODE_COUNT 128

typedef struct Node {
    u16          attr0;         /* +0x00 */
    u16          attr1;         /* +0x02 */
    u16          attr2;         /* +0x04 */
    u16          pad06;
    struct Node *next;          /* +0x08 */
    struct Node *prev;          /* +0x0C */
} Node;

typedef struct NodePool {
    Node  nodes[NODE_COUNT];    /* +0x000 */
    Node *freeHead;             /* +0x800 */
    Node *activeHead;           /* +0x804 */
    s32   prevCount;            /* +0x808 */
} NodePool;

extern NodePool gNodePool;

/* 0x08012B9C */
void FlushSpriteList(void)
{
    Node *node;
    vu16 *oam;
    s32   count;
    s32   left;

    count = 0;
    oam = OAM_BASE;
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
        left -= count;
        do {
            *oam = OBJ_HIDDEN;
            oam++;
            *oam = 0;
            oam++;
            *oam = 0;
            oam += 2;
        } while (--left != 0);
    }
    gNodePool.prevCount = count;
}
