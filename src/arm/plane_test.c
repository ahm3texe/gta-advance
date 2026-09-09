/* Triangle transform and visibility test — 0x0806A77C-0x0806A83F (196 bytes)
 *
 * MODE: ARM   <- the build chain switches to agbcc_arm on this marker
 *
 * Reads each of the three corners (three s16 each), transforms them against
 * the base triple, writes the transformed triples into the output buffer,
 * takes the CROSS PRODUCT of the first two corners, feeds it into a DOT
 * PRODUCT with the third corner and returns the result.
 *
 * The transform is ASYMMETRIC per component (measured from the ROM):
 *     x = input.x - base.x     (rsb)
 *     y = base.y + input.y     (add)
 *     z = base.z - input.z     (sub)
 *
 * `ldrh rN,[r0],#2` + `lsl #16` + `asr #16`: read an s16 and sign-extend it.
 * The output pointer skips 8 bytes after every triple (stmia + add #8), so
 * the target structure advances in 20-byte strides.
 *
 * The function is fully UNROLLED; there is no loop, so the source was written
 * unrolled as well.
 *
 * STATUS: PARKED — 171/196 differences, output 188 bytes (ROM 196).
 *
 * This is the project's FIRST attempt in ARM mode.  Until now no attempt was
 * technically possible at all: the build chain was bound to Thumb only.
 *
 * FLAG DISCOVERY (added permanently to agbcc_build.py):
 *   -fomit-frame-pointer                244 -> 240 bytes
 *   + -fno-schedule-insns               240 -> 216
 *   + -fno-schedule-insns2              216 -> 188  (ROM 196)
 * Without these, agbcc_arm set up a stack frame and spilled the intermediate
 * results.  Every ARM candidate will benefit from them.
 *
 * ALLOCATION CEILING — AN IMPORTANT LIMIT:
 * The ROM pushes ELEVEN registers (r3,r4,...,sl,fp,ip,lr) and never spills.
 * agbcc_arm was tested with ten live values: it ALWAYS pushes only eight
 * registers ({r4,r5,r6,r7,r8,r9,sl,lr}) and NEVER brings fp/ip into general
 * allocation.  Nine separate flags were tried (-mapcs-frame, -mno-apcs-frame,
 * -ffixed-fp, -mapcs-reentrant, -fcall-used-fp, -fcall-used-ip, -O3,
 * -fforce-mem, -fno-schedule-insns); only the last made any difference, and
 * none of them widened the register set.
 *
 * CRITICAL FINDING — THE ARM REGION IS REACHABLE FROM C:
 * The ARM code in the ROM uses barrel-shifter fusions (such as
 * `rsb r6, r3, r6, asr #16`), and it was tested on the suspicion that these
 * might be hand-written assembly.  agbcc_arm produces ALL THREE of these
 * patterns from C (measured):
 *     a - (b >> 16)   ->  sub r0, r0, r1, asr #16
 *     (b >> 16) - a   ->  rsb r0, r0, r1, asr #16
 *     a + (b >> 16)   ->  add r0, r0, r1, asr #16
 * So the 14920-byte ARM region is an ordinary matching problem.
 *
 * ELIMINATED (2): the explicit-shift form -- reading the source as u16,
 * keeping the `<< 16` in a separate local and fusing the `>> 16` into the
 * arithmetic (with both an advancing and an indexed output write).  BOTH
 * REGRESSED: 232/207 and 216/179.  Plain s16 plus indexed writes (188/171)
 * remained the best.
 *
 * ELIMINATED (1): producing an `ldm`/`stmia` block transfer through struct
 * assignment (three forms: array-indexed, advancing source, advancing source
 * and destination).  ALL THREE REGRESSED CLEARLY: 304 bytes / ~288
 * differences.  agbcc_arm expands struct assignments into MORE code, not
 * less.  The scalar form (240 bytes / 216 differences) remained the best.
 *
 * Next steps (not attempted):
 *   1. The output offsets advance in 20-byte strides; instead of
 *      dst[5]/dst[10] an array of 20-byte structs could be tried
 *   2. The cross-product term order should be matched to the ROM's mul/mla
 *      order (the ROM ends with one `mul` plus two `mla`, ours has separate
 *      multiplications)
 *   3. The corner reads are post-indexed in the ROM (`ldrh rN,[r0],#2`);
 *      using an advancing pointer in the source could produce that
 *
 * Compiler: agbcc_arm -mthumb-interwork -O2   (-fhex-asm does NOT exist on ARM)
 * Verification:  make c-match FILE=src/arm/plane_test.c
 */

#include "gba_types.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* 0x0806A77C */
s32 FUN_0806a77c(const s16 *src, s32 *dst, const s32 *base)
{
    s32 bx, by, bz;
    s32 ax, ay, az;
    s32 nx, ny, nz;
    s32 cx, cy, cz;

    bx = base[0];
    by = base[1];
    bz = base[2];

    ax = src[0] - bx;
    ay = by + src[1];
    az = bz - src[2];
    dst[0] = ax;
    dst[1] = ay;
    dst[2] = az;

    nx = src[3] - bx;
    ny = by + src[4];
    nz = bz - src[5];
    dst[5] = nx;
    dst[6] = ny;
    dst[7] = nz;

    cx = az * ny - ay * nz;
    cy = ax * nz - az * nx;
    cz = ay * nx - ax * ny;

    ax = src[6] - bx;
    ay = by + src[7];
    az = bz - src[8];
    dst[10] = ax;
    dst[11] = ay;
    dst[12] = az;

    return cx * ax + cy * ay + cz * az;
}
