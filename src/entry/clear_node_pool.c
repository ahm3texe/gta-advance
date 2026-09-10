/* Clear the node pool, then rebuild it — 0x08013434-0x0801344F
 *
 * 904 is `movs r2,#226 / lsls r2,#2`, and it is the size data/ram_map.csv
 * already records for the pool: 32 28-byte nodes and two points.
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/entry/clear_node_pool.c
 */

#include "gba_types.h"
#include "phase1_types.h"

#define POOL_SIZE  (226 << 2)   /* 904 */

extern Phase1NodePool gRam02022AC0;

extern void *Memset(void *dest, int value, u32 count);
extern void  FUN_08013450(void);

/* 0x08013434 */
void FUN_08013434(void)
{
    Memset(&gRam02022AC0, 0, POOL_SIZE);
    FUN_08013450();
}
