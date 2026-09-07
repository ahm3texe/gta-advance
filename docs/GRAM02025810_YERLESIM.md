# `gRam02025810` (0x02025810) — kanıta dayalı yapı düzeni önerisi

Bu belge ROM'dan türetilmiştir. Depodaki hiçbir dosya değiştirilmedi; `data/ram_map.csv`
ve `include/ram_symbols.h` olduğu gibi bırakıldı. Önerinin uygulanması insan kararıdır.

## 0. Yöntem ve kapsam

Tüm ROM tarandı — sadece 0x080308EC–0x08031D24 bandı değil.

**Daraltma:** `data/functions.csv`'deki 1934 fonksiyonu tek tek disassemble etmek yerine,
`baserom.gba` içindeki **4 bayt hizalı her sözcük** tarandı ve değeri
`[0x02025810, 0x02025810+0x1500)` aralığına düşenler literal havuz girişi olarak alındı.
Thumb kodu bir EWRAM adresine yalnızca literal havuz üzerinden erişebildiği için bu tarama
**tamdır** (kaçak yoktur). 120 fonksiyonun gövdesinde böyle bir havuz sözcüğü bulundu.

Bu 120 fonksiyon `arm-none-eabi-objdump` ile çözüldü ve register üzerinde soyut yorumlama
(abstract interpretation) yapıldı: havuzdan yüklenen taban register'ı izlendi, `adds/subs #imm`,
`movs #imm`, `lsls #n`, `add rD, rN`, yüksek register `mov`'ları ve dal hedefleri boyunca
taşındı; her `ldr/ldrh/ldrb/ldrsb/ldrsh/str/strh/strb` erişiminde blok-göreli ofset kaydedildi.

**Tam tabandan (0x02025810) türetilen 508 erişim, 72 ayrı ofset, 101 fonksiyon** bulundu.
Durum dağılımı: 73 `candidate`, 19 `discovered`, 8 `matching`, 1 `decompiled`.

### Analiz sırasında düzeltilen iki sistematik yanılgı

Bunları burada yazıyorum çünkü aynı hatayı elle inceleyen herkes yapacaktır:

1. **`ldrsb`/`ldrsh` sabit indeks, dizi DEĞİLDİR.** Thumb'da `ldrsb`/`ldrsh` için
   immediate-offset kodlaması yoktur; derleyici `movs r0, #10` + `ldrsb r0, [r7, r0]`
   üretir. İlk taramada bunların 47 tanesi "ofset 0'da dizi" görünüyordu. Gerçekte
   bunlar **işaretli skaler alanlardır** ve blok içindeki işaretlilik bilgisinin
   tek kaynağıdır.

2. **`lsls #5` dizi adımı değil, sabit üretmedir.** `movs r2, #155; lsls r2, r2, #5`
   = 155 × 32 = 4960 = **+0x1360**. Bu deyim 34 yerde geçiyor ve ilk taramada
   "adım 32'lik dizi" sanılmıştı. Hepsi sabit ofsetli skaler erişimdir.

Ayrıca 0x02025810 sözcüğünün alt yarısı olan `0x5810`, Thumb'da geçerli bir komut
kodudur (`ldr r0, [r2, r0]`). objdump havuz verisini kod olarak çözdüğü için altı
sahte "indeksli erişim" üretmişti; bunlar elenmiştir.

---

## 1. Ölçülen asgari boyut: **0x1390 = 5008 bayt**

Kayıtlı 5006 değeri **iki bayt eksiktir**, ama kayıttaki gerekçe doğruydu.

**Kesin kanıt — bloğu tek parça olarak sıfırlayan çağrı** (`FUN_080296A0`, 0x080296B0):

```
80296b0:  4f85      ldr  r7, [pc, #532]  @ (0x80298c8)  = 0x02025810   <- taban
80296b2:  4a86      ldr  r2, [pc, #536]  @ (0x80298cc)  = 0x00001390   <- uzunluk
80296b4:  1c38      adds r0, r7, #0
80296b6:  2100      movs r1, #0
80296b8:  f044 fb02 bl   0x806dcc0  <Memset>
```

`Memset(dest=r0, value=r1, count=r2)` imzası `data/functions.csv`'de belgeli
(0x0806DCC0). Yani **`Memset(gRam02025810, 0, 0x1390)`**.

Bu kanıtı güçlendiren iki nokta:

- `0x00001390` sabiti **tüm ROM'da tam olarak bir kez** geçiyor (0x080298CC) ve o da
  bu çağrının uzunluk argümanı. Başka bir nesneyle karıştırma ihtimali yok.
