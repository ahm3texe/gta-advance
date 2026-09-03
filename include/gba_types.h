#ifndef GUARD_GBA_TYPES_H
#define GUARD_GBA_TYPES_H

/* Tum kaynaklarin kullandigi temel tipler.
 *
 * Isaretlilik decomp'ta anlamli bir tercihtir, kozmetik degil: dar bir
 * parametrenin isaretli olmasi agbcc'nin giriste normalizasyon komutu
 * uretmesine yol acar (docs/COMPILER.md kural 15). Bu yuzden s8/s16 tipleri
 * de burada tanimli. */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef signed char    s8;
typedef signed short   s16;
typedef signed int     s32;

typedef volatile unsigned char  vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int   vu32;

#endif /* GUARD_GBA_TYPES_H */
