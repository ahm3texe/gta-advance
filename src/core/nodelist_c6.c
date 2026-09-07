/* Dugume varlik olusturup birincil ya da ikincil yuvaya baglama
 * 0x08053FF4, 292 bayt (ortada 16 bayt, sonda 12 bayt literal havuzu).
 *
 * Birincil kimlik (0x020272C8) dolu VE onun varligi kuruluysa yeni dugum
 * IKINCIL yuvaya (kimlik 0x02026F34, yuva 1, tur 2) baglanir; aksi halde
 * BIRINCIL yuvaya (kimlik 0x020272C8, yuva 0, tur 1). Iki dal ayni sirayi
 * izler: kilidi kur, kimligi yaz, varligi uret, ilet, yuvayi yapilandir,
 * alt nesneyi hazirla, secili yuvaysa iki bildirimi yap, dugum ile varligi
 * karsilikli bagla, kilidi sifirla, 1 dondur.
 *
 * ROM'DAN OLCULEN AYRINTILAR
 * --------------------------
 * - Epilog `pop {r1}; bx r1` ve `movs r0,#1`: r0 canli kaldigi icin donus
 *   adresi r1'e aliniyor -- fonksiyon DEGER donduruyor (kural 35 tersi).
 *   Daralma komutu yok, yani donus tipi 32 bit.
 * - Prolog `push {r4-r7,lr}` + `push {r8,r9}`: cagrilari asan alti deger
 *   var; r8/r9 dallara ozgu (asagi bak).
 * - THEN dalinda 0x0202F2DC adresi r9'a, sabit 2 r8'e aliniyor ve 2 hem
 *   kilide yazilirken hem de sondaki `bits |= 2`de kullaniliyor: agbcc
 *   ayni sabiti CSE ile tek yazmacta tutuyor, kaynakta iki kez duz `2`
 *   yazmak yeterli (ayri sabit yereli GEREKMIYOR). ELSE dalinda kilide 1
 *   yazildigi icin paylasim yok, sondaki 2 `movs r1,#2` ile kuruluyor --
 *   iki dalin farkli komut sayisi tam olarak bunun izi.
 * - Girisdeki `adds r2,r0,#0`: 0x020272C8 adresi ELSE dalindaki yazma icin
 *   saklaniyor. Bu kopya cagrilardan ONCE tuketiliyor (ilk `bl`den once
 *   `str r5,[r2]`), o yuzden caller-saved r2 yetiyor; ayri yerel gerekmiyor.
 * - Sondaki `str r0,[r1,#0]` ortak: agbcc iki dalin kuyrugunu KENDI
 *   birlestiriyor (cross-jumping) ve her dal kendi adres yazmacini r1'e
 *   kopyaliyor. Kuyrugu KAYNAKTA paylastirmak yanlis (asagi bak).
 * - Bildirim kapisi 0x02000D40 (gSlotSelector) IKI FARKLI komutla okunuyor:
 *   THEN `ldrh r0,[r7,#0]` + `cmp #1`, ELSE `movs r1,#0; ldrsh r0,[r7,r1]`
 *   + `cmp #0`. Thumb'da LDRSH'in anlik ofsetli bicimi yok, yani ELSE'te
 *   fazladan komutu derleyici bile bile odemis: alan ISARETLI. Olculdu --
 *   `u16` yapinca ELSE'teki ldrsh ldrh'ye dusuyor ve boyut 288'e iniyor
 *   (4 KISA). `s16` ile `== 1` sinamasi ldrh'ye daraliyor, `== 0` sinamasi
 *   daralmiyor; kural 47'nin (maske/genislik iki yonu de olculur) okuma
 *   tarafindaki hali.
 *
 * DENEYIP ELEDIGIM YAZIMLAR (hepsi olculdu)
 * -----------------------------------------
 * 1. Ortak kuyrugu (karsilikli baglama + kilidi sifirlama + `return 1`)
 *    if/else DISINA tek kopya olarak yazmak: 252 bayt, 40 KISA. agbcc o
 *    zaman yalniz kuyrugu degil `node->sub = obj` blogunu da tek kopyaya
 *    indiriyor. ROM iki tam kopya tutup yalnizca son store'u paylasiyor;
 *    bu ancak kuyruk HER IKI DALDA da yazilinca cikiyor.
 * 2. Kilit adresini TEK ortak yerelde tutmak (`lock` iki dalda da ayni
 *    degisken): 280 bayt, 12 KISA. Tek yerel tek pseudo demek, iki dal
 *    ayni yazmaci paylasiyor ve r8/r9 ayrimi kayboluyor. Kural 45: dal
 *    basina ayri yerel. Dal basina AYRI yerel (`lock` / `lock2`) ile
 *    esitleme yine 292/292 -- yani asagidaki dogrudan yazim ile ayni kodu
 *    veriyor, sadelik icin yerelsiz bicim secildi.
 * 3. `gSlotSelector` tipini `u16` yapmak: 288 bayt, 4 KISA (yukarida).
 * 4. FUN_08038608 sonucunu ayri yerele almak: 292/292, FARK YOK. Cagri
 *    zaten tek kullanimlik, satir ici yazim ROM'un sirasini veriyor.
 *
 * ESLESME: 292/292 bayt.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_c6.c
 */

