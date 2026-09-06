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
WriteU16LE   12 byte   BYTE-MATCHING  (0x08001124)
WriteU32LE   28 byte   BYTE-MATCHING  (0x08001130)
```

**Sekiz fonksiyonun tamamı byte-matching** (`WriteU16LE` sonradan
çözüldü; aşağıdaki "Açık kalan" bölümü kaldırıldı).

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
| 2 | Dizi elemanının üyesine erişimde **iki biçim de farklı kod üretir**; ROM'a bakıp seçilir | `dizi[i].alan` agbcc'ye alan ofsetini taban literaline katlatabilir (`.word taban+0x3c`); `p = &dizi[i]; p->alan` ofseti yükleme komutunda bırakır. Hangisinin doğru olduğu fonksiyona göre değişir — `entity_flags.c` ikinciyi, başka ölçümler birinciyi gerektirdi. Kural 23 bu kuralın ikinci yarısıydı, birleştirildi |
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
| 24 | Yedi bitlik alan için **bitfield** yazılır, maske değil | `x & 0x7F` yerine `u8 f : 7` — ROM `lsls #25`/`lsrs #25` çifti üretiyor |
| 25 | Genel değişken okuması, kullanıldığı yerde değil **ayrı deyimde** yapılabilir | `if (g[26] != 0)` ile `v = g[26]; if (v != 0)` farklı sıralama üretiyor |
| 26 | Dar **struct alanının** işaretliliği maskenin genişliğini belirler | `s8 flags` ile `flags &= ~4` maskeyi 32 bit tutuyor (`movs #5`/`negs`); `u8` ile bayta daraltıyor (`movs #251`). Kural 15'in alan hâli. İkinci ölçüm: `InitActor`'da 0x8A/0xA8 alanlarını `s8` yapmak farkı 40 → 14 bayta indirdi |
| 27 | Ham değer ve türevi **tek değişkende** tutulabilir | `index = id; index = (u16)(index - 1);` ayrı iki değişkenden farklı register dağıtımı veriyor |
| 30 | Seyrek `case` değerleri geniş bir aralığa yayılıyorsa **`||` karşılaştırma zinciri** yazılır, `switch` değil | `switch` (case 21..57 arasında 6 değer) agbcc'ye 37 girişli atlama tablosu ürettiriyor: 64 bayt yerine 212. ROM'un `cmp`/`beq` zinciri ancak `if (k == 57 \|\| k == 25 \|\| ...)` ile çıkıyor, **kaynak sırası ROM'un karşılaştırma sırasıyla aynı olmalı**. Kural 19'un (yoğun `switch` → atlama tablosu) tersi |
| 32 | Ardışık kelime kopyası için **struct atama** yazılır, `*dest++ = *src++;` değil | agbcc `*p++ = *q++;` üçlüsünü üç ayrı `ldr/str` üretir (12 komut). `typedef struct { u32 a,b,c; } Triple;` bildirip `*(Triple*)dest = *(Triple*)src;` yazmak `ldmia/stmia {r0,r1,r2}` çiftini tetikler (2 komut). `0x08031FB4`'te 54 → 27 bayt fark (ölçüm yapıldı; dosya sonradan parktan çıkarılıp silindi). `memcpy` çağrısı yaptırmaz — struct atama gerekiyor |
| 31 | Döngü sayacının işaretliliği `bls` (unsigned) vs `ble` (signed) dallanma seçimini belirler | `for (u32 i = 0; i <= N; i++)` → `bls`; `for (s32 i = 0; i <= N; i++)` → `ble`. ROM her ikisini de kullanır — hangisinin çıktığını sayaç tipi belirler. `slot_scan.c`'de tek başına 1 bayt farkı 0'a indirdi. `MaybeAdvance`'in park nedeni buydu; oradaki `u16 counter` yerine `int counter` denenebilir |
| 29 | İki dal aynı işi yapıyorsa **erken `return` + ortak kuyruk** yazılır, ortak değişkene atama değil | `if (k) { p->h = A; return; } ... p->h = B;` ROM'daki gibi iki ayrı kopya üretiyor; `handler = A else B; p->h = handler;` agbcc'ye dalları birleştirtiyor (cross-jumping) ve 39 bayt fark veriyor |
| 28 | Ölçekli tabana iki terim eklenirken **işaretçi aritmetiği** ile **dizi indeksi** farklı kod üretir | `*(t + x + (y << s))` her terimi ayrı ölçekliyor (`lsl` + `lsl` + iki toplama); `t[x + (y << s)]` önce toplayıp bir kez ölçekliyor. `IsTileTypeInRange`'de dizi biçimi 33 bayt fark **ve** gereksiz bir `push {r4,lr}` veriyordu, işaretçi biçimi 3'e indirip fonksiyonu yaprak yaptı |
| 33 | Değişkeni sabitle maskelemek için sabiti **ayrı sonuç yereline** koyup yerinde `&=` kullan | `return (flags & 3) << 8` sonucu `flags` register'ında tutarken; `mask = 3; mask &= flags; return mask << 8` sonucu sabitin register'ında tutar. `QueryEntity` 2 bayt farktan birebir eşleşmeye geçti |
| 34 | Aynı sıfır dönüşüne giden null kontrollerini gerekirse **açık erken dönüşler** olarak yaz | İç içe `if` eşdeğer semantiğe rağmen ortak sıfır bloğunu değer bloğundan sonra kurdu. `if (!p) return 0;` zinciri `ProbeObject` ve `GetInnerId`de ROM blok sırasını ve literal havuzu yerleşimini üretti |
| 35 | Çağrıdan sonra `pop {r0}; bx r0` varsa sarmalayıcının dönüş tipi büyük olasılıkla **`void`** | `u32` dönüşte r0 canlı kaldığı için agbcc dönüş adresini r1'e alır. `CallWithOffset` imzasını `void` yapmak 10 baytlık register/epilog farkını tamamen kapattı |
| 36 | Bellek adresi sabit yazımdan önce kurulacaksa hedef alanın **işaretçisini önce ayrı yerele al** | `tail = &actor->unk90; i = 0; *tail = i` sırası agbcc'ye önce adresi, sonra sıfır sabitini kurdurdu. `InitActor`ın son komut-sırası farkını kapattı |
| 37 | Aynı tabandan türeyen paralel yürüyüşlerde **tabanı da ayrı yerel olarak koru** | Doğrudan `cur = g; kind = cur + 100` tabanı `cur` ile birleştirdi. `base = g; kind = base + 100; cur = base` ROM'daki ayrı r0/r1/r2 yaşamlarını ve literal havuzu yerleşimini üretti; `HasWantedEntry` 25 bayt farktan eşleşmeye geçti |
| 38 | Erken karşılaştırma ile sondaki store aynı değeri taşısa bile ROM ayrı dal istiyorsa değeri ve **taban kopyasını karşılaştırmadan önce** ayır | `current = h->current; h2 = h; if (value == current) return;` biçimi agbcc'nin eşitlik yolunu sondaki store ile birleştirmesini engelledi ve `PushHistory`de 31 bayt farkı kapattı |
| 39 | ROM yalnız belirli bir yazımdan sonra belleği yeniden okuyorsa `volatile`ı **tüm alana değil o erişime** uygula | `*(volatile u16 *)&gSaveBuffer.distance` yalnız taşma kontrolündeki ikinci `ldrh`yi zorladı. Alanı bütünüyle volatile yapmak register baskısını artırıp `AddDistance`ı 53 bayt bozarken dar kullanım fonksiyonu eşleştirdi |
| 40 | ROM iki dalda ayrı taban yükleyip tek store paylaşıyorsa kontrol akışını **etiketlerle açık kur** | Yapısal `if/else` agbcc tarafından ters çevrilip tabanlar birleştirildi. `reset:`, `increment:` ve `store:` etiketleri `BumpOrReset`ın iki `ldr` + ortak `strb` düzenini üretti; bu, temiz C içinde kabul edilebilir düşük seviye CFG ifadesidir |
| 41 | Ölçekli ofset birden çok tabanda kullanılacaksa çarpımı **tek atamada**, alan tabanlarını ayrı yerellerde kur | `scaled = index; scaled *= 180` pseudo önceliğini artırıp r3/r4'ü ters çevirdi. `scaled = index * 180` ile `heldBase`/`extraBase` ayrımı `ReleaseSlot`ta ROM'un tek r3 ofset + iki taban desenini üretti |
| 42 | ROM azalan bir döngü sayacı kullansa da C'de **artan indeksli `for`** denenmeli | `FlushSpriteList`te `for (i = count; i < left; i++)`, agbcc tarafından `left - count` kadar azalan döngüye çevrilir. Derleyicinin ürettiği çıkarma sabit kurulumlarından sonra gelir; elle yazılmış `left -= count` önce geliyordu. Kalan 9 bayt fark kapandı: 112/112 |
| 43 | Sayaç ve işaretçi birlikte ilerliyorsa **ikisini `for` artırımında, ROM sırasıyla** ifade et | `InitSpritePool`da gövdedeki `node++`, sayaç azaltımından önce geliyordu. `for (...; ...; i++, node++)` biçimi ROM'un sayaç-önce sırasını üretti: 4 bayt fark kapandı, 124/124 |
| 44 | Karşılaştırma sabitini **yerel değişkene al**: `hi = 15; if (x < hi)` | agbcc literal karşılaştırmayı kanonikleştiriyor (`< 15` → `<= 14`, `bls`); ROM'da `cmp #15 / bcc` görülüyorsa literal yazımın hiçbir çeşidi tutmaz. Sabit değişkene alınınca kanonikleştirme atlanır ve sabit yine immediate olarak yayılır. `EitherInRange`: 8/44 → 44/44. Permuter'ın bulduğu kaldırma + kalan sınırın elle kaldırılması. |

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

