/* Dugumun yuvalarini bosaltip listeyi tazeleme — 0x08055C04-0x08055C8B
 *
 * Kimlikten dugumu buluyor. Dugum "kirli" (bit 0) ise ve ya seviyesi 1 ise
 * ya da zorlama bayragi verilmisse: dugumun yuva dizisindeki her kimlik icin
 * FUN_08052750 cagriliyor, ardindan yuvalar DMA ile bos kimlikle doldurulup
 * kirli biti temizleniyor. Zorlama varsa seviye 1'in ustundeyse 1'e cekiliyor.
 * Sonunda liste basi + kimlik ile FUN_08055D90 cagriliyor.
 *
 * +0x0B BAYTI BITFIELD: `ldrb` + `lsls #24` / `asrs #28` cifti, bit 4..7'de
 * ISARETLI 4 bitlik bir alan demek. Ayni alanin `== 1` karsilastirmasi ise
 * kaydirmasiz `movs #240 / ands / cmp #16` uretiyor — agbcc'nin bitfield
 * esitlik karsilastirmasini maskeye indirgemesi. `level = 1` atamasi da
 * `movs #15 / ands / movs #16 / orrs` veriyor; ucu de ROM ile birebir.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * Kural 43: sayac ve isaretci birlikte ilerliyor -> ikisi de `for`
 * artiriminda, ROM sirasiyla (`adds r5,#1` sonra `adds r4,#2`).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_c2.c
 */

#include "gba_types.h"

#define LEVEL_BASE  1

typedef struct SlotDesc {
    u8 pad00[6];
    u8 count;                   /* +0x06 */
} SlotDesc;

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
    u8           pad0A;
    u8           dirty : 1;     /* +0x0B bit 0    */
    u8           pad0B : 3;     /* +0x0B bit 1..3 */
    s8           level : 4;     /* +0x0B bit 4..7 */
    u8           pad0C[8];
    SlotDesc    *desc;          /* +0x14 */
    u16         *slots;         /* +0x18 */
} Node;

extern Node *gNodeListHead;         /* 0x02035A70 */

extern Node *FindNode(u32 id);
extern void  FUN_08052750(u32 a, u32 b);
extern void  FillSlotsWithNone(void *dest, u32 count);
extern void  FUN_08055d90(u32 *head, u32 id);

/* 0x08055C04 */
void ReleaseNodeSlots(u32 id, u32 force)
{
    Node *node;
    u16  *slot;
    int   i;

    node = FindNode(id);
    if (node == 0)
        return;

    if (node->dirty) {
        if (node->level == LEVEL_BASE || force != 0) {
            slot = node->slots;
            for (i = 0; i < node->desc->count; i++, slot++)
                FUN_08052750(*slot, 0);
            FillSlotsWithNone(node->slots, node->desc->count);
            node->dirty = 0;
        }
    }

    if (force != 0) {
        if (node->level > LEVEL_BASE)
            node->level = LEVEL_BASE;
    }

    FUN_08055d90((u32 *)&gNodeListHead, id);
}
