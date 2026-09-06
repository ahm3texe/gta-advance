/* Calisma zamaninda kullanilan bagimsiz global durum alanlarini sifirlar. */

#include "gba_types.h"

extern u32 gRam02035AE0;
extern u32 gRam02035B10;
extern u32 gRam02035B18;
extern u32 gRam02023704;
extern u32 gRam02011018;
extern u32 gRam02030328;
extern u32 gRam02035AC4;
extern u32 gRam02010F48;
extern u32 gRam02010F4C;
extern u32 gRam0202F4CC;
extern u32 gRam020110B0;
extern u32 gRam02035A98;
extern u8 gRam02027EE0;
extern u16 gRam02035AA0;
extern u16 gRam02035AA4;
extern u16 gRam02035A90;
extern u32 gRam02035AEC;
extern u32 gRam0201627C;
extern u8 gRam02035B1C;
extern u8 gRam02030370;
extern u8 gRam02035A9C;
extern u8 gRam02035B24;
extern u8 gRam02010FF0;
extern u32 gRam020302E4;

/* 0x0805643C */
void ResetRuntimeGlobals(void)
{
    u16 *stateA;
    u16 *stateB;

    gRam02035AE0 = 0;
    gRam02035B10 = 0;
    gRam02035B18 = 0;
    gRam02023704 = 0;
    gRam02011018 = 0;
    gRam02030328 = 0;
    gRam02035AC4 = 0;
    gRam02010F48 = 0;
    gRam02010F4C = 0;
    gRam0202F4CC = 0;
    gRam020110B0 = 0;
    gRam02035A98 = 0;
    gRam02027EE0 = 0;
    stateA = &gRam02035AA0;
    stateB = &gRam02035AA4;
    gRam02035A90 = 0;
    *stateB = 0;
    *stateA = 0;
    gRam02035AEC = 0;
    gRam0201627C = 0;
    gRam02035B1C = 0;
    gRam02030370 = 0;
    gRam02035A9C = 0;
    gRam02035B24 = 0;
    gRam02010FF0 = 0;
    gRam020302E4 = 0;
}
