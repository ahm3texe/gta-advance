# Yol haritası

Güncelleme: 2026-09-03. Uzun vadeli hedef tanımı [ROADMAP.md](ROADMAP.md)'de;
bu dosya *güncel durumu* ve *sıradaki işi* tutar. Çalışma kuralları
[WORKFLOW.md](WORKFLOW.md)'de, ölçülmüş derleyici davranışı (32 kural)
[COMPILER.md](COMPILER.md)'de.

## Şu an neredeyiz (ölçülmüş)

```
Fonksiyon haritası:   1.974 fonksiyon / 431.116 bayt kod
Byte-matching:        206 fonksiyon / 9.886 bayt   (%2,29)
Doğrulanmış ROM:      11.288 bayt (80 bölge) + 448 bayt libc = 11.736
Harita boşluğu:       ~35 KB (%7,5) — sınıflandırılmamış
Park (yazıldı,
eşleşmedi):           17 fonksiyon — Faz 2 test korpusu
Tutarlılık:           `make consistency` TEMİZ
make rom:             SHA-1 birebir, her commit'te doğrulanıyor
```

### Yüzde neden üç kez düştü

| Aşama | Payda | Görünen oran |
|---|---|---|
| Başlangıç | 291.535 | %2,34 |
| Sınır denetimi sonrası | 338.163 | %2,05 |
| Keşif sonrası | 413.699 | %2,38 |
| Kuyruk-çağrısı bölmesi sonrası | 433.403 | %2,28 |
| Hayalet/yinelenen temizliği sonrası | 431.116 | %2,29 |

Kapsama hiç düşmedi; **payda gerçeğe yaklaştı**. Ghidra kodun ~%33'ünü ya
hiç görmemiş ya yanlış sınırlamıştı. Ders: harita işi kapsama işinden önce
bitmeli — bu yüzden Faz 0 aşağıdaki gibi genişletildi.

---

## Faz 0 — Fonksiyon haritası (ANALİZLER TAMAM, UYGULAMA KISMİ)

Yapılan: sınır denetimi (647 düzeltme, 43 silme) → üç yöntemli keşif
(`bl` hedefi kesin, fonksiyon işaretçisi güçlü, prolog deseni olası;
+460 fonksiyon) → kuyruk-çağrısı bölmesi (32 kayıt → 54 fonksiyon).
Keşif artık **0 yeni** veriyor, "gövde içine düşen çağrı hedefi" uyarısı
**0**.

**Veri bütünlüğü ayrıca onarıldı** (denetim bulguları): 2 yinelenen kayıt,
2 hayalet kayıt (ROM'da çağıranı yok + gövde ortası), 2 fazla uzamış sınır,
`ram_map`'te EWRAM'ı 9,2 MB aşan boyut ve aynı adrese iki isim veren 3
sembol. `tools/check_consistency.py` bu sınıfların hepsini artık `make
check`'te yakalıyor.

### Faz 0 analizleri — üçü de ölçüldü

**1. Atlama tablosu taraması — TAMAM.** Kod bölgesinde 101 `mov pc, rN`,
bunların 93'ü gerçek switch tablosu (2.393 girdi, 734 benzersiz hedef).
93 tablonun 89'unun dağıtım komutu bilinen bir fonksiyonun içinde; 4'ü
sahipsiz. Hiçbir bilinen fonksiyona düşmeyen tablo girdisi: 20.
**5 yeni fonksiyon adayı** çıktı; üçü de aynı profilde: `push` prologu
yok (yaprak), ROM'un tamamında tek çağıranı yok — üç keşif yöntemimiz
de tam bu üç şarta dayandığı için yapısal olarak görünmezdiler.
*Not: kod oldukları kesin, ama girişin TAM yeri ±2 bayt belirsiz;
eklemeden önce tek tek disassemble edilmeli.*

**2. ARM bölgesi — ÖLÇÜLDÜ VE DÜZELTİLDİ.** `ARM_RANGES` sabiti yanlıştı:
dört ayrı aralık değil **tek bitişik bölge**, `[0x08067E04, 0x0806B84C)`
= **14.920 bayt**, belgelenen 8.384'ün 1,78 katı. Eski sabit yanlış
pozitif içermiyordu ama 6.524 baytı (%44) kaçırıyordu.

Kanıt (kendim ölçtüm): bölgedeki 3.730 kelimenin **tamamında** koşul
alanı `!= 0xF`; rastgele veri ya da Thumb'da ~1/16 kelimede `0xF`
beklenir. Hemen öncesi 0,9533 — sonrası 0,9747 oranında kalıyor, yani
sınırlar keskin. Üst sınır BIOS `swi` thunk'larının başı.

`0x08CA4514`'te bir **IWRAM bindirme tablosu** var; yapısı üçlüler
halinde *(IWRAM hedefi, ARM başı, ARM sonu)* ve ardışık girdiler
zincirleniyor (birinin sonu diğerinin başı). Bu, bölgenin gerçekten
çalıştırılan kod olduğunun bağımsız kanıtı.

`stmfd sp!` prologuyla **14 ARM fonksiyonu** ölçüldü ve haritaya eklendi
(6.792 B). Kalan ~8,1 KB kaydedici saklamayan ARM yaprakları — Thumb'daki
yaprak sorununun aynısı.

**İçerik ses sürücüsü DEĞİL** (ipucundaki hipotez yanlıştı): ROM'da
m4a/sappy imzası yok, bölgede tek bir ses yazmacı erişimi yok. Kod bir
**rasterleştirici**: afin doku eşleme, Cohen-Sutherland kırpma, 8bpp
döşeli çerçeve arabelleği adresleme.

**3. 35 KB boşluk sınıflandırması — TAMAM.** 35.146 baytın:
- **%55,1 (19.352 B) kod** — 200 yeni fonksiyon girişi (14.702 B) +
  4.650 B "kod kuyruğu", yani boyutu kısa yazılmış 29 fonksiyonun taşan
  gövdesi.
- **%42,3 (14.902 B)** kodun yapısal eklentisi: literal havuz, atlama
  tablosu, hizalama dolgusu.
- **yalnızca %2,5 (892 B)** sınıflandırılamayan veri.

512 B'den büyük 7 boşluğun (12.354 B) tamamı çözüldü. **Grafik/metin/veri
tablosu YOK** — `0x080000C0-0x08071E16` aralığındaki boşluklarda tek bir
yazdırılabilir metin bloğu ya da grafik verisi yok; ROM'un veri kısmı bu
aralığın dışında. Bu, asset'lerin ayrı bir bölgede toplandığını doğruluyor.

### Faz 0'dan çıkan iş listesi (uygulanmayı bekliyor)

Aşağıdakiler **ölçüldü ama haritaya işlenmedi** — her biri tek tek
doğrulama istiyor, toplu ekleme bu projede iki kez veri kaybettirdi:

- 200 boşluk fonksiyonu (61'i agbcc prologlu Thumb = düşük risk;
  123'ü prologsuz yaprak = orta risk; 16 ARM = risk yok sayılır)
- 29 fonksiyonun kısa yazılmış boyutu (4.650 B kuyruk)
- 5 atlama tablosu fonksiyonu (giriş yeri ±2 bayt doğrulanmalı)
- ~8,1 KB prologsuz ARM yaprağı

Bunlar işlenirse harita **~2.190 fonksiyona** çıkar. Şu anki 1.988 bir
**alt sınır**; kod paydası da (431.116 B) buna göre yeniden ölçülmeli.

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
