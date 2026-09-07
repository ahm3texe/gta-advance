/* Belirli turde rastgele karo arama — 0x080424EC-0x08042573
 *
 * gRam0202F3E0 baglaminin karo haritasindan (+0 baslik: u16 genislik, u16
 * yukseklik, +4 karo dizisi; baglam +0x38 satir kaydirmasi) en fazla 256
 * deneme ile, kenarlardan 8 karo iceride rastgele (x,y) secip karonun
 * 0x380 maskeli turu (>>7) istenene esitse konumu yazip 1 doner; yoksa 0.
 *
 * IKI OLCUM: gRam0202F3E0 bir isaretci DEGIL, +0'i isaretci olan bir
 * baglam (kaydirma ayni bloktan +0x38'den her turda yeniden okunuyor);
 * MapData'nin +0/+2 u16 genislik/yukseklik alanlari paylasilan govdede
 * dolgu oldugu icin cast ile okunuyor.
 * Karo adresi `(&tiles[x])[y << shift]` diye IKI AYRI OLCEKLI TERIMLE
 * yazilmali; `tiles[x + (y << shift)]` toplami once birlestirip 17 komut
 * sapiyor. Maske yereli sart degil (ikisi de eslesiyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/find_random_tile_of_kind.c
 */

#include "gba_types.h"
#define TRIES     256
#define KIND_MASK 0x380
/* map_tiles.c ile ayni gorunum (tutarlilik kapisi): genislik/yukseklik
 * MapData'nin ilk dort baytinda, burada u16 dizisi olarak okunuyor. */
typedef struct MapData {
    u8   pad0[4];
    u16 *tiles;             /* +4 */
} MapData;
typedef struct MapContext {
    MapData *data;          /* +0 */
    u8       pad4[0x34];
    int      tileShift;     /* +0x38  satir basina karo kaydirmasi */
} MapContext;
extern MapContext gRam0202F3E0;
extern u32 FUN_08032548(void);
u32 FindRandomTileOfKind(u32 kind, u32 *outX, u32 *outY)
{
    MapData *hdr; u16 *tiles; u32 w; u32 h; s32 i; u32 x; u32 y; s32 t;
    hdr = gRam0202F3E0.data;
    tiles = hdr->tiles;
    w = ((u16 *)hdr)[0];
    h = ((u16 *)hdr)[1];
    for (i = 0; i <= TRIES - 1; i++) {
        x = ((FUN_08032548() * (w - 16)) >> 16) + 8;
        y = ((FUN_08032548() * (h - 16)) >> 16) + 8;
        t = (&tiles[x])[y << gRam0202F3E0.tileShift];
        if (kind == ((KIND_MASK & t) >> 7)) {
            *outX = x;
            *outY = y;
            return 1;
        }
    }
    return 0;
}
