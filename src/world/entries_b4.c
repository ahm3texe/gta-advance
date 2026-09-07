/* Aktoru sinus tablosundan hiz vektoru kurup ilerletme -- 0x08025340-0x08025423
 *
 * 228 bayt, Thumb.  Iki bagimsiz is yapiyor:
 *
 *  1. Dorduncu arguman (off) NULL degilse ve icindeki iki isaretli bayt
 *     offsetinden en az biri sifir degilse, aktorun mevcut acisiyla o
 *     offseti dondurup (FUN_08029088) sonucu 41/32 ile olcekliyor ve
 *     16.16 konuma (+0x4C / +0x50) EKLIYOR.  Arada GetOwnerSlot(actor->owner)
 *     cagriliyor; donus degeri hemen r0 uzerine yazildigi icin KULLANILMIYOR.
 *  2. Aci + turn degerinden 10 bitlik tablo indeksi cikarip 0x08CA30D8'deki
 *     1024 girisli s16 sinus tablosundan sin ve cos okuyor, dorde katlayip
 *     +0x70 / +0x74'e yaziyor.  speed sifir degilse ayni vektoru speed ile
 *     carpip yine +0x4C / +0x50'ye ekliyor.
 *
 * KARDES: src/world/entries_b1.c AYNI AILEDEN DEGIL -- govdesi tamamen baska
 * (yuva arama).  Struct yerlesimi ve cagri imzalari icin asil kardes
 * src/world/state_offset.c (0x080260A8): oradaki Actor gorunumu bire bir
 * ayni ofsetleri kullaniyor (+0x4C px, +0x50 py, +0x68 angle, +0x84 owner)
 * ve FUN_08029088'in imzasi orada ROM'dan dogrulanmis.  Bu dosyada +0x70 ve
 * +0x74 (hiz vektoru) ek olarak aciliyor.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 *
 *  - Donus tipi void (kural 35): epilog `pop {r0}; bx r0`, r0 olu.
 *  - Arguman tipleri: r1 (turn) ve r2 (speed) hic daraltilmiyor, ikisi de
 *    ham kelime -> s32.  r3 (off) sifira karsi siniyor -> isaretci.
 *  - 0x08CA30D8'deki tablo ROM'dan okundu: 1024 giris, genlik 16384,
 *    [0]=0, [256]=16384, [512]=0 -> Q14 sinus.  cos icin indeks +256.
 *    Adres data/ram_map.csv'de YOK, oraya yazma yetkim de yok; bu yuzden
 *    sabit cast olarak yazildi (entries_b2.c'deki ayni gerekce: adres tek
 *    basina taban olarak kullaniliyor, kural 1'in taban+ofset katlanmasi
 *    burada soz konusu degil -- ROM da onu havuzdan tek kelime okuyor).
 *  - 41/32 olcegi shift zincirinden okundu: lsls#2 / adds / lsls#3 / adds
 *    = x*41, sonra asrs#5.  state_offset.c'deki SCALE_NUM/SCALE_SH ile ayni.
 *  - ox/oy yigin gozleri sp+4 ve sp+5, yani BITISIK iki bayt; FUN_08029088
 *    dorduncu argumani sp+4, besincisi sp+5 aliyor.
 *
 * DENENIP ELENEN YAZIMLAR (bu bolum en degerli kismi)
 *
 *  1. `if (off->dx != 0 || off->dy != 0)` -- iki BITISIK alanin sifira karsi
 *     karsilastirmasini agbcc (GCC 2.8.1 fold_truthop) TEK `ldrh r0,[r3,#32]`
 *     komutuna birlestiriyor.  Cikti 224 bayt, ROM 228: tam 2 komut eksik
 *     (ROM'daki ham bayt kopyasi `adds r2,r0,#0` ve dorduncu dal komutu).
 *     Alan tiplerini s8/u8 diye AYIRMAK da birlesmeyi engellemedi
 *     (olculdu: s8+u8 direkt erisim yine 224).  Bitfield (`signed char dx:8`)
 *     ve her ikisinin de bitfield olmasi da 224'te kaldi.
 *  2. Alanlari once yerele okumak (`rx = off->dx; ry = off->dy;`) birlesmeyi
 *     onluyor ama bu sefer agbcc iki adresi birbirinden turetiyor:
 *     `adds r0,r3,#33` sonra `subs r0,#1`.  ROM ikisini de r3'ten AYRI
 *     kuruyor (`adds r1,r3,#0; adds r1,#32` + `adds r0,r3,#0; adds r0,#33`).
 *     Bu yazimla en iyi sonuc 34 bayt farkti.
 *  3. Cozum kural 17/37: HER ALAN ICIN AYRI BIR ISARETCI YERELI.  `px` ve
 *     `py` iki bagimsiz adres omru yaratinca agbcc ikisini de tabandan
 *     yeniden kuruyor ve ROM'un dizilimi birebir cikiyor.
 *  4. `sx` (dx'in isaretli hali) ile `ry` (dy'nin HAM bayti) AYRI tiplerde
 *     olmali.  ROM dx'i lsls#24/asrs#24 ile genisletip GENISLETILMIS degeri
 *     siniyor, dy'yi ise HAM baytla siniyor ve genisletmeyi cagri
 *     kurulumuna erteliyor (0x8025372).  `s32 ry` yazinca fark 188'e
 *     firliyor; `u8 ry` + argumanda `(s8)ry` dogru bicim.
 *     `sx`/`ry` ara yerellerini hic kullanmayip dogrudan `*px`/`*py`
 *     yazmak da bozuyor (fark 192).
 *  5. Kuyruk (sinus tablosu) icin UC ayri olcum gerekti:
 *       - `gSinTable[...]` dogrudan: maske literali havuza ONCE giriyor,
 *         ROM'da once TABLO adresi var.  Fark 51.
 *       - `tbl = gSinTable;` en basta: literal sirasi duzeliyor ama `ldr`
 *         aci hesabindan ONCE cikiyor.  Fark 47.
 *       - Aciyi `ang` yerelinde ayirip `tbl = gSinTable;` atamasini onun
 *         HEMEN ARDINA koymak (kural 16: bildirim yeri degil ATAMA yeri):
 *         kuyruk birebir oturdu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/entries_b4.c
 */

