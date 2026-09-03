# Yol haritası

Güncelleme: 2026-09-03. Uzun vadeli hedef tanımı [ROADMAP.md](ROADMAP.md)'de;
bu dosya *güncel durumu* ve *sıradaki işi* tutar. Çalışma kuralları
[WORKFLOW.md](WORKFLOW.md)'de, ölçülmüş derleyici davranışı (32 kural)
[COMPILER.md](COMPILER.md)'de.

## Şu an neredeyiz (ölçülmüş)

```
Fonksiyon haritası:   1.978 fonksiyon / 433.403 bayt kod
Byte-matching:        205 fonksiyon / 9.864 bayt   (%2,28)
Doğrulanmış ROM:      11.264 bayt (96 bölge) + 448 bayt libc = 11.712
Harita boşluğu:       35.146 bayt (%7,5) — sınıflandırılmamış
Park (yazıldı,
eşleşmedi):           16 fonksiyon — Faz 2 test korpusu
make rom:             SHA-1 birebir, her commit'te doğrulanıyor
```

### Yüzde neden üç kez düştü

| Aşama | Payda | Görünen oran |
|---|---|---|
| Başlangıç | 291.535 | %2,34 |
| Sınır denetimi sonrası | 338.163 | %2,05 |
| Keşif sonrası | 413.699 | %2,38 |
| Kuyruk-çağrısı bölmesi sonrası | 433.403 | %2,28 |

Kapsama hiç düşmedi; **payda gerçeğe yaklaştı**. Ghidra kodun ~%33'ünü ya
hiç görmemiş ya yanlış sınırlamıştı. Ders: harita işi kapsama işinden önce
bitmeli — bu yüzden Faz 0 aşağıdaki gibi genişletildi.

---

## Faz 0 — Fonksiyon haritası (BÜYÜK ÖLÇÜDE TAMAM)

Yapılan: sınır denetimi (647 düzeltme, 43 silme) → üç yöntemli keşif
(`bl` hedefi kesin, fonksiyon işaretçisi güçlü, prolog deseni olası;
+460 fonksiyon) → kuyruk-çağrısı bölmesi (32 kayıt → 54 fonksiyon).
Keşif artık **0 yeni** veriyor, "gövde içine düşen çağrı hedefi" uyarısı
**0**.

### Kalan üç iş (kapanış ölçütü: boşlukların tamamı kod/veri etiketli)

1. **Atlama tablosu taraması.** `mov pc, rN` tablolarının girdileri ayrı
   fonksiyonlara işaret edebilir; sadece dolaylı çağrıyla ulaşılanlar üç
   yöntemin hiçbirinde görünmez.
2. **ARM bölgeleri.** ~8,4 KB dört aralık kabaca ölçüldü
   (`audit_boundaries.py ARM_RANGES`); gerçek sınırlar ve fonksiyon
   girişleri çıkarılmalı. Muhtemelen ses sürücüsü.
3. **35 KB boşluk sınıflandırması.** 7 boşluk 512 B'den büyük (12,3 KB) —
   önce bunlar. Veri mi (grafik/tablo/metin) ölü kod mu etiketlenmeli.

Tahmini gerçek toplam: **~2.000–2.050 fonksiyon**. 1.978 bir alt sınır.

---

## Faz 1 — Küçük fonksiyon hasadı (SÜRÜYOR, verimli)

Havuz (eşleşmemiş, oyun kodu):

| Boyut | Adet | Bayt | Kalanın payı |
|---|---|---|---|
| ≤64 | 625 | 19.552 | %4,6 |
| 65–256 | 716 | 94.284 | %22,3 |
| 257–512 | 223 | 79.931 | %18,9 |
| 513–1024 | 142 | 99.758 | %23,6 |
| 1025+ | 67 | 130.014 | %30,7 |

