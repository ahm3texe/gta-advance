# Derleyici kimliği: agbcc

## Sonuç

GTA Advance (Avrupa), **`old_agbcc`** ile derlenmiştir — Nintendo'nun resmî GBA
SDK'sıyla dağıttığı GCC 2.8.1 tabanlı derleyicinin *eski* varyantı. Aynı
derleyici ailesi pokeruby ve pokeemerald decomp'larında da kullanılıyor.

Bunun anlamı: **C'den byte-matching üretmek mümkün.** Proje semantik yeniden
inşaya mecbur değil.

## Kanıt 1 — kod kalıpları

Derleyiciler aynı işi yapmanın birden fazla yolundan hep aynısını seçer.
İlk 42 doğrulanmış fonksiyonda (o aşamada 2874 satır assembly):

| Kalıp | agbcc | modern GCC/Clang | ROM'da |
|---|---|---|---|
| Register kopyalama | `adds rX, rY, #0` | `movs rX, rY` | **84 / 0** |
| Fonksiyondan dönüş | `pop {rN}` + `bx rN` | `pop {..., pc}` | **28 / 0** |

Dönüş kalıbının sebebi: ARMv4T'de `pop {pc}` Thumb/ARM modu geçişi yapmaz,
`bx` yapar. Eski derleyiciler her zaman güvenli uzun yolu kullanırdı.

## Kanıt 2 — byte düzeyinde doğrulama

`src/save/save_helpers.c` içindeki C, agbcc ile derlenip ROM'la karşılaştırıldı:

```
ReadU8        4 byte   BYTE-MATCHING  (0x080010F8)
ReadU16LE    12 byte   BYTE-MATCHING  (0x080010FC)
ReadU32LE    24 byte   BYTE-MATCHING  (0x08001108)
WriteU8       4 byte   BYTE-MATCHING  (0x08001120)
WriteU16LE    8 byte   acik           (0x08001124)
WriteU32LE   28 byte   BYTE-MATCHING  (0x08001130)
```

**5/6 fonksiyon, tek satır C değişikliği olmadan.**

`WriteU32LE` belirleyici olan: 28 byte'ın tamamı birebir, üstelik maskeyi
literal havuzdan okumak yerine iki kez `mov #0xff` + `lsl` ile yeniden kurma
gibi ayırt edici bir tercihle. Yanlış derleyici bunu üretemez.

## Kanıt 3 — derleyici varyantı ayrımı

Aynı C kaynağı, altı derleyici/optimizasyon kombinasyonuyla denendi:

| Derleyici | `-O2` | `-O1` | `-O0` |
|---|---|---|---|
| `agbcc` | 3/6 | 3/6 | 0/6 |
| **`old_agbcc`** | **5/6** | 5/6 | 0/6 |

`old_agbcc -O2`, C'yi hiç değiştirmeden `ReadU16LE` ve `ReadU32LE`'yi de
tutturuyor. `agbcc`'nin bu ikisinde ürettiği fazladan işaretçi kopyası farkı,
iki varyant arasındaki register dağıtımı değişikliğinden geliyor.

`-O0` her ikisinde de sıfır veriyor (yığın çerçevesi ekliyor), yani ROM
optimize edilmiş derlenmiş.

## Bayraklar

```
old_agbcc -mthumb-interwork -O2 -fhex-asm
```

Bazı çeviri birimleri farklı derleyici veya seviye kullanıyor olabilir;
`make c-match FILE=... --cc=agbcc` ile diğer varyant denenebilir.

## C yazım kuralları (ölçülerek bulundu)

Her biri en az bir fonksiyonu eşleşmeden eşleşir hâle getirdi:

