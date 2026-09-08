/* Oyun nesne havuzlarini sirayla baslat — 0x0805131C.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x0805131C.json. */
#include "gba_types.h"

extern void InitSpritePool(void);
extern void NoOp080130F0(void);
extern void FUN_080138f4(void);
extern void FUN_080134a4(void);
extern void FUN_08014fb8(void);
extern void FUN_08037824(void);
extern void NoOp080427BC(void);
extern void ResetAnchorPartial(void);
extern void NoOp08051048(void);
extern void FUN_08041ed4(void);
extern void NoOp080519B8(void);
extern void FUN_08011d14(void);
extern void NoOp08035844(void);
extern void NoOp08030848(void);
void FUN_0805131c(void)
{
    InitSpritePool();
    NoOp080130F0();
    FUN_080138f4();
    FUN_080134a4();
    FUN_08014fb8();
    FUN_08037824();
    NoOp080427BC();
    ResetAnchorPartial();
    NoOp08051048();
    FUN_08041ed4();
    NoOp080519B8();
    FUN_08011d14();
    NoOp08035844();
    NoOp08030848();
}
