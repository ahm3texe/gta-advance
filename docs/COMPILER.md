# Derleyici kimliği: agbcc

## Sonuç

GTA Advance (Avrupa), **`old_agbcc`** ile derlenmiştir — Nintendo'nun resmî GBA
SDK'sıyla dağıttığı GCC 2.8.1 tabanlı derleyicinin *eski* varyantı. Aynı
derleyici ailesi pokeruby ve pokeemerald decomp'larında da kullanılıyor.

Bunun anlamı: **C'den byte-matching üretmek mümkün.** Proje semantik yeniden
inşaya mecbur değil.

## Kanıt 1 — kod kalıpları

Derleyiciler aynı işi yapmanın birden fazla yolundan hep aynısını seçer.
Doğrulanmış 42 fonksiyonda (2874 satır assembly):

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
| 4 | **Donanım/BIOS değişkeni `volatile` OLMAMALI** | `volatile` yükleme sırasını değiştirip ROM'dan saptırıyor |
| 5 | Dış semboller `.equ` ile assembler'a verilir | Linker'a bırakılınca interworking veneer'i sokuluyor |
| 6 | Bölüm adresi link betiğinde sabitlenir (`SUBALIGN(1)`) | agbcc `.text`'i 8'e hizalıyor, taban 8'in katı değilse her ölçüm kayıyor |
| 7 | Üretilen assembly'nin sonuna `.align 2, 0` eklenir | `as` Thumb bölümünü NOP ile dolduruyor, ROM sıfırla |

3 ve 4 birbirinin zıddı gibi görünüyor ama değil: `volatile` agbcc'de komut
sıralamasını değiştiren bir düğme. Yığın tamponunda ROM'un sırasını veriyor,
donanım değişkeninde bozuyor. Kural ezberlenmez, denenir.

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