| # | Kural | Neden |
|---|---|---|
| 1 | **RAM adresleri `extern` sembol olmalı**, `#define ((T*)0xADDR)` değil | Sabit olunca agbcc `taban+ofset`'i ayrı literale katlıyor; ROM tabanı register'da tutuyor |
| 2 | **Ara işaretçi değişkeni kullanma**, doğrudan `dizi[i].alan` yaz | `p = &dizi[i]; p->alan` farklı register dağıtımı üretiyor |
| 3 | **Yığındaki geçici tampon `volatile` olmalı** | Değilse agbcc adres alma ile sabit yüklemeyi yeniden sıralıyor |
| 4 | **`volatile` her erişim için ayrı denenir** | Sıralama düğmesidir, semantik değil: `gBiosIrqFlags` için kaldırmak, `REG_IF` için eklemek gerekti — aynı `x \|= sabit` biçiminde |
| 5 | Dış semboller `.equ` ile assembler'a verilir | Linker'a bırakılınca interworking veneer'i sokuluyor |
| 6 | Bölüm adresi link betiğinde sabitlenir (`SUBALIGN(1)`) | agbcc `.text`'i 8'e hizalıyor, taban 8'in katı değilse her ölçüm kayıyor |
| 7 | Üretilen assembly'nin sonuna `.align 2, 0` eklenir | `as` Thumb bölümünü NOP ile dolduruyor, ROM sıfırla |
| 8 | Dizi temizleme döngüsü **ileriye** yazılır (`i = 0; i < N; i++`) | agbcc bunu geriye giden işaretçi yürüyüşüne çeviriyor; ROM'daki biçim odur. Elle geriye yazmak farklı kod üretir |
| 9 | Döngü indeksi **işaretli** (`int`) olmalı | İşaretçi karşılaştırması işaretsiz dal (`bcs`) üretir; ROM işaretli (`bge`) kullanıyor |
| 10 | Zincirleme atama (`a = b = c`) ayrı satırlardan farklı kod üretir | ROM'daki biçim zincirleme; ayrı yazınca adres hesabı ters sıraya geçiyor |
| 11 | Çağrılar boyunca yaşayan adres **başta yerel değişkene** alınır | ROM onu callee-saved register'da tutuyor; kullanıldığı yerde okunursa derleyici hoist etmiyor |
| 12 | Kaydet/geri-yükle çifti olan register (`REG_IME`) `volatile` olmalı | Değilse derleyici iki kritik bölümün kaydetmelerini birleştiriyor |
| 13 | Uzunluk/boyut parametreleri **işaretsiz** olabilir | `save_slots`'ta tek fark `ble` ↔ `bls` idi: `length` `u32` olunca eşleşti |
| 14 | Tekrar eden byte kopyaları **açık yazılır**, döngüye sarılmaz | `-funroll-loops` ROM'unkinden farklı kod üretiyor (136B/127 fark → 256B/239); sekiz kopya elle yazılınca tam eşleşme |
| 15 | Dar parametrenin **işaretliliği** giriş normalizasyonunu belirler | `s16` parametre `lsls #16`/`lsrs #16` çifti üretir, `u16` üretmez. `WriteU16LE` bunun kanıtı |
| 16 | Kural 11'de **atama yeri** önemli, bildirim yeri değil | `payload = &g...` başta ilklendirilirse ömür fazla uzuyor ve dağıtım kayıyor (220/240 fark); atama kullanımın hemen önüne alınınca eşleşiyor |
| 17 | Tek adres için **her biri tek kullanımlı iki ayrı yerel** gerekebilir | Tek işaretçiyi iki yerde kullanmak agbcc'ye hesabı fonksiyon başına kaldırtıyor; ROM'daki `adds r3, r4, #0` ancak iki değişkenle çıkıyor |
| 18 | Argümanı **ayrı deyimde** yerele okumak argüman kurma sırasını çevirir | `f(480, gSlotCount)` sabiti önce kuruyor; `count = gSlotCount;` ara satırı ROM'un sırasını veriyor |
| 19 | Döngü öncesi atamaların **kaynak sırası** korunur | agbcc kaynak atamalarını kaynak sırasında, kendi ürettiği sayaç ilklendirmelerini döngü başına bitişik yayıyor; sırayı değiştirmek 13-14 bayt fark bırakıyor |
| 20 | 4 bayt hizalı ama halfword yazılan yığın yuvası için **`u16 x[2]` dizisi** | Dizi BLKmode olduğu için bildirim sırasında ve 4 bayta hizalı yerleşiyor; `x[0]=0` yine `strh` üretiyor, `(u32)x` adresi tek komutta veriyor. Skaler `u16` çerçeveyi 12 bayta düşürüyor, `u32` yazımı word yapıyor |
| 21 | Döngü içinde kullanılan **sabit atamaları döngünün içine** yazılır | Döngü önüne yazılırsa agbcc onu kaynak deyimi olarak preheader kopyalarından *önce* yayıyor; içine alınınca döngü-değişmezi taşıyıcısı preheader'ın sonuna koyuyor ve sıra ROM'unkine oturuyor |
| 22 | Aynı tabanın kopyası değil, **sabitten yeniden atama** yazılır | `b = a;` yazılırsa agbcc iki değişkeni birleştirip tek işaretçiye dönüyor; `b = (T *)ADRES;` ayrı ömür veriyor |
| 23 | Dizi elemanının üyesine erişirken **yerel işaretçi** kullanılır | `dizi[i].alan` yazılırsa agbcc alan ofsetini taban literaline katlıyor; `p = &dizi[i]; p->alan` ofseti yükleme komutunda bırakıyor (ROM'daki biçim) |
| 24 | Yedi bitlik alan için **bitfield** yazılır, maske değil | `x & 0x7F` yerine `u8 f : 7` — ROM `lsls #25`/`lsrs #25` çifti üretiyor |
| 25 | Genel değişken okuması, kullanıldığı yerde değil **ayrı deyimde** yapılabilir | `if (g[26] != 0)` ile `v = g[26]; if (v != 0)` farklı sıralama üretiyor |
| 26 | Dar **struct alanının** işaretliliği maskenin genişliğini belirler | `s8 flags` ile `flags &= ~4` maskeyi 32 bit tutuyor (`movs #5`/`negs`); `u8` ile bayta daraltıyor (`movs #251`). Kural 15'in alan hâli. İkinci ölçüm: `InitActor`'da 0x8A/0xA8 alanlarını `s8` yapmak farkı 40 → 14 bayta indirdi |
| 27 | Ham değer ve türevi **tek değişkende** tutulabilir | `index = id; index = (u16)(index - 1);` ayrı iki değişkenden farklı register dağıtımı veriyor |
| 30 | Seyrek `case` değerleri geniş bir aralığa yayılıyorsa **`||` karşılaştırma zinciri** yazılır, `switch` değil | `switch` (case 21..57 arasında 6 değer) agbcc'ye 37 girişli atlama tablosu ürettiriyor: 64 bayt yerine 212. ROM'un `cmp`/`beq` zinciri ancak `if (k == 57 \|\| k == 25 \|\| ...)` ile çıkıyor, **kaynak sırası ROM'un karşılaştırma sırasıyla aynı olmalı**. Kural 19'un (yoğun `switch` → atlama tablosu) tersi |
| 31 | Döngü sayacının işaretliliği `bls` (unsigned) vs `ble` (signed) dallanma seçimini belirler | `for (u32 i = 0; i <= N; i++)` → `bls`; `for (s32 i = 0; i <= N; i++)` → `ble`. ROM her ikisini de kullanır — hangisinin çıktığını sayaç tipi belirler. `slot_scan.c`'de tek başına 1 bayt farkı 0'a indirdi. `MaybeAdvance`'in park nedeni buydu; oradaki `u16 counter` yerine `int counter` denenebilir |
| 29 | İki dal aynı işi yapıyorsa **erken `return` + ortak kuyruk** yazılır, ortak değişkene atama değil | `if (k) { p->h = A; return; } ... p->h = B;` ROM'daki gibi iki ayrı kopya üretiyor; `handler = A else B; p->h = handler;` agbcc'ye dalları birleştirtiyor (cross-jumping) ve 39 bayt fark veriyor |
| 28 | Ölçekli tabana iki terim eklenirken **işaretçi aritmetiği** ile **dizi indeksi** farklı kod üretir | `*(t + x + (y << s))` her terimi ayrı ölçekliyor (`lsl` + `lsl` + iki toplama); `t[x + (y << s)]` önce toplayıp bir kez ölçekliyor. `IsTileTypeInRange`'de dizi biçimi 33 bayt fark **ve** gereksiz bir `push {r4,lr}` veriyordu, işaretçi biçimi 3'e indirip fonksiyonu yaprak yaptı |

## Register dağıtımının mekanizması

Kuralların çoğu (11, 16, 17, 20, 23, 27) aynı tek mekanizmanın yüzleridir.
agbcc'nin (GCC 2.8.1) global register dağıtıcısı sanal register'ları şu
önceliğe göre sıralayıp sırayla ilk uygun donanım register'ını verir:

```
öncelik = floor_log2(referans_sayısı) × referans_sayısı / ömür_uzunluğu
```

Yani bir değişkene **referans eklemek veya çıkarmak**, onun hangi register'a
düştüğünü ve dolayısıyla *tüm* dağıtımı çevirebilir. `ClearTextArea`'da
ölçülen:

| Değişken | referans / ömür | öncelik |
|---|---|---|
| `dma` | 9 / 52 | 5192 |
| `control` | 5 / 21 | 4761 |

Bu sırayla `dma` önce dağıtılıp `r3`'ü kapıyor, ROM'unkinin tersi. Blok 2'deki
ölü okuma `control` değişkenine atanınca `control` 6 referansa çıkıyor
(6/22 → 5454 > 5192), önce dağıtılıyor ve `r3`'ü alıyor — `dma` `r4`'e,
`dest` `r5`'e, IME tabanı `r6`'ya, stride `r7`'ye oturuyor: **ROM'un tam
dağılımı.** 13 bayt fark 1'e iniyor.

Bu, kural 17'nin ("tek adres için iki ayrı yerel") *neden* çalıştığının da
cevabıdır: ikinci yerel ömrü bölüp öncelikleri değiştirir.

**Pratik sonuç:** register uyuşmazlığında C'yi rastgele kurcalamak yerine
ilgili değişkenlerin referans sayısını ve ömrünü say; hangisinin önce
dağıtılması gerektiğini hesapla; referans ekleyip çıkararak sırayı çevir.

Bir uyarı: fonksiyonun tamamı tek bir genişletilmiş temel blokken **bedava
referans eklenemez**. Her reg-reg kopyası CSE tarafından yayılıp combine
tarafından siliniyor; `x = x`, ölü `x = 0`, `x |= 0`, `x + 0`, `x ^ x` hepsi
eleniyor. Referans kazandıran tek şey `volatile` bir erişimdir — o da bir
operand register'ını değiştirir.

Ölçüm için `old_agbcc -dg` (global) ve `-dl` (yerel) dağıtım dökümü üretir;
bu depodaki çağrımda dosyalar boş çıktı, formül dolaylı ölçümle doğrulandı.

Kural 20'nin arkasındaki mekanizma genellenebilir: **yığın yerleşimini belirleyen
şey bildirim sırası değil, tipin BLKmode olup olmadığıdır.** Bir agent 24
bildirim sırası permütasyonu deneyip yerleşimin hiç değişmediğini ölçtü.

**Kuralların birbirine bağlı olduğunu unutma.** 18. kural tek başına
denendiğinde hiçbir şeyi değiştirmiyordu; ancak 16 ve 17 uygulandıktan sonra
belirleyici oldu — çünkü sıra farkı bağımsız bir düğme değil, register
baskısının sonucuydu. Bu yüzden "denendi, tutmadı" kaydı tek başına
değerlendirilmemeli: etkisiz çıkan bir değişiklik, başka bir değişiklikle
birleştiğinde işe yarayabilir.

`volatile` agbcc'de bir **komut sıralama düğmesidir**, semantik bir işaret
değil. Aynı `x |= sabit` deyimi için `gBiosIrqFlags`'te kaldırmak,
`REG_IF`'te eklemek gerekti. Ezberlenmez — her erişim için iki yönü de dene.

Aynı şey adres biçimi için de geçerli: **kural 1 evrensel değildir.** RAM
sembolleri `extern` olmalı, ama agbcc'nin kaydırmayla üretebildiği adresler
(`0x03000000` = `0xc0 << 18` gibi) ROM'da sabit cast olarak yazılmış. Adres
ROM'da literal havuzdan mı okunuyor yoksa hesaplanıyor mu — diff bunu söyler.

## RAM adresleri extern sembol olmalı — en önemli kural

`EraseSaveSlot` ve `GetSaveSlotHeader` uzun süre eşleşmedi. Sebebin derleyici
sürümü olduğu sanıldı; **değildi.** Gerçek sebep C tarafındaydı:

```c
#define gSaveSlotHeaders ((SaveSlotHeader *)0x02000460)   /* YANLIS */
extern SaveSlotHeader gSaveSlotHeaders[3];                /* DOGRU  */
```

Adres bir derleme-zamanı sabiti olduğunda agbcc onu katlıyor: `base + 16`
ifadesini ayrı bir literal (`0x02000EE0`) hâline getiriyor ve tabanı register'da
tutmuyor. ROM ise tabanı bir kez yükleyip register'da saklıyor. Adres extern
sembol olunca derleyici katlayamıyor ve ROM'un ürettiği kodu üretiyor.

Bu tek değişiklikle `EraseSaveSlot` anında eşleşti; `GetSaveSlotHeader` ise
doğrudan üye erişimine geçirilince eşleşti:

```c
if (gSaveSlotHeaders[slot].marker == 0)   /* ara isaretci degiskeni degil */
    return 0;
return &gSaveSlotHeaders[slot];
```

**Kural: her RAM adresi `data/ram_map.csv`'ye yazılır ve C'de `extern` olarak
bildirilir.** `tools/agbcc_build.py` sembolü oradan çözer.

## Ölçülen ama etkisiz çıkanlar

Yukarıdaki sebep bulunmadan önce iki hipotez sonuna kadar test edildi. İkisi de
etkisiz çıktı; kayıt olarak duruyorlar ki tekrar denenmesin:

**Bayrak taraması** — 15 aday bayrak, iki derleyici üzerinde (`-fforce-addr`,
`-fforce-mem`, `-fno-strength-reduce`, `-fomit-frame-pointer`, `-fno-peephole`,
`-fcaller-saves`, `-fno-cse-follow-jumps`, `-fno-expensive-optimizations`,
`-fno-defer-pop`, `-fno-function-cse` ve diğerleri). Hiçbiri tek bayt
değiştirmedi.

**Derleyici sürümü** — pret/agbcc'nin `release` etiketi ayrıca derlendi.
İkilileri `master`'dan farklı ama çıktısı birebir aynı.

Yani sorun hiçbir zaman derleyicide değildi. Bu, negatif sonuçların "yol
kapalı" diye okunmasının nasıl yanıltabileceğinin örneğidir: asıl değişken
başka yerdeydi.

## Diğer iki tuzak

**Bölüm hizalaması.** agbcc `.text`'i 8'e hizalıyor. Taban adres 8'in katı
değilse (`0x08001094` gibi) linker bölümü ileri itiyor ve *önceden eşleşen
fonksiyonlar dahil* her ölçüm kayıyor. Link betiğinde bölüm adresi açıkça
sabitlenir (`SUBALIGN(1)`).

**Bölüm sonu dolgusu.** `as` Thumb bölümlerini NOP (`0x46C0`) ile doldurur,
ROM ise sıfırla. Üretilen assembly'nin sonuna `.align 2, 0` eklenir.

**Dış semboller `.equ` ile verilir, linker'a bırakılmaz.** Linker mutlak
sembolü Thumb fonksiyonu olarak tanımadığı için araya interworking veneer'i
sokar ve `bl` hedefi yanlış çıkar.

## Açık kalan

`WriteU16LE` (`0x08001124`, 12 byte): ROM girişte değeri 16 bite normalize
ediyor (`lsls #16` / `lsrs #16`), ürettiğimiz kod bu dört baytı atlıyor ve
kalan sekiz bayt birebir aynı çıkıyor.

En yakın gelen biçim `int` yerel değişken: kırpmayı üretiyor ama kaydırmayı
işaretli yapıyor (`asrs` yerine `lsrs` gerekiyor) — tek yarım-sözcük fark.
İşaretsiz cast eklenince kırpma tamamen kayboluyor.

Denenip tutmayanlar: `u32`/`int` parametre ve yerel değişken kombinasyonları,
`0xffff` maskesi, `(u16)`/`(u8)`/`(u32)` cast'ları, `v >>= 8`, `v / 256`,
`v & 255`, `i < 2` döngüsü, `*p++` yazımı, K&R parametre bildirimi,
`-traditional`, `-W`, `-funsigned-bitfields`, `-fshort-enums`,
`-mno-thumb-interwork`.


## Kurulum

İkili dosyalar depoya girmez (8.8 MB). Yerelde üretmek için:

```sh
make agbcc
```

`tools/setup_agbcc.sh`, pret/agbcc kaynağını çeker ve derler. agbcc 1998
dönemi C kaynağı olduğu için modern clang'in varsayılanlarıyla derlenmiyor;
betik gerekli uyumluluk bayraklarını taşıyan bir sarmalayıcı kuruyor.

## Doğrulama döngüsü

```sh
make c-match FILE=src/save/save_helpers.c
```

Her fonksiyonu ayrı ayrı derleyip `data/functions.csv`'deki adresinden ROM ile
karşılaştırır. Eşleşen fonksiyonun assembly karşılığı artık gereksizdir.
