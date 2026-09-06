/* Kimligi listede bulup sayacini artirma, yoksa yedek dugumu kurma.
 * 0x080543D0, 126 bayt.
 *
 * Liste basligi +0x00 bas, +0x04 yedek dugum. Liste kimlige gore SIRALI:
 * arama, gecerli kimlik arananI GECINCE duruyor.
 *
 *   bulunursa  -> dugumun +0x0B alanindaki UST DORTLU bir artirilip
 *                 dugum donduruluyor (basvuru sayaci gibi davraniyor)
 *   bulunmazsa -> yedek dugumun kimligi 0x7FEF (bos isareti) ise
 *                 listeden cikarilip yeni kimlikle kurulup geri takiliyor
 *   yedek bos degilse -> 0
 *
 * Ust dortlu `lsls #24 / asrs #28` ile ISARETLI okunuyor, yani ust dortlu
 * isaretli bir sayac. ALAN TIPI YINE DE u8: ROM `ldrb` ile okuyor; s8
 * yapinca derleyici `ldrsb` + `asrs #4`e katliyor (fark +6).
 *
 * DURUM: PARK, 126/126 boyut TUTUYOR, fark 7  (bu oturumda 41 -> 7).
 * Kalan 4 komut: (a) `ldrh r2,[r4,#8]`, ROM'da `adds r2,r5,#0`  [2 bayt]
 *                (b) found blogundaki ands/orrs/strb yon farki   [5 bayt]
 *
 * ----------------------------------------------------------------------
 * BU OTURUMDA OLCULEN UC MEKANIZMA (hepsi -da dokumleriyle dogrulandi)
 * ----------------------------------------------------------------------
 * 1) r5/r6 TAKASI -- `InsertSorted(list, spare, spare->id)`
 *    ROM r6=list r5=id, bizde tersiydi (~14 bayt). Sebep: global dagitici
 *    onceligi floor_log2(refs)*refs/omur; list (5 ref / 43 insn) = 2325,
 *    id (5 / 44) = 2272. list ONCE isleniyor ve find_reg EN KUCUK bos
 *    yazmaci (r5) veriyor. id'nin omru list'inkinden HER ZAMAN 1 fazla:
 *    prologda list once kopyalaniyor (def -1), cagri argumanlari ise her
 *    zaman r0,r1,r2 sirasinda uretiliyor (son kullanim +2). Yani duz
 *    yazimda bu esik ASLA asilamaz.
 *    COZUM: 3. argumani BELLEKTEN oku. gcc'nin expand_call'i MEM olan
 *    yazmac argumanlarini `copy_to_mode_reg` ile ONCEDEN hesapliyor;
 *    boylece id'nin son kullanimi `strh spare->id` oluyor: id 5 ref/44
 *    yerine 4 ref/26 (oncelik 3076) oluyor, id ONCE isleniyor, r5'i o
 *    aliyor. BEDELI: fazladan `ldrh r2,[r4,#8]` (2 bayt). Net +12.
 *
 * 2) `movs r1,#2 / negs r1,r1` -- (u8) DARALTMASI
 *    Duz yazimda derleyici bunu `subs r1,#4` diye tek komuta indiriyordu;
 *    2 bayt kayiyor, literal havuzu hizalamasi bozuluyor ve aradaki ~10
 *    komut kayarak fark 20 bayt buyuyordu.
 *    Sebep OLCULDU: donusum RELOAD sonrasindaki `move2add`; dokumlerde
 *    `const_int -4` ILK KEZ .greg'de beliriyor, .lreg'de yok. move2add
 *    bir DONANIM yazmacindaki sabiti izliyor ve yeni sabit daha pahaliysa
 *    (negatif sabit = movs+negs, 2 komut) `adds/subs` ile turetiyor. AMA
 *    izleme KIPE duyarli: reg_mode ayni degilse tetiklenmiyor.
 *    ROM'da `| 16` QImode (`*movqi_insn`) ve `| 2` de QImode; `& -2`
 *    SImode oldugu icin zincir kopuyor. Bizde `| 2` SImode idi -> zincir
 *    kurulup `subs r1,#4` cikiyordu. `k = (u8)(k | 2);` yazimi o sabiti
 *    QImode'a indiriyor ve ROM'un movs/negs kalibi aynen cikiyor.
 *    (-2'den -13'e olan `subs r1,#11` zinciri ROM'da da var, dokunma.)
 *
 * 3) `| 2` SIRASI -- ROM'daki gibi iki AND'den ONCE olmali; ama (u8)
 *    daraltmasi olmadan one alinirsa (2) devreye giriyor.
 *
 * ELENEN YOLLAR (tekrar denemeyin):
 *   - `k & -2` / `& 0xFFFFFFFE` / `& (0-2)` / maskeleri yerele alma
 *     (`m = ~1; k &= m;`): hepsi ayni RTL, hicbiri zinciri kirmiyor.
 *   - `| 2`yi iki AND arasina ya da sonuna almak: zincir kirilir ama
 *     komut SIRASI ROM'dan sapar (fark 15/16; (u8) yazimi 7).
 *   - `(k | 2) & ~1 & ~12` tek ifade: 122 bayt, ANDler birlesiyor.
 *   - `spare->slot = 0;` magazasini `| 16` ile `| 2` arasindan cikarmak:
 *     iki `orrs` tek `orrs #18`e katlaniyor. Yeri ROM'daki gibi kalmali.
 *   - InsertSorted 3. argumani `id` iken omru kisaltmanin BASKA yolu yok
 *     (arg kopyalari her zaman r0,r1,r2 sirasinda); `spare->slot = id`
 *     gibi fazladan referans da 46'ya cikariyor.
 *   - found blogundaki `ands r2,r0` yonu icin DENENEN VE ELENEN 20+ yazim:
 *       `(k2&15) | ((hi+1)<<4)`, `(15&k2)`, `(m15&k2)` (m15 yerel; ilk ya
 *       da son bildirim, blok icinde/disinda atanmis), sonucu s32/u8 k3
 *       yereline alma, `hi+1`i ayri komuta bolme, kaymayi t degiskenine
 *       alma, alani (`cur->kind`) ifade icinde TEKRAR okuma, k2'yi u8/u16
 *       yapma, k2'yi hic kullanmama. HEPSI 7 ya da daha kotu (14/22/25).
 *     Olculen sebep: `regmove` pasi iki adresli `andsi3`in hedefini HER
 *     ZAMAN k2'nin pseudo'suna bagliyor -- .combine'da
 *     `(set (reg 81) (and (reg 29) (reg 80)))`, .regmove'da
 *     `(set (reg 29) (and (reg 29) (reg 80)))` -- operand sirasindan
 *     BAGIMSIZ olarak (m15 ile operandlari cevirince de ayni). ROM sabitin
 *     yazmacina bagliyor. Kaynak duzeyinde bu secimi ceviren kaldirac
 *     bulunamadi; sonraki denemede regmove'un hangi kosulu kacirdigina
 *     bakilmali (aday: `reg/v` kullanici degiskeni onceligi).
 *
 * Kardes dosyalar: nodelist_c3.c bu fonksiyonun imzasini zaten
 * bildiriyordu (`Node *FUN_080543d0(Node **head, s32 index)`), nodelist_b6.c
 * de cagiriyor. Node yerlesimi oradan alindi.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_a1.c
 */