- Bağımsız alt sınır: en yüksek dokunulan bayt **+0x138D**, `FUN_0802DF18` içinde
  0x0802DFF4'te `strb r0, [r1, #0]` (r1 = taban + 0x138D). Bu tek başına ≥ 0x138E
  verir; 4'e hizalanınca 0x1390 çıkar. İki kanıt birbirini doğruluyor.

Ayrıca sınırın aşılmadığını komşu sembol de destekliyor: bir sonraki adlandırılmış
sembol `gRam02026CD0` = taban + 0x14C0, yani 0x1390'dan sonra 0x130 baytlık
adlandırılmamış boşluk var — çakışma yok.

> **Not:** Bu bir *ölçülen* boyuttur, *asgari* değil. Memset bloğun tamamını
> sıfırlıyor, dolayısıyla 0x1390 hem alt hem üst sınırdır.

---

## 2. Erişim tablosu

Genişlik: `B`=1 bayt, `H`=2 bayt, `W`=4 bayt. İşaret: `S` = `ldrsb`/`ldrsh` görüldü,
`U` = yalnız işaretsiz erişim. "Fn" = o ofsete erişen ayrı fonksiyon sayısı.

### Bölge A — 0x0000–0x003B: yoğun oyuncu durumu (skaler)

| Ofset | Gen. | İşaret | Dizi? | Fn | Erişim | Örnek fonksiyonlar | Güven |
|---|---|---|---|---|---|---|---|
| +0x00 | B | **S** | hayır | 10 | 19 | FUN_080296a0, FUN_0802a3f0, FUN_080309d4, ReleaseSlotHeld | Yüksek |
| +0x02 | H | **S** | hayır | 3 | 4 | FUN_080296a0, FUN_0802a480, FUN_08030a3c | Yüksek |
| +0x04 | B (+H) | **S** | hayır | 13 | 26 | FUN_08030114, FUN_08030dd8, HalvesEqual | Yüksek (çelişki: §5.1) |
| +0x05 | B | **S** | hayır | 11 | 18 | FUN_08029e44, FUN_0802fe44, FUN_08030b88 | Yüksek |
| +0x06 | B (+H) | **S** | hayır | 4 | 8 | FUN_08029e44, FUN_0802ab18, FUN_0805ee84 | Orta (çelişki: §5.1) |
| +0x07 | B | **S** | hayır | 3 | 8 | FUN_08029e44, FUN_0802ab18, FUN_08030114 | Orta |
| +0x08 | B | **S** | hayır | 6 | 12 | FUN_08029518, FUN_0802ac40, FUN_0802fe44 | Yüksek |
| +0x09 | B | **S** | hayır | 6 | 12 | FUN_08029518, FUN_0802ac40, FUN_08030114 | Yüksek |
| +0x0A | B | **S** | hayır | 8 | 10 | FUN_0802a5b4, FUN_08031004, FUN_08031054 | Yüksek |
| +0x0C | W | U | hayır | 9 | 17 | CopySrcToDest, FUN_08029e44, FUN_0802a858 | Yüksek |
| +0x10 | H | **S** | hayır | 5 | 13 | FUN_080296a0, FUN_0802f55c, FUN_08031134 | Yüksek |
| +0x12 | H | **S** | hayır | 5 | 12 | FUN_080296a0, FUN_08030114, FUN_080310c0 | Yüksek |
| +0x14 | W | U | hayır | 10 | 15 | RunMenuScreen, CaptureSessionSnapshot, FUN_08030ae4 | Yüksek |
| +0x18 | H | **S** | hayır | 5 | 10 | FUN_08004f74, FUN_08029e44, FUN_08030a60 | Yüksek |
| +0x1A | H | **S** | hayır | 4 | 8 | FUN_080296a0, FUN_08030114, FUN_08030a9c | Yüksek |
| +0x1C | B (+H) | U | hayır | 10 | 12 | **SetHudTime**, HalvesEqual, FUN_08030dd8 | Yüksek (çelişki: §5.1) |
| +0x1D | B | U | hayır | 9 | 11 | **SetHudTime**, FUN_0802fe44, FUN_0805e700 | Yüksek |
| +0x20 | W | U | hayır | 3 | 6 | FUN_080296a0, FUN_08029e44, FUN_0802ac40 | Orta |
| +0x24 | B | U | hayır | 1 | 2 | FUN_08029e44 | Düşük |
| +0x28 | W | U | hayır | 1 | 1 | FUN_080296a0 | Düşük |
| +0x2C | B | U | hayır | 3 | 5 | FUN_080296a0, FUN_08029e44, FUN_0802a5b4 | Orta |
| +0x2D | B | **S** | hayır | 5 | 7 | FUN_0802f55c, FUN_08031054, FUN_0802a5b4 | Orta |
| +0x2E | B | U | hayır | 3 | 3 | FUN_0802a5b4, FUN_0802f55c, FUN_08031054 | Orta |
| +0x30 | W | U | hayır | 4 | 7 | FUN_08029a88, FUN_08029b3c, FUN_080313f0 | Orta |
| +0x34 | W | U | hayır | 2 | 2 | FUN_080296a0, FUN_0802bdf0 | Düşük |
| +0x38 | H | U | hayır | 2 | 3 | FUN_080296a0, FUN_0802bdf0 | Düşük |
| +0x3A | H | U | hayır | 2 | 3 | FUN_080296a0, FUN_0802bdf0 | Düşük |

