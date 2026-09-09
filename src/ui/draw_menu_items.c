/* Draw menu rows and monetary values — 0x080011EC-0x080013AC
 *
 * Show at most eight rows. Resolve each row's text ID; if value is nonnegative,
 * draw a dollar-prefixed seven-digit number to its right, suppressing leading
 * zeroes. Use the game's BIOS Div wrapper and compute remainder as
 * value - quotient*divisor. agbcc converts constant products into shift chains,
 * as in the ROM.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm (docs/COMPILER.md)
 * Verification: make c-match FILE=src/ui/draw_menu_items.c
 */

#include "gba_io.h"

/* BG1 vertical scroll register. A constant cast produces a pool load
 * as in the ROM; agbcc cannot construct this address with shifts. */

#define MENU_VISIBLE_MAX  8    /* maximum simultaneous rows */
#define MENU_ROW_HEIGHT   16   /* row height in pixels */
#define STYLE_SELECTED    160  /* selected-row text style */
#define STYLE_NORMAL      192  /* other-row text style */
#define TITLE_X           120
#define TITLE_Y_OFFSET    32   /* header is 32 pixels above the first row */
#define LABEL_X           120
#define VALUE_LABEL_X     4
#define VALUE_X           236
#define VALUE_DIGITS      7    /* 0..9999999 */
#define VALUE_TEXT_SIZE   9    /* '$' + seven digits + terminator */

typedef struct MenuItem {
    u32 unk00;
    u32 label;   /* +4: text ID */
    int value;   /* +8: negative suppresses the number */
} MenuItem;

extern u8 gMenuPositionX;
extern u8 gActiveMenuItemCount;
extern MenuItem *gActiveMenuItems[20];

/* Names unresolved; use the names in data/functions.csv. */
extern void SetFontIndex(int style);            /* 0x0806434C sets text style */
extern u32  GetTextString(u32 textId);           /* 0x0805E6E0 resolves a text ID */
extern void DrawTextCentred(u32 text, int x, int y); /* 0x080643D8 draws text */
extern void DrawText(u32 text, int x, int y); /* 0x0806435C draws styled text */
extern void DrawTextRightAligned(const char *text, int x, int y); /* 0x08064460 draws a prepared string */
extern int  Div(int numerator, int denominator); /* 0x0806B858 BIOS Div (svc 6) */

/* 0x080011EC */
void DrawMenuItems(u32 *titleText, int selectedItem, int firstItem)
{
    int digits[VALUE_DIGITS];
    char text[VALUE_TEXT_SIZE];
    int count, row, item;
    int value, pos, started, i;
    int digit;

    /* Scroll the background to keep the selected row at a fixed screen position. */
    REG_BG1VOFS = -(gMenuPositionX + (selectedItem - firstItem) * MENU_ROW_HEIGHT);

    SetFontIndex(STYLE_SELECTED);
    if (*titleText != 0)
        DrawTextCentred(GetTextString(*titleText), TITLE_X,
                     gMenuPositionX - TITLE_Y_OFFSET);

    count = gActiveMenuItemCount;
    if (count > MENU_VISIBLE_MAX)
        count = MENU_VISIBLE_MAX;

    item = firstItem;
    for (row = 0; row < count; row++) {
        if (item == selectedItem)
            SetFontIndex(STYLE_SELECTED);
        else
            SetFontIndex(STYLE_NORMAL);

        if (gActiveMenuItems[item]->value < 0) {
            /* Label only: no number for this row. */
            DrawTextCentred(GetTextString(gActiveMenuItems[item]->label), LABEL_X,
                         gMenuPositionX + row * MENU_ROW_HEIGHT);
        } else {
            DrawText(GetTextString(gActiveMenuItems[item]->label),
                         VALUE_LABEL_X,
                         gMenuPositionX + row * MENU_ROW_HEIGHT);

            /* Split the value into digits. Reread the item and value after the
             * intervening calls, as the ROM does. */
            value = gActiveMenuItems[item]->value;
            digits[6] = Div(value, 1000000);
            value -= digits[6] * 1000000;
            digits[5] = Div(value, 100000);
            value -= digits[5] * 100000;
            digits[4] = Div(value, 10000);
            value -= digits[4] * 10000;
            digits[3] = Div(value, 1000);
            value -= digits[3] * 1000;
            digits[2] = Div(value, 100);
            value -= digits[2] * 100;
            digits[1] = Div(value, 10);
            value -= digits[1] * 10;
            digits[0] = value;

            /* Clear text[1..8], preserving '$' at text[0]. Count down with i != 0:
             * i > 0 emits bgt, whereas the ROM uses bne. */
            text[0] = '$';
            for (i = 8; i != 0; i--)
                text[i] = 0;

            /* Skip leading zeroes; after started becomes 1, write every digit.
             * This three-part condition matches the ROM's cmp #1 / cmp #0 /
             * digit-test chain. Simplifying to started || digits[digit] emits
             * two branches and prevents biv elimination, retaining an extra
             * counter. The ROM only walks a pointer: cmp r2,sp + bge. */
            started = 0;
            pos = 1;
            for (digit = VALUE_DIGITS - 1; digit >= 0; digit--) {
                if (started == 1 || (started == 0 && digits[digit] != 0)) {
                    text[pos] = digits[digit] + '0';
                    pos++;
                    started = 1;
                }
            }

            DrawTextRightAligned(text, VALUE_X,
                         gMenuPositionX + row * MENU_ROW_HEIGHT);
        }
        item++;
    }
}
