/* Oturum dugumu sifirlama — 0x0803C798-0x0803C7BF
 *
 * HENUZ ESLESMIYOR: 34 byte'lik ROM fonksiyonuna karsi 36 byte, 13 bayt
 * farkli. On alti komutun on besi birebir tutuyor; tek fark maskenin nasil
 * kuruldugu. ROM `movs r0,#5 / negs r0,r0` ile 32 bitlik -5 uretip u8 alanla
 * AND'liyor; bizimki maskeyi bayta daraltip `movs r0,#251` yaziyor.
 * Denenenler: ~4, -5, 0xFFFFFFFB, u32 yerel uzerinden, s32 yerel maske.
 * Hicbiri 32 bitlik bicimi vermedi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/misc/session_node.c
 */

typedef unsigned char u8;
typedef unsigned int  u32;

typedef struct {
    u8  unk00[40];
    u32 unk28;          /* 0x28 */
    u32 unk2C;          /* 0x2C */
} Node;

typedef struct {
    u8    unk00[28];
    Node *node;         /* 0x1C */
} Context;

typedef struct {
    Context *context;   /* 0x00 */
    u8       unk04[4];
    u8       flags;     /* 0x08 */
} Session;

#define SESSION_FLAG_BIT  4
#define NODE_RESET_VALUE  24

extern Session *gSessionPtr;

/* 0x0803C798 */
void ResetSessionNode(void)
{
    Node *node;

    node = gSessionPtr->context->node;
    if (node != 0) {
        node->unk28 = NODE_RESET_VALUE;
        node->unk2C = 0;
    }

    gSessionPtr->flags &= ~SESSION_FLAG_BIT;
}