Yöntem kanıtlandı: bitişik yaprak kümeleri + **kanıtlanmış deyim
kütüphanesi**. Bu oturumda ~30 bölgenin çoğu ilk denemede tam eşleşti.
Tekrar kullanılan deyimler: taşma korumalı sayaç, çift bağlı liste, DMA
IME-sarmalı blok, karo işaretçi aritmetiği, 148/180-baytlık tablo girişi,
`ldmia` struct ataması.

Beklenti: ≤64 havuzunun %60–70'i mevcut yöntemle düşer (~12–14 KB, yani
toplam ~%5,5–6). Sonrası Faz 2'ye bağlı.

---

## Faz 2 — Sistemik engeller (ASIL BAHİS)

Orta/büyük fonksiyonlar %85–95 komut hizalamasına gelip hep aynı birkaç
engelde takılıyor. 16 dosyalık park korpusu tam bunun için — **doğru çözüm
tek dosyayı değil, sınıfın tamamını açar.**

### Test korpusu (güncel farklar, 2026-09-03)

| Dosya | Fark | Engel sınıfı |
|---|---|---|
| clear_text_area | 1/132 | B1 register dağıtımı |
| maybe_advance | 1/42 | B2 dal yönü (`bls`/`bhi`) |
| entity_query | 2/60 | B2 |
| scan_all | 2/62 | B2 (+operand sırası) |
| is_ram_mode | 4/28 | B2 normalizasyon |
| object_query:GetInnerId | 4/20 | B2 |
| object_query:ProbeObject | 5/58 | B2 |
| offset_helpers | 10/24 | B1 epilog register |
| actor_init | 14/192 | B1 |
| kind_scan | 25/48 | B4 havuz yerleşimi |
| history_push | 31/48 | B3 taban kopyalama |
| distance_accum | 38/72 | B3/B4 |
| bump_or_reset | 41/60 | B3 paylaşılan store |
| release_slot | 47/48 | B3 iki-taban |
| area_cleanup | 56/84 | B1/B4 |
| entity_action | 140/210 | B5 blok yerleşimi |
| menu_screen | 946/1456 | B1 (tüm atama kaymış) |

### Engel sınıfları

- **B1 Register dağıtım sırası.** Mekanizma belgeli
  (`öncelik = floor_log2(ref) × ref / ömür`) ama elle uygulamak deneme
  yanılma. → `alloc_advisor.py`: diff verildiğinde hangi değişkenin
  referans/ömür dengesinin değişmesi gerektiğini hesaplasın.
- **B2 Dal yönü / karşılaştırma normalizasyonu.** agbcc bazı dalların
  yönünü C ifadesinden bağımsız seçiyor. Beş biçim aynı çıktıyı verdi
  (maybe_advance). Sistematik varyant taraması gerekiyor; ölçüt bayt
  DEĞİL **komut hizalaması** (bayt sayısı dal ofsetleriyle yanıltıyor).
- **B3 Taban kopyalama / iki-taban.** ROM bir tabanı yükleyip kopyalıyor
  ya da `index*N`'i bir kez hesaplayıp iki ayrı sabit tabana ekliyor.
  Kural 17/22 yetmedi; yeni bir C biçimi keşfedilmeli.
- **B4 Havuz yerleşimi.** Literal havuzunun gövde içine gömülme noktası
  komut akışını değiştiriyor; C'den doğrudan denetlenemiyor.
- **B5 Blok yerleşimi.** `return 0` bloğunun erken/geç konması
  (entity_action). Cross-jump ve dal maliyeti etkileşimi.
