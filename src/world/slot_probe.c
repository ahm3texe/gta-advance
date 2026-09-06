/* Yuvanin dort noktasini haritada sinama ve giris uretme
 *      0x080651E0-0x08065377, 408 bayt
 *
 * SpawnSlotEffect (0x08065130) bunu `PlaceProbeEntries(context, 1)` diye
 * cagiriyor; ikinci arguman KULLANILMIYOR (ROM r1'i ilk is olarak `mov r1,
 * sp` ile eziyor), donus degeri uretilen giris sayisi.
 *
 * Baglam +0x74'te 12 bayt adimli dort nokta tutuyor. Her noktanin x/y'si
 * 22 bit kaydirilarak (10.22 sabit nokta) harita gozune ceviriliyor.
 * Birinci dongu koordinatlari harita sinirlarina KIRPIYOR ve gozun 7..9
 * bitlerindeki alan 4 ise 52 evreli bir giris uretip sayaci artiriyor;
 * degilse `blocked` bayragini kuruyor. Bayrak kurulduysa yalnizca 471
 * numarali tetikleme yapiliyor. Kurulmadiysa ikinci dongu ayni sinamayi
 * kirpma YERINE eleme ile (sinir disi nokta atlanir) tekrarlayip 51 evreli
 * girisler uretiyor; dordunden fazlasi tutarsa sahibin +0x0A bayragina
 * 0x02 ekleniyor, ardindan baglamin kendi ucusu icin bir giris daha ve 470
 * numarali tetikleme geliyor.
 *
 * ROM tablosu gRom08852A1C = { -128, 128, -384, 384 }: 16 bayt tek blokta
 * (`ldmia/stmia` + `ldr/str`) yerel diziye kopyalaniyor, yani kaynakta
 * yerel bir toplu atama var. Kimlik `(id + off.v[i]) & 0x3FF` ile
 * turetiliyor.
 *
 * Olculen dort kural (hepsi bu fonksiyonda kanitlandi):
 *
 * 1. Goz degeri MUTLAKA `u32` yerele alinmali. Dogrudan
 *    `(*p & 0x380) >> 7` yazimi ifadeyi HImode'da tutuyor: `movs #0xe0 /
 *    lsls #2 / adds r0,r2,#0` fazladan kopyasi + 16/23 kaydirma ciftleri,
 *    yani dongu basina 4 fazla RTL komutu. `s32` yerel 2 bayt saptiriyor.
 * 2. Bu 4 komut, dongunun geri sayan sayacini da belirliyor: agbcc'nin
 *    `check_dbra_loop` gecisi givleri indirgemek icin
 *    `omur * 15 * kazanc >= komut_sayisi` istiyor; `off.v[i]` givinin
 *    kazanci 4 (1*15*4 = 60), HImode yaziminda dongu 63 komut oldugu icin
 *    giv indirgenmiyor, biv olu kalmiyor ve dongu `movs #3 / negs` yerine
 *    yukari sayiyor (loop dump: "giv of insn 170 not worth while, 60 vs 63").
 * 3. `gRam0201AEE8` iki dongude ORTAK bir yerel isaretciye alinamaz:
 *    paylasilan degisken referans sayisini 16'ya cikarip onceligi 3.2'ye
 *    tasiyor ve isaretci r0'i kapiyor (ROM: birinci dongude r3, ikincide
 *    r2). Dogrudan global erisim -- ya da dongu basina AYRI yerel --
 *    dogru dagitimi veriyor (kural 59'un uc-ayri-isaretci notuyla ayni kol).
 * 4. Satir ofseti AYRI bir yerele alinmali:
 *      row = y * gRam0201AEE8->width;  tile = *(tiles + x + row);
 *    Tek ifadede `... + y * gRam...->width` yazarsan `muls` hedefi y'nin
 *    yazmacina bagleniyor (`muls r1,r0`); ROM `muls r0,r1` ile carpimi
 *    ENIN yazmacina koyuyor. Operandi ters cevirmek (`width * y`) hedefi
 *    duzeltiyor ama fazladan bir yukleme pseudo'su uretip en/isaretci/y
 *    dagitimini bozuyor (23 bayt fark). Ara degisken ikisini birden veriyor.
 *
 * Toplam adres ifadesi `*(tiles + x + row)` iki AYRI olcekleme uretiyor
 * (ROM: `lsls #1` iki kez, ayri `adds`); `tiles[x + row]` dizi yazimi tek
 * olcekleme yapip 4 bayt saptiriyor.
 *
 * Elenen yazimlar: `tiles[y*w + x]` dizi indeksi; `tiles + y*w + x` ve
 * `tiles + w*y + x` toplama sirasi (34 bayt); `row = width * y` (171);
 * `cell = tiles + x; cell[y*w]` (boy tutmuyor); goz degeri `s32` (11);
 * en/boy icin `w`/`h` yerelleri (23); paylasilan `grid` yerel isaretcisi
 * (23); on bildirim sirasinin 72 permutasyonu (hicbiri etkilemiyor).
 *
 * ESLESME: 408/408 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/slot_probe.c
 */


