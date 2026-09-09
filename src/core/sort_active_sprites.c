/* Sorts the active sprite list with interrupts disabled — 0x08012C54-0x08012C73.
 * The ROM does not save the old IME value; it enables interrupts on exit.
 * Verification: make c-match FILE=src/core/sort_active_sprites.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

extern void SortSpriteList(Node **head);

/* 0x08012C54 */
void SortActiveSprites(void)
{
    REG_IME = 0;
    SortSpriteList(&gNodePool.activeHead);
    REG_IME = 1;
}
