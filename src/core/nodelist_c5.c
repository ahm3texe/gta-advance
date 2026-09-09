/* Acquire the node for an id and bind the area record — 0x08053650, 108 bytes.
 *
 * MATCH: 108/108 bytes (byte-matching).
 *
 * Reading the ROM:
 *   1) A node is obtained from the ordered list with
 *      FUN_080543D0(&gRam02035780, id) (there is NO NULL check).
 *   2) If bit 8 is set in the +0x1A flag byte of the id-th record of the
 *      28-byte record array in the ROM bank (gAreaBank +0x20), 1 is written to
 *      gRam020004A0.
 *   3) If the upper half of the node's +0x0B byte is 0x10, the record pointer
 *      is set up, four fields are cleared and two bits are cleared from +0x0B.
 *
 * THE TAIL BODY IS BYTE-FOR-BYTE THE SAME AS src/core/nodelist_a2.c
 * (FindOrInitAreaNode):
 *   record = id*28 + bank->records / mark=0 / init=0x3FF / a=b=c=0 /
 *   kind &= ~1 ... kind &= ~2.  The three mechanisms measured in that file were
 *   applied here directly:
 *     - so that the negative constant does not fall to CSE, `m = ~1; m &= k;`
 *       and `n = 3; n = -n; m &= n;` (the negation must be a SEPARATE
 *       statement),
 *     - the pointer arithmetic is written as an INTEGER,
 *       `(void *)(id * 28 + (s32)bank->records)`; otherwise gcc canonically
 *       moves the pointer to the front and produces the inverse of
 *       `adds r0,r0,r1`,
 *     - the symbol's address is held in a separate temporary
 *       (`bank = &gAreaBank;`) so that the load is emitted BEFORE the
 *       multiplication.
 *
 * The `id*28` product is built once in the ROM (`lsls #3 / subs / lsls #2`) and
 * lives in r2 across two uses; `bank->records`, by contrast, is read TWICE
 * (`ldr rX,[r5,#32]`), because the `gRam020004A0 = 1` store in between
 * invalidates the memory CSE. Leaving the second read as an EXPRESSION in the
 * source is enough: because the store kills the memory expression, the compiler
 * does not reuse the first read's register and emits a fresh `ldr`.
 *
 * THE ONE MEASURED DIFFERENCE — THE POSITION OF THE LOAD RELATIVE TO THE
 * MULTIPLICATION (8 bytes -> 0):
 *   The first draft built the first record's address in a single expression:
 *       rec = (AreaRecord *)(id * RECORD_SZ + (s32)bank->records);
 *   That produces 51 of 52 instructions correctly, but pushes the
 *   `ldr r1,[r5,#32]` instruction AFTER the multiplication; the ROM emits it
 *   BEFORE. (The same class was measured as mechanism 3 in nodelist_a2.c.)
 *   Taking the member read into a SEPARATE statement settles the order onto the
 *   ROM's:
 *       recs = (s32)bank->records;
 *       rec  = (AreaRecord *)(id * RECORD_SZ + recs);
 *   So "hold the symbol address in a temporary" (`bank`) IS NOT ENOUGH ON ITS
 *   OWN; the member read must have its own statement too. `recs` is ONLY for
 *   the first use -- writing `recs` at the second use as well would delete the
 *   ROM's second `ldr`.
 *
 * Rule 35: `pop {r4,r5}; pop {r0}; bx r0` -> a void return type.
 * Rule 1: gAreaBank / gRam020004A0 / gRam02035780 are extern symbols.
 *
 * AN ELIMINATED PATH (do not retry):
 *   - Building the record address in a single expression (above): the size
 *     MATCHES at 108/108 but `ldr r1,[r5,#32]` is in the wrong place, 8 bytes
 *     of difference.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_c5.c
 */

#include "gba_types.h"
#include "node_list.h"

#define RECORD_SZ   28          /* area record size */
#define INIT_FIELD  0x3FF       /* initial value written to +0x14 */
#define KIND_MASK   0xF0        /* high nibble of +0x0B */
#define KIND_READY  0x10        /* the level at which the tail runs */
#define REC_FLAG    8           /* the gate bit in the record's +0x1A flags */

/* The same layout as the Node in src/core/nodelist_a2.c. */
typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    slot;                 /* 0x0A */
    u8    kind;                 /* 0x0B */
    u8    pad0C[8];
    u16   init;                 /* 0x14 */
    u8    pad16;
    u8    mark;                 /* 0x17 */
    void *record;               /* 0x18 */
    s32   a;                    /* 0x1C */
    s32   b;                    /* 0x20 */
    s32   c;                    /* 0x24 */
} Node;

/* A 28-byte area record; only the flag byte is known. */
typedef struct AreaRecord {
    u8 pad00[26];
    u8 flags;                   /* 0x1A */
    u8 pad1B;
} AreaRecord;

/* The bank's view in this translation unit; +0x20 is the record array. */
typedef struct AreaBank {
    u8  pad00[0x20];
    u8 *records;                /* 0x20 */
} AreaBank;

/* gRam02035780 is declared `NodeC4 *` in node_list.h; giving the same symbol a
 * second extern type trips check_consistency's ram-extern check, so it is cast
 * through the address (the same solution as in nodelist_a2.c). */
#define NODE_LIST ((Node **)&gRam02035780)

extern AreaBank gAreaBank;
extern u32      gRam020004A0;

extern Node *FindOrClaimNode(Node **list, s32 id);

/* 0x08053650 */
void PrepareAreaNode(s32 id)
{
    AreaBank   *bank;
    AreaRecord *rec;
    Node       *node;
    s32         f;
    s32         k;
    s32         recs;
    s32         m;
    s32         n;

    node = FindOrClaimNode(NODE_LIST, id);

    bank = &gAreaBank;
    recs = (s32)bank->records;
    rec = (AreaRecord *)(id * RECORD_SZ + recs);
    f = REC_FLAG;
    f &= rec->flags;
    if (f != 0)
        gRam020004A0 = 1;

    k = node->kind;
    if ((k & KIND_MASK) == KIND_READY) {
        node->record = (void *)(id * RECORD_SZ + (s32)bank->records);
        m = ~1;
        m &= k;
        node->mark = 0;
        node->init = INIT_FIELD;
        node->a = 0;
        node->b = 0;
        node->c = 0;
        n = 3;
        n = -n;
        m &= n;
        node->kind = m;
    }
}
