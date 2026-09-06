/* Alan baglamini yeni bir tanimla kurma -- 0x08053834, 250 bayt.
 *
 * Uc arguman aliyor: alan baglami, tanim yapisi ve bir ad isaretcisi.
 * Once baglam DMA ile sifirlaniyor, sonra ada karsilik gelen kimlik
 * aranip tek yuvaya yaziliyor, ardindan tanimdaki kimlik dizisi
 * baglamin yuva dizisine kopyalanip her kimlik baglantiya diziliyor.
 * Kardesi FUN_08053794 (src/core/nodelist_a9.c) ile ayni yapi ailesi.
 *
 * DURUM: PARK, 246/250 (dort bayt KISA), fark 13 -- gercekte TEK KOMUT.
 *   Onceki tur da 246 idi ama fark 137 ve elenen yollar YAZILMAMISTI.
 *   Bu tur kural 50 (degisken BOLME) iki yerde uygulandi, 137 -> 13:
 *
 *   1. Iki dongu tek `i` sayacini paylasiyordu; tek allocno olunca omru
 *      uzuyor ve oncelik siralamasi ROM'unkinin tersine donuyordu
 *      (ROM: 1. dongu sayaci r4, 2. dongu sayaci r6, adim isaretcisi r5;
 *      bizde ikisi de r5, adim r4).  Ikinci donguye ayri `j` verildi:
 *      fark 33 -> 16, dagitim ROM ile birebir.
 *   2. `dst` hem FindFreeSlotRun donusunu hem sonraki `ctx->slots`
 *      okumasini tasiyordu; tek allocno cagrilari astigi icin callee-saved
 *      (r4) sectiriyordu.  ROM ilkini r1'de (caller-saved) tutuyor.
 *      Ilk deger ayri `run` yereline alindi: fark 16 -> 13.
 *   Ikisi de kural 50'nin "IKI AYRI URETIM YERI" kosulunu saglar
 *   (`i=0` iki kez / `FindFreeSlotRun()` ve `ctx->slots`), kopya tabanli
 *   bolme DEGILDIR -- o yuzden calisti.
 *
 * KALAN TEK FARK -- olculdu, mekanizmasi bulundu, kaldiraci YOK:
 *   ROM:   lsls r0,r0,#16 / lsrs r0,r0,#16 / adds r4,r0,#0 / ldr r0,=0x7FFF
 *   bizde: lsls r0,r0,#16 / lsrs r4,r0,#16 /                 ldr r0,=0x7FFF
 *   ROM u16 daraltmasini bir GECICIYE yapip idx'e KOPYALIYOR; biz dogrudan
 *   idx'e yaziyoruz.  2 bayt kopya + 2 bayt havuz hizalama dolgusu = 4.
 *
 *   RTL dokumuyle izlendi (old_agbcc -dr -dc, dokumler t.i.rtl / t.i.cse):
 *   agbcc genisletmede kopyayi ZATEN URETIYOR --
 *       (insn 145) reg62 = lshiftrt(reg63,16)     ; zext(raw)
 *       (insn 147) reg29 = reg62                  ; idx = <o gecici>
 *   ama t.i.cse'de insn 147 YOK: CSE reg29'un butun kullanimlarini reg62
 *   ile degistirip olu kopyayi atiyor; birakirsa da combine iki komutu
 *   birlestiriyor (reg62 kopyada oluyor).  Kopyanin ayakta kalmasi icin
 *   ya reg62 kopyadan SONRA da kullanilmali, ya da kopya ile idx'in ilk
 *   kullanimi ARASINDA bir CSE blok siniri (cok-onculu etiket) olmali.
 *   Ikisi de kaynaktan uretilemedi (asagi).  Bu, docs/COMPILER.md'deki
 *   "Yazmac kopyasi: kaynak duzeyinden uretilemeyen sinif" (FUN_08030e78 /
 *   FUN_08030f28, ikisi de dort bayt kisa, fazladan `adds rX,rY,#0`)
 *   ile AYNI imza.  Yeni mekanizma cikmadan dokunmayin.
 *
 * DENENIP ELENENLER (~55 varyant; belirtilmedikce 246 bayt / fark 13):
 *   - cast bicimleri: (u16)raw, raw & 0xFFFF, ((u32)raw<<16)>>16
 *   - tip/bildirim: raw u32/s32/u16, idx int/unsigned short/register,
 *     idx ile raw bildirim sirasi takasi
 *   - ara gecici: `half = raw; idx = half;` ve half/idx'in karsilastirma,
 *     saklama, cagri argumaninda 8 kombinasyonu -- CSE ikisini HER
 *     durumda tek yazmaca indiriyor (kural 50'nin kopya yasagi burada da)
 *   - `raw = (u16)raw; idx = raw;` (yerinde daraltma)
 *   - karsilastirma bicimleri: ID_NONE != idx, !(idx == ID_NONE),
 *     (idx - ID_NONE) != 0, idx > ID_NONE, (s32)/(u32) cast, ID_NONE'u
 *     yerele alma (kural 44)
 *   - `(idx ^ ID_NONE) != 0` -> 250 BAYT, fark 4; `idx < ID_NONE` -> 250
 *     bayt, fark 8.  Boyut tutuyor ama fazladan komut SABIT tarafinda
 *     (`ldr r1,=0x7FFF / adds r0,r1,#0 / cmp r0,r4`), ROM'unki DEGER
 *     tarafinda.  Yanlis komut; boyutu tutturmak icin KULLANMAYIN.
 *   - govde bicimleri: idx'i her dala ayri atama; `raw` yerine dogrudan
 *     `idx` ile goto zinciri; init + `break`; `while` bicimi; iki ayri
 *     `found` etiketi; testten once yapay birlesme etiketi (bir ve iki
 *     onculu) -- 246..258, hepsi daha kotu
 *   - cmp / `*slot =` / `FUN_08053650()` argumaninda idx yerine raw'in 8
 *     kombinasyonu (238..246, fark 16..58)
 *   - fonksiyon basinda `idx = ID_NONE` on-atamasi: 250 bayt / fark 18
 *     (sabit yuklemesi proloha tasiniyor, kopya yine cikmiyor)
 *   - `volatile int raw`: 250 bayt / fark 9 (yigin trafigi ekliyor)
 *
 * DENENMEMIS TEK YOL: decomp-permuter (tools/make_permuter_dir.py).
 *   Kalan fark tek bir reg-reg kopyasi oldugundan skor duzlugu dar;
 *   memory'deki "permuter mekanizmayi yuzeye cikarir, sonra programatik
 *   tarama kapatir" kalibina uygun tek aday bu.
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama:  make c-match FILE=src/core/nodelist_b2.c
 */
