/* StepDecay -- 0x08023974-0x08023A0B (152 bayt) -- ESLESTI
 *
 * Uc kapidan gecerse degeri azaltip tabana kirpiyor, komsu bayrakliysa
 * ikinci bir esikle daha kirpiyor, sonra dort girisli ROM tablosunda
 * kaydirmali karsilastirma yapip etiketi guncelliyor.
 *
 * ROM'DAN OKUNAN YERLESIM (esleseni dogruladi):
 *   r4 = obj, r5 = peer, r1 = delta, r3 = limit (taramada r6'ya kopya),
 *   r2 = kapida gRam adresi / uygulamadaki eski deger, r0 = gecici.
 *   Uc callee-saved: r4, r5, r6 -- `push {r4, r5, r6, lr}`.
 *
 * Sabit kaliplari: 0x1000000 = `0x80 << 17`, 0x20000 = `0x80 << 10`
 * (movs+lsls); 0x0063FFFF ve 0x0001FFFF havuzdan.
 * Kural 35: `pop {r1}; bx r1` -> r0 donus degeri tasiyor, imza u32.
 * Arg2 hicbir yerde okunmuyor (r2 hemen havuz adresiyle eziliyor) ama
 * arg3 node oldugu icin imzada yer tutucu olarak duruyor.
 *
 * IKI GENISLIKLI OKUMALAR -- ikisi de kaynakta AYRI ifade olmali:
 *   gRam02000224: kapida `ldrb` (0x8023994), uygulamada `ldr` (0x80239a8).
 *   obj->stamp   : kapida `ldrb r6,[r4,#12]` (0x8023996) ama uygulamada
 *                  `str r0,[r4,#12]` (0x80239aa). Yani AYNI alan bayt
 *                  okunup soz yaziliyor; kapiya `*(u8 *)&obj->stamp`
 *                  yazmak sart, duz `obj->stamp` `ldr` uretir.
 *
 * ESLESMEYI ACAN IKI KALDIRAC. Ikisi de saf DAGITIM meselesiydi: kontrol
 * akisi ve komut secimi bunlardan ONCE zaten dogruydu (152 bayt tutuyor,
 * 74 komuttan 65'i ayni), fark yalnizca hangi degerin hangi yazmaca
 * dustugundeydi.
 *
 *  1. KURAL 45, obj->value uzerinde. Deger UC ayri yerde yukleniyor
 *     (0x8023984 kapi, 0x80239ac uygulama, 0x80239e4 tarama) ve ROM
 *     ucunu de AYRI yazmaca koyuyor: r0 / r2 / r5. Uc yuklemeyi TEK
 *     `old` yereline yazmak agbcc'de tek bir pseudo uretiyordu
 *     (p28: 9 ref, oncelik 0.844) ve bu pseudo sirasi geldiginde r3'u
 *     kapip limit'i callee-saved r6'ya suruyordu; oradan zincirleme
 *     peer r2'ye, gRam adresi r5'e kayiyordu. Uc ayri yerel
 *     (old / cur / val) farki 51 -> 38'e dusurdu ve peer, gRam,
 *     uygulama ve tarama yazmaclarinin HEPSI ayni anda ROM'a oturdu.
 *
 *  2. limit'in CANLI ARALIK BOLUNMESI. ROM taramanin girisinde
 *     `adds r6, r3, #0` ile limit'i r3'ten r6'ya kopyaliyor, cunku r3
 *     tablo isaretcisine gerekiyor. Tek yerelle yazildiginda agbcc
 *     limit'i bastan r6'ya koyup bu kopyayi hic uretmiyordu -- yani
 *     bizde bir komut EKSIK kaliyordu; boyutun yine de 152 cikmasini
 *     havuz hizalamasi icin eklenen `movs r0, r0` dolgusu maskeliyordu.
 *     Tarama oncesinde `base = limit;` yazmak kopyayi geri getiriyor.
 *
 *     >>> BU, "kopya tabanli bolme HER ZAMAN elenir" notunun BILINEN
 *     ILK ISTISNASI. Burada elenmiyor cunku iki aralik ARASINDA birlesme
 *     noktalari var: `base` yalnizca tarama dongusunde, `limit` yalnizca
 *     kapida canli, ortusme yok. Onceki elemeler (lst = list) aralilarin
 *     IC ICE gectigi durumlardi -- ayirt edici olcut ortusme.
 *     `base = obj->limit;` (ikinci yukleme) yazimi da AYNEN esliyor,
 *     cunku CSE ikinci `ldr`i ayni kopyaya ceviriyor. Kaynakta kopya
 *     bicimi tercih edildi: ROM'da tek `ldr [r4,#4]` var, ikinci bir
 *     bellek okumasi yazmak okuyucuyu yaniltirdi.
 *
 * TARAMA DONGUSU: tablo isaretcisi ROM'da tumevarim degiskeni
 * (`adds r3,#4`), indeks degil -- `TABLE[i]` yazimi lsls+ldr ikilisi
 * uretiyor, yuruyen isaretci sart (kural 37). Artirim sirasi ROM'un
 * sirasi: once isaretci (0x80239f4), sonra sayac (0x80239f6). Sayac
 * isaretli: `cmp r2,#3` + `ble` (kural 31), yani `s32 i` ve `i <= 3`;
 * `u32` yazimi `bls` verirdi. Onsoz deyim sirasi da ROM'un sirasi:
 * i, base, val, entry.
 *
 * DENENIP ELENEN YAZIMLAR (hedef 152 bayt / 74 komut):
 *  - Ilk kurulum (ic ice kosul tahmini): 156 bayt, 145 fark.
 *  - dump_cfg.py goto zinciri, `span` tek yerelde: 156 bayt, 145 fark,
 *    push {r4,r5,r6,r7,lr} -- bir fazla callee-saved.
 *  - Tek `old` yereli + yuruyen isaretci: 152 bayt, 51 fark.
 *  - `TABLE[i]` indeksleme (do/while): 56 fark. `for` bicimi: 58 fark.
 *  - Onsoz deyimlerinin ALTI permutasyonu: hepsi 25 farkli komut --
 *    deyim sirasi burada kaldirac DEGIL, dagitim kaldiracti.
 *  - `u32 *ram = &gRam02000224;` (adresi yerele almak): 53 fark.
 *  - Kapiyi ic ice `if`lere acmak: 51 fark (&& zinciriyle ayni cikti).
 *  - Yerel bildirim sirasini degistirmek: 51 fark (etkisiz).
 *  - `limit = obj->limit;` ile AYNI yerele yeniden atama (bolme degil,
 *    yeniden tanim): 48 fark -- yerelin AYRI olmasi sart.
 *  - `base = limit;` deyimini `entry = TABLE;`den SONRA koymak: 4 fark.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/world/step_decay.c  -> 152/152 eslesti
 */