#include "gba_types.h"

#define ANGLE_ROUND  0x8000     /* 16.16 aciyi yuvarlamak icin yarim tur */
#define TABLE_MASK   0x3FF      /* 1024 girisli tablo */
#define QUARTER      256        /* ceyrek tur = cos icin indeks kaymasi */
#define SCALE_NUM    41         /* 41/32 ~ 1.28, state_offset.c ile ayni */
#define SCALE_SH     5

/* 0x08CA30D8: 1024 girisli Q14 sinus tablosu (ROM'dan okundu, bkz. baslik).
 * data/ram_map.csv'de karsiligi yok; sabit cast olarak yaziliyor. */
#define gSinTable    ((s16 *)0x08CA30D8)

/* Dorduncu argumanin dokunulan tek parcasi: +0x20 / +0x21'deki isaretli
 * bayt offset cifti.  Geri kalani bilinmiyor, pad birakildi. */
typedef struct Offsets {
    u8  pad00[0x20];
    s8  dx;              /* +0x20 */
    s8  dy;              /* +0x21 */
} Offsets;

/* src/world/state_offset.c'deki Actor gorunumuyle ayni ofsetler; hiz
 * vektoru (+0x70/+0x74) burada aciliyor.  owner'in icerigi bu ceviri
 * biriminde okunmuyor, sadece GetOwnerSlot'e veriliyor. */
typedef struct Actor {
    u8    pad00[0x4c];
    s32   px;            /* +0x4C, 16.16 */
    s32   py;            /* +0x50, 16.16 */
    u8    pad54[0x14];
    s32   angle;         /* +0x68, 16.16 */
    u8    pad6c[4];
    s32   vx;            /* +0x70 */
    s32   vy;            /* +0x74 */
    u8    pad78[0xc];
    void *owner;         /* +0x84 */
} Actor;

/* Imza state_offset.c'de ROM'dan dogrulandi. */
extern void FUN_08029088(s32 angle, s32 dx, s32 dy, s8 *outX, s8 *outY);
/* Donus degeri burada kullanilmiyor (r0 cagri sonrasi hemen eziliyor). */
extern void GetOwnerSlot(void *owner);

/* 0x08025340 */
void MoveActorAlongAngle(Actor *actor, s32 turn, s32 speed, Offsets *off)
{
    s32  idx;
    s32  vx;
    s32  vy;
    s8   ox;
    s8   oy;
    s16 *tbl;
    s32  ang;
    s8  *px;
    s8  *py;
    s32  sx;
    u8   ry;

    if (off != 0) {
        /* Kural 17/37: iki alan icin IKI AYRI isaretci yereli; tek taban
         * uzerinden erisim agbcc'ye adresleri birbirinden turettiriyor. */
        px = &off->dx;
        py = &off->dy;
        ry = (u8)*py;
        sx = *px;
        if (sx != 0 || ry != 0) {
            FUN_08029088((actor->angle + ANGLE_ROUND) >> 16, sx, (s8)ry,
                         &ox, &oy);
            GetOwnerSlot(actor->owner);
            ox = (ox * SCALE_NUM) >> SCALE_SH;
            oy = (oy * SCALE_NUM) >> SCALE_SH;
            actor->px += ox << 16;
            actor->py += oy << 16;
        }
    }

    ang = (actor->angle + turn + ANGLE_ROUND) >> 16;
    tbl = gSinTable;                    /* kural 16: atama yeri onemli */
    idx = ang & TABLE_MASK;
    vx = tbl[idx] << 2;
    vy = -(tbl[(idx + QUARTER) & TABLE_MASK] << 2);
    actor->vx = vx;
    actor->vy = vy;
    if (speed != 0) {
        actor->px += vx * speed;
        actor->py += vy * speed;
    }
}
