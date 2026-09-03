# Yol haritası

Bu belge *ne yapılacağını* ve *neden o sırayla* yapılacağını tutar.
[ROADMAP.md](ROADMAP.md) aşamaların tanımını, bu dosya güncel planı verir.

## Nerede duruyoruz

```
Doğrulanmış ROM:   7.872 bayt, 36 bölge
Matching kod:      6.830 / 291.535 bayt  (%2.34)
Byte-matching:     100 / 1505 fonksiyon
```

## Kalanın dağılımı — planın dayanağı

1405 fonksiyon, 284.705 bayt:

| Boyut | Adet | Bayt | Kalanın payı |
|---|---|---|---|
| ≤64 | 519 | 16.816 | %5.9 |
| 65–256 | 567 | 75.100 | %26.4 |
| 257–512 | 149 | 52.433 | %18.4 |
| 513+ | 125 | 140.267 | **%49.3** |

Tek cümleyle: **kalanın yarısı 513 bayttan büyük fonksiyonlarda.** Küçük
fonksiyonları toplamak bizi ancak ~%7'ye taşır; ötesi büyükleri çözmeyi
gerektirir.

---

## Faz 0 — Sınır denetimi (önce bu)

**Sorun:** `data/functions.csv`'deki sınırların **%25'i yanlış.** 401 aday
tarandı, 99'u kendi gövdesinin dışına dallanıyor. Ghidra atlama tablolarında
ve gövde içi literal havuzlarında duruyor.

Bu şimdiye kadar üç kez ölçüldü: `RunMenuScreen` 956 yerine 1414,
`IsTileTypeInRange` 56 yerine 60, ayrıca iki sahte "küçük fonksiyon kümesi"
aslında büyük fonksiyonların kuyruğuydu.

**Neden önce:** yanlış sınır her aşamada boşa emek demek. Ucuz, otomatikleşir
ve tüm sonraki fazları hızlandırır.

**İş:** `tools/audit_boundaries.py`
- gövde dışına dallanan adayı işaretle
- gerçek sonu bul: epilog (`pop`/`bx lr`) + sonraki prolog (`push`)
- gövde içi literal havuzlarını sınıra dahil et
- bitişik parçaları tek fonksiyonda birleştir
- `functions.csv`'yi düzelt, değişiklikleri raporla

**Çıktı:** güvenilir fonksiyon haritası. Kapsama artışı yok, ama her şeyin ön koşulu.

---

## Faz 1 — Küçük fonksiyon hasadı

**Havuz:** ≤64 bayt, 519 fonksiyon, 16.816 bayt.
Bunların 32 tanesi ≥3 fonksiyonluk bitişik küme (~3.354 bayt); gerisi dağınık.

**Yöntem:** bu oturumda kanıtlandı — altı bölgenin dördü *ilk denemede* tam
eşleşti. Bitişik kümeler tek bölgede toplanıyor, dağınık olanlar kendi
bölgesini alıyor.

**Gerçekçi beklenti:** havuzun %60–70'i → **~10.000–12.000 bayt.**
Toplam ~18.000 bayt (%6–7).

**Risk:** düşük. Yöntem çalışıyor.

---

## Faz 2 — Sistemik engeli kır (asıl kaldıraç)

**Sorun:** orta ve büyük fonksiyonlarda %85–95 komut hizalamasına gelip
register dağıtımı veya blok sıralamasında takılıyoruz. Dört fonksiyon
tam olarak burada park halinde:

| Dosya | Kalan | Engel |
|---|---|---|
| `clear_text_area.c` | 1/132 bayt | ölü okumanın hedef register'ı |
| `actor_init.c` | 14/192 bayt | 0xA8 tabanı r2 yerine r3 |
| `entity_action.c` | 85/98 komut | `return 0` bloğunun yeri |
| `menu_screen.c` | 503/694 komut | tüm atama bir register kaymış |

Bu dördü **test korpusu**: doğru çözüm dördünü birden ilerletmeli.

**İş A — otomatik varyant tarayıcı (`tools/sweep_variants.py`)**
Şimdiye kadar her fonksiyon için elle tarama betiği yazdım. Mekanik
dönüşümleri otomatikleştir:
- dal yönünü çevir (`if (x) A else B` ↔ `if (!x) B else A`)
- erken `return`'ü ayır / gövdeyi `if` içine al
- alan işaretliliği (`u8` ↔ `s8`)
- yerel değişken ↔ satır içi ifade
- deyim sırası permütasyonları
- işaretçi aritmetiği ↔ dizi indeksi

Ölçüt **bayt değil komut hizalaması** olmalı — bu oturumda bir varyant
baytı 140→133 indirirken hizalamayı 85/98'den 76/98'e düşürdü.

**İş B — dağıtım danışmanı**
`docs/COMPILER.md`'deki formül belgeli:
`öncelik = floor_log2(ref) × ref / ömür`. Bir diff verildiğinde hangi
değişkenin referans sayısının değişmesi gerektiğini hesaplayan araç.
Riskli: `old_agbcc -dl/-dg` dökümü bu depoda boş çıkıyor, formül dolaylı
ölçümle doğrulandı.

**Beklenti:** belirsiz ama getirisi en yüksek iş. Başarılı olursa Faz 3 ve 4
açılır; olmazsa proje ~%7'de tavan yapar.

---

## Faz 3 — Orta fonksiyonlar (65–512 bayt)

716 fonksiyon, 127.533 bayt (kalanın %45'i). Faz 2'nin çıktısına bağlı.
Faz 2 çalışırsa buradan **%15–20 toplam kapsama** ulaşılabilir.

---

## Faz 4 — Büyük fonksiyonlar (513+)

125 fonksiyon, 140.267 bayt (kalanın %49'u). En büyük 43 fonksiyon tek
başına 82.889 bayt. Bunlar ancak Faz 2 tamamen çözülünce gerçekçi.

`menu_screen.c` bu sınıfın ilk örneği ve zaten %72 hizalamada — iyi bir
göstergesi.

---

## Yatay işler (paralel yürür)

**Davranışsal doğrulama (mGBA).** Şu an tek ölçütümüz byte-matching. mGBA +
GDB ile fonksiyon düzeyinde davranış testi, byte-matching olmayan ama
semantik olarak doğru kodu da doğrulanabilir kılar. `ROADMAP.md` Aşama 5.

**Adlandırma kapsaması.** 1405 fonksiyonun çoğu hâlâ `FUN_xxxxxxxx`.
Byte-matching'i etkilemez ama okunabilirliği ve çağrı grafiğini etkiler.
Kullanıcı bunu daha önce erteledi.

**RAM haritası.** `data/ram_map.csv` büyüyor ama çoğu `provisional`.

---

## Sıradaki somut adım

**Faz 0'ı yaz ve çalıştır.** `tools/audit_boundaries.py` bir günlük iş değil,
birkaç saatlik; getirisi %25'lik veri hatasını temizlemek ve sonraki her
fazın boşa emeğini önlemek.

Ardından Faz 1'i sırayla hasat et — 32 küme hazır bekliyor.
