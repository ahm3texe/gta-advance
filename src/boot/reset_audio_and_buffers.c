/* Stop the audio DMA, then rebuild the work buffers — 0x08063BD8-0x08063BE5
 *
 * Compiler: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Verification:  make c-match FILE=src/boot/reset_audio_and_buffers.c
 */

#include "gba_types.h"

extern void StopAudioDmaOnCartFlag(void);
extern void InitWorkBuffers(void);

/* 0x08063BD8 */
void FUN_08063bd8(void)
{
    StopAudioDmaOnCartFlag();
    InitWorkBuffers();
}
