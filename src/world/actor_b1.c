/* Zincirde yakin bir "durum 2" aktoru var mi -- 0x080166E8, 128 bayt.
 *
 * gUnk0202F310 zincirini (baglanti +0x00'da) bastan sona geziyor. Kendisini
 * atliyor; dugumun POZUNUN +0x30 durum bayti 2 degilse atliyor. Kalanlar
 * icin sekizgen yaklasik uzaklik hesaplanip 0x100000 esigiyle
 * karsilastiriliyor; esigi asmayan ilk dugumde 1, zincir bitince 0 donuyor.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 *  - Poz secimi include/target_common.h icindeki satir-ici distance() ile
 *    AYNI DEGIL: burada yalnizca ZINCIRDEKI dugum icin 0x30 maskesine gore
 *    alt/poz secimi yapiliyor, KENDISININ pozu dogrudan +0x18'den okunuyor
 *    (`ldr r5,[r6,#24]`, maske blogunun ONUNDE). Ayrica null kontrolu ve
 *    DIST_MAX donusu yok. Bu yuzden paylasilan baslik kullanilmadi, hesap
 *    dosyaya yerel yazildi.
 *  - Kural 33: `movs r0,#48 / ldrb r2,[r4,#8] / ands r0,r2` -- sonuc SABITIN
 *    yazmacinda. `cur->kind & 0x30` yazimi sonucu yuklemenin yazmacinda
 *    tutuyor; sabit ayri yerele (`mask`) alinip yerinde `&=` gerekiyor.
 *  - Kural 49 / dongu bicimi: giris korumasi + ALTTAN donen do/while
 *    (`cmp r4,#0 / beq son` ... `ldr r4,[r4,#0] / cmp r4,#0 / bne govde`).
 *    KARDES DOSYADAN KOPYALANMADI, ROM'dan okundu.
 *  - Kural 35 tersi: epilog `pop {r4,r5,r6} / pop {r1} / bx r1` -- donus
 *    adresi r1'e aliniyor cunku r0 donus degerini tasiyor, imza u32.
 *  - Esik 0x100000 immediate degil: ROM `movs r0,#128 / lsls r0,#13` ile
 *    kuruyor; kaynakta duz sabit yazmak ayni ikiliyi uretiyor, kural 44'un
 *    kanoniklestirme tuzagi burada yok (karsilastirma yazmac-yazmac).
 *  - SON 7 BAYTI KAPATAN SEY -- BIRIKTIRME BILESIK ATAMA ILE YAZILMALI.
 *    Tek satirlik `result = dx + dy - (lo>>1) - (lo>>2) + (lo>>4);` yazimi
 *    56/63 komutu tutturuyor ama biriktiriciyi GECICI bir pseudo'ya koyuyor:
 *    bizde acc=r0 / kaydirma gecicisi=r1, ROM'da tam TERSI (acc=r1, gecici
 *    r0). Zincir dort ayri deyime bolununce (`result = dx + dy;` sonra
 *    `result -= ...` uculusu) biriktirici DOGRUDAN `result` pseudo'su oluyor
 *    ve ROM'un yazmac dagitimi birebir cikiyor. Kural 33'un ayni ailesi:
 *    sonucun HANGI degiskenin yazmacinda biriktigi kaynaktan secilir.
 *
 * DENENIP ELENENLER
 *  - `#include "target_common.h"` + paylasilan `distance(cur, self)`:
 *    a ve b icin IKI maske blogu ve bastaki null kontrolu uretiyor,
 *    ROM'da ikisi de yok.
 *  - `special(cur)` bicimi (`pose->state==2` sonucunu degiskende
 *    maddelestirmek, kural 48): ROM burada dogrudan `cmp #2 / bne`
 *    kullaniyor, maddelestirme fazladan movs/cmp cifti ekliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/actor_b1.c
 */

#include "gba_types.h"

#define POSE_STATE_ACTIVE 2
#define ALT_POSE_MASK     0x30
#define NEAR_LIMIT        0x100000

typedef struct Pose {
    s32 x;                  /* +0x00 */
    s32 y;                  /* +0x04 */
    u8  pad08[0x28];
    u8  state;              /* +0x30 */
} Pose;

typedef struct Actor {
    struct Actor *next;     /* +0x00 */
    u8    pad04[4];
    u8    kind;             /* +0x08 */
    u8    pad09[0x0f];
    Pose *pose;             /* +0x18 */
    u8    pad1c[4];
    u8   *alt;              /* +0x20 */
} Actor;

extern Actor *GetUnk0202F310(void);

u32 IsAnyActorNearby(Actor *self)
{
    Actor *cur;
    Pose *here;
    Pose *there;
    u32 mask;
    s32 dx;
    s32 dy;
    s32 lo;
    s32 result;

    cur = GetUnk0202F310();
    if (cur == 0) goto none;

scan:
    if (cur == self) goto next;
    if (cur->pose->state != POSE_STATE_ACTIVE) goto next;

    here = self->pose;
    mask = ALT_POSE_MASK;
    mask &= cur->kind;
    if (mask) there = (Pose *)(cur->alt + 4);
    else there = cur->pose;

    dx = there->x - here->x;
    if (dx < 0) dx = -dx;
    dy = there->y - here->y;
    if (dy < 0) dy = -dy;
    lo = dy;
    if (lo > dx) lo = dx;
    result = dx + dy;
    result -= lo >> 1;
    result -= lo >> 2;
    result += lo >> 4;
    if (result < 0) result = -result;
    if (result > NEAR_LIMIT) goto next;
    return 1;

next:
    cur = cur->next;
    if (cur != 0) goto scan;

none:
    return 0;
}
