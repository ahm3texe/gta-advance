# Açık sorunlar — devir belgesi

Bu belge, çözemediğim işleri dışarıdan biri devralabilsin diye yazıldı.
Her madde ölçülmüş durumla, kök nedenle ve elenen denemelerle birlikte.
Tahmin yok; her sayı `make c-match` ve `tools/diff_function.py` çıktısıdır.

Tarih: 2026-09-07 · Proje durumu: **%8,61** (455/1934 fonksiyon, 39.100/454.072 bayt)

---

## 0. Bilmen gerekenler

**Amaç.** GTA Advance (Europe) GBA ROM'unun byte-matching decompile'ı. Yazılan C,
`old_agbcc -mthumb-interwork -O2 -fhex-asm` ile derlendiğinde ROM'un baytlarını
**birebir** vermek zorunda. "Davranışı aynı" yetmez.

**Ölçüm.**
```
make c-match FILE=<kaynak.c>                      # tek gerçek ölçüt
python3 tools/diff_function.py <kaynak.c> <Ad>    # ROM ile yan yana komut farkı
python3 tools/disasm_function.py <adres>          # ROM'un kendisi
python3 tools/dump_cfg.py <Ad>                    # temel blok haritası
python3 tools/dump_alloc.py <kaynak.c> --function <Ad>   # agbcc yazmaç dağıtımı
```

**En önemli kural: bayt sayısına bakma, komut dizisine bak.** Bu oturumda iki kez
yanıltıcı çıktı. `ServiceLinkFrame`'de boyutu hiç değiştirmeyen bir düzeltme doğru
biçimi verdi; `sio_driver`'da boyut 2 bayt yakınken gövdenin yarısı yanlıştı.
`diff_function.py`'nin son satırındaki `N/M komut ayni` gerçek skordur.

**Kural kütüphanesi.** `docs/COMPILER.md`, 64 ölçülmüş kural. Aşağıdaki maddeler
45–63 arasına sık atıf yapar. Yeni bir kural bulursan oraya **ölçümüyle** yaz.

**Yasaklar.** `asm(".equ ...")` kullanma — `agbcc_build.py` sembolleri
`data/functions.csv` ve `data/ram_map.csv`'den çözüp `.equ`'yu kendisi üretir.
Sembol eksikse veri tablosuna kaydet. `%` ve `/` yardımcı çağrı üretir; ROM'da o
`bl` yoksa kullanma.

**Kabul ölçütü.** Skoru düşürse bile savunulamaz kaynak kabul edilmez: alakasız
bir değişkeni ikinci bir amaç için kullanmak, u16 sayaç için `unsigned long long`,
yan etkili çağrıyı tekrarlamak, `do{...}while(0)` sarmalayıcı, `bit1 = 2` gibi
uydurma sabit değişkenler. Bu oturumda permuter üç kez böyle aday üretti, üçü de
reddedildi.

---

## 1. `FUN_080657d8` — SIO sürücüsü · yazmaç çakışması giderildi, eşleşme açık

| | |
|---|---|
| adres / boyut | 0x080657D8 · **2374 bayt** |
| kaynak | `src/world/sio_driver.c` |
| güncel durum | **669/1174 komut aynı**, 505 farklı; C boyutu 2352 |
| önceki ölçüm | 575/1174 komut aynı; C boyutu 2356 |

**2026-09-07 devir sonrası sonuç.** TX ikinci yarısındaki r4 çakışması
kaldırıldı. Yeni dağılım ROM gibi `k=r4`, `tx2=r5`, `cur=r6`, tuş halkasının
taban kopyası `r7`. Bunun kalan farkların çoğunu kapatacağı hipotezi
**doğrulanmadı**: kazanç 94 komut; tam byte-matching hâlâ yok.

**Eski kök neden notuna düzeltme.** Başlangıç derlemesinde r4'ü tutan
`gRam020003C0` adresiydi; `gRam02000E80` r3'teydi. `.lreg/.greg` ölçümü:
p1012, L79, 6 referans / 20 komut ömür → r4.

**Korunan çözüm.** `PackLocalLinkTag` iki paralel halkadan ilgili kaydın
baytlarını paketliyor. İşaretçi önce halka tabanına kurulup `entry += index`
ile kayda ilerletiliyor. Böylece taban iki ayrı kısa ömürlü pseudo oluyor
(p1005 ve p1032; her biri 8 referans / 8 komut ömür → r1). İki kullanımda
inline edilen yardımcı yeni BL üretmiyor. Tek adımlı `entry = base + index`
yazımı tekrar 573/1174'e düşüyor. Mekanizma ve ölçüm **COMPILER kural 64**'te.

