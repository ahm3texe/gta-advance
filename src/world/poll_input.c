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
 * KALAN TEK FARK: omuz maskesi testinde ROM iki operandi da ayri
 * pseudo'ya KOPYALIYOR (`adds r1,r0,#0 / adds r0,r4,#0 / ands r0,r1`),
 * biz tek komutta yapiyoruz (`ands r0,r4`). Operand sirasini cevirmek
 * (kural 53) hicbir sey degistirmedi.
 *
 * PERMUTER KOSTURULDU (1296 yineleme): 875 -> 425, sifira ulasmadi.
 * En iyi aday `index`i `unsigned long long` yapiyor; skoru dusuruyor ama
 * u16 bir sayac icin gercek kaynak olamaz, REDDEDILDI.
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
