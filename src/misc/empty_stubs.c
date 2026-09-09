/* Empty stubs — single-instruction `bx lr` functions
 *
 * The ROM has 27 two-byte functions, each a single `bx lr`. All of them are
 * CALLED with `bl`, so they are real functions -- not boundary errors.
 *
 * To be honest about HOW VALUABLE THIS IS: 54 bytes in total, so it barely
 * moves the percentage. It raises the function COUNT, not the understanding.
 * An empty body produces `bx lr` REGARDLESS of the signature, so the
 * `void f(void)` signatures here are NOT VERIFIED -- they are merely the
 * simplest form. If a caller passes an argument, the consistency checker will
 * catch it and the signature will be corrected then.
 *
 * The toolchain's interworking veneer table (0x0806C0B8-0x0806C0D8, the
 * `bx r0`..`bx r8` sequence) was left out DELIBERATELY: that is not game code.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/empty_stubs.c
 */

#include "gba_types.h"

/* 0x0800db7c */
void NoOp0800DB7C(void)
{
}

/* 0x08010198 */
void NoOp08010198(void)
{
}

/* 0x080101e4 */
void NoOp080101E4(void)
{
}

/* 0x08011d4c */
void NoOp08011D4C(void)
{
}

/* 0x080127a0 */
void NoOp080127A0(void)
{
}

/* 0x080197fc */
void NoOp080197FC(void)
{
}

/* 0x08019c04 */
void NoOp08019C04(void)
{
}

/* 0x080289b4 */
void NoOp080289B4(void)
{
}

/* 0x080289b8 */
void NoOp080289B8(void)
{
}

/* 0x08033820 */
void NoOp08033820(void)
{
}

/* 0x08033898 */
void NoOp08033898(void)
{
}

/* 0x08034fb0 */
void NoOp08034FB0(void)
{
}

/* 0x08034fc8 */
void NoOp08034FC8(void)
{
}

/* 0x08034fcc */
void NoOp08034FCC(void)
{
}

/* 0x08035cd4 */
void NoOp08035CD4(void)
{
}

/* 0x0803b19c */
void NoOp0803B19C(void)
{
}

/* 0x08041ee0 */
void NoOp08041EE0(void)
{
}

/* 0x08041ef0 */
void NoOp08041EF0(void)
{
}

/* 0x08041ef4 */
void NoOp08041EF4(void)
{
}

/* 0x08051960 */
void NoOp08051960(void)
{
}

/* 0x080563bc */
void NoOp080563BC(void)
{
}

/* 0x0805ab78 */
void NoOp0805AB78(void)
{
}

/* 0x08062370 */
void NoOp08062370(void)
{
}

/* 0x08064df8 */
void NoOp08064DF8(void)
{
}

/* 0x08067370 */
void NoOp08067370(void)
{
}

/* 0x08070d94 */
void NoOp08070D94(void)
{
}

/* 0x08070d98 */
void NoOp08070D98(void)
{
}

