/* HasWantedEntry — 0x08028E3C-0x08028E6B
 *
 * Ilk fonksiyon gEntriesA'da 14 giriste (148 stride) +0 aktif ve
 * +100 == 101 olan var mi diye tariyor. Ikincisi bes fonksiyonluk
 * kare zinciri.
 *
 * BYTE-MATCHING. `base` yerelini `kind` ve `cur`dan ayri tutmak ROM'daki
 * r0 taban yuklemesi ile iki ayri isaretci yasam araligini korur; bu ayni
 * zamanda literal havuzunu dogru konuma yerlestirir.
 *
 * Kardesi FrameChain: src/world/frame_chain.c
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/kind_scan.c
 */

#include "gba_types.h"

#define ENTRY_STRIDE  148
#define END_OFFSET    0x818
#define WANTED_KIND   101

typedef struct Entry {
    u8  active;                 /* +0x00 */
    u8  pad01[99];
    u8  kind;                   /* +0x64 */
    u8  pad65[47];              /* stride 148 */
} Entry;

extern Entry gEntriesA[];

extern void FUN_08029130(void);
extern void FUN_08027f48(void);
extern void FUN_08019800(void);
extern void FUN_08019a64(void);
/* 0x08028E3C */
u32 HasWantedEntry(void)
{
    u8 *base;
    u8 *cur;
    u8 *kind;
    u8 *end;

    base = (u8 *)gEntriesA;
    kind = base + 100;
    cur  = base;
    end  = cur + END_OFFSET;

    do {
        if (*cur != 0) {
            if (*kind == WANTED_KIND)
                return 1;
        }
        kind += ENTRY_STRIDE;
        cur  += ENTRY_STRIDE;
    } while (cur <= end);

    return 0;
}
