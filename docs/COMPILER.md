# Derleyici kimliği: agbcc

## Sonuç

GTA Advance (Avrupa), **agbcc** ile derlenmiştir — Nintendo'nun resmî GBA
SDK'sıyla dağıttığı, GCC 2.8.1 tabanlı yamalı derleyici. Aynı derleyici
pokeruby ve pokeemerald decomp'larında da kullanılıyor.

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
WriteU8       4 byte   BYTE-MATCHING  (0x08001120)
WriteU32LE   28 byte   BYTE-MATCHING  (0x08001130)
```

`WriteU32LE` belirleyici olan: 28 byte'ın tamamı birebir, üstelik maskeyi
literal havuzdan okumak yerine iki kez `mov #0xff` + `lsl` ile yeniden kurma
gibi ayırt edici bir tercihle. Yanlış derleyici bunu üretemez.

## Bayraklar

```
-mthumb-interwork -O2 -fhex-asm
```

pokeemerald ile aynı; ilk denemede tuttu. Bazı çeviri birimleri farklı
optimizasyon seviyesi kullanıyor olabilir — eşleşmeyen fonksiyonlarda
`-O1` ve `old_agbcc` denenecek.

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
