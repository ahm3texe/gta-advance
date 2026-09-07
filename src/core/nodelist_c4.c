/* Uc dugum listesinin kare basi gezilmesi — 0x08053D48-0x08053DF7
 *
 * Once StepCountdownTimers cagriliyor (aldigi r0 argumanini kullanmiyor, kendi
 * tabanini yukluyor; yine de ROM argumani kuruyor: 0x02035760).
 * Ardindan UC ayri bagli liste ayni kalipla geziliyor:
 *
 *     dugum = *liste_basi;
 *     dugum != 0 ve dugum->id != 0x7FEF oldugu surece:
 *         dugum->kind > 0 ise listeye ozgu isleyici cagrilir
 *         dugum = dugum->next
 *
 * 0x7FEF listenin sonunu isaretleyen nobetci kimlik. ROM onu havuzdan bir
 * kez okuyup r5'te tutuyor (dongu-degismezi), ilk karsilastirmayi ise taze
 * yuklenen r0 ile yapiyor: bu, `for (n = head; n && n->id != SENTINEL; ...)`
 * biciminin agbcc tarafindan dondurulmus (rotated) halidir.
 *
 * Sonunda NoOp0805AB78 cagriliyor.
 *
 * Dugum yerlesimi (ROM'dan olculdu):
 *     +0x00  struct Node *next        ldr rX,[rY,#0]
 *     +0x08  u16 id                   ldrh rX,[rY,#8]
 *     +0x0B  yuksek yarim-bayt, ISARETLI 4 bit
 *
 * +0x0B'deki alan `ldrb / lsls #24 / asrs #28` uretiyor: bu tam olarak
 * bayt icinde 4. bitten baslayan ISARETLI 4-bit bitfield'in okunmasidir
 * (kural 24'un isaretli hali). Maskeyle yazmak yerine bitfield tanimlandi.
 *
 * `pop {r4,r5}; pop {r0}; bx r0` -> donus tipi void (kural 35).
 *
 * MEVCUT TIP YENIDEN KULLANILDI: src/world/node_search.c'deki Node ile ayni
 * yerlesim (+0 next, +8 id); burada +0x0B alani da gerektigi icin ayni ada
 * sahip ikinci bir tanim yerine dosyaya ozgu tam yerlesim yazildi. gRam...
 * isimleri data/ram_map.csv'dekilerle birebir ayni.
 *
 * ESLESME: 176/176 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_c4.c
 */

#include "gba_types.h"
#include "node_list.h"

/* 0x02035780 liste basinin hemen oncesindeki blok; StepCountdownTimers bunu
   temizliyor.  Ayri bir sembol olarak dogrulanmadi, o yuzden adres. */
#define NODE_BLOCK_BASE ((void *)0x02035760)

#define NODE_SENTINEL_ID 0x7FEF


/* 0x02035A80 icin ram_map'te sembol yok; ham adres cast'i kullanildi. */
#define gNodeListC (*(NodeC4 **)0x02035A80)

extern NodeC4 *gNodeListHead;       /* 0x02035A70 */

extern void StepCountdownTimers(void *arg);
extern void FUN_080534a8(NodeC4 *node);
extern void AllocateNodeSlots(NodeC4 *node);
extern void FUN_08052228(NodeC4 *node);
extern void NoOp0805AB78(void);

/* 0x08053D48 */
void StepNodeLists(void)
{
    NodeC4 *node;

    StepCountdownTimers(NODE_BLOCK_BASE);

    for (node = gRam02035780;
         node != 0 && node->id != NODE_SENTINEL_ID;
         node = node->next) {
        if (node->kind > 0)
            FUN_080534a8(node);
    }

    for (node = gNodeListHead;
         node != 0 && node->id != NODE_SENTINEL_ID;
         node = node->next) {
        if (node->kind > 0)
            AllocateNodeSlots(node);
    }

    for (node = gNodeListC;
         node != 0 && node->id != NODE_SENTINEL_ID;
         node = node->next) {
        if (node->kind > 0)
            FUN_08052228(node);
    }

    NoOp0805AB78();
}