Salt taban takma adı kullanan tanısal aday 677/1174 verdi; kaynak kabul
ölçütüne uymadığı için alınmadı. Korunan `entry` işaretçisi gerçek kayda
ilerler ve yalnızca o kaydın baytını okumak için kullanılır.

**Davranış.** `switch (gVBlankEnabled)` ile IDLE, READY, LIVE ve SETTLING.
LIVE, eşin önceki/şimdiki pencerelerini 32 girişlik halkalara işler; ACK'ten
ileri, kararlı bant için geri yürür; TX kaydını kurup gönderir. `mode == 2`
ise döner; yerel slot dolmazsa 240 kare sınırına kadar sürer.

**Korunan önceki bulgular.** TX/RX alanlarının yapı üyesi olması alias
sınıfını düzeltiyordu (553→568). TX ikinci yarısının ayrı `tx2` işaretçisi
ve iki pencerenin ayrı yerelleri 568→575 getirmişti; bunlar korunuyor.

**Kalan farklar.** Eski 0x08065834 yığın kopyası artık eşleşiyor. İlk
komut farkı 0x0806584C'de, epilogun konumu değiştiği için iç dalın
hedef adresi farklı. RX adres ilişkisi ROM'da `(blok+sabit)+i*16`, C'de
`(blok+i*16)+sabit`. Pencere ve çıkış bloklarında başka dağıtım/ifade
farkları sürüyor. TX etiket bloğunun iki taban yüklemesi artık ayrı, ancak
ROM'un `r3→r1` ve `r0→r1` adres toplamalarına karşı C r1'i yerinde ilerletiyor.
Eski doğrusal diff'ten alınmış blok eşleşme listesi güncel kanıt sayılmaz.

**Yeniden elenenler.** Yeni 669 tabanında RX alan-adresi üzerinden u16
okuma 654; LinkSlot dizi/cast biçimleri 602. Başlangıç tabanında operand
ters çevirmeleri 567–569; etiket yerelleri en çok 580; indeks yerelleri en
çok 586. `step`'i ikiye bölmek etkisiz. Volatile taramasında ROM'da olmayan
erişimler çıktığı için bu adaylar alınmadı.

**Yeniden üretim:** `python3 tools/probe_sio_tx.py`; asıl kapı
`make c-match FILE=src/world/sio_driver.c`. Probe önceki doğrudan yazımı,
tek adımlı ve iki adımlı yardımcıyı kaynak değiştirmeden karşılaştırır.

---

## 2. `FUN_08067014` — yüzde skoru

| | |
|---|---|
| adres / boyut | 0x08067014 · 368 bayt |
| kaynak | `src/world/link_score.c` |
| durum | **85/183 komut aynı**, 98 farklı |

**Ne yapıyor.** Bağlantı raporundaki eşikleri sayıp
`(100 * (baş + sayı)) / (başAlt + 23)` yüzdesini döndürüyor, 99'da kırpıyor,
eşik zaten aşılmışsa doğrudan 100. Bölme `FUN_0806c0f4` çağrısı olarak yazılmalı.

**Kalan fark.** ROM `report` işaretçisini **`ip`'de (r12)** tutuyor ve 15 erişimin
her birinden önce düşük bir yazmaca kopyalıyor (`mov rN, ip`). Biz doğrudan düşük
yazmaçta tutuyoruz. Ayrıca ROM +0x06 ve +0x07 baytlarını **her kullanımda yeniden
okuyor**, bizim derleme onları bir kez okuyup saklıyor.

**Neden kapanmadı (ölçüldü).** `dump_alloc`: `lap` üçlüsünde `<<` ara sonuçlarını
canlı tutup `lsrs`'i yeniden türetiyoruz — ROM ile aynı. Ama `rank` üçlüsünde CSE
tüm `(x<<k)>>27` ifadesini katlayıp çıkarılmış değeri yeniden kullanıyor ve iki
toplam testini de tek teste indiriyor. Fark **kap genişliği**: `lap` bir `u16`
kapta, HImode dönüşümleri CSE'yi kırıyor; `rank` bir `u32` kapta. `rankB` 13–17.
bitleri kapsadığı için kap **u32 olmak zorunda** (kural 61; ROM `ldr r0,[r1,#32]`
yapıyor). Düşük yazmaçlar boş kalınca `report` r3'te kalıyor ve ROM'un `ip` biçimi
hiç oluşmuyor.

