/* Sprite havuzunu baslatma — 0x08012B20-0x08012B9B.
 * 128 dugumu cift bagli serbest listeye dizer, aktif listeyi bosaltir.
 * prevCount=128 ilk FlushSpriteList'te kullanilmayan tum OAM yuvalarinin
 * gizlenmesini saglar. Ardindan DMA3, OAM'in 1024 baytini sifirlar;
 * DMA boyunca onceki REG_IME degeri saklanir ve geri yuklenir.
 * `i++, node++` sirasiyla for artirimi, ROM'un sayac azaltimini isaretci
 * kopyasindan once yayar; node++ govdedeyken 4 bayt siralama farki vardi.
 * Dogrulama: make c-match FILE=src/core/init_sprite_pool.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

/* 0x08012B20 */
void InitSpritePool(void)
{
    Node *node;
    s32 i;
    u16 ime;
    volatile u32 zero;

    gNodePool.freeHead = gNodePool.nodes;
    gNodePool.activeHead = 0;
    gNodePool.prevCount = NODE_COUNT;

    node = gNodePool.nodes;
    node->next = node + 1;
    node->prev = 0;
    node++;
    for (i = 1; i < NODE_COUNT - 1; i++, node++) {
        node->prev = node - 1;
        node->next = node + 1;
    }
    node->prev = node - 1;
    node->next = 0;

    ime = REG_IME;
    REG_IME = 0;
    zero = 0;
    REG_DMA3.src = (const void *)&zero;
    REG_DMA3.dst = (void *)OAM_BASE;
    REG_DMA3.control = 0x85000100;
    (void)REG_DMA3.control;
    REG_IME = ime;
}