### Bölge B — 0x003C–0x111B: **24 girişli, 180 baytlık yuva dizisi** (§3)

Aşağıdaki ofsetler dizinin **0. elemanının** alanlarıdır; hepsine `taban + 180*i + K`
biçiminde register indeksiyle erişiliyor.

| Blok ofseti | Eleman ofseti | Gen. | Dizi? | Fn | Örnek fonksiyonlar | Güven |
|---|---|---|---|---|---|---|
| +0x3C | +0x00 | B | **EVET** | 4 | FUN_080296a0, FUN_0802b394, FUN_0802b484 | Yüksek |
| +0x40 | +0x04 | W | **EVET** | 3 | FUN_080296a0, FUN_0802b394, FUN_0803095c | Yüksek |
| +0x44 | +0x08 | W | **EVET** | 3 | FUN_080296a0, FUN_0802b394, FUN_0803095c | Yüksek |
| +0x48 | +0x0C | W | **EVET** | 2 | FUN_080296a0, FUN_0802b394 | Yüksek |
| +0x4C | +0x10 | W (ptr) | **EVET** | 3 | **ReleaseSlot**, FUN_080296a0, FUN_0802b394 | Yüksek |
| +0x50 | +0x14 | W (+B) | **EVET** | 7 | ReleaseSlot, FUN_0802b4e8, FUN_0803095c | Yüksek (çelişki: §5.2) |
| +0x54 | +0x18 | W | **EVET** | 3 | FUN_080296a0, FUN_0802b394, FUN_0802b4e8 | Yüksek |
| +0x58 | +0x1C | adres alındı | **EVET** | 4 | FUN_080299b0, FUN_0802b484, ReleaseSlotHeld | Orta |
| +0xA0 | +0x64 | adres alındı | **EVET** | 1 | FUN_080299b0 | Düşük |
| +0xE8 | +0xAC | W | **EVET** | 3 | FUN_080296a0, FUN_0802b394, FUN_0802b4e8 | Yüksek |
| +0xEC | +0xB0 | B | **EVET** | 5 | FUN_080296a0, FUN_080299b0, FUN_0802b4e8 | Yüksek |
| +0xED | +0xB1 | B | **EVET** | 3 | FUN_080296a0, FUN_080299b0, FUN_0802b484 | Yüksek |
| +0xEE | +0xB2 | H | **EVET** | 2 | FUN_0802b394, **FUN_08030d0c** | Orta |

### Bölge C — 0x111C–0x114B: 12 ardışık u32

Boşluksuz, tam ardışık on iki sözcük. Hepsi yalnız `ldr`/`str`.

| Ofset | Fn | Erişim | Örnek fonksiyonlar | Güven |
|---|---|---|---|---|
| +0x111C | 2 | 8 | FUN_08029c20, FUN_0802df18 | Yüksek |
| +0x1120 | 2 | 3 | FUN_08029c20, FUN_0802df18 | Orta |
| +0x1124 | 2 | 3 | FUN_0802df18, FUN_0802e9e4 | Orta |
| +0x1128 | 6 | 9 | FUN_0802e048, FUN_0802f55c, **FUN_08031034** | Yüksek |
| +0x112C | 4 | 6 | FUN_0802b01c, FUN_0802ead4, **ResetMapView** | Yüksek |
| +0x1130 | 5 | 5 | FUN_080296a0, FUN_0802e048, **ResetMapView** | Yüksek |
| +0x1134 | 1 | 3 | FUN_0802b01c | Düşük |
| +0x1138 | 2 | 3 | FUN_0802af40, **FUN_08030d4c** | Orta |
| +0x113C | 3 | 4 | FUN_0802af40, FUN_0802e9e4, **FUN_08031204** | Orta |
| +0x1140 | 2 | 2 | FUN_0802e9e4, **FUN_08031204** | Orta |
| +0x1144 | 2 | 4 | FUN_08029d44, FUN_0802af40 | Orta |
| +0x1148 | 3 | 8 | FUN_080296a0, FUN_08029d44, FUN_0802af40 | Yüksek |