**Elenen.** Kural 55'in yerel-kopya kolunu `queryB` dörtlüsüne uygulamak skoru
**59/183'e düşürüyor** — ROM orada baytları yeniden okuyor, yani doğrudan üye
erişimi şart (kural 55'in tersi doğrulandı). Toplamlardaki `(s32)` cast'lerini
kaldırmak hiçbir şey değiştirmiyor.

**Durum: AÇIK.** Kap genişliği zorunluluğu ile CSE davranışı çelişiyor; kaynak
tarafında bir yol olabilir ama bulunamadı.

---

## 3. `WaitForPartner` — bağlantı bekleme ekranı

| | |
|---|---|
| adres / boyut | 0x0806620C · 320 bayt |
| kaynak | `src/world/input_extra.c` |
| durum | **134/139 komut aynı**, 5 farklı (12 bayt) |

**Kalan fark.** Yalnızca döngü ön-başlığındaki **değişmez taşıma sırası**:
```
0x08066238  ROM: movs r6,#2 / ldr r7,=gBiosIrqFlags / movs r4,#1
            biz: movs r6,#2 / movs r4,#1 / ldr r7,=gBiosIrqFlags
0x080662C4  ROM: ldr r7,=gVBlankEnabled / ldr r6,=gBiosIrqFlags
            biz: tersi (havuz kelimeleri de takas)
```

**Neden kapanmadı (kural 58).** Taşıma sırası kaynaktaki kullanım sırasını izliyor,
ama iki gereksinim **birbirini dışlıyor**:

| yazım | sonuç |
|---|---|
| `if (armed == 0) {B} else {A}` | ROM'un blok yerleşimi doğru, taşıma sırası ters → 12 bayt |
| `if (armed != 0) {A} else {B}` | taşıma sırası doğru, derleyici B'yi döngü üstüne çıkarıp giriş atlaması ekliyor → 316 bayt |

Elle taşıma (`bit1 = 2; irq = &gBiosIrqFlags;`) birinci döngüyü **tam kapatıyor**
(8/320) ve kaynak düzeyi ilklemenin `loop.c` taşımalarından önce üretildiğini
kanıtlıyor — ama ikinci döngüde bir yazmaç boşaltıp sabitin de taşınmasına yol
açıyor ve r8'e taşıyor (336/344). Ayrıca `bit1 = 2` savunulabilir kaynak değil.

**Elenen.** Kural 54 ile `armed`'ı iki yerele bölmek → **17 bayt** (iki döngü de r5
istiyor, tek değişken doğru). İç içe `if` çıkış koşulu → 12, değişmedi.
`gBiosIrqFlags` volatile → **24 bayt**. `gVBlankEnabled` volatile → 12, değişmedi.
Permuter 1222 yinelemede 80 → 20; en iyi aday `VBlankIntrWait`'i iki kez çağırdığı
ve `held` değişkenini alakasız bir amaç için kullandığı için reddedildi.

**Durum: AÇIK ama dar.** Beş komutluk sıralama. Kural 58'in dışlaması aşılabilirse
kapanır.

---

## 4. `PollInput` — tuş okuma

| | |
|---|---|
| adres / boyut | 0x080656F4 · 228 bayt |
| kaynak | `src/world/poll_input.c` |
| durum | **65/112 komut aynı** (4 bayt) |

**Kalan fark.** Omuz maskesi testinde ROM **her iki operandı da** taze yazmaçlara
kopyalıyor:
```
ROM: adds r1,r0,#0 / adds r0,r4,#0 / ands r0,r1
biz: ands r0,r4
```

**Neden kapanmadı — RTL düzeyinde kanıtlandı.** `old_agbcc -da` dökümü:
`combine` sonrası komut `(set (reg 37) (and (reg/v 22) (reg 36)))` ve reg 36
(0x300 sabiti) **`REG_DEAD`** taşıyor. Bu yüzden `regmove`'un `fixup_match_1`
geçişi hedefi ölü sabitin üzerine adlandırıyor ve üç komutu bire indiriyor.
ROM'un biçimi ancak **iki operanddan hiçbiri ölmezse** ya da
`reg_is_remote_constant_p` tetiklenirse (sabit başka bir temel blokta kurulursa)
oluşur. `keys` zaten ölmüyor; geriye kalan tek kol 0x300'ün ikinci bir kullanımı
olurdu ve **ROM'da öyle bir kullanım yok**.