## `agbcc` varyantı 18 park fonksiyonunda denendi — hiçbiri eşleşmedi

2026-09-05. Proje kurulduğundan beri ölçülen 44 kuralın tamamı `old_agbcc` ile
türetilmişti; `tools/agbcc_build.py` içindeki `--cc=agbcc` seçeneği belgeliydi
ama hiç kullanılmamıştı. Park listesindeki 18 fonksiyonun tamamı iki derleyiciyle
yeniden ölçüldü.

**`agbcc` hiçbirini eşleştirmedi.** Dördünde fark azaldı, üçünde arttı:

| fonksiyon | old_agbcc | agbcc |
|---|---|---|
| FUN_0800cb08 | 291 | 146 |
| FUN_08030e78 | 34 | 14 |
| FUN_08030f28 | 28 | 15 |
| TryEngageTarget | 211 | 208 |
| FUN_08045aa4 | 12 | **40** |
| RunMenuScreen | 949 | **955** |
| StepEntryTimer | 55 | **64** |

İki VRAM doldurucusundaki iyileşme **yanıltıcı**. `agbcc` boyutu 36 yerine 40
tutturuyor, ama sebebi ROM'un yaptığı şey değil: gövde `old_agbcc` ile birebir
aynı, üstüne gereksiz bir `push {lr}` / `pop {r0}; bx r0` sarmalı biniyor (+4
bayt). ROM'daki +4 ise fazladan bir yazmaç kopyası artı hizalama dolgusu. Aynı
boyuta başka yoldan varan bir tesadüf, ilerleme değil.

