/* Girisin iki kimlik dizisini gezip her birini bosaltma. 0x08052DDC, 154 bayt.
 *
 * Giris etkinse (+0x0B ust dortlusu sifirdan farkli) iki dizi geziliyor:
 *   +0x34'teki dizi  -> her kimlik FUN_08055be8'e
 *   +0x30'daki dizi  -> her kimlik FindOrRecycleNode'a; donen dugumun
 *                       +0x0B bayraklarinda 2 biti YOK ve 1 biti VARSA,
 *                       dugumun kendi +0x18 dizisi de gezilip her kimlik
 *                       FUN_08052828'e (ikinci arguman 1) veriliyor
 *
 * Dizi uzunluklari kayittan (+0x28) her turda YENIDEN okunuyor: ROM ic
 * donguye girmeden once `ldrb [r9,#6]` yapip cikista tekrar yapiyor, yani
 * kosul degiskene alinmamis.
 *
 * Ic donguden sonra dis dongunun sayaci ve yurutucusu geri yukleniyor
 * (`adds r7,r4,#1` / `mov r8,r5` sonra `adds r4,r7,#0` / `mov r5,r8`);
 * bu, artirmalarin cagridan HEMEN SONRA yazildigini gosteriyor.
 *
 * DURUM: PARK, 154/154 boyut TUTUYOR, fark 17 (154 baytin 137'si dogru).
 *   ilk taslak                                   150/154, fark 4 kisa
 *   kaydi bayrak sinamalarindan once yazmaca al   154/154, fark 22
 *   kural 43 (sayac+isaretci `for` artiriminda)   154/154, fark 22
 *   sayaclari isaretcilerden once bildir          154/154, fark 17
 *
 * Kalan 17 baytin TAMAMI tek bir takas: ROM sayaci r4'te isaretciyi r5'te
 * tutuyor, bizimki tersi. Bildirim sirasi bunu CEVIRMIYOR -- uc yazim
 * denendi (sayaci once ata, isaretci-sayac serpistirilmis bildirim, i/j
 * ters), ucu de 17 veya daha kotu. Bilinen yazmac dagitimi sinifi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a3.c
 */

#include "gba_types.h"

typedef struct Record {
    u8 pad00[5];
    u8 slotCount;               /* +0x05 */
    u8 idCount;                 /* +0x06 */
} Record;

typedef struct Node {
    u8      pad00[0x0b];
    u8      kind;               /* +0x0B */
    u8      pad0C[8];
    Record *record;             /* +0x14 */
    u16    *ids;                /* +0x18 */
} Node;

typedef struct Entry {
    u8      pad00[0x0b];
    u8      kind;               /* +0x0B */
    u8      pad0C[0x1c];
    Record *record;             /* +0x28 */
    u8      pad2C[4];
    u16    *slots;              /* +0x30 */
    u16    *ids;                /* +0x34 */
} Entry;

extern void  FUN_08055be8(u16 id);
extern Node *FindOrRecycleNode(s32 id);
extern void  FUN_08052828(u16 id, s32 flag);

/* 0x08052DDC */
void FUN_08052ddc(Entry *entry)
{
    Record *rec;
    Record *rec2;
    Node *node;
    s32 i;
    s32 j;
    u16 *p;
    u16 *p2;
    u16 *q;

    rec = entry->record;
    if ((entry->kind & 0xf0) == 0) return;

    /* Kural 43: sayac ve yurutucu ikisi de `for` artiriminda, ROM'un
     * sirasiyla (once sayac, sonra isaretci). */
    p = entry->ids;
    for (i = 0; i < rec->idCount; i++, p++) {
        FUN_08055be8(*p);
    }

    /* IKINCI dongunun isaretcisi AYRI yerel olmali.  Tek `p` kullanmak
     * refs'i 14'e cikariyor; oncelik floor_log2(refs)*refs/omur oldugu icin
     * 3*14/29 = 1.448 ile sayacin 1.185'ini geciyor ve r4'u kapiyor.
     * Bolununce 2*7/14 = 1.000'e dusuyor, sayac once dagitilip r4'u aliyor
     * -- ROM'un yerlesimi.  Belirleyici olan oran degil, floor_log2'nin bir
     * basamak dusmesi. */
    p2 = entry->slots;
    for (i = 0; i < rec->slotCount; i++, p2++) {
        node = FindOrRecycleNode(*p2);
        if (node != 0) {
            /* ROM ikisini de bayrak sinamalarindan ONCE yazmaca aliyor;
             * kaydi dongu kosulunda birakmak her turda yeniden okutuyor. */
            q = node->ids;
            rec2 = node->record;
            if ((node->kind & 2) == 0 && (node->kind & 1) != 0) {
                for (j = 0; j < rec2->idCount; j++, q++) {
                    FUN_08052828(*q, 1);
                }
            }
        }
    }
}