### Bölge D — 0x114C–0x134B: 512 baytlık tampon (§3)

Skaler erişim yok. `FUN_08029E44` içinde `taban+0x114C` (= 0x0202695C) **adres olarak**
bir grafik yordamına geçiriliyor. 0x114C–0x134C tam **0x200 = 512 bayt**.

### Bölge E — 0x134C–0x138F: sayaçlar ve bayraklar

| Ofset | Gen. | İşaret | Fn | Erişim | Örnek fonksiyonlar | Güven |
|---|---|---|---|---|---|---|
| +0x134C | W | U | 4 | 6 | FUN_080296a0, FUN_08029918, FUN_08029e44 | Yüksek |
| +0x1358 | W | U | **30** | 32 | **StepThenCheck**, FUN_08030d84, ResetMapView | **Çok yüksek** |
| +0x135C | W | U | 2 | 2 | FUN_080296a0, FUN_0802b01c | Düşük |
| +0x1360 | W | U | **28** | 38 | FUN_08030458, **FUN_08030d4c**, **FUN_08030db4** | **Çok yüksek** |
| +0x1364 | W | U | 7 | 8 | FUN_08029918, FUN_0802e048, **ResetMapView** | Yüksek |
| +0x1368 | W | U | 5 | 6 | FUN_0802df18, FUN_0802ead4, **ResetMapView** | Yüksek |
| +0x136C | W | U | 5 | 5 | FUN_0802b01c, FUN_0802e048, **ResetMapView** | Yüksek |
| +0x1370 | H | U | 1 | 3 | FUN_08029e44 | Düşük |
| +0x1373 | B | U | 2 | 6 | FUN_080296a0, FUN_0802eb50 | Orta |
| +0x1374 | B | U | 2 | 2 | FUN_0802f55c, **FUN_080310c0** | Orta |
| +0x1375 | B | U | 1 | 1 | FUN_08029e44 | Düşük |
| +0x1376 | B | U | 2 | 4 | FUN_080296a0, FUN_08029e44 | Orta |
| +0x1377 | B | U | 2 | 3 | FUN_08029e44, FUN_08032318 | Orta |
| +0x1378 | W | U | 3 | 3 | FUN_080296a0, FUN_0802e3fc, FUN_0802f55c | Orta |
| +0x137C | B | U | 4 | 9 | FUN_0802de70, FUN_0802ead4, **ResetMapView** | Yüksek |
| +0x137D | B | U | 2 | 8 | **BumpStepCounter**, FUN_08029e44 | Yüksek |
| +0x137E | B | U | 2 | 5 | **CleanupAreaTiles**, ShowLevelBadge | Yüksek |
| +0x1380 | H | U | 4 | 6 | FUN_08029c20, FUN_0802af40, FUN_0802df18 | Yüksek |
| +0x1382 | H | U | 2 | 8 | FUN_08029c20, FUN_08029d44 | Orta |
| +0x1384 | H | U | 4 | 6 | FUN_08029c20, FUN_0802af40, FUN_0802df18 | Yüksek |
| +0x1388 | W | U | 1 | 2 | FUN_0802bdf0 | Düşük |
| +0x138C | B | U | 1 | 1 | FUN_0802af40 | Düşük |
| +0x138D | B | U | 1 | 1 | FUN_0802df18 (0x0802DFF4) | Yüksek (boyut kanıtı) |

---

## 3. Dizi ve tampon geometrisi (register indeksli ofsetler)

Görev tanımı `lsls #1` → u16, `lsls #2` → u32 kuralını soruyor. **Bu blokta o desen
hiç yok.** Tek gerçek dizi `muls` ile adreslenen 180 baytlık yuva dizisidir; `lsls #5`
görünen 34 yerin hepsi sabit üretmedir (§0).

### Yuva dizisi: taban+0x3C, adım 180 (0xB4), 24 eleman

Adım ve eleman sayısı doğrudan koddan okunuyor (`ReleaseSlot`, 0x080308AC):

