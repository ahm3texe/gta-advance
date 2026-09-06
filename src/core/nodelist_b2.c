/* Alan baglamini yeni bir tanimla kurma -- 0x08053834, 250 bayt.
 *
 * Uc arguman aliyor: alan baglami, tanim yapisi ve bir ad isaretcisi.
 * Tanimdaki sayilara gore baglamin dizilerini kuruyor ve dugumleri
 * baglantiya diziyor. Kardesi FUN_08053794 (src/core/nodelist_a9.c) ile
 * ayni yapi ailesini paylasiyor.
 *
 * DURUM: PARK, 246/250 (dort bayt KISA), fark 137.
 *
 * KAYNAGI: bu dosyayi yazan ajan KOTA SINIRINA takilip yarida kaldi;
 * dosyayi "TASLAK" basligiyla birakti ve ELENEN YOLLARI KAYDETMEDI.
 * Yani hangi yazimlarin denenip tutmadigi BILINMIYOR. Buraya donen kisi
 * denemelere bastan baslayacak; ne denediginizi YAZIN.
 *
 * SONRAKI ADIM: once teshis --
 *   python3 tools/dump_alloc.py src/core/nodelist_b2.c FUN_08053834 --rom
 * callee-saved sayimlari (r4..r7) ROM ile tutuyorsa dagitim dogrudur ve
 * kural 50 kaldiraci (degisken bolme) BOS EMEKTIR; sapiyorsa kural 50
 * anlamlidir. Dort bayt kisa olmasi kaynakta bir seyin EKSIK oldugunu da
 * gosteriyor, once komut komut diff alin.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b2.c
 */
#include "gba_io.h"

#define ID_NONE       0x7FFF
#define DMA_FILL_32   0x85000008

typedef struct AreaName {
    const char *name;
    u8          pad04[24];
} AreaName;

typedef struct AreaBank {
    u8        pad00[8];
    s32       nameCount;
    u8        pad0C[20];
    AreaName *names;
} AreaBank;

typedef struct AreaDesc {
    u8   pad00[4];
    u8   count;
    u8   pad05[3];
    u16 *ids;
} AreaDesc;

typedef struct AreaCtx {
    u8        pad00[10];
    u8        ready;
    u8        pad0B[9];
    AreaDesc *desc;
    u16      *slots;
    u16      *single;
} AreaCtx;

extern AreaBank gAreaBank;
extern u32      gRam02030C00;

extern s32  FUN_0806dd18(const char *a, const char *b);
extern u16 *FindFreeSlotRun(int count);
extern void FUN_08053650(s32 id);
extern void LinkAreaEntryIfEligible(s32 index);

/* 0x08053834 */
u32 FUN_08053834(AreaCtx *ctx, AreaDesc *desc, const char *name)
{
    volatile u32 fill;
    u16  ime;
    int  i;
    int  raw;
    u16  idx;
    u16 *slot;
    u16 *dst;
    u16 *src;

    gRam02030C00 = 0;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = (void *)&fill;
    REG_DMA3.dst = ctx;
    REG_DMA3.control = DMA_FILL_32;
    REG_DMA3.control;
    REG_IME = ime;

    if (name == 0) {
        raw = ID_NONE;
        goto found;
    }
    for (i = 0; i < gAreaBank.nameCount; i++) {
        if (FUN_0806dd18(gAreaBank.names[i].name, name) == 0) {
            raw = i;
            goto found;
        }
    }
    raw = ID_NONE;
found:
    idx = raw;

    if (idx != ID_NONE) {
        slot = FindFreeSlotRun(1);
        ctx->single = slot;
        if (slot == 0)
            goto fail;
        *slot = idx;
        FUN_08053650(idx);
    }

    dst = FindFreeSlotRun(desc->count);
    ctx->slots = dst;
    if (desc->count != 0) {
        if (dst != 0)
            goto ready;
fail:
        return 0;
    }
ready:

    dst = ctx->slots;
    src = desc->ids;
    for (i = 0; i < desc->count; i++, dst++, src++) {
        *dst = *src;
        LinkAreaEntryIfEligible(*dst);
    }

    ctx->desc = desc;
    ctx->ready = 0xFF;
    return 1;
}
