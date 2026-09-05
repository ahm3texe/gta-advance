/* Sikistirilmis bir kare acip DMA ile ekrana yaziyor. 0x08003454, 152 bayt.
 *
 * gRam02001440 bir {tablo tabani, indeks} cifti tutuyor; tablodaki her giris
 * 8 bayt: ilk kelime LZ77 sikistirilmis KARO verisi, ikincisi varsa PALET.
 * Karo her zaman aciliyor ve 0x06000040'a, palet varsa 0x05000000'a
 * gonderiliyor. Sonra indeks bir artiyor ve tablonun sonuna (ilk kelimesi 0
 * olan girise) gelinince sifirlaniyor -- yani kareler donguye giriyor.
 *
 * Iki aktarim da kesmeler kapaliyken yapiliyor: REG_IME saklanip sifirlaniyor,
 * DMA3 kurulduktan sonra geri yaziliyor. Ayni kalip flush_palette_queue.c'de
 * de var.
 *

 * DMA kurulduktan sonra denetim yazmaci GERI OKUNUYOR (`ldr r0,[r4,#8]`).
 * Islevsel gorunmuyor ama ROM'da duruyor ve atlanirsa dort bayt eksiliyor;
 * flush_palette_queue.c'de de ayni satir var.
 *
 * Ghidra'nin sondaki "Could not recover jumptable" uyarisi YANILTICI: orada
 * tablo yok, `pop {r0}; bx r0` dizisinin interworking donusu var (kural 35,
 * donus tipi void). Ayni yanilgi FUN_08017628'de de olmustu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/cutscene_frame.c
 */

#include "gba_types.h"
#include "gba_io.h"
#include "ram_symbols.h"

#define TILE_DEST 0x06000040
#define PAL_DEST  0x05000000
#define TILE_CTRL 0x80004B00
#define PAL_CTRL  0x80000080

typedef struct Frame {
    const void *tiles;          /* +0x00 */
    const void *palette;        /* +0x04 */
} Frame;

typedef struct FrameTable {
    Frame *frames;              /* +0x00 */
    s32    index;               /* +0x04 */
} FrameTable;

extern void LZ77UnCompWram(const void *src, void *dst);

void ShowNextCutsceneFrame(void)
{
    FrameTable *table;
    void *buf;
    u16 ime;
    const void *pal;
    s32 i;

    buf = (void *)gRam02001450;
    table = (FrameTable *)gRam02001440;

    LZ77UnCompWram(table->frames[table->index].tiles, buf);
    ime = REG_IME;
    REG_IME = 0;
    REG_DMA3.src = buf;
    REG_DMA3.dst = (void *)TILE_DEST;
    REG_DMA3.control = TILE_CTRL;
    REG_DMA3.control;
    REG_IME = ime;

    pal = table->frames[table->index].palette;
    if (pal != 0) {
        LZ77UnCompWram(pal, buf);
        ime = REG_IME;
        REG_IME = 0;
        REG_DMA3.src = buf;
        REG_DMA3.dst = (void *)PAL_DEST;
        REG_DMA3.control = PAL_CTRL;
        REG_DMA3.control;
        REG_IME = ime;
    }

    /* ROM indeksi YERINDE artiriyor (`adds r0,#1`) ve ARTMIS degerle
     * indeksliyor; `i + 1` diye ayri bir deger uretmek ofseti 8'e katliyor
     * ve uc komut sapiyor. */
    i = table->index;
    i++;
    table->index = i;
    if (table->frames[i].tiles == 0) table->index = 0;
}
