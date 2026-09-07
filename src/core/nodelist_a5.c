/* Dugumun yuva dizisini kurup kaydini tazeleme - 0x08052BBC-0x08052C65, 170 bayt.
 *
 * DURUM: BYTE-MATCHING (170/170).
 *
 * Ne yapiyor: dugumun +0x14 tanimindan (SlotDesc) yuva sayisini alip
 * FindFreeSlotRun ile ardisik bos yuva blogu ariyor, blogu dugumun +0x18
 * alanina yaziyor, tanimin +0x10 kimlik dizisini bu bloga kopyalayip her
 * kimlik icin RebuildAreaEntry cagiriyor, sonra +0x0A'ya 0xFF yazip
 * "kirli" bitini kuruyor. Kuyrukta bit hala kuruluysa FUN_08051d10'a
 * tanimin +0x14 isaretcisi ve +0x07 baytiyla gidiliyor.
 *
 * Struct ve imzalar kardeslerden ALINDI, uydurulmadi:
 *   Node yerlesimi + 0x0B bitfield'i (dirty:1 / pad:3 / level:4 ISARETLI)
 *     ve SlotDesc.count(+0x06)  -> src/core/nodelist_c2.c (ReleaseNodeSlots)
 *   FindFreeSlotRun(int) -> u16*  -> src/world/slot_table.c
 *   RebuildAreaEntry(s32, u32)    -> src/core/nodelist_b6.c (imza AYNEN
 *     korundu; ROM ikinci argumana dugum isaretcisini veriyor, bu yuzden
 *     cagri yerinde (u32) donusumu var - imzayi degistirmek check_consistency
 *     acisindan gereksiz risk)
 *   ReleaseNodeSlots(u32, u32)    -> src/core/nodelist_c2.c
 * SlotDesc'in +0x07 (mode), +0x10 (ids), +0x14 (table) alanlari bu ROM
 * govdesinden olculdu; +0x14 FUN_08051d10 icinde 0'a karsi sinanip
 * bir global'e SOZCUK olarak yazildigi icin ISARETCI, +0x07 `ldrb`.
 *
 * OLCULEN AYRINTILAR
 *
 * 1) IKI AYRI SIFIR DEGISKENI (islerin kilidi buydu).
 *    ROM prologda `movs r1,#0 / mov r9,r1` ile r9'u sifirliyor; r9 dongude
 *    HIC artirilmiyor. Dongunun kendi sayaci r7. r9 iki yerde okunuyor:
 *      - dongu GIRIS korumasi   `cmp r9,r1 / bcs` (r1 = desc->count)
 *      - dongudensonraki  test  `mov r0,r9 / cmp r0,#0 / beq`
 *    Yani kaynakta iki degisken var: `n` (r9) ve dongu sayaci `i` (r7).
 *    Dongu `for (i = n; i < desc->count; i++)` yazilinca agbcc korumayi
 *    n'den, govde sayacini sifirdan uretiyor - ROM'daki tam bu ikilik.
 *    n hicbir yerde atanmadigi icin `if (n != 0)` govdesi (0x08052C3C,
 *    ReleaseNodeSlots cagrisi) ULASILAMAZ; Ghidra bu yuzden
 *    "Removing unreachable block (ram,0x08052c3c)" diyip blogu ATTI ve
 *    dongu korumasini da `count != 0`a indirdi. ROM'da blok DURUYOR,
 *    dolayisiyla taslak degil ROM esas alindi.
 *
 * 2) `node->slots` ATAMASI ILE `dst` AYRI (2 bayt buradaydi).
 *    ROM: `adds r1,r0,#0 / str r1,[r6,#24]` ... `cmp r1,#0` ...
 *         `adds r4,r1,#0`  <- fazladan kopya.
 *    Yani cagri sonucu once alana yaziliyor, sifir sinamasi ayni degerden
 *    yapiliyor, gezinme isaretcisi ise SONRA kuruluyor. Sonucu dogrudan
 *    `dst`e alip `node->slots = dst` yazmak bu kopyayi yok ediyor.
 *
 * 3) Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 * 4) Kural 47: +0x0B ust dortlusu `lsls #24 / asrs #28` ile okunuyor ->
 *    ISARETLI 4 bitlik alan, `level > 0` karsilastirmasi isaretli.
 *    Ayni baytin 0. biti ise `movs #1 / ands` ile duz maskeleniyor.
 * 5) desc->count her turda YENIDEN okunuyor (dongu icinde `ldrb [r8,#6]`),
 *    yani kosul yerele alinmamis.
 *
 * DENEYIP ELEDIGIM YAZIMLAR
 *   a. Tek degisken (`i` hem dongu sayaci hem son test):  150/170 bayt.
 *      agbcc `i`yi r2'ye koyup her `bl` etrafinda YIGINA DOKUYOR
 *      (`sub sp,#4` + `str/ldr [sp,#0]`); ROM'un `mov r8/mov r9` yuksek
 *      yazmac cifti hic olusmuyor. Yani eksik olan sey optimizasyon degil,
 *      KAYNAKTA BIR DEGISKEN DAHA olmasiydi (yukarida 1).
 *   b. (a) + iki degisken, ama `node->slots = dst` bicimi:  168/170,
 *      fark yalnizca yukaridaki 2. maddedeki kopya ve ondan turetilen
 *      yazmac dagitimi (ROM r8 icin gecici olarak r2, bizimki r0/r1).
 *      2. maddeyi duzeltince yazmac dagitimi da KENDILIGINDEN duzeldi;
 *      ayrica bir dagitim mudahalesi gerekmedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a5.c
 */
#include "gba_types.h"

typedef struct SlotDesc {
    u8    pad00[6];
    u8    count;                /* +0x06 */
    u8    mode;                 /* +0x07 */
    u8    pad08[8];
    u16  *ids;                  /* +0x10 */
    void *table;                /* +0x14 */
} SlotDesc;

typedef struct Node {
    struct Node *next;          /* +0x00 */
    u8           pad04[4];
    u16          id;            /* +0x08 */
    u8           slot;          /* +0x0A */
    u8           dirty : 1;     /* +0x0B bit 0    */
    u8           pad0B : 3;     /* +0x0B bit 1..3 */
    s8           level : 4;     /* +0x0B bit 4..7 */
    u8           pad0C[8];
    SlotDesc    *desc;          /* +0x14 */
    u16         *slots;         /* +0x18 */
} Node;

extern u16  *FindFreeSlotRun(int count);
extern Node *RebuildAreaEntry(s32 index, u32 value);
extern void  ReleaseNodeSlots(u32 id, u32 force);
extern void  FUN_08051d10(Node *node, void *table, u32 mode);

/* 0x08052BBC */
void AllocateNodeSlots(Node *node)
{
    SlotDesc *desc;
    u16 *dst;
    u16 *src;
    u32  i;
    u32  n;

    desc = node->desc;
    n = 0;
    if (node->dirty == 0) {
        if (node->level > 0) {
            node->slots = FindFreeSlotRun(desc->count);
            if (desc->count != 0 && node->slots == 0) return;
            dst = node->slots;
            src = desc->ids;
            for (i = n; i < desc->count; i++, dst++, src++) {
                *dst = *src;
                RebuildAreaEntry(*dst, (u32)node);
            }
            node->slot = 0xFF;
            node->dirty = 1;
        }
        if (node->dirty == 0) return;
    }
    if (n != 0) ReleaseNodeSlots(node->id, 1);
    if (node->dirty != 0) FUN_08051d10(node, desc->table, desc->mode);
}