#include "gba_types.h"

typedef struct Node Node;
typedef struct Entity Entity;

/* Olusturma kilidi: dal basina 2 (ikincil) veya 1 (birincil) yazilip is
 * bitince sifirlaniyor. GEREKLI: data/ram_map.csv'ye 0x0202F2DC icin 4
 * baytlik bir kayit eklenmeli (adi ben veremem, bu yuzden #define). */
#define gSpawnLock ((u32 *)0x0202F2DC)

#define SLOT_PRIMARY_KIND    1      /* birincil yuvaya verilen tur */
#define SLOT_SECONDARY_KIND  2      /* ikincil yuvaya verilen tur  */
#define NODE_BOUND_BIT       2      /* +0x18'de "varliga bagli" biti */

/* Dugum yerlesimi kardes src/core/nodelist_b3.c ile ayni aileden:
 * +0x18 bayrak kelimesi, +0x28 alt nesne (buradaki varlik). */
struct Node {
    u8      pad00[0x18];
    u32     bits;               /* +0x18 */
    u8      pad1C[12];
    Entity *sub;                /* +0x28 */
};

/* Varlik: +0x2C dugume geri isaret ediyor (nodelist_b3.c ile ayni alan). */
struct Entity {
    u8      pad00[0x14];
    void   *unk14;              /* +0x14 alt nesne, FUN_0801d8a0'a gidiyor */
    u8      pad18[0x14];
    Node   *node;               /* +0x2C */
};

/* FUN_08038608'in donusu; yalniz +0x08 alani kullaniliyor ve o alan
 * ConfigureSlot'un `kind` argumani (src/world/slot_config.c). */
typedef struct SlotEntry {
    u8  pad00[8];
    u32 kind;                   /* +0x08 */
} SlotEntry;

/* Ayni 4 baytlik kelimeye farkli gorunum: src/world/node_search.c ve
 * src/core/nodelist_a7.c bunlari `u32` (kimlik degeri) olarak biliyor.
 * Tek sembole iki extern tur vermek check_consistency'nin ram-extern
 * denetimine takiliyor, o yuzden YERLESIK tip korunup kullanim yerinde
 * cast ediliyor (bkz. include/ram_symbols.h basligi). */
extern u32 gRam020272C8;        /* birincil kimlik / dugum */
extern u32 gRam02026F34;        /* ikincil kimlik / dugum  */
extern s16   gSlotSelector;     /* 0x02000D40: etkin yuva secici */

extern Entity    *SubmitObject(void *object, Node *node);   /* 0x08038234 */
extern void       ForwardZeroArg4(Entity *e, u32 arg, u32 zero);
extern SlotEntry *FUN_08038608(void *object);
extern void       ConfigureSlot(Entity *e, u32 kind, int which);
extern void       FUN_0801d8a0(void *sub, u32 zero);
extern void       FUN_08008f74(u32 arg);
extern void       BindActorToCoordSlot(Entity *e, u32 kind, u32 zero);
extern void       FUN_0800a484(u32 zero);

/* 0x08053FF4 */
u32 SpawnNodeObject(Node *node, u32 arg)
{
    Node   *cur;
    Entity *obj;

    cur = (Node *)gRam020272C8;
    if (cur != 0 && cur->sub != 0) {
        /* Birincil kimlik zaten kurulu: ikincil yuvaya bagla. */
        *gSpawnLock = SLOT_SECONDARY_KIND;
        gRam02026F34 = (u32)node;
        obj = SubmitObject(0, node);
        ForwardZeroArg4(obj, arg, 0);
        ConfigureSlot(obj, FUN_08038608(0)->kind, 1);
        FUN_0801d8a0(obj->unk14, 0);
        if (gSlotSelector == 1)
            FUN_08008f74(arg);
        BindActorToCoordSlot(obj, SLOT_SECONDARY_KIND, 0);
        if (gSlotSelector == 1)
            FUN_0800a484(0);
        /* Kuyruk her iki dalda da tam yazilmali (elenen yol 1). */
        node->sub = obj;
        node->bits |= NODE_BOUND_BIT;
        obj->node = node;
        *gSpawnLock = 0;
        return 1;
    } else {
        /* Birincil yuva bos: kimligi buraya yaz. */
        *gSpawnLock = SLOT_PRIMARY_KIND;
        gRam020272C8 = (u32)node;
        obj = SubmitObject(0, node);
        ForwardZeroArg4(obj, arg, 0);
        ConfigureSlot(obj, FUN_08038608(0)->kind, 0);
        FUN_0801d8a0(obj->unk14, 0);
        if (gSlotSelector == 0)
            FUN_08008f74(arg);
        BindActorToCoordSlot(obj, SLOT_PRIMARY_KIND, 0);
        if (gSlotSelector == 0)
            FUN_0800a484(0);
        node->sub = obj;
        node->bits |= NODE_BOUND_BIT;
        obj->node = node;
        *gSpawnLock = 0;
        return 1;
    }
}
