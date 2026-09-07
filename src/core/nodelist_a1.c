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
 * DURUM: BYTE-MATCHING, 126/126.  (Onceki oturum 41 -> 7'ye indirmisti;
 * bu oturumda kalan 7 bayt iki olcumle kapandi -- asagida (0) ve (1).)
 *
 * ----------------------------------------------------------------------
 * OLCULEN MEKANIZMALAR (hepsi -da dokumleriyle / diff_function ile dogrulandi)
 * ----------------------------------------------------------------------
 * 0) found blogu: BIRIKTIRICIYI SABITE KUR, KAYMAYI ONCE HESAPLA  [5 bayt]
 *    ROM: `movs r2,#15 / ands r2,r0 / orrs r2,r1 / strb r2,[r3,#11]` --
 *    yani iki adresli `andsi3`in hedefi SABITIN yazmaci, k2'nin degil.
 *    `(k2 & 15) | ...` gibi her IFADE yaziminda regmove hedefi k2'nin
 *    pseudo'suna bagliyor (onceki oturumun 20+ denemesi hep bunda takildi).
 *    KALDIRAC: sabiti bir yerele atayip UZERINE bileske atama yapmak --
 *        m = 15;  m &= k2;  m |= t;  cur->kind = m;
 *    Boylece expand daha en basta `(set m (and m k2))` uretiyor; regmove'un
 *    ceviresek bir seyi kalmiyor. Bu tek basina yonu duzeltti (7 -> ...),
 *    AMA sirayi bozdu: `movs r2,#15 / ands r2,r0` kaymadan ONCE cikti.
 *    Ikinci yari: `(hi + 1) << 4` AYRI bir deyimle (`t`) once hesaplanmali.
 *    Ikisi birlikte ROM'un komut sirasini aynen veriyor (fark 12 -> 4).
 *
 * 1) r5/r6 TAKASI -- COZUM: DONGUYU GERCEK do/while YAP  [4 bayt]
 *    ROM r6=list r5=id; duz yazimda tersiydi (~10 bayt). Sebep: global
 *    dagitici onceligi floor_log2(refs)*refs/omur; list 5 ref/45 = 0.2222,
 *    id 5 ref/46 = 0.2174. list ONCE isleniyor ve find_reg EN KUCUK bos
 *    yazmaci (r5) veriyor. id'nin omru list'inkinden HER ZAMAN 1 fazla:
 *    prologda list once kopyalaniyor (def -1), cagri argumanlari ise her
 *    zaman r0,r1,r2 sirasinda uretiliyor (son kullanim +2).
 *    KALDIRAC (yeni, genel): tarama dongusu `goto scan` ile yazilinca gcc
 *    NOTE_INSN_LOOP_BEG/END NOTU URETMIYOR, dolayisiyla flow.c'nin
 *    `REG_N_REFS += loop_depth` agirliklandirmasi calismiyor ve dongu ici
 *    referanslar 1 sayiliyor. Ayni govde giris korumali `do { } while` ile
 *    yazilinca dongu notlari cikiyor: id'nin dongudeki IKI `cmp` referansi
 *    agirlik kazaniyor (5 -> 7), floor_log2(7)*7/46 = 0.304 ile list'in
 *    0.2222'sini geciyor, id ONCE isleniyor ve r5'i aliyor.
 *    list dongu icinde HIC gecmedigi icin onun agirligi degismiyor --
 *    esik bu yuzden asilabiliyor.
 *    Bunun yan faydasi: 3. argumani artik BELLEKTEN okumaya gerek yok,
 *    `InsertSorted(list, spare, id)` dogrudan `adds r2,r5,#0` veriyor
 *    (onceki cozumun bedeli olan fazladan `ldrh r2,[r4,#8]` gitti).
 *    NOT: dongu bicimi ROM'dan okundu -- giris korumali do/while; `while`
 *    ya da `for` yazimi ust testi one alip dallanmayi tersine ceviriyor.
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
 *   - InsertSorted 3. argumanini `spare->id` yapip BELLEKTEN okumak:
 *     r5/r6'yi duzeltiyor ama fazladan `ldrh r2,[r4,#8]` biraktigi icin
 *     4 bayt fark kaliyor. (1)'deki dongu kaldiraci bunun yerini aldi.
 *   - `id`nin omrunu kisaltmanin BASKA yolu yok (arg kopyalari her zaman
 *     r0,r1,r2 sirasinda); `spare->slot = id` gibi fazladan referans da
 *     omru 46'ya cikariyor.
 *   - found blogundaki `ands r2,r0` yonu icin denenen ve elenen 20+ IFADE
 *     yazimi (cozum ifade degil, BILESKE ATAMA -- bkz. (0)):
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
 *     yazmacina bagliyor. Cozum regmove'u ikna etmek degil, ona hic is
 *     birakmamak: `m = 15; m &= k2;` (bkz. (0)).
 *   - `m |= (hi + 1) << 4;` tek deyimde: yon dogru ama sira ters
 *     (`movs #15/ands` kaymadan once cikiyor, fark 12). Kaymayi ayri bir
 *     `t` deyimine almak sart.
 *
 * Kardes dosyalar: nodelist_c3.c bu fonksiyonun imzasini zaten
 * bildiriyordu (`Node *FindOrClaimNode(Node **head, s32 index)`), nodelist_b6.c
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
Node *FindOrClaimNode(NodeList *list, s32 id)
{
    Node *cur;
    Node *spare;
    s32 k;
    s32 cid;
    s32 hi;
    s32 k2;
    s32 m;
    s32 t;

    cur = list->head;
    spare = list->spare;
    /* GERCEK dongu deyimi sart: `goto` ile yazilinca gcc dongu notu
     * uretmiyor, `id`nin dongu ici iki referansi agirlik kazanmiyor ve
     * r5 list'e gidiyor -- basliktaki (1). */
    if (cur != 0) {
        do {
            cid = cur->id;
            if (cid == id) goto found;
            if (cid > id) break;
            cur = cur->next;
        } while (cur != 0);
    }

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
    InsertSorted(list, spare, id);
    return spare;

    /* ROM bu govdeyi fonksiyonun SONUNDA tutuyor (`beq` ileri atliyor);
     * dongunun icine yazmak blogu one aliyor ve dallanma tersine donuyor. */
found:
    /* AYRI yerel: `k` hem insert hem found dalinda kullanilinca 19 referansa
     * cikip r2'ye dusuyordu; bolununce 12'ye inip ROM'un r0'ini aliyor. */
    k2 = cur->kind;
    hi = (s32)(k2 << 24) >> 28;
    /* Kayma ONCE ayri bir deyimde: ROM `adds #1 / lsls #4`i maskeden once
     * kuruyor; tek ifadede yazilinca `movs #15 / ands` one geciyor. */
    t = (hi + 1) << 4;
    /* Sabiti yerele kurup UZERINE bileske atama: iki adresli `ands`in
     * hedefi boylece sabitin yazmaci oluyor -- basliktaki (0). */
    m = 15;
    m &= k2;
    m |= t;
    cur->kind = m;
    return cur;

none:
    return 0;
}
