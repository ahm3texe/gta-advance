/* Dugumu cikarip serbest listeye alma — 0x08012968-0x080129FD  [ESLESTI]
 *
 * Dugumu cift bagli listeden cikariyor, sonra serbest listenin basina
 * ekliyor. Cikarma kesmeler KAPALIYKEN yapiliyor (REG_IME 0 -> 1).
 *
 * Dort yol var: (next ve prev varsa) ortadan cikarma, (yalniz next)
 * bastan cikarma, (yalniz prev) sondan cikarma, (ikisi de yoksa) tek
 * eleman. Hepsi ortak yeniden-etkinlestirme noktasinda birlesiyor, bu
 * yuzden kontrol akisi ETIKETLERLE yazildi.
 *
 * Ofsetler: liste basi +0x804 HAVUZDAN yukleniyor, serbest liste +0x800
 * `movs r0,#128 / lsls r0,#4` ile KURULUYOR. Ikisini ayni bicimde yazmak
 * farkli kod uretir.
 *
 * Kural 35: `pop {r0}; bx r0` -> donus tipi void.
 *
 * ESLESMEYI ACAN UC OLCUM (fark 53 -> 25 -> 20 -> 0)
 * --------------------------------------------------
 * (1) `base = &gNodePool;` DALLARA YAZILMAZ, BIRLESME NOKTASINA yazilir.
 *     ROM'un dort dal sonundaki `ldr r2,[pc,#x]` komutlari DORT KAYNAK
 *     ATAMASI DEGIL; agbcc'nin TEK bir birlesme-noktasi sabit atamasini
 *     her onculun sonuna YERLESTIRMESIDIR. Dallara yazildiginda `base`
 *     allocno'su dort tanim yeri yuzunden "hicbir yerde olmuyor" (greg
 *     dokumunde "dies in N places" notu YOK) ve live_length 256'ya
 *     sisiyor; oncelik 2*6/256 = 0.047 ile en sona dusuyor, r2'yi
 *     zamaninda kapatamiyor. Birlesme noktasina alininca sira duzeliyor:
 *     base r2'yi aliyor, prev2 r3'e, node r4'e kayiyor.
 *     OLCUM: dallarda base -> live_length 256, oncelik 0.047, sira 7/7.
 * (2) DAL BASINA AYRI prev YERELI (kural 45): no_next yolunun prev'i
 *     ayri bir yerel (`prev2`). ROM ilk dalda r0, no_next dalinda r3
 *     kullaniyor -- iki AYRI allocno demek.
 * (3) Serbest liste basi icin AYRI yerel (`head`). Onceki surum `next`i
 *     yeniden kullaniyordu; bu, next'in refs'ini 5 -> 11'e, omrunu
 *     12 -> 23'e cikarip onu r1 yerine r2'ye itiyordu.
 * (4) `neither` dalinda yazim DOGRUDAN SEMBOL uzerinden
 *     (`gNodePool.activeHead = prev2;`). Bu, adres hesabini base'den
 *     ayri bir pseudo'ya aldiriyor. Tek basina bile 53 -> 25 getirdi.
 *
 * DENENIP ELENEN YOLLAR (tekrar denemeyin)
 * ----------------------------------------
 * - `base` yerelini tamamen kaldirip her yerde dogrudan sembol: 114 bayt,
 *   COK DAHA KOTU. Yerel gerekli -- ama BIRLESME NOKTASINDA.
 * - `base = &gNodePool;` dort dalda + `base->activeHead = ...`: 53 fark.
 * - Ayni sey + neither'da dogrudan sembol: 25 fark.
 * - Yapisal if/else ile yazmak: dort ayri bicim denendi (tam yapisal,
 *   dis goto ic yapisal, dis yapisal ic goto, karisik) -- HEPSI AYNI
 *   20 farki verdi. agbcc kontrol akisi bicimini normallestiriyor;
 *   blok sirasi buradan DEGISMIYOR.
 * - `neither` dalinda `slot = &gNodePool.activeHead; *slot = prev2;`:
 *   142 bayt, boyut bozuluyor.
 * - `neither` dalinda base atamasini yazimdan ONCE koymak: 54 fark.
 * - `neither` dalinda prev2 yerine next yazmak: fark degismiyor (20).
 * - Kuyrukta `slot` yerelini kaldirip `base->freeHead` yazmak: 20 fark.
 * - Kuyrukta `node->prev`/`node->next` sirasini takas: 25 fark, kotu.
 * - Kaynak yerel bildirim sirasi: pseudo numaralarini kaydiriyor ama
 *   oncelik esitligi olmadigi icin dagitimi DEGISTIRMIYOR.
 * - Ilk denemede 148 bayt: serbest liste kosulunu ters cevirmek (ROM
 *   sifir durumunu DUSEREK giriyor, `bne` ile sifir-olmayani atliyor)
 *   boyutu tutturdu. Bu bicim korunmali.
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
    Node *prev2;
    NodePool *base;
    Node *head;
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
    goto reenable;

no_prev:
    next->prev = prev;
    gNodePool.activeHead = node->next;
    goto reenable;

    /* KURAL 45: bu dalin prev'i AYRI yerel. ROM ilk dalda r0, burada r3
       kullaniyor; tek yerel yazmak ikisini tek allocno'ya baglayip
       node'u r4 yerine r3'te birakiyordu. */
no_next:
    prev2 = node->prev;
    if (prev2 == 0)
        goto neither;
    prev2->next = next;
    goto reenable;

neither:
    gNodePool.activeHead = prev2;

    /* base BURADA atanir; dallarda DEGIL. Bkz. baslik notu (1). */
reenable:
    base = &gNodePool;
    REG_IME = 1;

    /* SIRA: ROM sifir durumunu DUSEREK giriyor (`bne` ile sifir-olmayani
       atliyor). `if (head != 0)` yazmak `beq` uretip blok sirasini
       ters ceviriyordu.
       `head` AYRI yerel: `next`i yeniden kullanmak onu r2'ye itiyordu. */
    slot = &base->freeHead;
    head = *slot;
    if (head == 0) {
        *slot = node;
        node->next = head;
        node->prev = head;
        return;
    }
    node->next = head;
    node->prev = 0;
    *slot = node;
}
