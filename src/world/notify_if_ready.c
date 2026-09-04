/* Hazirsa bildir — 0x08030884-0x0803089F
 *
 * Nesne bos degilse ve +0x18 alani sifirdan farkliysa FUN_0802B2B4'u
 * (nesne, 0, 1) ile cagiriyor.
 *
 * Iki kural birlikte:
 *   - kural 34: ic ice `if` yerine ERKEN DONUS zinciri ROM'un blok
 *     sirasini uretiyor
 *   - kural 35: `pop {r0}; bx r0` -> donus tipi void
 *
 * ROM ayrica argumani `adds r1, r0, #0` ile ayri bir register'a kopyaliyor
 * (kural 37); bu, kaynakta ayri bir yerel gerektirmiyor cunku parametre
 * zaten hem sorgu hem cagri argumani olarak iki rolde kullaniliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/notify_if_ready.c
 */

#include "gba_types.h"

typedef struct Node {
    u8  pad00[0x18];
    u32 ready;                  /* +0x18 */
} Node;

extern void FUN_0802b2b4(Node *node, u32 arg1, u32 arg2);

/* 0x08030884 */
void NotifyIfReady(Node *node)
{
    if (node == 0)
        return;
    if (node->ready == 0)
        return;
    FUN_0802b2b4(node, 0, 1);
}
