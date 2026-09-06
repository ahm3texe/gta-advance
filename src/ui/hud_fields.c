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
 * DURUM (2026-09-06): UC FONKSIYON DA BYTE-MATCHING (40/40/40).
 *   Once E78 ve F28 uzun sure "36/40, dort bayt kisa" diye parktaydi ve
 *   eski not "tikanma kaynak duzeyinde degil, yazmac dagitiminda; bilinen
 *   kaldirac yok" diyordu.  O TESHIS YANLISTI.  Asagidaki uc mekanizma
 *   duvari yikti; ucu de olculdu.
 *
 * ---------------------------------------------------------------
 * MEKANIZMA 1 — FAZLADAN `adds rX,rY,#0` KOPYASI NEREDEN GELIYOR
 * ---------------------------------------------------------------
 * Kopya, karo sabitinin HANGI MAKINE KIPINDE (mode) maddelestigine bagli:
 *
 *   u16 tile = 0xF0E8;  *a = tile;
 *       -> `tile` bir YEREL oldugu icin PROMOTE_MODE onu SImode yazmaca
 *          yukseltiyor; store'un kaynagi `(subreg:HI (reg:SI tile))`
 *          oluyor ve cse bu subreg'i dogrudan store'a katliyor.
 *          Sonuc: tek `ldr r3,=0xf0e8`, 36 bayt.  Eksik olan buydu.
 *
 *   *a = 0xF0E8;   (sabit DOGRUDAN store ifadesinde)
 *       -> store'un kaynagi HImode bir sabit; agbcc `*movhi_insn`
 *          uretiyor ve buyuk HImode sabitini havuzdan bir scratch'e
 *          cekip oradan kopyaliyor:  `ldr r0,=0xf0e8` + `adds r2,r0,#0`.
 *          ROM'daki fazladan komut TAM OLARAK BUDUR.
 *
 * DOGRULAMA (ROM taramasi): 0xF0E8'i yukleyen 33 `ldr` komutunun 33'u de
 * hemen ardindan bir yazmac kopyasi yapiyor (`adds rY,rX,#0` ya da hi-reg
 * `mov`).  ROM bu sabiti HICBIR yerde dogrudan kullanmiyor — hepsi HImode
 * store kaynagi.  Ayni imza kardeslerde de var: 0x080311DC, 0x0803157C,
 * 0x08031594, 0x08030DE6, 0x08031266.
 *
 * ---------------------------------------------------------------
 * MEKANIZMA 2 — SAYAC NEDEN ARTAN YAZILIYOR (kural 42'nin ikinci yuzu)
 * ---------------------------------------------------------------
 * HImode sabiti dongu-degismezi oldugu icin loop.c onu preheader'a
 * tasiyor; tasinan komutlar preheader'in SONUNA, kaynak komutlarindan
 * SONRA yaziliyor.  Sayac kaynakta yazilirsa (`for (i = 7; i >= 0; i--)`)
 * `movs r0,#7` sabitten ONCE gelir; scratch artik r0'i kullanamaz,
 * ayirici besinci yazmaci (r4) acar ve `push {r4,lr}` cikar: 44 bayt.
 * ARTAN dongu (`for (i = 0; i < 8; i++)`) yazilirsa sayaci DERLEYICI
 * uretir (azalan cevrim) ve onun ilklendirmesi tasinan sabitten SONRA
 * yayilir; scratch r0 ile sayac r0 cakismaz, yaprak fonksiyon yigin
 * kullanmadan 40 bayt olur.  ROM'un sirasi budur:
 *     ldr r0,=0xf0e8 / adds r2,r0,#0 / movs r0,#7
 *
 * ---------------------------------------------------------------
 * MEKANIZMA 3 — E78'DEKI SON 5 BAYT: OMUR UZATAN NO-OP (kural 50)
 * ---------------------------------------------------------------
 * Ilk iki adimdan sonra E78 40/40 boyutunda ama `a` ile karo yazmaclari
 * TERS (bizde a=r2/karo=r3, ROM'da a=r3/karo=r2).  Olculen tablo:
 *     sayac  refs 7  omur 18  oncelik 0.778  -> r0
 *     b      refs 7  omur 22  oncelik 0.636  -> r1
 *     a      refs 7  omur 24  oncelik 0.583  -> r2   (ROM'da 4. sirada)
 *     karo   refs 5  omur 18  oncelik 0.556  -> r3   (ROM'da 3. sirada)
 * Kural 50: karo'nun refs'i 5'te kilitli (1 dis tanim + 2x2 dongu ici
 * kullanim; refs dongu derinligiyle AGIRLIKLI, dongu ici her kullanim 2
 * sayiliyor -- bu oturumda dogrulandi).  Artirmak fazladan komut ister.
 * Tek kaldirac `a`'nin OMRUNU uzatmak: 14/26 = 0.538 < 0.556 olunca sira
 * ROM'unki olur.  Omur = tanim noktasindan fonksiyon sonuna kadarki komut
 * sayisi (birim basina 2); yani `a`'nin tanimi ile karo'nun tanimi ARASINA,
 * sonunda KAYBOLAN bir komut sokmak gerekiyor.
 *
 * `a++; a--;` cifti tam bunu yapiyor: global dagitim aninda iki komut
 * olarak duruyorlar (a'nin omru 24 -> 32), son gecisler onlari siliyor ve
 * CIKTIDA HICBIR IZ BIRAKMIYORLAR.  Olculdu: fark 5 -> 0, cikti yine 40
 * bayt, tek komut bile eklenmedi.
 * Ayni sonucu veren esdeger yazimlar (hepsi olculdu, hepsi eslesti):
 *   `a += 1; a -= 1;` | `a += 2; a -= 2;` | `a = &a[1]; a = &a[-1];`
 *   `a = COL_A_LEFT + 1; a--;`
 * Etkisiz: `a = a + 0;` (front-end katliyor, fark 5'te kaliyor).
 * Cifti `b`'ye uygulamak yanlis yon (b'nin omru a'yi geciyor, fark 7-8).
 *
 * ---------------------------------------------------------------
 * F28: DIZI INDISI YAZIMI
 * ---------------------------------------------------------------
 * F28'de iki isaretci de `a[i]` / `b[i]` ile yaziliyor; ikisi de giv'e
 * (turetilmis dongu degiskeni) donusuyor, kaynaktaki `ldr` tanimlari
 * OLUYOR ve giv ilklendirmeleri preheader'da tasinan sabitten SONRA
 * yayiliyor.  ROM'un havuz sirasi (karo, sag sutun, sol sutun) ve govde
 * sirasi (iki store, sonra iki artirim) ancak boyle cikiyor.  E78'de ise
 * artirimlar store'larla ARALIKLI oldugu icin isaretciler kaynak
 * degiskeni olarak kaliyor (`*a = ...; a++;`) ve `ldr`leri sabitten ONCE
 * yayiliyor.  Kural 49: kardes fonksiyonun dongu bicimini KOPYALAMA.
 *
 * ---------------------------------------------------------------
 * TARANAN VE ELENENLER (tekrar denemeyin)
 * ---------------------------------------------------------------
 * Onceki oturumlardan:
 *   - 144 bildirim/atama sirasi (4! x 3!)  -> hepsi 36 bayt
 *   - 13 bayrak kumesi: O0/O1/O2/O3/Os, -fno-omit-frame-pointer,
 *     -fforce-mem, -fforce-addr, -fno-strength-reduce, -fno-defer-pop,
 *     -fcaller-saves, -fno-cse-follow-jumps  -> hicbiri 36'yi degistirmedi
 *   - yapisal aileler: for/while/do-while, *p++ / *p=t;p++, karo tipi
 *     u16/s32/int/vu16, ara kopya degiskeni, sayac degiskeni, q = p + 32
 *     -> hepsi 36 bayt.  HEPSI KARO'YU BIR YERELDE TUTUYORDU; asil hata
 *     oydu (bkz. MEKANIZMA 1).  "Elendi" kaydi bu yuzden yaniltmisti.
 *   - `agbcc` (old_agbcc degil) 40 bayt uretiyor ama sebebi gereksiz bir
 *     `push {lr}` / `pop {r0}; bx r0` sarmali; govde ayni.  Yol kapali.
 * Bu oturumda olculup elenenler:
 *   - tek alanli struct/union ile HImode zorlama -> insv (and+orr), 48 bayt
 *   - `t1 = C; t2 = C;` iki sabit yereli -> kopya CIKIYOR ama iki yerel de
 *     donguda yasadigi icin besinci yazmac (r4) acilir, push, 44 bayt
 *   - `t1 = C; t2 = t1;` kopya tabanli bolme -> combine 2->1 birlestirip
 *     kopyayi siliyor, 36 bayt (kaynagi olen kopya HER ZAMAN eleniyor)
 *   - `*p = t; p[32] = t; p++` tek isaretci + 0x40 ofset -> giv olusmuyor,
 *     donguda `add r0,r1,#0 / add r0,#0x40`, 36 bayt, iki havuz kelimesi
 *   - u32/s16 sayac, `i != 8` sinir bicimi -> 44-56 bayt (maskeleme/push)
 *   - zincirleme `*a = *b = C` -> volatile geri okuma ekliyor, 48 bayt
 *   - sabit dongu icinde YERELE atanip oradan store (`tile = C; *a = tile`)
 *     -> yine SImode, kopya yok, 36 bayt
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
    s32 i;

    a = COL_A_LEFT;
    /* Omur uzatan no-op cifti: CIKTIDA KOMUT URETMIYOR, ama global yazmac
       dagitimi aninda `a`'nin omrunu 24'ten 32'ye cikariyor; onceligi
       0.583'ten 0.438'e dusuyor, karo sabiti r2'yi ve `a` r3'u aliyor —
       ROM'un dagilimi.  Gerekce ve olcum icin dosya basligi, MEKANIZMA 3. */
    a++;
    a--;
    b = COL_A_RIGHT;
    /* Karo sabiti DOGRUDAN store'da: yerele alinirsa u16 yerel SImode'a
       yukselir ve tek `ldr` kalir; ROM'un fazladan `adds r2,r0,#0`
       kopyasi ancak HImode store sabitinden cikar.  Sayac ARTAN olmali:
       azalan yazilirsa sayac ilklendirmesi tasinan sabitten once gelir,
       scratch r4'e tasar ve push acilir (44 bayt). */
    for (i = 0; i < 8; i++) {
        *a = TILE_BLANK;
        a++;
        *b = TILE_BLANK;
        b++;
    }
}

/* 0x08030F28 */
void FUN_08030f28(void)
{
    vu16 *a;
    vu16 *b;
    s32 i;

    a = COL_B_LEFT;
    b = COL_B_RIGHT;
    /* SIRA: ikisine de yazip SONRA ikisini birden ilerlet (E78 tersini
       yapiyor).  Dizi indisi yazimi her iki isaretciyi de giv yapiyor;
       giv ilklendirmeleri preheader'da tasinan karo sabitinden SONRA
       yayiliyor, ROM'un havuz sirasi boyle cikiyor. */
    for (i = 0; i < 6; i++) {
        a[i] = TILE_BLANK;
        b[i] = TILE_BLANK;
    }
}