```
80308b0:  2917      cmp  r1, #23          <- indeks üst sınırı 23  => 24 eleman
80308b4:  4d0b      ldr  r5, [pc, #44]    = 0x02025810
80308b6:  20b4      movs r0, #180  @ 0xb4 <- adım 180
80308ba:  4343      muls r3, r0
80308be:  304c      adds r0, #76   @ 0x4c <- eleman + 0x10
80308c0:  181c      adds r4, r3, r0
```

`FUN_080296A0` aynı diziyi `taban + 180*i + {60,64,68,72,76,80,84,88,232,236,237}`
ile geziyor. 236 ve 237, 180'den büyük olduğu için **eleman tabanı 0x4C olamaz**;
tek tutarlı çözüm eleman tabanı = **0x3C**'dir:

- 236 − 60 = 176 = eleman+0xB0 ✓ (180 içinde)
- 237 − 60 = 177 = eleman+0xB1 ✓
- 238 − 60 = 178 = eleman+0xB2, `strh` → eleman+0xB2..0xB3 ⇒ **eleman tam 0xB4 = 180'de kapanıyor** ✓

Dizinin iki ucu da bağımsız olarak kapanıyor — bu, düzenin en güçlü kanıtı:

```
0x3C + 24 × 180 = 0x3C + 0x10E0 = 0x111C
```

ve **+0x111C**, dizi bittikten sonra sabit ofsetle erişilen ilk bayttır. Yani
"+0xEE ile +0x111C arasındaki 4142 baytlık boşluk" bir boşluk değil, dizinin
1..23 numaralı elemanlarıdır.

### 512 baytlık tampon: taban+0x114C

`FUN_08029E44` (0x0802A25C) `0x0202695C` = taban+0x114C'yi havuzdan yükleyip
adres olarak geçiriyor. 0x114C + 0x200 = **0x134C**, ki bu da Bölge E'nin ilk
skaleridir. Bu bölge de iki ucundan kapanıyor.

### Diğer register indeksli erişimler

Yok. Bölge A, C ve E'deki her erişim sabit ofsetlidir.

---

## 4. Önerilen C yapısı

Bilinmeyen her bölge **dolgu**dur; uydurma alan yoktur. Alan adları kasıtlı olarak
nötr (`unkNN`) tutuldu — anlamı bilinen üç alan dışında. Ad vermek ayrı bir iştir;
bu belge yalnız düzeni sabitler.