**Kural 59 burada geçmiyor** ve bu kuralın sınırıdır: 0x300 düşük baytta olmadığı
için `u8` yerel maskeyi kırpıyor — anlambilimsel olarak yanlış, ölçüldü 216 bayt.

**Durum: KAPALI.** Kaynak tarafında kol yok, kanıtlandı. Buraya bir daha zaman
harcanmamalı.

---

## 5. `BuildLinkPacket` — paket kurma

| | |
|---|---|
| adres / boyut | 0x0806673C · 92 bayt |
| kaynak | `src/world/link_packet.c` |
| durum | **43/45 komut aynı**, 2 farklı (20 bayt) |

**Kalan fark.** Döngü sonrası ROM `gRam02036338` adresini r4'te **tutuyor**
(`ldr r2,[r4,#0]`), biz havuzdan yeniden yüklüyoruz (`ldr r0,[pc]; ldr r2,[r0,#0]`).

**Neden kapanmadı — ölçüldü.** agbcc aynı adres sabiti için **iki ayrı pseudo**
üretiyor:
```
27  mem[.LC0]  refs 5  ömür 46  öncelik 0,217  -> r4
74  mem[.LC0]  refs 2  ömür  4  öncelik 0,500  -> r0
```
İki `.LC0` pseudo'su **L0** (döngü öncesi) ve **L2** (döngü sonrası) bloklarında.
CSE'nin genişletilmiş temel blok yolu **L1'de kesiliyor** çünkü döngünün geri
kenarı var; L2'nin tek öncülü L1 ve L1'in iki öncülü var. **Hiçbir kaynak ifadesi
L2'nin L0'ın pseudo'sunu yeniden kullanmasını sağlayamaz.**

Bilinen tek kol bir adres yereli (`CommBlock **slot = &gRam02036338;`) — uydurma
değişken sınıfı, reddedildi.

**Elenen.** Sayacı s32/u32, do-while/while/for, indeksli erişim (u32 sayaç 29 → 20
bayt indirdi, gerisi etkisiz); kuyruk bloğunu yerele almak; iki kuyruk atamasının
sırasını değiştirmek (36 bayta kötüleşti); `total`'ı u32 yapmak; döngü öncesi
`LinkPacket *pkt` yakalayıp kuyrukta kullanmak (**32 bayt** — ROM döngüden sonra
`ldr r1,[r2,#28]`'i yeniden okuyor).

**Durum: KAPALI.** Neden kol olmadığı derleyici içi mekanizmayla açıklandı.

---

## 6. `ReceiveLinkPackets` — paket alma

| | |
|---|---|
| adres / boyut | 0x08066798 · 212 bayt |
| kaynak | `src/world/link_receive.c` |
| durum | **58/104 komut aynı** (8 bayt) |

**Kalan fark.** ROM **üç** yüksek yazmaç kullanıyor (r8 = global adresi, r9 = hedef,
sl = −13 sabiti), biz **iki**. Eksik 8 bayt tam olarak o üçüncü yazmacın push/pop
çifti. ROM'un yazmaç dosyası bizimkinin tam olarak bir kaydırılmışı:
```
ROM: r4=payload  r5=slot  r6=i  r7=i+1  r8=addr  r9=dest  sl=-13
```

**Neden kapanmadı — ölçüldü.** Bizim `slot` allocno'su (p30, refs 6, ömür 14,
öncelik 0,857) **hiç çağrı geçmiyor**, bu yüzden `find_reg` ona en düşük boş
yazmacı veriyor (r1). ROM'da `slot` callee-saved r5'te. `slot`'u ikinci `CpuSet`'e
kadar canlı tutan her yazım **GCSE tarafından bozuluyor** — ortak `slot + 4`
ifadesini `if` dışına, r4'e çıkarıyor ve orada `slot`'u öldürüyor.

**Tuzak (dosyada kayıtlı).** Son iki deyimi `block->` üzerinden yazmak ROM'un
`push {r5,r6,r7}`'sini **üretiyor**, ama boyut 200'e düşüyor çünkü ROM orada global
adresi üçüncü kez yeniden okuyor. Yani `gRam02036338->` doğru biçim; aranan baskı o
değil.