**Sonuç: `agbcc_build.py`'ye dosya başına derleyici seçim işaretçisi
EKLENMEDİ.** `KIP: ARM` benzeri bir `DERLEYICI: agbcc` işaretçisi ancak bir
eşleşme onunla mümkün olsaydı gerekliydi; öyle bir eşleşme yok. Eklemek, hiçbir
şey kazandırmadan derleme katmanına bir dal daha sokardı.

`FUN_0800cb08`'in 291'den 146'ya inmesi (348 bayt, hâlâ eşleşmiyor) tek başına
tekrar bakılmaya değer olabilir; ötekiler için bu yol kapalı.

## Yazmaç kopyası: kaynak düzeyinden üretilemeyen sınıf

`FUN_08030e78` / `FUN_08030f28` (ikisi de 36/40, dört bayt kısa) bu sınıfın
temiz örneği. `old_agbcc` ROM'un gövdesini komut komut aynı üretiyor; tek eksik
ROM'daki fazladan `adds r2, r0, #0`. ROM karo sabitini önce r0'a yükleyip r2'ye
taşıyor çünkü dağıtıcı r0'ı döngü sayacına bırakıyor; bizimki sabiti doğrudan
r3'e verip kopyadan kaçınıyor.

Bu kopyayı üretmek için taranan ve elenen her şey:

- **144 bildirim/atama sırası** (4! bildirim × 3! atama) — hepsi 36 bayt
- **13 bayrak kümesi** — `-O0/-O1/-O2/-O3/-Os`, `-fno-omit-frame-pointer`,
  `-fforce-mem`, `-fforce-addr`, `-fno-strength-reduce`, `-fno-defer-pop`,
  `-fcaller-saves`, `-fno-cse-follow-jumps` — hiçbiri 36'yı değiştirmedi
- **Yapısal aileler** — `for` / `while` / `do-while`, `*p++` ile `*p=t; p++`,
  karo tipi `u16` / `s32` / `int` / `vu16`, ara kopya değişkeni, sayaç
  değişkeni, `q = p + 32` — hepsi 36 bayt

Yani bu, kural 44'ün (karşılaştırma kanonikleştirmesi) tersi bir durum: orada
kaynakta bir kaldıraç vardı, burada yok. `tools/sweep_variants.py`'nin kapsam
sınırı notunda yazan "yükleme/saklama yazmaç dağıtımı" sınıfı tam olarak budur.
Yeni bir mekanizma bulunmadan bu fonksiyonlara dönmeyin.

## Kural 45 — dal başına ayrı yerel değişkenler blok birleşmesini engeller

Uzun `if/else if` zincirlerinde birden çok dalın gövdesi birbirinin aynıysa,
agbcc onları çapraz atlamayla birleştirir ve bir dalın kodu tamamen kaybolur.
ROM'da o dallar ayrı fiziksel kopyalar olarak duruyorsa, orijinal kaynakta
**her dalın kendi yerel değişkenleri** vardır.

`FUN_080260a8`'de (1518 bayt, 12 dallı zincir) ölçüldü. Bütün dallara ortak bir
`bias`/`shift` çifti verince `case 8`'in gövdesi tümüyle `0x1f`/`0x28` ile
birleşti: bizde `cmp #8` ile `cmp #30` arası **4 bayt**, ROM'da **156**. Her dala
üçer ayrı yerel açınca 1312 → 1428 bayt.

**Nasıl fark edilir:** ROM'un yığın gözlerine bakın. Aynı işi yapan iki dal
farklı `sp` ofsetleri kullanıyorsa (burada `case 8` → `sp+8/12/16`,
`case 0x1f` → `sp+32/36/40`) yereller ayrıdır. Ghidra bunu zaten doğru
gösteriyor (`local_48/44/40` ve `local_30/2c/28` gibi ayrı adlar); çıktıdaki
yerel adlarını "gürültü" diye atmayın, **yapısal bilgi taşıyorlar**.

İkinci bir belirti: ROM'da tekrar eden bir havuz sabitinin kaç kez geçtiğini
sayın. Burada `0x03FFFFFF` altı ayrı havuz kelimesinde duruyor, yani maskeyi
kullanan altı fiziksel blok var. Bizim çıktımızda beş çıkması, bir bloğun
birleştiğinin doğrudan kanıtıydı.

Bayraklarla çözülmez: `-fno-thread-jumps`, `-fno-cse-follow-jumps`,
`-fno-expensive-optimizations` ve `-O1` denendi, hiçbiri birleşmeyi kaldırmadı.
Kaldıraç kaynakta, yerel değişken ayrımında.

