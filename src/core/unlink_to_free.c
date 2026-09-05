/* Dugumu cikarip serbest listeye alma — 0x08012968-0x080129FD
 *
 * Dugumu cift bagli listeden cikariyor, sonra serbest listenin basina
 * ekliyor. Cikarma kesmeler KAPALIYKEN yapiliyor (REG_IME 0 -> 1).
 *
 * Dort yol var: (next ve prev varsa) ortadan cikarma, (yalniz next)
 * bastan cikarma, (yalniz prev) sondan cikarma, (ikisi de yoksa) tek
 * eleman. Hepsi ortak yeniden-etkinlestirme noktasinda birlesiyor, bu
 * yuzden kontrol akisi ETIKETLERLE yazildi -- yapisal if/else agbcc'ye
 * farkli blok sirasi urettiriyor (bkz. src/core/insert_sorted.c).
 *
 * Ofsetler: liste basi +0x804 HAVUZDAN yukleniyor, serbest liste +0x800
 * `movs r0,#128 / lsls r0,#4` ile KURULUYOR. Ikisini ayni bicimde yazmak
 * farkli kod uretir.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * HENUZ ESLESMIYOR: 53/150. BOYUT DOGRU (150/150).
 *
 * Ilk denemede 148 baytti; serbest liste kosulunu ters cevirmek (ROM
 * sifir durumunu DUSEREK giriyor, `bne` ile sifir-olmayani atliyor)
 * boyutu tutturdu.
 *
 * Ortak sprite_pool.h gorunumune gecis byte farkini degistirmedi.
 * no_next yolunun prev yerelini ayirmak yine 53 fark verdi; serbest
 * liste icin ayri next yereli 144 bayt / 117 farka geriledi.
 *
 * Kalan engel TEK bir kaydirma: ROM `node`u r4'te tutuyor, bizimki r3'te
 * (+0x02: `adds r4,r0,#0` vs `adds r3,r0,#0`). Bu kayma fonksiyonun geri
 * kalanina yayiliyor.
 *
 * Denendi: `base` yerelini kaldirip dogrudan sembol kullanmak -- 114 bayt,
 * COK DAHA KOTU. O yerel gerekli.
 *
 * Bu, oturumda olculen "bir fazla canli deger" sinifinin ayni ornegi
 * (bkz. docs/COMPILER.md register dagitimi bolumu). tools/dump_alloc.py
 * ile pseudo tablosu okunabilir.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/unlink_to_free.c
 */

#include "gba_io.h"
#include "sprite_pool.h"

/* 0x08012968 */
void UnlinkToFree(Node *node)
{
    Node *next;
    Node *prev;
    NodePool *base;
    Node **slot;

    if (node == 0)
        return;

    REG_IME = 0;

    next = node->next;
    if (next == 0)
        goto no_next;

    prev = node->prev;
    if (prev == 0)
        goto no_prev;

    next->prev = prev;
    node->prev->next = node->next;
    base = &gNodePool;
    goto reenable;

no_prev:
    next->prev = prev;
    base = &gNodePool;
    base->activeHead = node->next;
    goto reenable;

no_next:
    prev = node->prev;
    if (prev == 0)
        goto neither;
    prev->next = next;
    base = &gNodePool;
    goto reenable;

neither:
    base = &gNodePool;
    base->activeHead = prev;

reenable:
    REG_IME = 1;

    /* SIRA: ROM sifir durumunu DUSEREK giriyor (`bne` ile sifir-olmayani
       atliyor). `if (next != 0)` yazmak `beq` uretip blok sirasini
       ters ceviriyordu. */
    slot = &base->freeHead;
    next = *slot;
    if (next == 0) {
        *slot = node;
        node->next = next;
        node->prev = next;
        return;
    }
    node->next = next;
    node->prev = 0;
    *slot = node;
}
