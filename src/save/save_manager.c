/* Kayit yoneticisi: slot tarama, yukleme ve yazma — 0x08000C28-0x08000DDC
 *
 * Uc kayit slotu EEPROM'da 160 byte arayla durur. Her slotun ilk 12 byte'i
 * EWRAM'daki gSaveSlotHeaders tablosuna kopyalanir. Slot ancak baslik
 * byte'i (marker) ile slotun 156. byte'indaki tumleyeninin toplami 255
 * oldugunda gecerlidir; marker sifirsa slot bostur.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/save/save_manager.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed int     s32;

typedef struct {
    const void *src;
    void *dst;
    u32 control;
} DmaChannel;

/* Kural 1'in tersi yonu (read/write_eeprom_range.c ile ayni): DMA3 sabit
 * cast olarak yazilir ama struct uyesi olarak, cunku ROM tabani (0x040000D4)
 * register'da tutup ofsetle (`ldr r0, [r2, #8]`) eriyor. */
#define REG_DMA3   (*(volatile DmaChannel *)0x040000D4)
#define REG_IME    (*(volatile u16 *)0x04000208)   /* kural 12: volatile */
#define DMA_ENABLE 0x80000000

/* Uc kayit slotunun EWRAM'daki basligi; 12 byte'lik girisler. */
typedef struct {
    u8 marker;
    u8 data[11];
} SaveSlotHeader;

#define SAVE_SLOT_COUNT     3
#define SAVE_SLOT_STRIDE  160   /* EEPROM'da slotlar arasi mesafe        */
#define SAVE_HEADER_SIZE   12   /* slot basinda tutulan baslik boyutu    */
#define SLOT_COMPLEMENT   156   /* marker'in tumleyeni slotun bu byte'i  */
#define MARKER_CHECKSUM   255   /* marker + tumleyen bu degeri vermeli   */
#define EEPROM_DEVICE_TYPE  4   /* tanimlama rutinine verilen tip kodu   */
#define SAVE_SETTLE_LOOPS   3   /* EEPROM erisimi sonrasi kisa bekleme   */

extern SaveSlotHeader gSaveSlotHeaders[SAVE_SLOT_COUNT];
extern u8 gSaveBuffer[SAVE_SLOT_STRIDE];

/* 0x02000EB8: sifirdan farkliyken EEPROM tanimli ve erisim surüyor. */
extern u32 gEepromAvailable;

/* Kayit yoneticisi acilirken sifirlanan durum sozcukleri; rolleri
 * data/ram_map.csv'de henuz cozulmedi. */
extern u32 gState0;
extern u32 gState1;
extern u8  gState2;
extern u16 gState3;

/* Kayit G/C bolgesine giris/cikis (WAITCNT ve benzeri kurulum); adlari
 * data/functions.csv'de henuz cozulmedi. */
extern void FUN_080337a8(void);
extern void FUN_08033b74(void);

/* EEPROM tanimlama: sifirdan farkli bir u16 ile hata bildirir. */
extern u16 FUN_0806bd34(u32 deviceType);

/* Yazma oncesi hazirlik, ve slot marker'i icin deger uretici. */
extern void FUN_0802fe44(void);
extern u32  FUN_08032548(void);

extern u32 ReadEepromRange(u32 offset, u8 *dest, s32 length);
extern u32 WriteEepromRange(u32 offset, const u8 *src, s32 length);

/* 0x08000C28 — EEPROM'u tanit, uc slot basligini oku ve dogrula.
 *
 * Donus: EEPROM kullanilabilir ise 1, tanimlama basarisiz ise 0.
 *
 * Tarama dongusunun iki byte ofseti (baslik tablosundaki ve EEPROM'daki)
 * acik yerel degisken olmali. `gSaveSlotHeaders[i]` / `i * SAVE_SLOT_STRIDE`
 * yazilinca agbcc ikisini de dongu-indeksinden turetilmis giv'e ceviriyor ve
 * ucuncu bir giv (`i * 160 + 156`) daha uretip onu her adimda 160 artiriyor;
 * ROM'da o ucuncu register yok. Acik degisken kullanilinca giv uretilmiyor ve
 * on-dongu sirasi da ROM'unkiyle ayni oluyor (once baslik ofseti, sonra
 * EEPROM ofseti, sonra sabit). */
