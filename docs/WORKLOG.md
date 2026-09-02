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