#include "gba_types.h"
#include "map_grid.h"

#define SPOT_COUNT     4
#define TILE_FIELD     0x380        /* bit 7..9 */
#define TILE_SHIFT     7
#define TILE_WANTED    4
#define ID_MASK        0x3FF
#define PHASE_PLACE    52
#define PHASE_SPAWN    51
#define OWNER_BIT      2
#define SOUND_OK       470
#define SOUND_FAIL     471
#define CROWD_LIMIT    3

typedef struct Triple {
    s32 x;                      /* +0x00 */
    s32 y;                      /* +0x04 */
    s32 z;                      /* +0x08 */
} Triple;

typedef struct Owner {
    u8  pad00[10];
    u16 flags;                  /* +0x0A */
} Owner;

typedef struct Probe {
    Triple head;                /* +0x00, son cagrida kaynak uclu */
    u8     pad0c[2];
    s16    id;                  /* +0x0E */
    u8     pad10[8];
    u32    ready;               /* +0x18 */
    u8     pad1c[0x64 - 0x1C];
    Owner *owner;               /* +0x64 */
    u8     pad68[0x74 - 0x68];
    Triple spots[SPOT_COUNT];   /* +0x74 */
} Probe;

typedef struct Offsets {
    s32 v[SPOT_COUNT];
} Offsets;

extern const Offsets gRom08852A1C;

extern void CreateEntry(Triple *src, u32 arg1, u32 phase, u32 owner);
extern void FUN_08035168(s32 sound);

/* 0x080651E0 */
s32 PlaceProbeEntries(Probe *probe, s32 mode)
{
    Offsets off;
    s32     count;
    s32     blocked;
    s32     id;
    s32     placed;
    s32     i;
    s32     x;
    s32     y;
    u32     tile;
    s32     row;

    count = 0;
    blocked = 0;
    off = gRom08852A1C;

    if (probe->ready == 0)
        return 0;

    id = probe->id;

    for (i = 0; i < SPOT_COUNT; i++) {
        x = probe->spots[i].x >> 22;
        y = probe->spots[i].y >> 22;
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x >= gRam0201AEE8->width)
            x = gRam0201AEE8->width - 1;
        if (y >= gRam0201AEE8->height)
            y = gRam0201AEE8->height - 1;
        row = y * gRam0201AEE8->width;
        tile = *(gRam0201AEE8->tiles + x + row);
        if (((tile & TILE_FIELD) >> TILE_SHIFT) == TILE_WANTED) {
            CreateEntry(&probe->spots[i], (id + off.v[i]) & ID_MASK,
                        PHASE_PLACE, (u32)probe->owner);
            count++;
        } else {
            blocked = 1;
        }
    }

    if (blocked == 0) {
        placed = 0;
        for (i = 0; i < SPOT_COUNT; i++) {
            x = probe->spots[i].x >> 22;
            y = probe->spots[i].y >> 22;
            if (x >= 0 && y >= 0) {
                if (x < gRam0201AEE8->width && y < gRam0201AEE8->height) {
                    row = y * gRam0201AEE8->width;
                    tile = *(gRam0201AEE8->tiles + x + row);
                    if (((tile & TILE_FIELD) >> TILE_SHIFT) == TILE_WANTED) {
                        CreateEntry(&probe->spots[i], id, PHASE_SPAWN,
                                    (u32)probe->owner);
                        placed++;
                    }
                }
            }
        }
        if (placed > CROWD_LIMIT)
            probe->owner->flags |= OWNER_BIT;
        CreateEntry(&probe->head, id, PHASE_SPAWN, (u32)probe->owner);
        FUN_08035168(SOUND_OK);
    } else {
        FUN_08035168(SOUND_FAIL);
    }

    return count;
}
