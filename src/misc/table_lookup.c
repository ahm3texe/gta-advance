/* Language selection and localized text lookup — 0x0805E6C4-0x0805E6FF
 *
 * The table at 0x08EC46D4 is the game's TEXT POINTER TABLE: 618 strings per
 * language, 3090 entries in total for five languages.  gLanguage selects which
 * language is in use; the third function returns a string pointer from that
 * language's block.
 *
 * What the table is was verified FROM FOUR INDEPENDENT DIRECTIONS:
 *   1. The setter clamps the index to 0..4          -> exactly five values
 *   2. The reader's multiplier 0x9A8 = 2472 = 618*4 -> 618 strings per language
 *   3. The table size 3090 entries = 618 * 5        -> five languages
 *   4. The trace log: five options on the boot language screen
 *      (gActiveMenuItemCount 0->5, docs/GAME_FLOW.md)
 * Furthermore, the first pointer of each language block lands immediately
 * after that language's name (DEUTSCH / FRANCAIS / ITALIANO).  Details:
 * docs/TEXT_MAP.md
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/misc/table_lookup.c
 */

#include "gba_types.h"

#define LANGUAGE_MAX     4       /* five languages: 0..4 */
#define STRINGS_PER_LANG (0x9A8 / 4)   /* 618 */

extern u32 gLanguage;
extern u32 gTextTable[][STRINGS_PER_LANG];   /* 0x08EC46D4, read-only */

/* 0x0805E6C4 */
void SetLanguage(u32 index)
{
    if (index <= LANGUAGE_MAX)
        gLanguage = index;
}

/* 0x0805E6D4 */
u32 GetLanguage(void)
{
    return gLanguage;
}

/* 0x0805E6E0 */
u32 GetTextString(u32 index)
{
    return gTextTable[gLanguage][index];
}