**Elenen.** Sayaçları u32 yapmak iç döngüyü düzeltti (29 → 8 bayt). Payload'ı
değişken/ifade olarak yazmak, `if` içinde yerele almak — etkisiz. Permuter 1519
yinelemede 1095 → 195; en iyi aday `total`'ı hem dış döngü sınırı hem iç toplam
olarak kullandığı için (**semantik olarak yanlış**) reddedildi.

**Durum: AÇIK ama dar.** Tek bir dağıtım gerçeğine indirgendi.

---

## 7. `ServiceLinkFrame` — SIO kare servisi

| | |
|---|---|
| adres / boyut | 0x0806686C · 152 bayt |
| kaynak | `src/world/link_service.c` |
| durum | **51/75 komut aynı** (8 bayt) |

**Kalan fark.** ROM global adresini **r4'te** (callee-saved) tutuyor, biz r3'te.
Eksik 8 bayt o push/pop çifti ve hizalaması.

**Bu turda yapılan gerçek düzeltme.** BIOS bayrağını
`*(volatile u16 *)&gBiosIrqFlags` ile yazmak **bayt sayısını değiştirmiyor** ama
`else` dalını ROM ile komut komut aynı yapıyor: volatile olmayan biçim sabiti
bellekten önce yüklüyor (`mov r0,#0x80 / ldrh r3,[r1] / orr`), volatile ROM'un
sırasını veriyor (`ldrh r0,[r2] / mov r1,#128 / orrs`) ve `REG_IME`'yi de ROM gibi
r3'e taşıyor. **Önceki not "değişiklik yok" diyordu — o yargı yalnızca bayt
sayısına bakıyordu ve yanlıştı.**

**Elenen.** İki tampon takasını iç içe geçirmek → 19/152; `ready = 0`'ı takaslardan
önce yapmak → 39/152. İkisi de 152 bayta **ulaşıyor** ama komutları ROM'dan
uzaklaştırıp `block`'u r2'den r3'e kaydırıyor, yani yakınsayamaz. Alınmadı.

**Durum: AÇIK ama dar.**

---

## 8. Yapısal borçlar

**Adlandırma.** 100 eşleşmiş fonksiyon hâlâ `FUN_` yer tutucu adı taşıyor.
`tools/check_consistency.py` her koşuda sayıyı basıyor. `tools/rename_symbol.py`
adı tablolarda, kaynaklarda, başlıklarda ve belgelerde tek işlemde değiştiriyor —
elle sed yapma, üç kez build kırdı.

**RAM haritası.** 231 sembolün büyük kısmı hâlâ `provisional`. Boyutların çoğu
tahmin. Ölçülmüş olanlar `Memset`/`CpuSet` çağrılarından gelenlerdir; notlarında
"OLCULDU" yazar. Bir sembolün boyutunu düzeltmeden önce ROM'un literal havuzuna
bak: taban + ofset mi yükleniyor, yoksa mutlak adres mi? Bu ayrım
`gRam02025810`'da beni yanılttı (8 bayt sanmıştım, en az 0x137E çıktı).

**ARM bölgesi.** 14.920 bayt. `agbcc_arm` 8 yazmaçta tıkanıyor, bu yapılandırmayla
kapalı. Yeniden denenmemeli.

---

## 9. Nereye bakılmalı (öneri)

Kalan işin dağılımı:

| bant | fonksiyon | bayt | ROM payı |
|---|---|---|---|
| < 120 bayt | ~740 | ~44.000 | %9,7 |
| 120–560 bayt | ~550 | ~142.000 | %31,3 |
| ≥ 560 bayt | ~185 | ~228.000 | **%50,2** |

Yukarıdaki 4 ve 5 numaralı maddeler **kapalı**, zaman harcanmamalı.
3, 6, 7 dar ama hepsi aynı sınıfta: agbcc'nin yazmaç dağıtımı, kaynak kolu belirsiz.

**1 numaralı maddede** (SIO sürücüsü, 2.374 bayt) blok 79 r4 çakışması
giderildi; 505 komut farkı sürüyor. Sonraki çalışma RX adres ilişkisi ve
pencere/çıkış bloklarını güncel diff üzerinden ayırmalı. 120–560 bandının kardeş
olmayan kısmında isabet oranı çok daha yüksek — bu oturumda oradan yazılan
fonksiyonların çoğu ilk denemede tuttu.
