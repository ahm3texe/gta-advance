# Çalışma günlüğü

## 2026-09-02 — İlk büyük tersine mühendislik geçişi

### Ortam ve koruma

- Avrupa ROM'u SHA-1 ile doğrulandı; ROM ve save/state çıktıları Git dışında tutuldu.
- Ghidra 12.1.3, OpenJDK 21, mGBA 0.10.5 ve ARM GNU 16.2 araç zinciri hazırlandı.
- Ghidra ARMv4T projesi, otomatik fonksiyon haritası, decompile dışa aktarımı ve CSV senkronizasyonu kuruldu.

### Haritalanan modüller

- ARM giriş, IRQ dispatcher, VBlank/VCount ve interrupt tablosu.
- `GameInit` başlangıç/ana döngü iskeleti.
- `CRAWSAVE` metadata katmanı ve Nintendo `EEPROM_V124` kullanan düşük/yüksek seviyeli save fonksiyonları.
- Üç oyun kayıt slotu, marker/tümleyen checksum düzeni ve little-endian serileştirme yardımcıları.
- İlk UI menü öğesi süzme, çizim, ekran başlatma ve grafik yükleme yardımcıları.
- Büyük `RunMenuScreen` giriş/alt menü akışı ilk kez belgelendi; henüz matching değil.

### Doğrulanmış ölçüm

- Ghidra fonksiyon adayı: 1497.
- Byte-matching fonksiyon: 42.
- Matching fonksiyon gövdesi: 4660 / 290837 byte (`1,60%`).
- Literal/padding dahil matching ROM alanı: 5340 benzersiz byte.
- En büyük kesintisiz matching aralık: `0x080000C0–0x08001457`, 5016 byte.
- İkinci matching UI aralığı: `0x08001DC0–0x08001F03`, 324 byte.

### Otomatik doğrulama

- `make progress`: fonksiyon ve ROM-bölgesi metriklerini gösterir.
- `make matching`: 21 ikili parçayı yeniden derler, her parçayı ROM ile karşılaştırır ve birleşik aralık raporu üretir.
- `make doctor`: ROM hash'ini ve gerekli araçların kurulu olduğunu denetler.
- `make dashboard-dev`: 1497 fonksiyonun büyüklük, durum ve modül bilgilerini etkileşimli treemap üzerinde gösterir; arama, filtreleme ve ayrıntı paneli sağlar.
- `make dashboard-build`: güncel CSV verisini üretip dashboard'un dağıtım derlemesini doğrular.

### Sıradaki teknik hedef

1. `RunMenuScreen` (`0x08001458`) kontrol akışını ve input/action tablolarını kesinleştirmek.
2. `0x08001F04` sonrası UI yardımcılarını sınıflandırmak.
3. mGBA breakpoint/watchpoint oturumuyla menü değişkenlerini dinamik olarak doğrulamak.
4. Compiler parmak izini belirleyip uygun matching assembly parçalarını okunabilir C'ye taşımak.

## 2026-09-03 — Derleyici kimliği çözüldü, C'ye geçiş başladı

### Bulgu

ROM'un **`old_agbcc`** ile derlendiği byte düzeyinde kanıtlandı. Bu, projenin
semantik yeniden inşaya mecbur olmadığı, C'den byte-matching hedefleyebileceği
anlamına gelir.

Kanıt zinciri:

1. **Kod kalıpları.** Doğrulanmış 42 fonksiyonda register kopyalama 84 kez
   `adds rX, rY, #0` (agbcc kalıbı), 0 kez `movs rX, rY` (modern kalıp);
   fonksiyondan dönüş 28 kez `pop {rN}` + `bx rN`, 0 kez `pop {..., pc}`.
2. **Byte doğrulaması.** `src/save/save_helpers.c` yazıldı; 6 fonksiyonun 5'i
   ROM ile birebir eşleşti. `WriteU32LE`'nin 28 byte'ı, maskeyi literal
   havuzdan okumak yerine iki kez `mov`+`lsl` ile kurma gibi ayırt edici bir
   tercihle birlikte tam eşleşti.
3. **Varyant ayrımı.** Altı kombinasyon denendi: `agbcc -O2/-O1` 3/6,
   **`old_agbcc -O2/-O1` 5/6**, her ikisi `-O0` 0/6. Aynı C kaynağı, tek satır
   değişiklik olmadan.

Bayraklar: `old_agbcc -mthumb-interwork -O2 -fhex-asm`.

### Açık kalan

`WriteU16LE` (0x08001124): ROM girişte anlamsal olarak gereksiz bir 16-bit
kırpma yapıyor, `old_agbcc` bunu eliyor. Dokuz farklı C biçimi denendi,
hiçbiri tutmadı; denenenler kaynak dosyada listeli. Assembly kaynağı geçerli
kalıyor.

### Eklenen araçlar

- `make agbcc` — derleyiciyi yerelde üretir (`tools/setup_agbcc.sh`).
  agbcc 1998 dönemi C kaynağı olduğu için modern clang uyumluluk sarmalayıcısı
  gerekiyor; betik bunu kuruyor.
- `make c-match FILE=...` — C dosyasındaki her fonksiyonu ROM ile karşılaştırır.
- `make diff FILE=... FUNC=...` — tek fonksiyonun ROM halini derlenmiş haliyle
  yan yana, komut komut gösterir. Eşleşmeyen fonksiyonu düzeltirken "nerede
  sapıyor" sorusunun cevabı.
- `make doctor` artık derleyiciyi de denetliyor.

objdiff değerlendirildi ama kurulmadı: iki *nesne dosyası* karşılaştırıyor,
yani ROM tarafında da hedef `.o` üreten bir splat/dtk boru hattı gerektiriyor.
Bu kurulana kadar `make diff` aynı işi bizim veri modelimizle yapıyor.

### Depo hijyeni

İlk commit atıldı (154 dosya). Git dışında tutulanlar: ROM, Ghidra projesi,
Ghidra decompiler çıktısı (`analysis/decompiler/`), üretilen dashboard verisi
ve agbcc ikilileri. Decompiler çıktısı ROM'dan türetilmiş materyal olduğu için
deponun kendi yayın politikasına uygun biçimde yerelde bırakıldı.

### Sıradaki teknik hedef

1. `save_helpers` bloğundaki kalan `EraseSaveSlot`, `GetSaveSlotHeader` ve
   `WriteU16LE`'yi C'ye taşımak; blok tamamlanınca `matching_regions.csv`'yi
   `.c` build'ine çevirip `save_helpers.s`'i kaldırmak.
2. Yaprak fonksiyonlardan devam ederek save ve ui modüllerini C'ye taşımak.
3. mGBA yamalama/çalıştırma döngüsü — byte-matching olmayan fonksiyonlar için
   davranışsal doğrulama.
4. `RunMenuScreen` kontrol akışı.