#include "gba_types.h"

#define SPARE_ID 0x7FEF

typedef struct Node {
    struct Node *next;          /* 0x00 */
    u8    pad04[4];
    u16   id;                   /* 0x08 */
    u8    slot;                 /* 0x0A */
    u8    kind;                 /* 0x0B */
} Node;

typedef struct NodeList {
    Node *head;                 /* 0x00 */
    Node *spare;                /* 0x04 */
} NodeList;

extern void ListRemove(NodeList *list, Node *node);
extern void InsertSorted(NodeList *list, Node *node, s32 id);

/* 0x080543D0 */
Node *FUN_080543d0(NodeList *list, s32 id)
{
    Node *cur;
    Node *spare;
    s32 k;
    s32 cid;
    s32 hi;
    s32 k2;

    cur = list->head;
    spare = list->spare;
    if (cur == 0) goto insert;

scan:
    cid = cur->id;
    if (cid == id) goto found;
    if (cid > id) goto insert;
    cur = cur->next;
    if (cur != 0) goto scan;

insert:
    if (spare->id != SPARE_ID) goto none;

    ListRemove(list, spare);
    spare->id = id;
    k = (spare->kind & 15) | 16;
    /* Aradaki magaza `| 16` ile `| 2`nin tek `orrs`a katlanmasini
     * engelliyor (ROM'da iki ayri `orrs` var); (u8) ise sabiti QImode'a
     * indirip move2add zincirini kiriyor -- basliktaki (2). */
    spare->slot = 0;
    k = (u8)(k | 2);
    k = k & ~1;
    k = k & ~12;
    spare->kind = k;
    /* 3. arguman BELLEKTEN: id'nin omrunu kisaltip r5'i ona kaptiriyor,
     * basliktaki (1). Deger `id` ile ayni, hemen ustte oraya yazildi. */
    InsertSorted(list, spare, spare->id);
    return spare;

    /* ROM bu govdeyi fonksiyonun SONUNDA tutuyor (`beq` ileri atliyor);
     * dongunun icine yazmak blogu one aliyor ve dallanma tersine donuyor. */
found:
    /* AYRI yerel: `k` hem insert hem found dalinda kullanilinca 19 referansa
     * cikip r2'ye dusuyordu; bolununce 12'ye inip ROM'un r0'ini aliyor. */
    k2 = cur->kind;
    hi = (s32)(k2 << 24) >> 28;
    cur->kind = ((hi + 1) << 4) | (k2 & 15);   /* ROM once (hi+1)<<4 kuruyor */
    return cur;

none:
    return 0;
}
