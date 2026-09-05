/* HUD alan yazicilari — 0x08030B60, 0x08030E78, 0x08030F28
 *
 * 0x08030B60: iki degeri 0..99 ve 0..59 araligina KIRPIP gRam02025810
 *   blogunun +28 ve +29 baytlarina yaziyor.  Sinirlar dakika:saniye
 *   gorunumunu isaret ediyor.
 *
 *   NOT: bu fonksiyon blogun +28/+29'una yaziyor, yani gRam02025810
 *   TABAN olarak kullaniliyor -- ram_map'te 8 bayt kayitli olmasi bu
 *   bolgeyi tek nesne saymadigi icin; ayni kalip menu_screen.c (+20),
 *   counter_saturate.c (+20 -> +12) ve halves_equal.c (+4/+28) icinde
 *   de var.  include/ram_symbols.h'daki `extern u8 gRam02025810[]`
 *   bildirimi bu paylasimi zaten kabulleniyor.
 *
 * 0x08030E78 ve 0x08030F28: iki VRAM sutununa ayni karo degerini
 *   (0xF0E8) yaziyorlar.  Fark yalnizca sirada: E78 bir sutuna yazip
 *   ilerliyor sonra otekine; F28 IKI SUTUNA DA yazip sonra ikisini
 *   birden ilerletiyor.  Bu sira kaynaktan geliyor, ayni yazim
 *   ikisini birden uretmiyor.
 *
 * DURUM — 0x08030E78 ve 0x08030F28 PARK (36/40, dort bayt KISA).
 *
 * ONCEKI TESHIS YANLISTI, DUZELTILDI (2026-09-05).
 *
 * Eski not "iki VRAM adresi arasindaki fark 0x40 oldugu icin agbcc
 * ikincisini havuzdan yuklemek yerine birinciden hesapliyor, bir havuz
 * kelimesi eksiliyor" diyordu.  OLCUM bunu curutuyor: bizim havuzumuzda
 * da ROM'unkinde de UC kelime var (iki adres + karo) ve iki taraf da
 * UC ayri `ldr rX,[pc]` uretiyor.  Havuz birlesmesi diye bir sey YOK.
 * Eski notun "r0'a yukleyip r2'ye kopyalama saglandi" cumlesi de
 * yanlisti; o kopya hicbir zaman uretilemedi.
 *
 * GERCEK TEK FARK: ROM'da fazladan bir YAZMAC KOPYASI var.
 *
 *   ROM                        old_agbcc (bizim)
 *   ldr r3,[pc]   ; p          ldr r2,[pc]   ; p
 *   ldr r1,[pc]   ; q          ldr r1,[pc]   ; q
 *   ldr r0,[pc]   ; karo       ldr r3,[pc]   ; karo
 *   adds r2,r0,#0 ; <-- FAZLA  (yok)
 *   movs r0,#7                 movs r0,#7
 *   ...govde AYNI...           ...govde AYNI...
 *   bx lr                      bx lr
 *   (2 bayt hizalama dolgusu)  (dolgu yok)
 *
 * ROM karo sabitini once r0'a yukleyip r2'ye TASIYOR, cunku ayirici r0'i
 * dongu sayacina birakiyor.  Bizim ayiricimiz karoyu dogrudan r3'e verip
 * kopyadan kaciniyor.  Eksik 4 bayt = 2 (kopya komutu) + 2 (dolgu).
 *
 * TARANAN VE ELENEN (hepsi olculdu, hicbiri kopyayi uretmedi):
 *   - 144 bildirim/atama sirasi (4! x 3!)  -> hepsi 36 bayt
 *   - 13 bayrak kumesi: O0/O1/O2/O3/Os, -fno-omit-frame-pointer,
 *     -fforce-mem, -fforce-addr, -fno-strength-reduce, -fno-defer-pop,
 *     -fcaller-saves, -fno-cse-follow-jumps  -> hicbiri 36'yi degistirmedi
 *   - yapisal aileler: for/while/do-while, *p++ / *p=t;p++,
 *     karo tipi u16/s32/int/vu16, ara kopya degiskeni, sayac degiskeni,
 *     q = p + 32  -> hepsi 36 bayt
 *
 * agbcc DENENDI, ISE YARAMIYOR: `agbcc` (old_agbcc degil) 40/40 uretiyor
 * ama bu YANILTICI.  Boyutu tutturmasinin sebebi ROM'un fazladan kopyasi
 * degil, gereksiz bir `push {lr}` / `pop {r0}; bx r0` sarmali (+4 bayt).
 * Govde old_agbcc ile birebir ayni; fark 14 ve 20'de sabit kaliyor.
 * Yani agbcc bir ADIM ILERI DEGIL, ayni yere baska yoldan varan bir
 * tesaduf.  Bu yuzden derleyici-secim isaretcisi eklenmedi.
 *
 * SONUC: tikanma KAYNAK DUZEYINDE DEGIL, YAZMAC DAGITIMINDA.  Bilinen bir
 * kaynak kaldiraci yok; yeni bir mekanizma bulunmadan tekrar denemeyin.
 *
 * 0x08030B60 ESLESTI (40/40).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/hud_fields.c
 */

#include "gba_types.h"
#include "ram_symbols.h"

#define MINUTES_MAX 99
#define SECONDS_MAX 59

#define TILE_BLANK  0xF0E8

/* Iki fonksiyonun yazdigi VRAM sutunlari (karo haritasi). */
#define COL_A_LEFT  ((vu16 *)0x06009AAE)
#define COL_A_RIGHT ((vu16 *)0x06009AEE)
#define COL_B_LEFT  ((vu16 *)0x060098D2)
#define COL_B_RIGHT ((vu16 *)0x06009912)

/* 0x08030B60 */
void SetHudTime(s32 minutes, s32 seconds)
{
    if (minutes < 0)
        minutes = 0;
    if (seconds < 0)
        seconds = 0;
    if (minutes > MINUTES_MAX)
        minutes = MINUTES_MAX;
    if (seconds > SECONDS_MAX)
        seconds = SECONDS_MAX;

    gRam02025810[28] = minutes;
    gRam02025810[29] = seconds;
}

/* 0x08030E78 */
void FUN_08030e78(void)
{
    vu16 *a;
    vu16 *b;
    u16 tile;
    s32 i;

    a = COL_A_LEFT;
    b = COL_A_RIGHT;
    /* Karo degeri KENDI degiskeninde: dogrudan yazilinca agbcc sabiti
       r4'e yukleyip push {r4,lr} zorluyordu (+4 bayt).  ROM sabiti r0'a
       yukleyip r2'ye kopyaliyor ve r0'i sayac olarak yeniden kullaniyor,
       boylece yigin kullanmayan bir yaprak kaliyor. */
    tile = TILE_BLANK;
    for (i = 7; i >= 0; i--) {
        *a = tile;
        a++;
        *b = tile;
        b++;
    }
}

/* 0x08030F28 */
void FUN_08030f28(void)
{
    vu16 *a;
    vu16 *b;
    u16 tile;
    s32 i;

    tile = TILE_BLANK;
    a = COL_B_LEFT;
    b = COL_B_RIGHT;
    for (i = 5; i >= 0; i--) {
        /* SIRA: ikisine de yazip SONRA ikisini birden ilerlet
           (0x08030E78 tersini yapiyor). */
        *a = tile;
        *b = tile;
        b++;
        a++;
    }
}
