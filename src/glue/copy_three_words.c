/* Copy three words between two records — 0x0804FB2C-0x0804FB3B
 *
 * The ROM moves them with `ldmia r0!,{r1,r3,r4}` and `stmia r2!,{r1,r3,r4}`,
 * which is what agbcc emits for a struct assignment of exactly twelve bytes.
 * Three separate word assignments produce three loads and three stores instead.
 *
 * The `push {r4}` is for the third register the block move needs, not for
 * anything the source names.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/glue/copy_three_words.c
 */

#include "gba_types.h"

typedef struct Triple {
    u32 a;
    u32 b;
    u32 c;
} Triple;

typedef struct TripleOwner {
    u8      pad00[0x18];
    Triple *record;             /* +0x18 */
} TripleOwner;

/* 0x0804FB2C */
void FUN_0804fb2c(TripleOwner *dest, TripleOwner *src)
{
    *dest->record = *src->record;
}