**Nerede geçerli — `tools/scan_dispatch.py`.** Kural yalnızca bir durum
değişkenine göre dallanan uzun `if/else` zincirlerinde işe yarar, o yüzden
adayları ROM'dan doğrudan bu imzayla arıyoruz: aynı yazmaca karşı ard arda
gelen `cmp rX,#imm` halkaları, içlerinde yeterince **farklı sıfırdan büyük**
değer. `cmp rX,#0` halkaları boş kontroldür, elenir.

Araç kendini doğruluyor: `FUN_080260a8` listede tam da eşleştirilen değerlerle
çıkıyor (35, 33, 8, 30, 31, 40, 50, 32, 7, 6). Eşiği 6 halka / 5 farklı değer
alınca **13 aday, 18.788 bayt** kalıyor; en büyüğü `FUN_08017628` (1536 bayt,
26 farklı değer, 35 halka).

**Bir yol denendi ve terk edildi:** aynı tarama önce Ghidra çıktısı üzerinden,
"birbirinin aynı ard arda ifade blokları" sayılarak yapıldı. Yanlış pozitif
veriyor — `FUN_080108f4`'te 139 tekrar saydı ama o fonksiyon tekrar eden dal
gövdesi değil, tekrar eden küresel erişim kalıbı içeriyor (40 ayrı `DAT_`
global, iç içe döngüler). Metin benzerliği yanlış ölçüt; imza makine kodunda.

