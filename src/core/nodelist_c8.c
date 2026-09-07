/* Dort geri sayim sayacini kare gecikmesi kadar azaltma — 0x08053AD8 (108 bayt)
 *
 * ROM govdesi ayni on komutluk blogu DORT KEZ, elle acilmis halde tasiyor:
 *
 *     ldr r1,[r2,#N] / cmp #0 / ble   -> sayac pozitif mi
 *     movs #192 / lsls #18 / ldr      -> 0x03000000'daki kare gecikmesi
 *     subs / str                      -> sayac -= gecikme
 *     cmp #0 / bge / movs #0 / str    -> sifirin altina inmesin
 *
 * Yani kaynak da dort kez ayni deyimi yaziyor; agbcc -O2 dongu acmiyor,
 * acilmis blok sayisi dogrudan kaynaktaki deyim sayisidir.
 *
 * OLCULEN 1 — taban EXTERN SEMBOL olmali, `#define ((s32 *)0x02035B30)` degil:
 *   Sabit ifade yazilinca agbcc taban+ofseti DORT AYRI literale katliyor
 *   (`.word 0x2035b30`, `+4`, `+8`, `+12`) ve her blok kendi havuz okumasini
 *   yapiyor: 124 bayt, 26 komut fark. Kural 1'in tam ornegi.
 *   ELENEN ARA YOL — yerel isaretci (`s32 *p = (s32 *)0x02035B30;`):
 *   tabani yazmacta tutuyor ve 50 komutun 50'si tutuyor, AMA ROM'un
 *   `ldr r0,<havuz>` + `adds r2,r0,#0` ciftini tek `ldr r2,<havuz>`e
 *   indirgiyor -> 104 bayt (2 bayt kopya + 2 bayt hizalama dolgusu eksik).
 *   O fazladan kopyayi yalnizca gercek sembol referansi uretiyor: sembol
 *   adresi kendi pseudo'suna (r0) yukleniyor, ilk elemani oradan okuduktan
 *   sonra kalan uc blok icin ayri bir pseudo'ya (r2) kopyalaniyor.
 *
 * OLCULEN 2 — kare gecikmesi HER BLOKTA YENIDEN okunuyor:
 *   ROM dort blokta da `movs #192 / lsls #18 / ldr r0,[r0]` uretiyor, tek
 *   yazmaca alip tasimiyor: aradaki `str`'ler okumayi olduruyor (GCC 2.8.1
 *   takma-ad cozumlemesi sabit adresli MEM'i dizi yazmalariyla cakisir
 *   sayiyor). Bu yuzden kaynakta dogrudan `gFrameDelay` yaziliyor; yerel
 *   kopyaya almak (`u32 d = gFrameDelay;`) tek okuma birakip uc bloktan
 *   altisar bayt dusururdu.
 *
 * OLCULEN 3 — 0x03000000 SABIT IFADE olmali, sembol degil (kural 1'in TERSI):
 *   ROM adresi `movs #192 / lsls #18` ile KURUYOR, literal havuzdan
 *   okumuyor. Extern sembol olsaydi havuza inerdi. src/interrupt/vblank_intr.c
 *   ve irq_helpers.c ayni tercihi ayni adres icin olcmustu; oradaki
 *   `#define gFrameDelay (*(u32 *)0x03000000)` bicimi aynen kullanildi.
 *   Iki adresin zit davranmasinin sebebi kaydirmayla kurulabilirlik:
 *   0x03000000 = 192 << 18, 0x02035B30 kurulamaz.
 *
 * Kural 31: `ble`/`bge` isaretli -> sayaclar s32, `cmp #0` ile karsilastiriliyor.
 * Kural 35: `bx lr`, push yok -> void donus, yaprak fonksiyon.
 *
 * Cikarma `s32 - u32` oldugu icin ara sonuc unsigned; ROM'un `subs` komutu
 * ayni. Kirpma testi geri yazilmis s32 lvalue uzerinden yapiliyor, o yuzden
 * karsilastirma isaretli kaliyor (`bge`).
 *
 * ESLESME: 108/108 bayt (elle assemble+link ile dogrulandi, asagiya bak).
 *
 * GEREKLI KAYIT: data/ram_map.csv'de 0x02035B30 icin giris YOK. Derleme
 * katmani `.equ`yu oradan uretiyor, bu yuzden dosya ancak su satir eklenince
 * `make c-match` ile dogrulanabilir:
 *     0x02035B30,16,gCountdownTimers,decomp,provisional,"Dort s32 geri sayim
 *     sayaci; 0x08053AD8 her karede gFrameDelay kadar azaltip sifirda kirpiyor"
 * Kaydi eklemek bana yasak (data altindaki csv dosyalarina dokunulmuyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_c8.c
 */

#include "gba_types.h"

/* 0x03000000 — VBlank'te yazilan kare gecikmesi (data/ram_map.csv: gFrameDelay).
 * Sabit ifade olarak yaziliyor; gerekce OLCULEN 3. */
#define gFrameDelay (*(u32 *)0x03000000)

/* 0x02035B30 — dort adet s32 geri sayim sayaci. Alanlarin tek tek anlami
 * bilinmiyor, o yuzden isimlendirilmemis dizi olarak birakildi. */
extern s32 gCountdownTimers[4];

/* 0x08053AD8 */
void StepCountdownTimers(void)
{
    if (gCountdownTimers[0] > 0) {
        gCountdownTimers[0] -= gFrameDelay;
        if (gCountdownTimers[0] < 0)
            gCountdownTimers[0] = 0;
    }

    if (gCountdownTimers[1] > 0) {
        gCountdownTimers[1] -= gFrameDelay;
        if (gCountdownTimers[1] < 0)
            gCountdownTimers[1] = 0;
    }

    if (gCountdownTimers[2] > 0) {
        gCountdownTimers[2] -= gFrameDelay;
        if (gCountdownTimers[2] < 0)
            gCountdownTimers[2] = 0;
    }

    if (gCountdownTimers[3] > 0) {
        gCountdownTimers[3] -= gFrameDelay;
        if (gCountdownTimers[3] < 0)
            gCountdownTimers[3] = 0;
    }
}
