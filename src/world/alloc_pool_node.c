/* Havuz dugumu ayirma — 0x08033674-0x080336EB (120 bayt)
 *
 * DURUM: 27/57 komut, YAKIN ISKA (eslesmiyor).
 *
 * FUN_08032cc4'ten dugum alinir (yoksa 0), +0x21 = 16. gAddrTable+0xB18'deki
 * 8 girisli tabloda dugum aranir; yoksa ayni tabloya (ROM'da 0x02027330 +
 * 0xB2C diye ikinci bir sembolden!) ilk bos yuvaya yazilir. Donus:
 * ((dugum - gAddrTable) / 44) << 24 | (dugumun +0'i & 0xFFFFFF).
 *
 * KALAN FARK, YERLESIM: ROM ikinci dongunun "bos yuva bulundu" blogunu
 * (`str r2,[r1]; b tail`) BIRINCI dongunun ONUNE koyuyor ve dugumun +0'ini
 * her dongu oncesi yeniden okuyor. Denenen: iki `for` + goto (27),
 * taban yerelleri (27), ic ice while (15). Blok yerlesimini veren kaynak
 * bicimi bulunamadi; muhtemelen ikinci arama ayri bir yardimci/makro.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/alloc_pool_node.c
 */

#include "gba_types.h"
#define NODE_SIZE 44
#define SLOTS     8
typedef struct Node { u32 first; u8 pad04[0x1D]; u8 b21; } Node;
typedef struct AddrEntry {
    u32 addr;                   /* +0x00 */
    u8  pad04[40];              /* stride 44 */
} AddrEntry;
extern AddrEntry gAddrTable[];
extern u8 gRam02027330[];
extern Node *FUN_08032cc4(void);
u32 AllocPoolNode(void)
{
    Node *n; u32 first; s32 i; u32 *slot;
    n = FUN_08032cc4();
    if (n == 0)
        return 0;
    n->b21 = 16;
    first = n->first;
    slot = (u32 *)((u8 *)gAddrTable + 0xB18);
    for (i = 0; i <= SLOTS - 1; i++) {
        if ((Node *)*slot == n)
            goto done;
        slot++;
    }
    first = n->first;
    slot = (u32 *)(gRam02027330 + 0xB2C);
    for (i = 0; i <= SLOTS - 1; i++) {
        if (*slot == 0) {
            *slot = (u32)n;
            break;
        }
        slot++;
    }
done:
    return ((((u8 *)n - (u8 *)gAddrTable) / NODE_SIZE) << 24) | (first & 0xFFFFFF);
}
