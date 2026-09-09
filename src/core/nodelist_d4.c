/* Clear the node pools and thread them onto the free lists
 * 0x08054450-0x0805456F  (288 bytes)
 *
 * The function has two stages:
 *
 *   1) Five separate DMA3 fills. Four are 32-bit zero fills; the last is a
 *      16-bit 0x7FEF (empty id) fill. Every transfer is done with REG_IME saved
 *      and cleared, then restored at the end -- the same pattern as
 *      src/core/init_sprite_pool.c and src/world/slot_table.c. The source is a
 *      fixed cell on the stack (the "source fixed" bit is set in the DMA
 *      control word).
 *
 *   2) Three list headers are emptied with List2Init, after which ALL entries
 *      of the three pools get id 0x7FEF and are pushed onto the head of their
 *      own list with List2PushFront.
 *
 * The pool sizes fell out of the DMA lengths and the loop strides, and the two
 * confirm each other:
 *
 *      0x02032B60  128 x 52 bytes = 6656 = 0x680 words  -> list 0x02035A80
 *      0x02034560   64 x 72 bytes = 4608 = 0x480 words  -> gNodeListHead
 *      0x020357F0   16 x 40 bytes =  640 = 0x0A0 words  -> gRam02035780
 *      0x02030D50  gSlotArray, 128 x 60 = 7680 = 0x780 words
 *      0x02030C10  gSlotIds,   160 x u16, filled with 0x7FEF
 *
 * The three pool bases and the 0x02035A80 list header are NOT in
 * data/ram_map.csv. Because this file does not write under data/, they stand as
 * #define constant casts and the names are TEMPORARY. The symbols need to be
 * added (see new_symbols in the task output).
 *
 * FOUR MEASURED DETAILS (each changed the match on its own):
 *
 * 1) `zero` and `fill` are volatile: the ROM RE-WRITES the same value to the
 *    stack on every transfer (`str r5,[sp,#0]` four times). Without volatile,
 *    agbcc considers the second and later stores redundant and deletes them.
 *
 * 2) The first loop was written with an ASCENDING SIGNED index (`i <= 127`) and
 *    ARRAY INDEXING (`entryA[i]`). That makes the pointer a derived induction
 *    variable (giv) of `i`; agbcc eliminates `i` entirely, builds the final
 *    value as `base + 127*52` (0x19CC) and emits a SIGNED `ble` -- a
 *    continuation of rule 42. A hand-written pointer comparison (`p <= end`)
 *    gave an UNSIGNED `bls`.
 *
 * 3) In the same loop a SEPARATE LOCAL for the base is required (rule 37/22).
 *    Written directly as `POOL_A[i]`, agbcc cannot eliminate `i` and keeps the
 *    counter (`adds r4,#1 / cmp r4,#127`); with the intermediate local
 *    `entryA = POOL_A;` and `entryA[i]`, the ROM's `cmp r6,r4 / ble` form comes
 *    out.
 *
 * 4) The second and third loops use a DESCENDING counter and a WALKING POINTER:
 *    there the pointer is a separate basic induction variable, so the counter
 *    cannot be eliminated and the ROM's `subs`/`cmp`/`bge` comes out. The
 *    increment order is pointer first, counter second, as in the ROM (rule 43).
 *    The empty id is given to these two loops in a SEPARATE STATEMENT
 *    (`id = ID_NONE;`): a bare constant in the loop body was moved as a loop
 *    invariant to the END of the preheader (base, counter, constant), whereas
 *    the ROM wants the constant FIRST (constant, base, counter). Writing it as
 *    a source statement fixes the order. The first loop does not need this: its
 *    constant becomes a common subexpression with the `fill = ID_NONE` of the
 *    16-bit DMA fill and stays in r5.
 *
 * Rule 35: `pop {r0}; bx r0` -> a void return type.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/core/nodelist_d4.c
 */

#include "gba_io.h"
#include "node_list.h"

#define ID_NONE      0x7FEF

/* DMA3 control bits: enable + source fixed (+ 32-bit). */
#define DMA_FILL_32  0x85000000
#define DMA_FILL_16  0x81000000

/* gSlotIds: 160 u16 ids (the same view as src/world/slot_table.c). */
#define SLOT_COUNT       160
/* gSlotArray: 128 slots of 60 bytes (the same as src/world/slot_scan.c). */
#define SLOT_ARRAY_LEN   128
#define SLOT_ARRAY_WORDS (SLOT_ARRAY_LEN * 15)