```c
/* gRam02025810 — 0x02025810, oyuncu ilerleme blogu.
 *
 * OLCULEN BOYUT: 0x1390 = 5008 bayt.
 *   Kanit: FUN_080296A0 @0x080296B8 -> Memset(gRam02025810, 0, 0x1390).
 *   0x00001390 sabiti tum ROM'da yalnizca orada geciyor.
 *   Bagimsiz alt sinir: FUN_0802DF18 @0x0802DFF4 strb -> taban+0x138D.
 *
 * Isaretlilik yalnizca ldrsb/ldrsh goruldugunde iddia edilmistir. "u" yazan
 * alanlar "isaretsiz oldugu kanitlandi" demek DEGIL, "isaretli erisim
 * gorulmedi" demektir.
 */

/* Yuva dizisi elemani — 180 bayt. Adim `ReleaseSlot`ta muls ile olculdu. */
typedef struct ProgressSlot {
/* +0x00 */ u8   unk00;
/* +0x01 */ u8   pad01[3];
/* +0x04 */ u32  unk04;
/* +0x08 */ u32  unk08;
/* +0x0C */ u32  unk0C;
/* +0x10 */ void *held;        /* ReleaseSlot: nesne isaretcisi, +0x0C bayragi temizleniyor */
/* +0x14 */ u32  unk14;        /* CELISKI: str/ldr ile u32, iki yerde ldrb — bkz. §5.2 */
/* +0x18 */ u32  unk18;
/* +0x1C */ u8   unk1C[0x90];  /* +0x1C ve +0x64 adres olarak aliniyor; icerigi BILINMIYOR */
/* +0xAC */ u32  unkAC;
/* +0xB0 */ u8   unkB0;
/* +0xB1 */ u8   unkB1;
/* +0xB2 */ u16  unkB2;
} ProgressSlot;                /* sizeof == 0xB4 == 180 */

typedef struct Progress {
    /* ---- Bolge A: yogun oyuncu durumu ---- */
/* +0x0000 */ s8   unk00;
/* +0x0001 */ u8   pad01;
/* +0x0002 */ s16  unk02;
/* +0x0004 */ s8   unk04;      /* +0x04/+0x05 cifti bazi yerlerde u16 okunuyor — §5.1 */
/* +0x0005 */ s8   unk05;
/* +0x0006 */ s8   unk06;      /* +0x06/+0x07 cifti bazi yerlerde u16 okunuyor — §5.1 */
/* +0x0007 */ s8   unk07;
/* +0x0008 */ s8   unk08;
/* +0x0009 */ s8   unk09;
/* +0x000A */ s8   unk0A;
/* +0x000B */ u8   pad0B;
/* +0x000C */ u32  unk0C;
/* +0x0010 */ s16  unk10;
/* +0x0012 */ s16  unk12;
/* +0x0014 */ u32  cash;       /* menu_screen.c: gorevden cikma ucreti ile karsilastiriliyor */
/* +0x0018 */ s16  unk18;
/* +0x001A */ s16  unk1A;
/* +0x001C */ u8   hudMinutes; /* SetHudTime 0x08030B60: 0..99 kirpiliyor */
/* +0x001D */ u8   hudSeconds; /* SetHudTime 0x08030B60: 0..59 kirpiliyor */
/* +0x001E */ u8   pad1E[2];
/* +0x0020 */ u32  unk20;
/* +0x0024 */ u8   unk24;
/* +0x0025 */ u8   pad25[3];
/* +0x0028 */ u32  unk28;
/* +0x002C */ u8   unk2C;
/* +0x002D */ s8   unk2D;
/* +0x002E */ u8   unk2E;
/* +0x002F */ u8   pad2F;
/* +0x0030 */ u32  unk30;
/* +0x0034 */ u32  unk34;
/* +0x0038 */ u16  unk38;
/* +0x003A */ u16  unk3A;

    /* ---- Bolge B: yuva dizisi, 0x003C..0x111C ---- */
/* +0x003C */ ProgressSlot slots[24];      /* 24 * 180 = 4320 = 0x10E0 */

    /* ---- Bolge C: 12 ardisik u32, 0x111C..0x114C ---- */
/* +0x111C */ u32  unk111C;
/* +0x1120 */ u32  unk1120;
/* +0x1124 */ u32  unk1124;
/* +0x1128 */ u32  unk1128;
/* +0x112C */ u32  unk112C;
/* +0x1130 */ u32  unk1130;
/* +0x1134 */ u32  unk1134;
/* +0x1138 */ u32  unk1138;
/* +0x113C */ u32  unk113C;
/* +0x1140 */ u32  unk1140;
/* +0x1144 */ u32  unk1144;
/* +0x1148 */ u32  unk1148;

    /* ---- Bolge D: 512 baytlik tampon, adres olarak geciriliyor ---- */
/* +0x114C */ u8   buffer114C[0x200];      /* FUN_08029E44 @0x0802A25C */

    /* ---- Bolge E: sayaclar ve bayraklar ---- */
/* +0x134C */ u32  unk134C;
/* +0x1350 */ u8   pad1350[8];             /* DOKUNULMAYAN */
/* +0x1358 */ u32  pending;                /* step_then_check.c; 30 ayri fonksiyon okuyor */
/* +0x135C */ u32  unk135C;
/* +0x1360 */ u32  unk1360;                /* 28 ayri fonksiyon okuyor */
/* +0x1364 */ u32  unk1364;
/* +0x1368 */ u32  unk1368;
/* +0x136C */ u32  unk136C;
/* +0x1370 */ u16  unk1370;
/* +0x1372 */ u8   pad1372;                /* DOKUNULMAYAN */
/* +0x1373 */ u8   unk1373;
/* +0x1374 */ u8   unk1374;
/* +0x1375 */ u8   unk1375;
/* +0x1376 */ u8   unk1376;
/* +0x1377 */ u8   unk1377;
/* +0x1378 */ u32  unk1378;
/* +0x137C */ u8   unk137C;
/* +0x137D */ u8   stepWarnFlag;            /* link_state_step.c / BumpStepCounter */
/* +0x137E */ u8   unk137E;                 /* CleanupAreaTiles */
/* +0x137F */ u8   pad137F;                 /* DOKUNULMAYAN */
/* +0x1380 */ u16  unk1380;
/* +0x1382 */ u16  unk1382;
/* +0x1384 */ u16  unk1384;
/* +0x1386 */ u8   pad1386[2];              /* DOKUNULMAYAN */
/* +0x1388 */ u32  unk1388;
/* +0x138C */ u8   unk138C;
/* +0x138D */ u8   unk138D;                 /* en yuksek dokunulan bayt */
/* +0x138E */ u8   pad138E[2];              /* Memset uzunluguna kadar dolgu */
} Progress;                                 /* sizeof == 0x1390 == 5008 */
```