**Ghidra çıktısının yarım olduğunu gösteren uyarılar** aday elemede şart:
`Could not recover jumptable` (33 çıktının 19'unda), `Removing unreachable
block` (bu sinsi — çıktı derli toplu görünür ama bloklar düşmüştür,
`FUN_0802e3fc` böyleydi), `Bad instruction`, `truncated`.

## Kural 46 — switch'in aralığı, ağaç ile atlama tablosu arasındaki seçimi belirler

agbcc bir `switch`i ya **karşılaştırma ağacına** ya da **atlama tablosuna**
çevirir; kararı case kümesinin yoğunluğuna göre verir. ROM'da hiç `mov pc,rX`
yoksa ağaç seçilmiştir ve bizim kaynağımız da ağaç üretmelidir, yoksa boyut
tutmaz.

`FUN_08017628`'de (1536 bayt, ~40 case) ölçüldü. Case'leri 1..0x97 arasında
yazınca küme yoğun kaldı ve agbcc atlama tablosu üretti: **1814 bayt** (296
fazla), 45 karşılaştırma, bir `mov pc,r0`. Ghidra'nın havuz sabiti gibi
gösterdiği dört büyük değeri (0x4005, 0x4026, 0x4027, 0x4028) case olarak
ekleyince aralık 1..0x4028'e açıldı, gcc mecburen ağaç üretti: **1510 bayt**,
92 karşılaştırma (ROM ile birebir), sıfır `mov pc`. Tek değişiklik, 304 bayt.

**Teşhis:** ROM'da ve kendi çıktınızda `mov pc,rX` (0x4687/0x468F/0x4697)
sayın, sonra `cmp rX,#imm` sayılarını karşılaştırın. Sayılar tutuyorsa ağaç
şekli doğrudur; sizde `mov pc` varken ROM'da yoksa case kümeniz fazla yoğun,
uzaktaki case'leri kaçırmışsınızdır.

**Ağaç okunurken tuzak:** bir aralıkta tek case kaldığında gcc eşitlik yerine
`<` ile ayırır, o yüzden o case ROM'da `cmp` olarak görünmez. `FUN_08017628`'de
0x0E böyle: ROM'da `cmp #14` yok ama case var, (0x0D, 0x0F) aralığına tek
değer kaldığı için `< 0x0F` ile ayrılmış. Eksik case sanıp çıkarmayın.

## Ghidra: ARM kipi tuzağı ve yanlış "atlama tablosu" etiketi

Ghidra'nın otomatik analizi bazı Thumb girişlerini **ARM kipinde** çözmeye
çalışıp `bad instruction data` ile bırakıyor ve o adreste fonksiyon bile
tanımlamıyor. Bizim sınır tarayıcımızın bulduğu 7 girişten 5'i böyleydi
(`FUN_08017628` dahil). Çözüm: `TMode` yazmacını 1 yapıp bölgeyi temizleyip
yeniden sokmak — `tools/ghidra/ExportDecompileBatch.java` bunu boyut verilince
koşulsuz yapıyor. **Koşulsuz olmalı:** bozuk bir fonksiyon önceki koşudan
kalmış olabilir, "yoksa oluştur" yetmez. Bu onarım 8.586 baytlık beş
fonksiyonu okunabilir hale getirdi.

Ayrıca Ghidra `pop {r4,r5,r6}; pop {r0}; bx r0` dizisinin sonundaki `bx r0`ı
"çözülemeyen atlama tablosu" sanabiliyor. `FUN_08017628`'de öyle oldu; orada
tablo yok, sadece void dönüşün interworking biçimi var.

## Kural 47 — bayt alanının işaretliliği maskenin nasıl kurulacağını belirler

`x &= ~15` bir **`u8`** alan üzerinde yazılırsa agbcc sabiti `0xF0`'a indirger
(`movs r1,#240`). Aynı satır **`s8`** alan üzerinde yazılırsa alan `int`'e
yükselir ve maske `-16` olarak kurulur (`movs r1,#16 / negs r1,r1`) — bir komut
daha, iki bayt.

`FUN_08016768`'de ölçüldü: alanları `u8` bırakınca fonksiyon iki bayt kısa
kalıyor ve ayrıca komut sıralaması kayıyordu; `s8` yapınca ikisi birden düzeldi
ve fonksiyon **eşleşti**.

**Kural dar, genellemeyin (2026-09-06 düzeltmesi).** Belirleyici olan alanın
işaretliliği değil, AND sonucunun hangi genişlikte tüketildiği. `FUN_080526b8`'de
`node->kind &= ~12` bileşik ataması **`u8` alanda da** `-13` üretiyor ve eşleşiyor,
çünkü işlem int genişliğinde yapılıyor. `FUN_08016768`'de sonuç dar tipe
indirgenerek kullanıldığı için `u8` alanda sabit `0xF0`'a katlanıyordu. Yani
ROM'da `negs` görmek "alan işaretli" demek değil; "sabit int genişliğinde
kurulmuş" demek. Alan tipini ancak başka bir kanıt varken değiştirin.

**Ayrı bir tuzak — ara yerele almayın.** Aynı işi `s32 k = field & ~12;
field = k;` diye yazmak agbcc'yi depolamadan önce `lsls #24 / asrs #24`
normalleştirmesi sokmaya itiyor: iki fazla komut, ROM'da yok. Bileşik atamayı
doğrudan yazın.

Maskeyi geniş tipte bir yerele almak (`s32 m = ~15; x &= m;`) `negs`i geri
getiriyor ama sabiti ifadeden ÖNCE yaydığı için adres hesabıyla sırası ters
düşüyor. Doğru çözüm alanın tipini düzeltmek, maskeyi taşımak değil.

## Kural 48 — koşulun sonucunu değişkende maddeleştirmek

ROM'da `movs r0,#0 / ... / movs r0,#1 / cmp r0,#0 / beq` dizisi görüyorsanız
kaynak koşulu doğrudan dallanmıyor, **sonucu bir değişkene yazıp onu sınıyor**:

```c
ok = 0;
if (ent->kind == 4) ok = 1;
if (ok) FUN_0803c400(ent);
```

Kısa devreli `if (ent != 0 && ent->kind == 4)` yazımı doğrudan dallanma üretir
ve altı bayt eksiltir. Kalıp projede daha önce `target_follow.c`'de de görülmüştü.

## DMA kurulumundan sonra denetim yazmacı geri okunuyor

ROM DMA3 denetim kelimesini yazdıktan sonra onu bir kez **geri okuyor**
(`ldr rX,[rY,#8]`). İşlevsel görünmüyor ama atlanırsa her DMA bloğu iki bayt
eksilir. `cutscene_frame.c` (eşleşti) ve `flush_palette_queue.c` ikisinde de var:

```c
REG_DMA3.control = TILE_CTRL;
REG_DMA3.control;          /* geri okuma; atlanirsa iki bayt eksik */
REG_IME = ime;
```

## Kural 49 — ROM seyrek gövdeleri fonksiyonun SONUNDA tutar

agbcc `if` gövdelerini kaynak sırasında yayıyor. ROM'da bir dal **ileri**
atlıyorsa (`beq` uzağa, `bne` uzağa) o gövde fonksiyonun sonundadır; aynı
gövdeyi akışın içine yazmak bloğu öne alır ve dallanmayı tersine çevirir.

`FUN_080543D0`'da (126 bayt) üç kez arka arkaya uygulandı ve her seferinde
kazandırdı:

| taşınan | önce | sonra |
|---|---|---|
| "bulundu" gövdesi döngüden sona | 128 B | fark 87 |
| `return 0` sona | fark 87 | **fark 56** |

Yazım biçimi: erken dönüş yerine sona `goto`, gövdeyi `return`dan sonra
etiketle. Proje bu biçimi zaten kullanıyor (`target_follow.c`, `menu_screen.c`).

```c
    if (spare->id != SPARE_ID) goto none;   /* `return 0;` DEGIL */
    ...
    return spare;
found:                                       /* seyrek govdeler sonda */
    ...
    return cur;
none:
    return 0;
```

**Nasıl fark edilir:** ROM'daki koşullu dallanmanın YÖNÜNE bakın. İleri
atlıyorsa hedef gövde ileridedir; sizin çıktınızda aynı yerde `bne` varken
ROM'da `beq` (ya da tersi) görüyorsanız blok sıranız terstir.

**Yan bulgu — döngü döndürme.** Aynı fonksiyonda `break` kullanmak agbcc'yi
döngüyü döndürmeye (`b` ile alttaki teste atlama) itti. ROM'un giriş
koruması + alttan dönen biçimi için `break` yerine açık `goto` yazın.

**AYNI AILEDEKI FONKSIYONLAR AYNI DONGU BICIMINI KULLANMAYABILIR.**
`FUN_080543D0` ve `FUN_08054744` neredeyse ikiz (ikisi de sıralı listede
kimlik arayıp yedek düğümü kuruyor) ama ROM'da farklı derlenmişler:

| | ROM'un biçimi | doğru yazım |
|---|---|---|
| FUN_080543D0 | giriş koruması + alttan dönen do/while | `if (cur == 0) goto ...` + `goto scan` |
| FUN_08054744 | döndürülmüş `for` (`b` ile teste atlama) | `goto test;` + `step:` / `test:` |

`FUN_08054744`'te düz `for` yazmak agbcc'ye ilk turu **soydurdu** (kimlik
karşılaştırması çıktıda iki kez); 176/164 bayt, 12 fazla. Açık atlamalarla
ROM'un biçimini yazınca boyut tuttu ve fark 162'den 107'ye indi.

Ders: kardeş dosyanın döngü biçimini KOPYALAMAYIN, her fonksiyonun biçimini
ROM'dan okuyun. Struct'lar ve çağrılan imzaları paylaşılabilir, kontrol
akışı paylaşılamaz.

## Kural 50 — yazmaç önceliğini kaynaktan çevirmenin tek yolu: değişkeni BÖL

agbcc'nin dağıtım sırası ölçüldü ve doğrulandı (2026-09-06, üç fonksiyon, 39
allocno elle yeniden türetildi):

