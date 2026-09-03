# Yol haritası

Bu belge *ne yapılacağını* ve *neden o sırayla* yapılacağını tutar.
[ROADMAP.md](ROADMAP.md) aşamaların tanımını, bu dosya güncel planı verir.

## Nerede duruyoruz

```
Doğrulanmış ROM:   7.872 bayt, 36 bölge
Matching kod:      6.830 / 333.317 bayt  (%2.05)
Byte-matching:     100 / 1462 fonksiyon
```

> **Faz 0 tamamlandı.** Yüzde %2.34'ten %2.05'e *düştü* çünkü payda
> düzeldi: Ghidra fonksiyonları kesik saydığı için toplam kod boyutu
> 41.782 bayt eksik ölçülüyordu. Kapsama kaybedilmedi, ölçüt dürüstleşti.

## Kalanın dağılımı — planın dayanağı

1362 fonksiyon, 326.487 bayt (Faz 0 sonrası, düzeltilmiş sınırlarla):

| Boyut | Adet | Bayt | Kalanın payı |
|---|---|---|---|
| ≤64 | 462 | 15.518 | %4.8 |
| 65–256 | 549 | 73.478 | %22.5 |
| 257–512 | 163 | 58.321 | %17.9 |
| 513+ | 148 | 179.090 | **%54.9** |

Tek cümleyle: **kalanın yarısı 513 bayttan büyük fonksiyonlarda.** Küçük
fonksiyonları toplamak bizi ancak ~%5-6'ya taşır; ötesi büyükleri çözmeyi
gerektirir.

---

## Faz 0 — Fonksiyon haritasını doğru kur

Bu faz iki iş içeriyordu; başta yalnızca birincisini görmüştüm.

### 0a. Sınır denetimi ✅

`tools/audit_boundaries.py`: **647 sınır düzeltildi, 43 sahte kayıt
silindi.** 1505 → 1462 kayıt, 291.535 → 333.317 bayt.

### 0b. Eksik fonksiyon keşfi ✅ (kısmen)

**Bunu baştan planlamamıştım — hata buydu.** Sınırları düzeltmek yetmez;
Ghidra fonksiyonların bir kısmını *hiç görmemiş*.

`tools/discover_functions.py` iki yöntem kullanıyor:

- **Çağrı hedefi (kesin):** bilinen kodun içindeki her `bl` hedefi tanım
  gereği bir fonksiyon girişidir. Yanlış pozitif olamaz, yaprak
  fonksiyonları da bulur. → **49 fonksiyon**
- **Prolog deseni (olası):** boşluklarda `push {..,lr}` araması, dört
  sıkı filtreyle. → **358 fonksiyon**

Sonuç: 1466 → **1873 fonksiyon**, 338.163 → **419.717 bayt**.

### Yüzdenin üç kez düşmesinin nedeni

| Ne zaman | Payda | Oran |
|---|---|---|
| Başlangıç | 291.535 | %2.34 |
| Sınır denetimi sonrası | 338.163 | %2.05 |
| Prolog keşfi sonrası | 413.699 | %2.38 |
| Çağrı hedefi keşfi sonrası | 419.717 | %2.35 |

Kapsama hiç kaybedilmedi; her seferinde **payda gerçeğe yaklaştı**.
Ders: payda güvenilir değilse yüzde de değil. Harita işi kapsama işinden
**önce** bitmeliydi.

### Faz 0'da kalan iş

1. **51 sınır hatası.** `bl` hedefi bilinen bir fonksiyonun *içine*
   düşüyor — ya iki fonksiyon tek kayıtta birleşmiş ya sınır yanlış.
   Örnek: `0x080026F8`, `0x080031EA`, `0x08005532`.
2. **48.816 bayt boşluk** (%10,5) — 27 tanesi 256 bayttan büyük
   (28.228 bayt). Veri mi kod mu ayrılmalı.
3. **ARM bölgeleri** kabaca ölçüldü (~8,4 KB, dört aralık); gerçek
   sınırları çıkarılmalı.
4. **Dolaylı çağrılar.** Yalnızca fonksiyon işaretçisiyle çağrılan
   fonksiyonlar `bl` taramasında görünmez; atlama tabloları ve
   işleyici dizileri ayrıca taranmalı.

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
