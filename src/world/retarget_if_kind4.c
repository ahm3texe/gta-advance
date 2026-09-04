/* Tur 4 ise isleyiciyi degistirme — 0x0803F6D8-0x0803F6F7
 *
 * Ilk alani CAGRIDAN ONCE okuyup saklıyor, FUN_0803CDA8'i cagiriyor, sonra
 * saklanan nesnenin +8 baytini sinayip 4 ise +0x08'deki isleyiciyi
 * degistiriyor. Nesne isaretcisinin cagri boyunca YASAMASI gerektigi icin
 * ayri bir yerelde tutuluyor (ROM: r4).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/retarget_if_kind4.c
 */

#include "gba_types.h"


#define KIND_DONE 4

typedef struct Obj {
    u8 pad00[8];
    u8 kind;                    /* +0x08 */
} Obj;

typedef struct Holder {
    Obj  *first;                /* +0x00 */
    u8    pad04[4];
    void *handler;              /* +0x08 */
} Holder;

extern void FUN_0803cda8(Holder *holder);
/* Saklanan fonksiyon isaretcisinde THUMB BITI (bit 0) kurulu olmali.
   `__thumb` sonekli sembol, adresi | 1 olarak cozumlenir
   (tools/agbcc_build.py). `bl` hedefinde bit eklemek dal ofsetini
   bozacagi icin ayri sembol kullaniliyor. */
extern u8 FUN_0803ef18__thumb[];

/* 0x0803F6D8 */
void RetargetIfKind4(Holder *holder)
{
    Obj *obj;

    obj = holder->first;
    FUN_0803cda8(holder);
    if (obj->kind == KIND_DONE)
        holder->handler = FUN_0803ef18__thumb;
}