```
öncelik = floor_log2(refs) * refs / ömür        (eşitlikte küçük pseudo önce)
```

Sıra geldiğinde `find_reg` çakışma çizgesindeki **en küçük boş** yazmacı verir;
allocno bir çağrıyı aşıyorsa yalnızca callee-saved (r4+) adaylara bakar.

**Kaldıraç `floor_log2`'nin basamak fonksiyonu olmasında.** Bir değişkeni ikiye
bölmek refs'i ve ömrü kabaca yarıya indirir — oran hemen hemen aynı kalır ama
`floor_log2(refs)` bir tam basamak düşer, öncelik ~1/3'e iner.

`FUN_08052DDC`'de ölçüldü: iki dış döngü tek `p` işaretçisini paylaşırken
`3*14/29 = 1.448`, sayacın `1.185`'ini geçip r4'ü kapıyordu. İkinci döngüye
ayrı işaretçi verilince `2*7/14 = 1.000`'e düştü, sayaç önce dağıtılıp r4'ü
aldı: **fark 17 → 0, byte-matching.**

**BÖLME YALNIZCA İKİ AYRI ÜRETİM YERİ VARSA İŞE YARAR.** Kopya tabanlı bölme
(`lst = list;`) her zaman eleniyor — 16 yazım denendi, hiçbiri yeni allocno
üretmedi. `p = entry->ids` / `p2 = entry->slots` gibi gerçekten iki ayrı
kaynaktan doğan değerler bölünebilir.

**refs döngü derinliğiyle ağırlıklandırılmıyor** — döngü içindeki referanslar
ekstra sayılmıyor, yani tabloyu kaynaktan elle saymak mümkün.

**Araç eksiği:** `tools/dump_alloc.py` `.greg` dökümünün yalnızca öncelik
tablosunu okuyor. Asıl cevap altındaki **`Register dispositions`** bölümünde:
pseudo → donanım yazmacı haritası. "Hangi değişkenim r4'ü kaptı" sorusunun tek
satırlık cevabı orada; araç onu basmıyor.

## Kural 47'ye karşı örnek — işaretlilik BAZEN fark ediyor

`FUN_080526B8`'de (nodelist_b6.c) `+0x0B` alanının `u8` mi `s8` mi olduğu fark
etmemişti. `FUN_08052750`'de **fark ediyor**: `u8` alanda `kind &= ~1` tek
komuta katlanıyor (`movs r0,#254`), ROM ise `movs r0,#2 / negs r0,r0` ile −2
kuruyor. Alan `s8` olmalı. Ters yönde bedel yok: `s8` iken de `kind & 1` ve
`(kind & 0xF) | 0x10` hâlâ `ldrb` üretiyor, agbcc 0x100'den küçük maskede
sign-extend eklemiyor.

Yani kural 47'nin kararı "AND sonucunun genişliği" değil, **her iki yönü de
ölçmek**. İki satırlık deney, tahminden ucuz.

## Sorun gerçekten dağıtım mı? — `dump_alloc.py --rom` ile önce bunu ölç

Kural 50'yi uygulamadan önce **sorunun dağıtım olup olmadığını** ölçün. Araç
artık ROM'un `push` listesini ve yazmaç başına operand sayımlarını yan yana
basıyor; ayrım tek bakışta görünüyor.

**Ölçülen iki karşıt örnek (2026-09-06):**

`FUN_080543D0` — fark 41. Callee-saved trafiği ROM ile **birebir aynı**:
r3 9/9, r4 11/11, r5 8/8, r6 7/7, r7 1/1. Fark yalnızca r0/r1/r2'de
(23/28, 26/23, 7/5). Yani dağıtım **zaten doğru**; 41 bayt geçici yazmaç ve
komut seçiminden geliyor. Buraya değişken bölme uygulamak **boş emek** —
üç dosyanın körlemesine kurcalanmasının sebebi tam da bu ayrımın
yapılmamasıydı.

`FUN_08054744` — fark 80. Callee-saved trafiği **sapıyor**: r3 ROM 13 /
bizde 6, r4 ROM 16 / bizde 13, r2 ROM 10 / bizde 16. ROM ağırlığı r3+r4'te
tutuyor, biz r2'ye yıkıyoruz. Sorumlu ölçüldü: bir allocno 12 refs / 64
ömürle en yoğun ikinci değer ama **hiç çağrı aşmıyor**, o yüzden `find_reg`
ona en küçük boş yazmacı (r2) veriyor ve callee-saved adaylara hiç bakmıyor.
Kural 50 kaldıracının anlamlı olduğu dosya bu.

`FUN_08052DDC` (eşleşen) kalibrasyon referansı: `push` listesi ve **tüm**
operand sayımları ROM ile birebir. Aracın doğru okuduğunun kanıtı.