u32 InitSaveManager(void)
{
    u8 complement;            /* yigindaki tek byte'lik tampon; kural 3'un
                                 gerektirdigi `volatile` burada gerekmiyor,
                                 iki bicim de ayni byte'lari veriyor */
    u32 available;
    u32 headerOffset;
    u32 offset;
    s32 complementBias;
    int i;

    FUN_080337a8();

    gState0 = 0;
    gState1 = 0;
    gState2 = 0;
    gState3 = 0;
    REG_IME = 0;

    while (REG_DMA3.control & DMA_ENABLE)
        ;

    gEepromAvailable = 1;
    available = 1;

    /* Kural 8: ileri dongu; agbcc bunu geriye giden isaretci yuruyusune
     * cevirir ve ROM'daki bicim odur. */
    for (i = 0; i < SAVE_SLOT_COUNT; i++)
        gSaveSlotHeaders[i].marker = 0;

    if (FUN_0806bd34(EEPROM_DEVICE_TYPE) != 0) {
        available = 0;
    } else {
        headerOffset = 0;
        offset = 0;
        /* Tumleyen byte'in ofseti negatif tutulur: ROM sabiti register'da
         * -156 olarak saklayip cikarma yapiyor (`mov r1, r8` / `sub r0, r5,
         * r1`). `offset + 156` yazilinca agbcc iki adet ani-deger toplamasi
         * uretiyor ve sabit register'a hic tasinmiyor. */
        complementBias = -SLOT_COMPLEMENT;

        for (i = 0; i < SAVE_SLOT_COUNT; i++) {
            SaveSlotHeader *header =
                (SaveSlotHeader *)((u8 *)gSaveSlotHeaders + headerOffset);

            if (ReadEepromRange(offset, (u8 *)header, SAVE_HEADER_SIZE) == 0)
                header->marker = 0;
            else if (ReadEepromRange(offset - complementBias,
                                     (u8 *)&complement, 1) == 0)
                header->marker = 0;
            else if (header->marker + complement != MARKER_CHECKSUM)
                header->marker = 0;

            headerOffset += SAVE_HEADER_SIZE;
            offset += SAVE_SLOT_STRIDE;
        }
    }

    gEepromAvailable = 0;
    REG_IME = 1;
    FUN_08033b74();
    return available;
}

/* 0x08000D20 — bir slotu gSaveBuffer'a okur ve saglamasini dogrular. */
u32 LoadSaveSlot(u32 slot)
{
    SaveSlotHeader *header;
    u32 loaded;
    int i;

    if (slot >= SAVE_SLOT_COUNT)
        return 0;

    /* GetSaveSlotHeader'in (0x080010D4) satir ici hali: bos slot icin bos
     * isaretci, sonra isaretci sinamasi. Iki asamayi birlestirmek ROM'daki
     * `cmp r1, #0` sinamasini yok ediyor. */
    if (gSaveSlotHeaders[slot].marker == 0)
        header = 0;
    else
        header = &gSaveSlotHeaders[slot];

    if (header == 0)
        return 0;

    loaded = ReadEepromRange(slot * SAVE_SLOT_STRIDE, gSaveBuffer,
                             SAVE_SLOT_STRIDE);
    if (loaded != 0) {
        if (gSaveBuffer[SLOT_COMPLEMENT] + gSaveBuffer[0] != MARKER_CHECKSUM)
            return 0;
        if (gSaveBuffer[0] == 0)
            return 0;
    }

    for (i = SAVE_SETTLE_LOOPS; i >= 0; i--)
        ;

    return loaded;
}

/* 0x08000D80 — gSaveBuffer'i marker/tumleyen ciftiyle damgalayip slota yazar. */
u32 WriteGameSaveSlot(u32 slot)
{
    SaveSlotHeader *header;
    u32 written;
    u32 marker;
    int i;

    FUN_0802fe44();

    if (slot >= SAVE_SLOT_COUNT)
        return 0;

    /* Marker sifir olamaz: sifir slotu bos gosterir. */
    do {
        marker = FUN_08032548();
        gSaveBuffer[0] = marker;
    } while ((u8)marker == 0);

    gSaveBuffer[SLOT_COMPLEMENT] = ~gSaveBuffer[0];
    /* Kural 2'nin tersi yonu: burada ara isaretci degiskeni SART. Dogrudan
     * `gSaveSlotHeaders[slot] = ...` yazilinca agbcc taban adresi indeks
     * hesabindan once yukluyor; ROM once `slot * 12`'yi kuruyor, tabani
     * sonra okuyor. */
    header = &gSaveSlotHeaders[slot];
    *header = *(SaveSlotHeader *)gSaveBuffer;

    written = WriteEepromRange(slot * SAVE_SLOT_STRIDE, gSaveBuffer,
                               SAVE_SLOT_STRIDE);

    for (i = SAVE_SETTLE_LOOPS; i >= 0; i--)
        ;

    return written;
}
