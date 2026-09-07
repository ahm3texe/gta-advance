/* Secili dugumu iki oyuncu icin dogurma — 0x08054E40-0x08054F1B
 *
 * Oyuncu 1 (0x020272C8) ve gGameState[12] kuruluysa oyuncu 2 (0x02026F34)
 * icin ayni is: secim etkinse ReleaseSelectedNode, kimlik 0x3FF degilse
 * FindOrRecycleNode + FUN_08054870 ile konum kurulur, degilse secili
 * nesnenin 12 baytlik konumu (bayrak 0x30'a gore +0x20+4 ya da +0x18)
 * kopyalanir; sonra SpawnNodeObject.
 *
 * IKI OLCUM: FindOrRecycleNode sonucu AYRI YERELE alinmali (arguman icine
 * yazilinca agbcc global okumasini cagridan ONCE yapip r4'te sakliyor);
 * 12 baytlik struct kopyasinin HEDEF adresi (`dst = &pos`) bayrak
 * testinden once yerele alinmali, ROM `mov r1,sp`yi testten once uretiyor.
 * Global iki ayri u32 sembol olarak (node_search.c ile ayni tip) alinip
 * Sel* olarak cast ediliyor.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/spawn_selected_for_players.c
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
