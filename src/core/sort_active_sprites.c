/* Aktif sprite listesini kesmeler kapaliyken siralar — 0x08012C54-0x08012C73.
 * ROM eski IME degerini saklamaz; cikista kesmeleri etkinlestirir.
 * Dogrulama: make c-match FILE=src/core/sort_active_sprites.c
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