**Kural: önce `--rom` çalıştır.** Callee-saved sayımları tutuyorsa dağıtım
doğrudur, başka yere bak.

## agbcc pseudo → C değişken adı: PARAMETRELER DIŞINDA İMKÂNSIZ

Denendi ve ölçüldü: agbcc'de `-g` yok, `-gstabs` "invalid debug option"
veriyor, üretilen `.s`'te tek bir `.stab` satırı yok, RTL insn'lerinin satır
numarası alanı −1. Derleyici bu dökümlere yerel değişken adı **hiç yazmıyor**.

Parametreler çıkarılabiliyor: prologda `NOTE_INSN_FUNCTION_BEG`'den önceki
`(set (reg/v N) (reg H rH))` kalıbı argüman sırasını veriyor, ad da kaynaktaki
**tanım** imzasından okunuyor. Yerel değişkenler için ad sütunu boş kalır ve
yerine ilk tanım ifadesi basılır (`mem[p22+40]` gibi) — ad uydurulmaz.

İki tuzak, ikisi de araç yazılırken yakalandı: `note` düğümleri taranmazsa
`NOTE_INSN_FUNCTION_BEG` görülmez ve **çağrı dönüş değerleri parametre
sanılır**; ayrıca parametre adı için dosyadaki ilk geçişe bakmak yanlıştır —
başlık yorumundaki eski imza gerçek tanımla çelişebiliyor (`nodelist_a1.c`'de
tam bu oldu).

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

## Kapanan: WriteU16LE

Bir dönem açık kalmıştı: ROM girişte değeri 16 bite normalize ediyordu
(`lsls #16` / `lsrs #16`) ve ürettiğimiz kod bu dört baytı atlıyordu.
Kural 15 (dar parametrenin işaretliliği giriş normalizasyonunu belirler)
bulunduktan sonra kapandı; `src/save/save_helpers.c` şimdi 8/8
byte-matching.

## Kurulum

İkili dosyalar depoya girmez (8.8 MB). Yerelde üretmek için:

```sh
make agbcc
```

`tools/setup_agbcc.sh`, pret/agbcc kaynağını
`config/toolchain.lock.json` içindeki uyumlu revizyona sabitler ve derler.
agbcc 1998 dönemi C kaynağı olduğu için modern clang'in varsayılanlarıyla
derlenmiyor; izlenen `tools/agbcc_host_cc.sh` gerekli uyumluluk bayraklarını
taşıyor. Kurulum sonunda 23 fonksiyonluk sabit temsil corpus'unun parmak izi
doğrulanır; yeni kaynak eklenmesi bu kilidi kendiliğinden değiştirmez.

## Doğrulama döngüsü

```sh
make c-match FILE=src/save/save_helpers.c
```

