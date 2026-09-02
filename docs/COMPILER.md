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

## Açık kalan

`WriteU16LE` (0x08001124, 8 byte): ROM girişte anlamsal olarak gereksiz bir
16-bit kırpma yapıyor, `old_agbcc` bunu eliyor. Denenen ve tutmayan C
biçimleri `src/save/save_helpers.c` içinde listeli. Çözülene kadar
`src/save/save_helpers.s` geçerli kaynaktır.

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