#define POOL_A_COUNT 128
#define POOL_A_WORDS (POOL_A_COUNT * 13)   /* 52 bytes / entry */
#define POOL_B_COUNT 64
#define POOL_B_WORDS (POOL_B_COUNT * 18)   /* 72 bytes / entry */
#define POOL_C_COUNT 16
#define POOL_C_WORDS (POOL_C_COUNT * 10)   /* 40 bytes / entry */

/* The doubly linked list header and node: the same as src/core/list_ops2.c. */
typedef struct Node2 {
    struct Node2 *next;         /* +0x00 */
    struct Node2 *prev;         /* +0x04 */
} Node2;

typedef struct List2 {
    Node2 *head;                /* +0x00 */
    Node2 *tail;                /* +0x04 */
    int    count;               /* +0x08 */
} List2;

/* All three pools carry the same header; only their strides differ. */
typedef struct EntryA {
    struct EntryA *next;        /* +0x00 */
    struct EntryA *prev;        /* +0x04 */
    u16 id;                     /* +0x08 */
    u8  pad0A[42];              /* stride 52 */
} EntryA;

typedef struct EntryB {
    struct EntryB *next;        /* +0x00 */
    struct EntryB *prev;        /* +0x04 */
    u16 id;                     /* +0x08 */
    u8  pad0A[62];              /* stride 72 */
} EntryB;

typedef struct EntryC {
    struct EntryC *next;        /* +0x00 */
    struct EntryC *prev;        /* +0x04 */
    u16 id;                     /* +0x08 */
    u8  pad0A[30];              /* stride 40 */
} EntryC;

typedef struct Slot {
    u8  pad00[0x28];
    u32 mark;                   /* +0x28: empty when 0 */
    u8  pad2C[0x10];            /* stride 60 */
} Slot;

/* Those recorded in ram_map.csv are extern; the rest are constant casts. */
extern u16  gSlotIds[SLOT_COUNT];       /* 0x02030C10 */
extern Slot gSlotArray[SLOT_ARRAY_LEN]; /* 0x02030D50 */
extern NodeC4 *gNodeListHead;           /* 0x02035A70 */

#define POOL_A ((EntryA *)0x02032B60)
#define POOL_B ((EntryB *)0x02034560)
#define POOL_C ((EntryC *)0x020357F0)
#define LIST_A ((List2 *)0x02035A80)

extern void List2Init(List2 *list);
extern void List2PushFront(List2 *list, Node2 *node);

/* 0x08054450 */
void BuildNodeFreeLists(void)
{
    EntryA *entryA;
    EntryB *entryB;
    EntryC *entryC;
    s32 i;
    s32 j;
    s32 k;
    u16 ime;
    u16 id;
    volatile u32 zero;
    volatile u16 fill;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = POOL_A;
    REG_DMA3.control = DMA_FILL_32 | POOL_A_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = POOL_B;
    REG_DMA3.control = DMA_FILL_32 | POOL_B_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = POOL_C;
    REG_DMA3.control = DMA_FILL_32 | POOL_C_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = gSlotArray;
    REG_DMA3.control = DMA_FILL_32 | SLOT_ARRAY_WORDS;
    REG_DMA3.control;
    REG_IME = ime;

    ime = REG_IME;
    REG_IME = 0;
    fill = ID_NONE;
    REG_DMA3.src = (const void *)&fill;
    REG_DMA3.dst = gSlotIds;
    REG_DMA3.control = DMA_FILL_16 | SLOT_COUNT;
    REG_DMA3.control;
    REG_IME = ime;

    List2Init(LIST_A);
    List2Init((List2 *)&gNodeListHead);
    List2Init((List2 *)&gRam02035780);

    entryA = POOL_A;
    for (i = 0; i <= POOL_A_COUNT - 1; i++) {
        entryA[i].id = ID_NONE;
        List2PushFront(LIST_A, (Node2 *)&entryA[i]);
    }

    id = ID_NONE;
    entryB = POOL_B;
    for (j = POOL_B_COUNT - 1; j >= 0; entryB++, j--) {
        entryB->id = id;
        List2PushFront((List2 *)&gNodeListHead, (Node2 *)entryB);
    }

    id = ID_NONE;
    entryC = POOL_C;
    for (k = POOL_C_COUNT - 1; k >= 0; entryC++, k--) {
        entryC->id = id;
        List2PushFront((List2 *)&gRam02035780, (Node2 *)entryC);
    }
}