#include "gba_types.h"

#define DELTA_MAX   0x0063FFFF
#define PEER_BIT    (0x80 << 17)
#define SPAN_LIMIT  0x0001FFFF
#define SPAN_RESET  (0x80 << 10)
#define TABLE       ((s32 *)0x08342AC8)

typedef struct Obj {
    u8  tag;                    /* +0x00  strb, tarama sonucu */
    u8  pad01[3];
    s32 limit;                  /* +0x04  asrs ile kullaniliyor => isaretli */
    s32 value;                  /* +0x08 */
    u32 stamp;                  /* +0x0C  bayt okunur, soz yazilir */
} Obj;

typedef struct Peer {
    u8  pad00[24];
    u32 flags;                  /* +0x18 */
} Peer;

typedef struct Node {
    u8    pad00[44];
    Peer *peer;                 /* +0x2C */
} Node;

extern u32 gRam02000224;

/* 0x08023974 */
u32 StepDecay(Obj *obj, s32 delta, u32 unused, Node *node)
{
    Peer *peer;
    s32 limit;
    s32 base;
    s32 old;
    s32 cur;
    s32 val;
    s32 next;
    s32 i;
    s32 *entry;

    peer = 0;
    if (node != 0)
        peer = node->peer;

    if (delta == 0)
        return 0;

    /* Uc kapi; hepsi birden gecerse "henuz sirasi degil" deyip cikiyor. */
    old = obj->value;
    limit = obj->limit;
    if (old < limit && delta <= DELTA_MAX
        && *(u8 *)&gRam02000224 == *(u8 *)&obj->stamp)
        return 0;

    obj->stamp = gRam02000224;

    /* Azaltip tabana kirp. Sifir zaten tabanda, ona dokunulmuyor. */
    cur = obj->value;
    if (cur != 0) {
        next = cur - delta;
        obj->value = next;
        if (next <= 0)
            obj->value = 1;
    }

    /* Komsu bayrakliysa ikinci esik: ust bolgeden dusenler tabana oturur.
     * Karsilastirilan `cur` AZALTMADAN ONCEKI deger (ROM'da r2). */
    if (peer != 0 && (peer->flags & PEER_BIT) != 0
        && cur > SPAN_LIMIT && obj->value <= SPAN_LIMIT)
        obj->value = SPAN_RESET;

    /* Dort girisli tabloda kaydirmali esik taramasi; en son gecen kazanir.
     * `base` limit'in tarama omru: r3 tablo isaretcisine gerektiginden
     * ROM burada `adds r6, r3, #0` kopyasini uretiyor (bkz. baslik, 2). */
    i = 0;
    base = limit;
    val = obj->value;
    entry = TABLE;
    do {
        if (val <= (base >> *entry))
            obj->tag = i;
        entry++;
        i++;
    } while (i <= 3);

    return 1;
}
