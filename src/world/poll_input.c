/* Tus okuma ve giris gecmisi — 0x080656F4-0x080657D7
 *
 * KEYINPUT'u ters cevirip 10 bitlik basili tus maskesini cikariyor.
 * Omuz tuslari basili DEGILKEN dort yon birden basiliysa donanim
 * kapanisina gidiyor (yumusak sifirlama kisayolu).
 *
 * Baglanti durumu 2 iken her kare giris 32 elemanlik halka tampona
 * yaziliyor; giris bir onceki kareden farkliysa bir sayac artirilip
 * "son degisim" indeksi kaydediliyor. Indeksler arasi fark 12'yi asarsa
 * gecmis penceresi ileri kaydiriliyor. Iki yan tampon da o karenin
 * sayac ve pencere degerlerini sakliyor.
 *
 * Halka tamponlarin boyutlari ROM'dan OLCULDU: 0x08066144'teki Memset
 * cagrilari 64 (32 x u16) ve 32 (32 x u8) veriyor, indeksleme de `& 31`.
 *
 * DURUM: PARK — 224/228 bayt, 4 eksik. Uc kol bulundu ve uygulandi:
 *
 *  1. Iki tus testi AYRI `if` olmali. Tek `&&` ifadesinde GCC 0x300 ve
 *     0x00F ayrik oldugu icin ikisini tek maskeye katliyor (`0x30F`) ve
 *     ROM'un iki ayri daliyla uyusmuyor.
 *  2. `index` s32 olmali. u16 bildirilince agbcc her atamada 16 bit
 *     kirpma (`lsls #16 / lsrs #16`) ekliyor; ROM kirpmiyor.
 *  3. Bayt daraltmasi `(u8)` cast'i degil `& 0xFF` olmali. Cast 24 bit
 *     kaydirma cifti uretip fazladan bir callee-saved yazmac (r8)
 *     tuketiyor -- olculdu, dump_alloc pseudo 119/120/121.
 * Bu uc adim farki 185 -> 4 bayta indirdi.
 *
 * KALAN FARK (91 ROM komutuna karsi 90 bizde; 2 bayt komut + 2 bayt
 * havuz hizalamasi):
 *   a) Omuz maskesi testinde ROM iki operandi da ayri pseudo'ya
 *      KOPYALIYOR (`adds r1,r0,#0 / adds r0,r4,#0 / ands r0,r1`), biz tek
 *      komutta yapiyoruz (`ands r0,r4`).  (ROM +2 komut)
 *   b) Sayac artiminin hemen ardindaki `strb`de ROM adres yazmacini (r1)
 *      hala canli tutup dogrudan kullaniyor, biz `mov r1,ip` ile ip'den
 *      geri yukluyoruz.  (bizde +1 komut)
 * (b) saf dagitim: ROM dizi tabanini r1'de, indeks gecicisini r0'da
 * tutuyor; biz tersini yapiyoruz.
 *
 * (a)'NIN MEKANIZMASI RTL DOKUMUNDEN OLCULDU (2026-09-07, `old_agbcc -da`):
 * combine `(set (reg 37) (and (reg/v 22) (reg 36)))` uretiyor ve reg 36
 * (0x300 sabiti) burada OLUYOR (REG_DEAD).  regmove'un `fixup_match_1`i
 * bu yuzden hedefi 37 -> 36 olarak yeniden adlandiriyor ve tek komut
 * cikiyor.  ROM'un uc komutlu bicimi ancak HICBIR operand olmediginde
 * (ya da `reg_is_remote_constant_p` tetiklendiginde, yani sabitin `set`i
 * BASKA bir temel blokta oldugunda) olusur.  `keys` (reg 22) zaten
 * olmuyor; kalan tek kol sabitin de olmemesi, yani 0x300'un fonksiyonda
 * ikinci bir kullanimi -- ROM'da boyle bir kullanim YOK.  Kaynak duzeyinde
 * kol bulunamadi.
 * DIKKAT: kural 59'un `u8` yereli BURADA UYGULANMAZ; 0x300 dusuk baytta
 * degil, `u8` yerel maskeyi kirpip anlami bozuyor (olculdu: 216 bayt).
 *
 * PERMUTER KOSTURULDU (1296 yineleme): 875 -> 425, sifira ulasmadi.
 * En iyi aday `index`i `unsigned long long` yapiyor; skoru dusuruyor ama
 * u16 bir sayac icin gercek kaynak olamaz, REDDEDILDI.
 *
 * ELENEN YAZIMLAR (hepsi 224 bayt, degisiklik yok; 2026-09-07):
 * maske sonucunu u16/u32/s32 yerele almak; `u8` yerele almak (216, anlam
 * bozuk); `!(keys & MASK)`; `mask = KEY_SHOULDERS;` ara degiskeni; tek
 * `&&` ifadesi; `MASK & keys` (kural 53); `shoulders = keys; shoulders
 * &= MASK;` (u16/u32/s32); `if (... > 0) ; else`; bos `else` dali; omuz
 * ve dpad maskelerini once birlikte hesaplamak.  `keys`i u32/s32/int
 * yapmak 220 bayta DUSURUYOR (daha kotu).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/poll_input.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "game_state.h"

#define KEY_MASK        0x3FF
#define KEY_SHOULDERS   0x300
#define KEY_DPAD        0x00F
#define HISTORY_MASK    31
#define WINDOW_SPAN     12
#define LINK_STATE_LIVE 2
#define RESET_ARG       0xFF

extern u16 gVBlankEnabled;
extern u16 gRam0200048C;
extern u16 gRam02000420[];
extern u8  gRam020003C0[];
extern u8  gRam02000E80[];
extern u8  gRam02036320;
extern u8  gRam0203632C;

extern void ShutdownAndReset(void);
extern void FUN_0806b88c(u32 arg);

/* 0x080656F4 */
void PollInput(void)
{
    u16 keys;
    s32 index;

    keys = ~REG_KEYINPUT & KEY_MASK;

    if ((keys & KEY_SHOULDERS) == 0) {
        if ((keys & KEY_DPAD) == KEY_DPAD)
            ShutdownAndReset();
    }

    gGameState.pressed = keys & ~gGameState.half04;
    gGameState.held    = keys;

    if (gVBlankEnabled == LINK_STATE_LIVE) {
        index = gRam0200048C + 1;
        gRam0200048C = index;

        gRam02000420[index & HISTORY_MASK] = keys;

        if (gGameState.held != gRam02000420[(gRam0200048C - 1) & HISTORY_MASK]) {
            gRam02036320++;
            gRam0203632C = index;
        }

        if (((gRam0200048C - gRam0203632C) & 0xFF) > WINDOW_SPAN)
            gRam0203632C = gRam0200048C - WINDOW_SPAN;

        gRam020003C0[gRam0200048C & HISTORY_MASK] = gRam02036320;
        gRam02000E80[gRam0200048C & HISTORY_MASK] = gRam0203632C;
    }

    if ((keys & KEY_DPAD) == KEY_DPAD)
        FUN_0806b88c(RESET_ARG);
}