- **B6 Ayrık sınıflar.** `-O0` derlenen LibraryAssert kümesi (dosya
  başına bayrak desteği gerekir — `agbcc_build.py`'ye küçük ekleme);
  çarpım faktörizasyonu (`x*28`, tune_adjust).

### İş listesi (sırayla)

1. `tools/sweep_variants.py` — mekanik dönüşümleri (dal çevir, erken
   return ayır, u8↔s8, yerel↔satıriçi, işaretçi↔dizi, bildirim sırası)
   otomatik uygula, **komut hizalamasıyla** puanla, korpus üzerinde koş.
   Bugüne kadar elle yazdığım tarama betiklerinin genelleştirilmişi.
2. `agbcc_build.py`'ye dosya başına bayrak → LibraryAssert (-O0) kümesini
   aç. Ucuz, bağımsız, ~200 B.
3. `tools/alloc_advisor.py` — B1 için referans/ömür hesabı. Riskli
   (`-dl/-dg` dökümleri bu depoda boş çıkıyor; dolaylı ölçümle).
4. B3 için hedefli araştırma: release_slot (47/48!) en net örnek — tek
   fonksiyon, tek desen.

**Karar kapısı:** bu dört işten sonra korpustan en az 8/16 dosya
kapanmazsa, Faz 3-4 beklentisi aşağı çekilir ve ağırlık davranışsal
doğrulamaya kayar (aşağıda).

---

## Faz 3 — Orta fonksiyonlar (65–512 B, 174 KB)

Faz 2 çıktısına bağlı. Sweep aracı + deyim kütüphanesiyle buradan
**toplam %15–20 kapsama** hedeflenebilir.

## Faz 4 — Büyük fonksiyonlar (513+ B, 230 KB, kalanın %54'ü)

Ancak Faz 2 tamamen çözülürse gerçekçi. `menu_screen.c` (503/694 komut)
sınıfın göstergesi: yapı doğru, tüm register ataması bir kaymış.

---

## Yatay işler (fazlara paralel)

- **Davranışsal doğrulama (mGBA).** ROADMAP Aşama 5. İlk somut adım:
  headless mGBA ile `out/gtaadvance.gba`'yı N kare koşturup kare hash'i
  karşılaştıran bir betik. Bugün bayt-birebir olduğumuz için önemsiz
  görünür ama Faz 2 başarısız olursa *semantik-doğru-ama-eşleşmeyen* C
  için tek doğrulama yolu bu olacak.
- **Varlık boru hattı.** ROM'un %97,2'si veri (15,5 MB). Decompile
  edilmez, çıkarılır: betikler depoda, çıkarılan varlıklar `.gitignore`'da
  yerel kalır (ROADMAP hukuki sınırı). Kod ilerledikçe formatlar zaten
  koddan çözülüyor; öncelik düşük.
- **Adlandırma kapsaması + çağrı grafiği.** 1.773 fonksiyon hâlâ
  `FUN_xxxxxxxx`. Byte-matching'i etkilemez; okunabilirlik işi.
  (Kullanıcı ertelemişti.)
- **`tools/rename_symbol.py`.** Yeniden adlandırma bu oturumda YEDİ kez
  başka dosyayı kırdı. Tek komutla tüm referansları güncelleyen araç.
- **Veri hijyeni.** `make check` artık dashboard JSON'unu da yeniliyor;
  park dosyaları `decompiled` statüsünde; bölge kayıtları `make matching`
  ile her commit'te denetleniyor.

---

## Dürüst zaman tahmini

Oturum başına ~2.500–3.000 bayt hasat hızıyla (bu oturum ölçümü):

| Senaryo | Sonuç |
|---|---|
| Faz 2 başarılı (≥8/16 korpus kapanır) | Orta fonksiyonlar açılır; %15–20 birkaç ayda, sonrası büyüklere bağlı. Tam byte-matching: **12–18+ ay**, düzenli çalışmayla |
| Faz 2 kısmen (3–7/16) | ~%8–10'da yavaşlama; büyükler kapalı kalır |
| Faz 2 başarısız (<3/16) | ~%6–8 tavan; proje "tam byte-matching"ten "byte-matching çekirdek + davranışsal-doğru gövde" hedefine döner |

Belirsizliğin kaynağı süre değil, **B1–B5'in çözülüp çözülemeyeceği**.
Karar kapısı bunu erken (birkaç oturum içinde) netleştirecek.

## Sıradaki somut adım

1. Faz 0 kalanları (atlama tablosu taraması en ucuzu) — yarım oturum
2. `sweep_variants.py` + korpus koşusu — Faz 2'nin ilk gerçek testi
3. Karar kapısına göre devam
