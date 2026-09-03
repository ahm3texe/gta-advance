/* Menu satirlarini ve parasal degerleri cizer — 0x080011EC-0x080013AC
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/ui/draw_menu_items.c
 *
 * Ekranda en fazla sekiz menu satiri gosterilir. Her satirin metni bir metin
 * kimliginden cozulur; satirin `value` alani negatif degilse metnin sagina
 * "$" ile baslayan, bastaki sifirlari atilmis yedi haneli bir sayi yazilir.
 * Bolme icin oyunun kendi BIOS Div sarmalayicisi cagrilir, kalan ise
 * `deger - bolum * bolen` olarak elde edilir; derleyici sabit carpmalari
 * kaydirma zincirlerine ceviriyor (ROM'daki bicim budur).
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* BG1 dikey kaydirma yazmaci. Sabit cast olarak yazilir: agbcc bu adresi
 * kaydirmayla uretemez, ROM'daki gibi literal havuzdan okur. */
#define REG_BG1VOFS (*(volatile u16 *)0x04000016)

#define MENU_VISIBLE_MAX  8    /* ayni anda cizilen en fazla satir */
#define MENU_ROW_HEIGHT   16   /* satir yuksekligi, piksel */
#define STYLE_SELECTED    160  /* secili satirin metin stili */
#define STYLE_NORMAL      192  /* diger satirlarin metin stili */
#define TITLE_X           120
#define TITLE_Y_OFFSET    32   /* baslik ilk satirin 32 piksel uzerinde */
#define LABEL_X           120
#define VALUE_LABEL_X     4
#define VALUE_X           236
#define VALUE_DIGITS      7    /* 0..9999999 */
#define VALUE_TEXT_SIZE   9    /* '$' + yedi hane + sonlandirici */

typedef struct MenuItem {
    u32 unk00;
    u32 label;   /* +4: metin kimligi */
    int value;   /* +8: negatifse satirda sayi gosterilmez */
} MenuItem;

extern u8 gMenuPositionX;
extern u8 gActiveMenuItemCount;
extern MenuItem *gActiveMenuItems[20];

/* Isimleri henuz cozulmedi; data/functions.csv'deki adlar kullanildi. */
extern void FUN_0806434c(int style);            /* 0x0806434C metin stilini ayarlar */
extern u32  FUN_0805e6e0(u32 textId);           /* 0x0805E6E0 metin kimligini cozer */
extern void FUN_080643d8(u32 text, int x, int y); /* 0x080643D8 metni cizer */
extern void FUN_0806435c(u32 text, int x, int y); /* 0x0806435C metni stiliyle cizer */
extern void FUN_08064460(const char *text, int x, int y); /* 0x08064460 hazir dizi cizer */
extern int  FUN_0806b858(int numerator, int denominator); /* 0x0806B858 BIOS Div (svc 6) */

/* 0x080011EC */
void DrawMenuItems(u32 *titleText, int selectedItem, int firstItem)
{
    int digits[VALUE_DIGITS];
    char text[VALUE_TEXT_SIZE];
    int count, row, item;
    int value, pos, started, i;
    int digit;

    /* Secili satir ekranda hep ayni yerde kalsin diye arka plan kaydirilir. */
    REG_BG1VOFS = -(gMenuPositionX + (selectedItem - firstItem) * MENU_ROW_HEIGHT);

    FUN_0806434c(STYLE_SELECTED);
    if (*titleText != 0)
        FUN_080643d8(FUN_0805e6e0(*titleText), TITLE_X,
                     gMenuPositionX - TITLE_Y_OFFSET);

    count = gActiveMenuItemCount;
    if (count > MENU_VISIBLE_MAX)
        count = MENU_VISIBLE_MAX;

    item = firstItem;
    for (row = 0; row < count; row++) {
        if (item == selectedItem)
            FUN_0806434c(STYLE_SELECTED);
        else
            FUN_0806434c(STYLE_NORMAL);

        if (gActiveMenuItems[item]->value < 0) {
            /* Sadece etiket: satirda gosterilecek sayi yok. */
            FUN_080643d8(FUN_0805e6e0(gActiveMenuItems[item]->label), LABEL_X,
                         gMenuPositionX + row * MENU_ROW_HEIGHT);
        } else {
            FUN_0806435c(FUN_0805e6e0(gActiveMenuItems[item]->label),
                         VALUE_LABEL_X,
                         gMenuPositionX + row * MENU_ROW_HEIGHT);

            /* Deger hanelerine ayrilir; ustteki cagrilar araya girdigi
             * icin ogeyi ve degerini yeniden okuyoruz (ROM da oyle yapiyor). */
            value = gActiveMenuItems[item]->value;
            digits[6] = FUN_0806b858(value, 1000000);
            value -= digits[6] * 1000000;
            digits[5] = FUN_0806b858(value, 100000);
            value -= digits[5] * 100000;
            digits[4] = FUN_0806b858(value, 10000);
            value -= digits[4] * 10000;
            digits[3] = FUN_0806b858(value, 1000);
            value -= digits[3] * 1000;
            digits[2] = FUN_0806b858(value, 100);
            value -= digits[2] * 100;
            digits[1] = FUN_0806b858(value, 10);
            value -= digits[1] * 10;
            digits[0] = value;

            /* text[1..8] temizlenir, text[0] '$' olarak kalir. Geriye sayan
             * bicim sart: `i > 0` yazilirsa agbcc `bgt` uretiyor, ROM `bne`
             * kullaniyor — bu yuzden kosul `i != 0`. */
            text[0] = '$';
            for (i = 8; i != 0; i--)
                text[i] = 0;

            /* Bastaki sifirlar atlanarak haneler yazilir. `started` bir kez
             * 1 olduktan sonra kalan tum haneler yazilir.
             *
             * Kosulun bu uc parcali bicimi ROM'un urettigi dallanma
             * zinciriyle (cmp #1 / cmp #0 / hane testi) birebir ayni.
             * Sadelestirilmis `started || digits[digit]` bicimi iki dal
             * uretiyor ve ustelik biv eleme calismadigi icin dongude
             * fazladan bir sayac birakiyordu (ROM sadece isaretciyi
             * yuruyor: `cmp r2, sp` + `bge`). */
            started = 0;
            pos = 1;
            for (digit = VALUE_DIGITS - 1; digit >= 0; digit--) {
                if (started == 1 || (started == 0 && digits[digit] != 0)) {
                    text[pos] = digits[digit] + '0';
                    pos++;
                    started = 1;
                }
            }

            FUN_08064460(text, VALUE_X,
                         gMenuPositionX + row * MENU_ROW_HEIGHT);
        }
        item++;
    }
}
