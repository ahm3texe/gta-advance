/* Serbest listeden dugum ayirma — 0x08012C0C-0x08012C53
 *
 * Bos listenin basindan bir dugum alip aktif listenin basina takiyor.
 * Liste baglantilari REG_IME ile korunuyor (kesmeler kapali).
 * Bos liste bossa 0 donuyor.
 *
 * Havuz duzeni disassembly'den olculdu:
 *     +0x000  dugumler (2048 bayt = 128 x 16)
 *     +0x800  bos liste basi     -> `movs #128 / lsls #4` ile kuruluyor
 *     +0x804  aktif liste basi   -> HAVUZDAN yukleniyor (2052 kurulamıyor)
 * Iki ofsetin farkli uretilmesi, 2048'in (8-bit << kaydirma) ile ifade
 * edilebilmesinden, 2052'nin edilememesinden kaynaklaniyor.
 *
 * Taban duz yuklenip ofset ayri kuruluyor -> dizi aritmetigi degil
 * YAPI UYESI erisimi (bkz. flush_palette_queue.c'deki ayni ders).
 *
 * `pop {r1}; bx r1` ve r0'da deger -> DEGER donduruyor (kural 35'in tersi).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/alloc_node.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

/* 0x08012C0C */
Node *AllocNode(void)
{
    Node *node;
    Node *old;

    node = gNodePool.freeHead;
    if (node == 0)
        return 0;

    gNodePool.freeHead = node->next;
    node->prev = 0;
    old = gNodePool.activeHead;
    node->next = old;
    REG_IME = 0;
    gNodePool.activeHead = node;
    if (old != 0)
        old->prev = node;
    REG_IME = 1;
    return node;
}
