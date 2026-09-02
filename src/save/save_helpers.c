/* Kayit serilestirme yardimcilari — 0x080010F8-0x0800114B
 *
 * Bu dosya assembly'den C'ye tasima calismasinin ilkidir. Byte-matching
 * olan fonksiyonlar asagida isaretlidir; kalanlar hala src/save/save_helpers.s
 * icindeki assembly ile uretiliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
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

/* 0x080010FC — byte-matching */
u16 ReadU16LE(const u8 *p)
{
    return (p[1] << 8) | p[0];
}

/* 0x08001108 — byte-matching */
u32 ReadU32LE(const u8 *p)
{
    return (p[1] << 8) | p[0] | (p[2] << 16) | (p[3] << 24);
}

/* 0x08001120 — byte-matching */
void WriteU8(u8 *p, u8 v)
{
    p[0] = v;
}

/* 0x08001124 — HENUZ ESLESMIYOR (8 byte'in 6'si farkli)
 *
 * ROM giriste degeri 16 bite kirpiyor:
 *     lsls r1, r1, #16 ; lsrs r1, r1, #16
 * Bu kirpma anlamsal olarak gereksiz (strb zaten alt bayti alir), bu yuzden
 * old_agbcc onu eliyor. Denenip TUTMAYANLAR:
 *   u32/int parametre + u16 yerel; 0xffff maskesi; (u16) cast; (u8) castlar;
 *   v >>= 8 yerinde kaydirma; v / 256; v & 255; i<2 dongusu; *p++ yazimi.
 * TEK IPUCU: p[1] once yazilinca old_agbcc kirpmayi uretiyor, ama sirayi
 * bozdugu icin tutmuyor. Muhtemelen deger ayri bir u16 nesneden geliyor.
 * Bu fonksiyon cozulene kadar src/save/save_helpers.s icindeki assembly
 * gecerli kaynaktir. */
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
