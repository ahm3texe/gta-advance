/* Kayit sistemi baslatma — 0x0800082C-0x0800091B
 *
 * EEPROM kitapligini baslatir, slot sayisini 1..16 araligina sinirlar,
 * CRAWSAVE metadata imzasini dogrular veya olusturur ve slot basina kayit
 * boyutunu sekiz byte'a hizalar.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  python3 tools/verify_c_function.py src/save/init_save_system.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed int     s32;

#define REG_IME (*(volatile u16 *)0x04000208)

#define SAVE_SLOT_MIN     1
#define SAVE_SLOT_MAX     16
#define SAVE_SLOT_FLAGS   16
#define EEPROM_TOTAL      480
#define EEPROM_BLOCK_MASK 7

extern u8  gSaveMetadata[32];
extern s32 gSavePayloadSize;
extern s32 gSaveSlotCount;

extern void FUN_0806bd34(s32 mode);
extern s32  FUN_0806c0f4(s32 dividend, s32 divisor);
extern u32  ReadSaveMetadata(u8 *dest);
extern u32  WriteSaveMetadata(const u8 *src);

/* 0x0800082C — 240/240 byte BYTE-MATCHING
 *
 * Uc yazim ayrintisi olculerek bulundu; ucu de gerekli:
 *
 *  1. Bayrak temizleme dongusu ILERIYE yazilir (COMPILER.md kural 8).
 *     agbcc bunu kendisi ters cevirip ROM'daki asagi yuruyen isaretciye
 *     (adds r2,#31 / subs r2,#1, sayac 15..0) donusturuyor. Elle geriye
 *     yazmak farkli kod uretiyor — olculen ilk-fark ofsetleri:
 *       i=0..15,  [16+i] ileri  -> @176  (dongu TAM eslesiyor)
 *       i=15..0,  [16+i] geri   -> @152
 *       i=16..31, [i]    ileri  -> @112
 *       i=0..15,  [31-i]        -> @160
 *
 *  2. Bolme cagrisinin bolen argumani ONCE yerel degiskene alinir
 *     (COMPILER.md kural 11). ROM once arguman 2'yi kuruyor
 *     (ldr r0,=&gSaveSlotCount / ldr r1,[r0]), sonra sabit olan arguman
 *     1'i (movs r0,#240 / lsls r0,#1). Cagriya dogrudan gSaveSlotCount
 *     yazilirsa agbcc sabiti once kuruyor ve sira ters cikiyor (fark 23).
 *
 *  3. &gSavePayloadSize icin IKI AYRI isaretci degiskeni gerekiyor; her
 *     biri yalnizca BIR kez kullanilir. ROM adresi callee-saved r4'te
 *     tutuyor ve if blogunun icinde r3'e kopyaliyor (adds r3,r4,#0).
 *     Tek isaretciyi iki yerde kullanmak agbcc'nin &gSavePayloadSize
 *     hesabini fonksiyon basina kaldirmasina yol aciyor: fonksiyon 236
 *     byte'a dusuyor ve ilk fark 48. ofsete geri kayiyor. Ayni adres icin
 *     ikinci bir yerel degisken kullanmak kopyayi geri getiriyor.
 *     Isaretcisiz tum bicimler (global dogrudan) 41 bayt farkta kaliyor. */
s32 InitSaveSystem(s32 slotCount)
{
    s32 size;
    s32 i;
    s32 slots;
    s32 *payloadSize;

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

        /* Slot bayraklarini temizle — ileriye yaz, bkz. yukarida (1) */
        for (i = 0; i < SAVE_SLOT_MAX; i++)
            gSaveMetadata[SAVE_SLOT_FLAGS + i] = 0;

        WriteSaveMetadata(gSaveMetadata);
    }

    /* Geri donuste okunacak adres; cagrilar boyunca register'da yasar */
    payloadSize = &gSavePayloadSize;

    slots = gSaveSlotCount;
    size = FUN_0806c0f4(EEPROM_TOTAL, slots);
    gSavePayloadSize = size;

    /* Slot basina boyutu sekiz byte'in altina hizala */
    if (size & EEPROM_BLOCK_MASK) {
        s32 *alignedSize = &gSavePayloadSize;

        do {
            size--;
        } while (size & EEPROM_BLOCK_MASK);

        *alignedSize = size;
    }

    return *payloadSize;
}