#include "gba_io.h"

#define ID_NONE       0x7FFF
#define DMA_FILL_32   0x85000008

typedef struct AreaName {
    const char *name;
    u8          pad04[24];
} AreaName;

typedef struct AreaBank {
    u8        pad00[8];
    s32       nameCount;
    u8        pad0C[20];
    AreaName *names;
} AreaBank;

typedef struct AreaDesc {
    u8   pad00[4];
    u8   count;
    u8   pad05[3];
    u16 *ids;
} AreaDesc;

typedef struct AreaCtx {
    u8        pad00[10];
    u8        ready;
    u8        pad0B[9];
    AreaDesc *desc;
    u16      *slots;
    u16      *single;
} AreaCtx;

extern AreaBank gAreaBank;
extern u32      gRam02030C00;

extern s32  FUN_0806dd18(const char *a, const char *b);
extern u16 *FindFreeSlotRun(int count);
extern void FUN_08053650(s32 id);
extern void LinkAreaEntryIfEligible(s32 index);

/* 0x08053834 */
u32 FUN_08053834(AreaCtx *ctx, AreaDesc *desc, const char *name)
{
    volatile u32 fill;
    u16  ime;
    int  i;
    int  j;                 /* kural 50: 2. dongu AYRI sayac -- birlestirme */
    int  raw;
    u16  idx;
    u16 *slot;
    u16 *run;               /* kural 50: yuva kosusunun ILK degeri ayri */
    u16 *dst;
    u16 *src;

    gRam02030C00 = 0;

    ime = REG_IME;
    REG_IME = 0;
    fill = 0;
    REG_DMA3.src = (void *)&fill;
    REG_DMA3.dst = ctx;
    REG_DMA3.control = DMA_FILL_32;
    REG_DMA3.control;
    REG_IME = ime;

    if (name == 0) {
        raw = ID_NONE;
        goto found;
    }
    for (i = 0; i < gAreaBank.nameCount; i++) {
        if (FUN_0806dd18(gAreaBank.names[i].name, name) == 0) {
            raw = i;
            goto found;
        }
    }
    raw = ID_NONE;
found:
    idx = raw;

    if (idx != ID_NONE) {
        slot = FindFreeSlotRun(1);
        ctx->single = slot;
        if (slot == 0)
            goto fail;
        *slot = idx;
        FUN_08053650(idx);
    }

    /* `run` ve `dst` ayri: ROM ilkini caller-saved r1'de tutuyor, ikincisi
     * cagrilari astigi icin r4'te. Tek yerele indirmek dagitimi bozuyor. */
    run = FindFreeSlotRun(desc->count);
    ctx->slots = run;
    if (desc->count != 0) {
        if (run != 0)
            goto ready;
fail:
        return 0;
    }
ready:

    dst = ctx->slots;
    src = desc->ids;
    for (j = 0; j < desc->count; j++, dst++, src++) {
        *dst = *src;
        LinkAreaEntryIfEligible(*dst);
    }

    ctx->desc = desc;
    ctx->ready = 0xFF;
    return 1;
}