> **Mevcut kaynaklarla uyum uyarısı:** `src/world/step_then_check.c` ve
> `src/world/area_cleanup.c` şu anda `Progress` adını **kendi yerel, kısa**
> tanımlarıyla kullanıyor (`->pending`, `->pendingCleanup`, `->unk14`,
> `->cash`). Yukarıdaki tam yapı paylaşılan bir başlığa konursa o yerel
> tanımlar çakışır ve kaldırılmalıdır. Ayrıca `area_cleanup.c`'deki
> `pendingCleanup` ile `step_then_check.c`'deki `pending` **aynı ofset mi**
> (ikisi de +0x1358 mi) doğrulanmalı — bu belge ikisini tek alan sayıyor.

---

## 5. Çözemediğim çelişkiler ve belirsizlikler

### 5.1 İki u8'in bir u16 gibi okunması — üç yerde

Bu **gerçek bir çelişki değil, gerçek bir örtüşme**; ama C'de nasıl yazılacağı
insan kararıdır (iki `u8` mi, `union` mu, yoksa `u16` mi).

| Ofset | Bayt erişimi | Yarım söz erişimi |
|---|---|---|
| +0x04 / +0x05 | 17 `strb`, 3 `ldrb`, 3 `ldrsb` (13 fonksiyon) | 3 `ldrh` — `HalvesEqual` (0x08030BEE), FUN_0805EFF0, FUN_080625A0 |
| +0x06 / +0x07 | 4 `strb`, 1 `ldrb`, 3 `ldrsb` | 1 `ldrh` — FUN_0805EE84 (0x0805EF0E) |
| +0x1C / +0x1D | 10 `strb`, 1 `ldrb` (`SetHudTime` ikisine de yazıyor) | 1 `ldrh` — `HalvesEqual` (0x08030BF2) |

`SetHudTime` +0x1C'ye dakikayı, +0x1D'ye saniyeyi ayrı ayrı `strb` ile yazıyor,
dolayısıyla **ayrı iki u8 olduğu kesin**. `HalvesEqual` bu çifti 0xFFFF ile
maskeleyip tek `ldrh` ile karşılaştırıyor — muhtemelen "ikisi de sıfır mı"
testinin derleyici tarafından birleştirilmiş hâli.

Yukarıdaki yapıda **iki ayrı `s8`/`u8` olarak** yazdım, çünkü yazma tarafı
tartışmasız bayt bazlı. Ama `src/world/halves_equal.c` zaten `u16` okuyor;
byte-matching için orada bir `union` gerekebilir. **Bunu insan kararına bırakıyorum.**

### 5.2 `ProgressSlot.unk14` (blok +0x50): u32 mu, u8 mi?

En rahatsız edici çelişki. Aynı ofset:

```
ReleaseSlot   0x080308D8  str  r1, [r0, #0]     <- u32 yaziliyor (sifir)
FUN_0802B394  0x0802B456  str  r2, [r1, #0]     <- u32
FUN_0802B4E8  0x0802B6E8  ldr  r1, [r0, #0]     <- u32
FUN_080299B0  0x08029A10  ldrb r0, [r0, #0]     <- u8 !
FUN_0802B484  0x0802B496  ldrb r5, [r0, #0]     <- u8 !
```

Yazan taraf her zaman u32, okuyan tarafın ikisi u8. İki makul açıklama var ve
**ROM kanıtıyla ayırt edemiyorum**:

- Alan u32; iki yer yalnız düşük baytını bayrak/boolean olarak test ediyor
  (little-endian'da `ldrb` düşük baytı verir, sıfırlık testi için yeterli).
- Alan aslında `u8` + 3 dolgu ve `str` ile sıfırlama yan bayrakları da siliyor.

`u32` yazdım çünkü **yazma** genişliği u32 ve `ReleaseSlot` byte-matching olarak
zaten `u32` ile doğrulanmış. Bunu değiştiren, `ReleaseSlot`'un eşleşmesini
bozmadığını doğrulamalı.

### 5.3 Yuva elemanının +0x1C..0xAB aralığı (144 bayt) tamamen bilinmiyor

Yalnız iki nokta *adres olarak* alınıyor (eleman+0x1C ve eleman+0x64); ne
okunuyor ne yazılıyor, bir fonksiyona geçiriliyor. İçeride ne olduğunu
söyleyemem. Dolgu bıraktım. Eleman+0x64'ün kanıtı **tek** fonksiyondan geliyor
(FUN_080299B0) — düşük güven.

### 5.4 Bölge D'nin gerçekten 512 bayt olduğu kesin değil

`taban+0x114C` adresinin bir grafik yordamına geçtiğini görüyorum; 512 sayısı
**aynı komut dizisindeki `movs r2,#128; lsls r2,#2`** değerinden ve
0x134C'ye kadar hiç skaler erişim olmamasından türetildi. Uzunluğun o çağrının
*bu* tampona ait olduğunu %100 doğrulayamadım (aynı blokta ikinci bir tampon
işaretçisi 0x02026DA0 de var, ki o bu bloğun dışında). Boyut olarak doğru
kapanıyor ama **orta güven**.

### 5.5 Tek fonksiyonla desteklenen alanlar (düşük güven)

+0x24, +0x28, +0x34, +0x38, +0x3A, +0x1134, +0x1370, +0x1375, +0x1388,
+0x138C, +0x138D. Bunların bir kısmı daha büyük bir alanın parçası olabilir
(örneğin +0x38/+0x3A tek bir u32 olabilir; +0x1388 ile +0x138C bitişik bir
yapı olabilir). Ayırt edecek kanıt yok.

### 5.6 Bant sayısı uyuşmazlığı

Görev 0x080308EC–0x08031D24 bandında **24** yazılmamış fonksiyon diyor.
Ben o bantta bloğa erişen **26** fonksiyon buldum; bunların 4'ü zaten
`matching` (`SetHudTime`, `HalvesEqual`, `CleanupAreaTiles`,
`CaptureSessionSnapshot`), yani **22 yazılmamış**. Aradaki fark muhtemelen
bandın alt sınırının 0x080308EC değil 0x080308AC'den (`ReleaseSlot`)
başlaması ya da farklı bir listeden gelmesi. Kimin sayımının doğru olduğunu
belirleyemedim; kararı alan kişi kendi listesiyle karşılaştırmalı.

---

## 6. Tek yapı mı, bitişik duran ayrı bölgeler mi?

**Tek tahsis, dört mantıksal bölge.** Ayrı sembollere bölünmemeli.

Ayrı bölgelere bölmeme gerekçeleri:

1. **Tek `Memset` bloğun tamamını kapsıyor.** `Memset(taban, 0, 0x1390)` 0x0000'dan
   0x138F'e kadar her şeyi tek işlemde sıfırlıyor. Bu, bloğun tek bir nesne olarak
   tahsis edildiğinin doğrudan kanıtıdır. Ayrı nesneler olsaydı (bu depodaki
   `gRam020246F0` ailesinde olduğu gibi) ayrı DMA/memset çağrıları görürdük.
2. **ROM literal havuzunda neredeyse yalnız taban var.** 508 erişimin tamamı taban
   register'ından türetiliyor. Yalnız beş katlanmış sabit var (+0x24, +0x4C, +0x58,
   +0xA0, +0x114C) ve **hepsi ölçülen boyutun içinde** — yani derleyicinin aynı
   nesne içinde ofset katlaması, ayrı nesneye işaretçi değil.
3. **Büyük boşlukların tamamı açıklandı.** İlk bakışta üç dev boşluk var
   (4142, 516 ve 4320 bayt); üçü de dizi/tampon olarak kapandı ve **her biri iki
   ucundan da tam oturuyor**:
   - 0x3C + 24×180 = 0x111C = bir sonraki skaler ✓
   - 0x114C + 0x200 = 0x134C = bir sonraki skaler ✓
   - eleman içi son alan 0xB2+2 = 0xB4 = ölçülen adım ✓

   Rastlantı olamayacak kadar iyi kapanıyor. Blokta **açıklanamayan tek bir
   büyük boşluk kalmadı**; geriye kalan dolgular en fazla 8 bayt.

Bununla birlikte bölgeler **anlamca farklı**: A ve E oyuncu/oturum durumu (yoğun,
küçük, kısmen işaretli alanlar), B bir nesne yuvası tablosu, D bir grafik tamponu.
Bu yüzden yukarıdaki yapıda alt bölgeleri yorumla ayırdım ve B'yi ayrı bir
`ProgressSlot` tipine çıkardım — ama **tek `Progress` yapısı** olarak, tek
sembolle bildirilmeli.

---

## 7. `data/ram_map.csv` için önerilen düzeltme (uygulanmadı)

Mevcut satır boyutu `5006` yazıyor. Önerilen: **`5008`**, gerekçe alanına
Memset kanıtı. Bunu ben değiştirmedim — üç ajan eşzamanlı dosya yazıyor.
