/* Kayit sistemi baslatma — 0x0800082C-0x0800091B
 *
 * EEPROM kitapligini baslatir, slot sayisini 1..16 araligina sinirlar,
 * CRAWSAVE metadata imzasini dogrular veya olusturur ve slot basina kayit
 * boyutunu sekiz byte'a hizalar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/init_save_system.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed int     s32;

#define REG_IME (*(volatile u16 *)0x04000208)

#define SAVE_SLOT_MIN     1
#define SAVE_SLOT_MAX     16
#define SAVE_SLOT_FLAGS   16
#define SAVE_METADATA_END 31
#define EEPROM_TOTAL      480
#define EEPROM_BLOCK_MASK 7

extern u8  gSaveMetadata[32];
extern s32 gSavePayloadSize;
extern s32 gSaveSlotCount;

extern void FUN_0806bd34(s32 mode);
extern s32  FUN_0806c0f4(s32 dividend, s32 divisor);
extern u32  ReadSaveMetadata(u8 *dest);
extern u32  WriteSaveMetadata(const u8 *src);

/* 0x0800082C — HENUZ ESLESMIYOR (240 byte'in 197'si tutuyor)
 *
 * Yapi dogru; iki kume fark kaldi:
 *   1. Slot bayraklarini temizleyen dongude ROM isaretciyi +31'den asagi
 *      yuruturken bizimki +16'dan yukari yuruyor. Sayac (15..0) ayni.
 *      Denenen bicimler: [31-i] artan, [16+i] artan, [16+i] azalan,
 *      [31-i] azalan, acik isaretci yuruyusu, i=16..31 ileri. En iyisi
 *      [16+i] artan (41 bayt fark).
 *   2. ROM &gSavePayloadSize'i bolme cagrisindan ONCE callee-saved bir
 *      register'a aliyor ve ucunu de oradan yaziyor; bizimki her seferinde
 *      literal havuzdan okuyor. Yerel isaretci denendi: fonksiyon basinda
 *      tanimlaninca 220 bayta cikiyor, kullanim yerinde tanimlaninca 80.
 *
 * Blok tamamlanana kadar src/save/init_save_system.s gecerli build
 * kaynagidir. */
s32 InitSaveSystem(s32 slotCount)
{
    s32 size;
    s32 i;

    REG_IME = 0;
    FUN_0806bd34(4);
    REG_IME = 1;

    gSaveSlotCount = slotCount;
    if (slotCount <= 0)
        gSaveSlotCount = SAVE_SLOT_MIN;
    else if (slotCount > SAVE_SLOT_MAX - 1)
        gSaveSlotCount = SAVE_SLOT_MAX;

    ReadSaveMetadata(gSaveMetadata);

    if (gSaveMetadata[0] != 'C' || gSaveMetadata[1] != 'R'
        || gSaveMetadata[2] != 'A' || gSaveMetadata[3] != 'W'
        || gSaveMetadata[4] != 'S' || gSaveMetadata[5] != 'A'
        || gSaveMetadata[6] != 'V' || gSaveMetadata[7] != 'E'
        || gSaveMetadata[8] != (u8)gSaveSlotCount) {
        gSaveMetadata[0] = 'C';
        gSaveMetadata[1] = 'R';
        gSaveMetadata[2] = 'A';
        gSaveMetadata[3] = 'W';
        gSaveMetadata[4] = 'S';
        gSaveMetadata[5] = 'A';
        gSaveMetadata[6] = 'V';
        gSaveMetadata[7] = 'E';
        gSaveMetadata[8] = gSaveSlotCount;

        for (i = 0; i < SAVE_SLOT_MAX; i++)
            gSaveMetadata[SAVE_SLOT_FLAGS + i] = 0;

        WriteSaveMetadata(gSaveMetadata);
    }

    size = FUN_0806c0f4(EEPROM_TOTAL, gSaveSlotCount);
    gSavePayloadSize = size;

    if (size & EEPROM_BLOCK_MASK) {
        do {
            size--;
        } while (size & EEPROM_BLOCK_MASK);
        gSavePayloadSize = size;
    }

    return gSavePayloadSize;
}