Her fonksiyonu ayrı ayrı derleyip `data/functions.csv`'deki adresinden ROM ile
karşılaştırır. Eşleşen fonksiyonun assembly karşılığı artık gereksizdir.
| 39 | Butun register'lar beklenenden **bir yukaridaysa**, kaynakta dokunulmadan **iletilen fazladan bir parametre** vardir | `SubmitPack`te ROM `{r4,r5,r6}` + r2/r3 kullanirken bizimki `{r3,r4,r5}` + r1/r2 uretiyordu; farkin tamami tek register kaymasiydi. r1'i KURAN komut yoktu, yani deger gelen parametreydi. Ikinci parametreyi imzaya ekleyip cagriya iletmek 8 bayt farki sifirladi. Once `FUN_08060db4(void)` -> arguman eklemek 16'dan 8'e indirmisti |
| 40 | ROM kisa omurlu ara degerleri **scratch register**'da (r0-r3) tutuyorsa, kaynakta da **blok kapsamli ayri gecici** kullanilmali; tek bir yeniden kullanilan yerel onlari callee-saved'e itiyor | `ClipBounds`ta ROM `push {r4,r5,lr}` uretirken bizimki `push {r4,r5,r6,lr}` uretiyordu: tek `cand` degiskeni tum fonksiyon boyunca yasayip r2'yi tutuyor, ROM ise `pad` oldugunde onun register'ini yeniden kullaniyor. Her bileseni `{ s32 cand = ...; if (...) ...; }` bloguna almak alti bagimsiz kisa omurlu gecici uretti. Ayrica tekrarlanan bellek okumalari dar volatile gorunumden yapilmali; derleyici aksi halde ortak alt ifade olarak onbellege alip omru uzatiyor (96 bayt -> ROM'un 100 bayti). Ikisi birlikte 26 farki 0'a indirdi. UYARI: teknik ISLEVSEL DEGIL, YEREL -- ayni hamle CleanupAreaTiles'i 7'den 85'e kotulestirdi, SetBg1Enable'i degistirmedi |

## Register dagitimi: kontrollu deneyle olculdu

Bu tablo tahmin degil, kurulu `old_agbcc` ikilisi uzerinde yapilan
kontrollu deneylerin sonucudur (probe: her varyant derlenip prolog `push`
listesi okundu). Uc park dosyasinin ve 372 baytlik olceklendirme
denemesinin ortak engeli buydu.

### Yaprak fonksiyon (cagri YOK)

| ayni anda canli deger | prolog |
|---|---|
| 2 | push yok |
| 3 | push yok |
| 4 | push yok |
| 5 | `push {r4, lr}` |
| 6 | `push {r4, r5, lr}` |
| 7 | `push {r4, r5, r6, lr}` |

Yani **dorde kadar canli deger `r0`-`r3`'e sigar**; besinci `r4`, altinci
`r5`, yedinci `r6`.

### Cagri varsa

Cagri `r0`-`r3`'u ezdigi icin cagri boyunca yasayan HER deger
callee-saved ister:

| cagri boyunca canli | prolog | not |
|---|---|---|
| 1 | `push {r4, lr}` | |
| 2 | `push {r4, r5, lr}` | |
| 3 | `push {r4, r5, r6, lr}` | |
| 4+ | `push {r4, r5, r6, lr}` | liste BUYUMEZ, yigina tasar |

Dortte komut sayisi 13'ten 20'ye firliyor: r7'ye gecmek yerine spill
ediyor. **`r7` ancak 7+ canli degerde geliyor.**

### Etkisi olmayanlar (olculdu)

- **Cagri SAYISI**: 3 canli deger, 1/2/3 cagri -> hepsi `{r4,r5,r6}`.
- **Isaretci mi skaler mi**: 4 deger, ikisi de `{r4,r5,r6}`.
- **Sabitin nerede kuruldugu**: dongu icinde/disinda/dogrudan -> ayni kod.
  agbcc sabiti hoist ediyor.
- **Kopya**: `w = v` HIC yasamiyor. Tek yerel, kopyali iki yerel ve
  ikisi de sabitten kurulan iki yerel -> ucu de AYNI kod. Kural 37'nin
  ("tabani ayri yerele al") sinirı budur: ayri isim vermek kopya
  URETMEZ, ancak degerin iki farkli KULLANIM YERI varsa uretir.

### Nasil kullanilir

ROM'un prologu kac callee-saved register istediginizi soyler; oradan
ROM'un kac canli degeri oldugunu geri hesaplayin ve kaynagi o sayiya
getirin. Fazladan bir `push` demek fazladan bir canli deger demektir.

UYARI 1: deger SAYISINI dusurmek gerekir, DEGISTIRMEK degil.
CleanupAreaTiles'ta sayaci kaldirip yerine bitis isaretcisi koydum -- sayi
altida kaldi, prolog degismedi ve fark 7'den 60'a cikti.

UYARI 2 -- KURALIN ASIL SINIRI: kaynaktaki yerel sayisi, dagiticinin
canli kumesi DEGILDIR. CleanupAreaTiles'ta `block` yerelini iki ayri
sekilde dongu disina cikardim (satir ici cagri; dongu sonrasi atama) ve
prolog IKISINDE DE degismedi -- agbcc sabit adresi zaten hoist ettigi
icin `block` hicbir zaman dongu boyunca canli degildi.

Yani kural izole deneyde olculebiliyor ama gercek bir fonksiyona
uygulamak icin dagiticinin canli kumesini GORMEK gerekiyor; kaynaktaki
yerelleri saymak yaniltiyor.

BU ADIM COZULDU: agbcc `-dg` bayragini kabul ediyor ve global dagitim
dokumu (`.greg`) uretiyor. Dokumde dagiticinin KENDI oncelik listesi
yazili -- her pseudo icin `refs` ve `live_length`. `tools/dump_alloc.py`
bunu okuyup tabloyu basiyor.

FORMUL DOKUME KARSI DOGRULANDI. Bir probe fonksiyonunda dokumun yazdigi
sira:
    R25 refs=7 omur=18 -> 0.778
    R23 refs=7 omur=26 -> 0.538
    R22 refs=7 omur=28 -> 0.500
    R27 refs=4 omur=20 -> 0.400
    R26 refs=3 omur=18 -> 0.167
    R24 refs=2 omur=24 -> 0.083
`floor_log2(refs) * refs / omur` ile hesaplanan sira BIREBIR ayni.

ILK OLCUM SASIRTICI: CleanupAreaTiles'ta 12 pseudo-register ve 5 spill
var; ben alti canli deger sayiyordum. Kaynaktaki her yerel birden fazla
pseudo'ya aciliyor, bu yuzden elle saymak yaniltiyordu.

SPILL SAYISI OLCUT DEGIL -- 285 fonksiyonda olculdu ve hipotez CURUDU:

    eslesen 277 fonksiyon : spill ort 3.85, MAX 170, %43'u sifir spill
    eslesmeyen 8 fonksiyon: spill ort 19.75, max 99, %12'si sifir spill

Eslesen fonksiyonlarda 170 spill'e kadar ornek var; yani yuksek spill
eslesmeyi ENGELLEMIYOR. Tersi de dogru: EitherInRange 3 pseudo ve SIFIR
spill tasiyor ama hala 8 bayt farkli (onun engeli karsilastirma sabiti
kanonikleştirmesi, dagitimla ilgisi yok). HalvesEqual 1 pseudo/2 spill,
yine eslesmiyor.

Yani `dump_alloc.py` bir fonksiyonun KENDI varyantlarini karsilastirmak
icin gecerli bir olcut (A bicimi B bicimine gore kac pseudo/spill
uretiyor), ama fonksiyonlar ARASI bir esik yok. "Spill'i su sayinin
altina indir" diye bir hedef kurulamaz.

Eslesmeyenlerin pseudo ortalamasi yuksek (34.4 vs 7.0) ama bu yaniltici:
o sekiz fonksiyon zaten bilerek secilmis en zor ornekler.
