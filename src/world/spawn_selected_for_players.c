/* Spawning the selected node for both players — 0x08054E40-0x08054F1B
 *
 * The same work for player 1 (0x020272C8) and, when gGameState[12] is set, for
 * player 2 (0x02026F34): if the selection is active, ReleaseSelectedNode; if
 * the id is not 0x3FF the position is set up with FindOrRecycleNode +
 * FUN_08054870, otherwise the selected object's 12-byte position (from +0x20+4
 * or +0x18 according to flag 0x30) is copied; then SpawnNodeObject.
 *
 * TWO MEASUREMENTS: the FindOrRecycleNode result must be taken into a SEPARATE
 * LOCAL (written inside the argument, agbcc does the global read BEFORE the
 * call and keeps it in r4); the DESTINATION address of the 12-byte struct copy
 * (`dst = &pos`) must be taken into a local before the flag test, since the
 * ROM emits `mov r1,sp` before the test.  The global is taken as two separate
 * u32 symbols (the same type as in node_search.c) and cast to Sel*.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/world/spawn_selected_for_players.c
 */

#include "gba_types.h"
#define NO_ID   0x3FF
#define POS_FLAG 0x30
typedef struct Pos3 { u32 a; u32 b; u32 c; } Pos3;
typedef struct Obj { u8 pad00[8]; u8 posFlags; u8 pad09[15]; u8 *pos; u8 pad1c[4]; u8 *posAlt; } Obj;
typedef struct Sel { u8 pad00[0x28]; Obj *obj; u8 pad2c[4]; u32 active; } Sel;
typedef struct Node Node;
extern u32 gRam020272C8;
extern u32 gRam02026F34;
extern u8  gGameState[];
extern void  ReleaseSelectedNode(u32 player);
extern Node *FindOrRecycleNode(u32 id);
extern void  FUN_08054870(Sel *sel, Node *node, Pos3 *pos);
extern void  SpawnNodeObject(Sel *sel, Pos3 *pos);
void SpawnSelectedForPlayers(u16 id)
{
    Pos3 pos; Sel *sel; Obj *obj; u8 *src; Node *node; Pos3 *dst;
    if (((Sel *)gRam020272C8)->active != 0)
        ReleaseSelectedNode(1);
    if (id != NO_ID) {
        node = FindOrRecycleNode(id);
        FUN_08054870((Sel *)gRam020272C8, node, &pos);
    } else {
        sel = (Sel *)gRam020272C8;
        obj = sel->obj;
        if (obj == 0)
            return;
        dst = &pos;
        if (POS_FLAG & obj->posFlags)
            src = obj->posAlt + 4;
        else
            src = obj->pos;
        *dst = *(Pos3 *)src;
    }
    SpawnNodeObject((Sel *)gRam020272C8, &pos);
    if (gGameState[12] == 0)
        return;
    if (((Sel *)gRam02026F34)->active != 0)
        ReleaseSelectedNode(2);
    if (id != NO_ID) {
        node = FindOrRecycleNode(id);
        FUN_08054870((Sel *)gRam02026F34, node, &pos);
    } else {
        sel = (Sel *)gRam02026F34;
        obj = sel->obj;
        if (obj == 0)
            return;
        dst = &pos;
        if (POS_FLAG & obj->posFlags)
            src = obj->posAlt + 4;
        else
            src = obj->pos;
        *dst = *(Pos3 *)src;
    }
    SpawnNodeObject((Sel *)gRam02026F34, &pos);
}
