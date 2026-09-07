/* Alan temizligi -- 0x08030CB4-0x08030D0B, 88 bayt.  ESLESIYOR (fark 0).
 *
 * Bekleyen temizlik bayragi kuruluysa iki VRAM satirina ucer yarim soz
 * dolduruyor, bir blogu serbest birakiyor ve bayragi temizliyor.
 *
 * COZUM: dongu bicimi ISARETCI ARTIRIMI DEGIL, DIZI INDISLEME.
 * Isaretcileri (left/right) dongu oncesi yerellere yukleyen her yazim
 * 7 baytta takiliyordu; indisli yazim (TILE_ROW_LEFT[i] = ...) 0 verdi.
 * Uretilen kod ayni: iki isaretci yine 2'ser artiyor -- ama artik onlari
 * KAYNAK degil, agbcc'nin kuvvet indirgemesi (strength reduction) uretiyor.
 * Bunun tek gorunur farki KOMUT SIRASI ve o sira 7 baytin tamamiydi.
 *
 * MEKANIZMA (olculdu, farki aciklayan tek sey bu):
 * ROM'un dongu oncesi sirasi soyle:
 *     movs r3, #0          i = 0            <- duz deyim
 *     ldr  r4, =0x02026E80 block            <- duz deyim
 *     ldr  r1, =0xF0E8     dolgu sabiti     <- DONGUDEN CIKARILMIS degismez
 *     adds r0, r1, #0      kopya            <- cse2'nin sabit yuklemeyi
 *                                              kopyaya cevirmesi
 *     ldr  r2, =0x06009858 right            <- KUVVET INDIRGEME baslangici
 *     ldr  r1, =0x06009818 left             <- KUVVET INDIRGEME baslangici
 * Kritik nokta: agbcc dongu optimizasyonunda ONCE degismezleri (movables)
 * loop_start'in onune yaziyor, SONRA kuvvet indirgemenin urettigi isaretci
 * baslangiclarini yine loop_start'in onune yaziyor. Ikinci ekleme birinciden
 * SONRA gelir. Yani hoist edilmis sabit + kopya, isaretci yuklemelerinin
 * ONUNDE cikar. Isaretcileri kaynakta duz deyim olarak yazarsan onlar
 * on-blokta (preheader) EN BASA gelir, hoist edilen sabit ise EN SONA --
 * ROM'un tam tersi. Bu sira farki asla kapanmiyordu.
 *
 * IKI YAN OLCUM, ayni mekanizmayi dogruluyor:
 *   - `ldr r1, =sabit` + `adds r0, r1, #0` ciftinin kaynagi bir C kopyasi
 *     DEGIL. Sabit dongu ICINDE satir ici yazilinca agbcc onu disari
 *     tasiyor; cse2 tasinan yuklemeyi (deger zaten bir yazmacta oldugu icin)
 *     kopyaya ceviriyor ve kopya artik silinemiyor. Kaynak seviyesinde
 *     `fill2 = fill;` yazmak BUNU URETMEZ -- cse1 sabiti yayar, kopya olur,
 *     cikti 84 bayt (ROM 88).  Iki fill'i de canli tutup kopyayi
 *     yasatmak ise fazladan bir callee-saved istiyor: push {r4,r5,lr}.
 *   - Iki yazim da (fill degiskeni / satir ici sabit) ayni sabiti kullanir
 *     ama havuz yeri farklidir; havuz sirasi ldr komut sirasini izliyor.
 *
 * ELENEN YAZIMLAR (hepsi olculdu, tekrar denemeyin):
 *   Isaretci-artirimli dongu ailesi -- hicbiri 7'nin altina inmedi:
 *     fill + fill2 ikisi de canli (onceki en iyi)          7
 *     fill/fill2 kullanimini takas etmek                   7
 *     iki kopya zinciri (fill2=fill; fill=fill2)           7
 *     block atamasini i'den once almak                    10
 *     satir ici sabit + fill degiskeni karisik (iki yon)  12
 *     dongu icinde satir ici sabit, fill yok              21
 *     fill'i dongu oncesi yukleyip icerde satir ici       21
 *     her iki store da fill2 (kopya elenir, 84 bayt)      61
 *     kopyayi dongu icine almak                           61
 *     u32->u16 / u16->u32 / s16 kopya                     61
 *     uclu kopya zinciri, `register` anahtar sozcugu      61
 *     int fill, for-dongusu + satir ici                   61
 *     *right = *left = fill  /  *left = *right = fill  61/60
 *     kopyayi isaretci atamalarindan sonraya almak        64
 *     fill'i dongu icinde atamak                          64
 *   Indisli dongu ailesi -- yalnizca sira ayrintisi kaldi:
 *     RIGHT'i once yazmak                                  2
 *     for (i = 0; i <= 2; i++) bicimi                      4
 *     fill degiskeni kullanmak (sabit disari cikmiyor)    61
 *   Daha onceki turlardan (yapisi artik gecersiz ama not kalsin):
 *     kural 40 dar volatile ile fill2 okumasi             85
 *     blogu dongu oncesi yuklememek                       23
 *     sayaci s32 yapmak                                   22
 *   Bildirim sirasi bu fonksiyonda ETKISIZ (uc permutasyon da ayni).
 *   Permuter 8.871 yineleme kosturdu, 7'nin altina inmedi: skoru komut
 *   agirlikli sezgisel, bayt esitligi degil; ara skorlarina guvenmeyin.
 *
 * GENEL DERS (kural 49'un tamamlayicisi): ROM'da bir dongu isaretci
 * artiriyorsa bu KAYNAKTA isaretci artirildigi anlamina GELMEZ. Dongu
 * oncesi komut sirasina bakin: sabit yuklemeler isaretci yuklemelerinin
 * ONUNDEYSE isaretciler kuvvet indirgemeden geliyordur, yani kaynak
 * indisli yazilmistir.
 *
 * Ayni kumedeki eslesen uc fonksiyon: src/world/area_flags.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/area_cleanup.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define TILE_FILL      0xF0E8
#define TILE_RUN       3
#define TILE_ROW_LEFT   ((u16 *)0x06009818)
#define TILE_ROW_RIGHT  ((u16 *)0x06009858)

typedef struct Progress {
    u8 pad0000[0x137E];
    u8 pendingCleanup;          /* +0x137E */
} Progress;

extern u32      gRam02026E80;

extern void ReleaseObject(u32 *block);

/* 0x08030CB4 */
void CleanupAreaTiles(void)
{
    u32 *block;
    u32 i;

    if (((Progress *)gRam02025810)->pendingCleanup == 0)
        return;

    i = 0;
    block = &gRam02026E80;
    do {
        TILE_ROW_LEFT[i] = TILE_FILL;
        TILE_ROW_RIGHT[i] = TILE_FILL;
        i++;
    } while (i <= TILE_RUN - 1);

    ReleaseObject(block);
    ((Progress *)gRam02025810)->pendingCleanup = 0;
}
