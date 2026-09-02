/* Kayit serilestirme yardimcilari — 0x080010F8-0x0800114B
 *
 * Bu dosya assembly'den C'ye tasima calismasinin ilkidir. Byte-matching
 * olan fonksiyonlar asagida isaretlidir; kalanlar hala src/save/save_helpers.s
 * icindeki assembly ile uretiliyor.
 *
 * Dogrulama:  make c-match FILE=src/save/save_helpers.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* 0x080010F8 — byte-matching */
u8 ReadU8(const u8 *p)
{
    return p[0];
}

/* 0x080010FC — HENUZ ESLESMIYOR
 * ROM once isaretciyi kopyaliyor (adds r1, r0, #0) ve kaydirmayi ayri bir
 * register'a yapiyor; agbcc bu haliyle kopyayi ureteme. Ifade sirasi veya
 * gecici degisken denenecek. */
u16 ReadU16LE(const u8 *p)
{
    return (p[1] << 8) | p[0];
}

/* 0x08001108 — HENUZ ESLESMIYOR (24 byte'in 9'u farkli)
 * Yapi dogru, register dagitimi farkli. */
u32 ReadU32LE(const u8 *p)
{
    return (p[1] << 8) | p[0] | (p[2] << 16) | (p[3] << 24);
}

/* 0x08001120 — byte-matching */
void WriteU8(u8 *p, u8 v)
{
    p[0] = v;
}

/* 0x08001124 — HENUZ ESLESMIYOR
 * ROM giriste 16 bite kirpiyor (lsl #16 / lsr #16); parametre tipi veya
 * cagri sozlesmesi farkli olabilir. */
void WriteU16LE(u8 *p, u16 v)
{
    p[0] = v;
    p[1] = v >> 8;
}

/* 0x08001130 — byte-matching
 * Maskeyi literal havuzdan okumak yerine iki kez mov+lsl ile yeniden kurmasi
 * agbcc'ye ozgu; 28 byte'in tamami birebir. */
void WriteU32LE(u8 *p, u32 v)
{
    p[0] = v;
    p[1] = (v & 0xff00) >> 8;
    p[2] = (v & 0xff0000) >> 16;
    p[3] = v >> 24;
}
