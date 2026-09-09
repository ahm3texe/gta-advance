/* Show a level badge — 0x080306C8-0x08030847 (384 bytes)
 *
 * STATUS: 146/181 instructions, NEAR MISS; size matches.
 * Exit if gFrameCounterEwram is zero. Draw its two digits with DrawTwoDigits'
 * tile scheme at 0x0600981A/0x0600985A, replacing a leading zero with empty
 * tile. Select descriptor gRom08BD3448.slots[34]->slots[49]->slots
 * [language*6+index] using GetLanguage. Exit if gRam02025810[0x137E] already
 * equals index+1. If its old tag is nonzero, clear two three-tile rows and
 * ReleaseObject(gRam02026E80). Write the new tag, initialize attributes
 * (75-w/2,8-h/2,+0x26=0), call FUN_08013cfc with 0 and 1, then
 * FUN_08014ee4 and FUN_08015038.
 *
 * The remaining 35 instructions differ only in register roles, not logic/count:
 * - top/bot ROM r3/r2 vs ours r2/r3. Tried bot-first assignment, declaration
 *   order, and pointer instead of digits[i] (146 -> 142).
 * - Clear-loop k/blank/top/bot are cyclically shifted (ROM k=r2, blank
 *   r7->r3, bot=r1, top=r0). Tried k first, do-while, for, bot++ first (144-146).
 * - Tag r9 and size-byte r7 roles.
 * Rule-44 class. Before another attempt, inspect InitSessionAttr's cast/array
 * difference and DrawTwoDigits' base local.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/ui/show_level_badge.c
 */

#include "gba_types.h"
#define TILE_BLANK    0xF0E8
#define DIGIT_TOP     0xF8
#define DIGIT_BOTTOM  0x102
#define DIGIT_BLANK   10
#define TILE_ATTR     0xF000
#define TOP_ROW       ((vu16 *)0x0600981A)
#define BOT_ROW       ((vu16 *)0x0600985A)
#define CLR_TOP       ((vu16 *)0x06009818)
#define CLR_BOT       ((vu16 *)0x06009858)
#define BADGE_SLOT    0x137E
#define ROOT_SLOT     34
#define LANG_SLOT     49
typedef struct RomNode { u32 pad00; struct RomNode **slots; u8 pad08[8]; u8 **size; } RomNode;
extern RomNode gRom08BD3448;
extern u32 gFrameCounterEwram;
extern u8  gRam02025810[];
extern u32 gRam02026E80;
extern const u8 gRom08CA635C[];
extern s32  Div(s32 numerator, s32 denominator);
extern u32  GetLanguage(void);
extern void ReleaseObject(u8 *attr);
extern void FUN_08014ffc(u8 *dest, u32 count, void *src, u32 *value);
extern void FUN_08013cfc(u8 *dest, RomNode *desc, u32 arg);
extern void FUN_08014ee4(u8 *dest, const u8 *value);
extern void FUN_08015038(u8 *dest);
void ShowLevelBadge(s32 value, s32 index)
{
    s32 digits[8]; vu16 *top; vu16 *bot; s32 d; s32 i; s32 tag;
    RomNode *desc; u8 *size; u8 *attr; s32 w; s32 h; u32 blank; u32 k;
    if (gFrameCounterEwram == 0)
        return;
    digits[1] = Div(value, 10);
    digits[0] = value - digits[1] * 10;
    if (digits[1] == 0)
        digits[1] = DIGIT_BLANK;
    top = TOP_ROW;
    bot = BOT_ROW;
    tag = index + 1;
    for (i = 0; i < 2; i++) {
        d = digits[i];
        if (d == DIGIT_BLANK) {
            *top-- = TILE_BLANK;
            *bot = TILE_BLANK;
        } else {
            *top-- = (DIGIT_TOP + d) | TILE_ATTR;
            *bot = (DIGIT_BOTTOM + d) | TILE_ATTR;
        }
        bot--;
    }
    desc = gRom08BD3448.slots[ROOT_SLOT]->slots[LANG_SLOT]->slots[GetLanguage() * 6 + index];
    size = *desc->size;
    if (gRam02025810[BADGE_SLOT] == tag)
        return;
    if (gRam02025810[BADGE_SLOT] != 0) {
        blank = TILE_BLANK;
        bot = CLR_BOT;
        top = CLR_TOP;
        for (k = 0; k <= 2; k++) {
            *top = blank;
            *bot = blank;
            bot++;
            top++;
        }
        ReleaseObject((u8 *)&gRam02026E80);
        gRam02025810[BADGE_SLOT] = 0;
    }
    gRam02025810[BADGE_SLOT] = tag;
    attr = (u8 *)&gRam02026E80;
    FUN_08014ffc(attr, 0, 0, 0);
    w = 75 - (size[0] >> 1);
    h = 8 - (size[1] >> 1);
    if (attr != 0) {
        *(u16 *)(attr + 8) = w;
        *(u16 *)(attr + 10) = h;
        if (attr != 0)
            attr[0x26] = 0;
    }
    FUN_08013cfc(attr, desc, 0);
    FUN_08013cfc(attr, desc, 1);
    FUN_08014ee4(attr, gRom08CA635C);
    FUN_08015038(attr);
}
