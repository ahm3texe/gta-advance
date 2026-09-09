#ifndef GUARD_GBA_TYPES_H
#define GUARD_GBA_TYPES_H

/* Base types shared by the project.
 *
 * Signedness affects code generation: a narrow signed parameter can cause
 * agbcc to emit normalization instructions on entry (docs/COMPILER.md,
 * rule 15). The s8 and s16 types are provided so declarations can express
 * the signedness needed to reproduce the ROM instructions. */

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
