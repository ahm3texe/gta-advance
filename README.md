# GTA Advance (Europe) decompilation workspace

Bu depo, kullanıcının kendi sağladığı **Grand Theft Auto Advance Avrupa GBA ROM'u** üzerinde temiz ve ölçülebilir bir tersine mühendislik çalışması için hazırlanmıştır.

> Bu depo orijinal ROM'u, oyundan çıkarılmış telifli varlıkları veya Rockstar/Digital Eclipse kaynak kodunu dağıtmaz. `baserom.gba` yalnızca yerel doğrulama girdisidir ve Git tarafından yok sayılır.

## Şu anki durum

- ROM doğrulandı: 16 MiB, başlık `GTA ADVANCE`, oyun kodu `BGTP`, sürüm `0`.
- Avrupa ROM SHA-1: `06230842626da504f92396074f7c655e100f5d44`
- GBA giriş dalı hedefi: `0x080000C0`.
- Güncel Ghidra haritası: 1.497 fonksiyon adayı, yaklaşık 290 KiB aday fonksiyon gövdesi.
- Byte-matching başlangıç/IRQ kaynakları: `AgbMain`, `IntrMain`, `VBlankIntr`, `InitInterrupts`, dört IRQ yardımcısı, `VCountIntr` ve `ResetDisplayAndInterrupts`.
- EEPROM/save modülü ve serileştirme yardımcıları `0x0800082C–0x0800114B` boyunca kesintisiz 2336/2336 byte matching'dir; ayrıntı [SAVE_SYSTEM.md](docs/SAVE_SYSTEM.md) içindedir.
- ROM girişinden ilk UI başlatma fonksiyonunun sonuna kadar `0x080000C0–0x08001457` aralığı kesintisiz 5016/5016 byte yeniden üretilmektedir.
- Uzak menü yardımcılarıyla birlikte toplam 5340 benzersiz ROM byte ve 42 fonksiyon byte-matching'dir.
- Ek olarak 448 byte standart kütüphane bölgesi agbcc `libc.a`'ya karşı doğrulanmıştır; toplam doğrulanmış ROM alanı **5788 byte**.
- Kaynağın C'ye taşınması sürüyor: matching byte'ların **%25.5**'i artık okunabilir C'den üretiliyor, kalanı hâlâ assembly.
- Derleyici `old_agbcc` olarak doğrulandı; yeni kaynaklar doğrudan C ile yazılıyor ve assembly karşılıkları blok tamamlandıkça emekli ediliyor. Ayrıntı [COMPILER.md](docs/COMPILER.md).
- **Derleyici kimliği çözüldü: agbcc.** ROM'un Nintendo'nun GBA SDK'sıyla gelen GCC 2.8.1 türevi ile derlendiği byte düzeyinde doğrulandı — `ReadU8`, `WriteU8` ve 28 byte'lık `WriteU32LE` doğrudan C'den birebir üretiliyor. Ayrıntı ve kanıt [COMPILER.md](docs/COMPILER.md) içinde. Bu, projenin C'den byte-matching hedefleyebileceği anlamına gelir.
- Makinede Git, Make, Python 3, Ghidra 12.1.3, OpenJDK 21, mGBA 0.10.5, ARM GNU araç zinciri 16.2 ve agbcc var.

## İlk adım

ROM'u yerel ve Git-dışı `baserom.gba` dosyasına hazırlayın:

```sh
make prepare-rom ROM_ZIP="/Users/muhammetyildirim/Downloads/Grand Theft Auto Advance (Europe) (En,Fr,De,Es,It).zip"
```

Ardından ortam raporunu ve ilerlemeyi görün:

```sh
make doctor
make progress
make matching
```

Sonraki teknik odak, belgelenmiş büyük `RunMenuScreen` fonksiyonunun sınırlarını ve girdi/eylem tablosunu kesinleştirmek; ardından grafik, giriş ve dünya alt sistemlerine geçmektir. Ayrıntılı sıra [ROADMAP.md](docs/ROADMAP.md) dosyasındadır.

Başlangıç analizi [BOOT_SEQUENCE.md](docs/BOOT_SEQUENCE.md), mevcut açık çalışma ve ROM içi iz araştırması [PRIOR_ART.md](docs/PRIOR_ART.md), geçici RAM sembolleri ise [ram_map.csv](data/ram_map.csv) içindedir.

## “Kaynak kodunu çıkarmak” ne demek?

ROM'un içinde orijinal C kaynak dosyaları, değişken adları ve yorumlar bulunmaz. Hedefimiz makine kodunu analiz ederek eşdeğer C/ARM assembly yazmak ve üretilen ROM'u orijinalle karşılaştırmaktır. Orijinal kaynak kodun birebir isimleri ve yorumları geri getirilemez; davranış ve mümkünse byte eşleşmesi yeniden kurulabilir.

## İlerleme kaydı

Ghidra adayları [functions.csv](data/functions.csv) dosyasına otomatik yazılır; doğrulanmış isim ve durumlar [function_overrides.csv](data/function_overrides.csv) içinde tutulur. Durumlar:

- `candidate`: Ghidra otomatik olarak buldu, henüz insan/araç çapraz kontrolü yok.
- `discovered`: adres, mod ve sınır yeterli güvenle doğrulandı.
- `documented`: davranış ve çağrılar belgelendi.
- `decompiled`: C karşılığı yazıldı ama byte eşleşmesi yok.
- `matching`: derlenen çıktı hedef assembly ile eşleşiyor.

`make progress` güncel özeti üretir. Literal havuzları dahil doğrulanmış gerçek ROM parçaları [matching_regions.csv](data/matching_regions.csv) içinde tutulur; `make matching` hepsini yeniden derleyip ROM'a karşı denetler. Oturum günlüğü [WORKLOG.md](docs/WORKLOG.md) içindedir.

## C'ye taşıma iş akışı

Derleyici `old_agbcc` olarak doğrulandığı için yeni fonksiyonlar doğrudan C
ile yazılır; assembly yalnızca henüz eşleşmeyenler için geçerli kaynaktır.

Bir C dosyasındaki her fonksiyonu ROM ile karşılaştır:

```sh
make c-match FILE=src/save/save_helpers.c
```

Eşleşmeyen bir fonksiyonun nerede saptığını gör:

```sh
make diff FILE=src/save/save_helpers.c FUNC=WriteU16LE
```

Sol sütun ROM'daki gerçek kod, sağ sütun senin C'nden üretilen kod; farklı ve
eksik komutlar işaretlenir. Eşleşmeyen fonksiyonlarda **assembly'yi değil C'yi**
değiştirirsin — hangi C biçiminin hangi assembly'yi ürettiğini öğrenmek işin
kendisidir.

Derleyiciyi kurmak için (ikililer depoya girmez):

```sh
make agbcc
```

Bir blok tamamen C'den eşleşene kadar `data/matching_regions.csv` assembly
kaynağını kullanmaya devam eder; böylece build hiçbir aşamada bozulmaz.

### Assembly kaynakları neden siliniyor

Bir fonksiyon C'den byte-matching olduğunda assembly karşılığı **build kaynağı
olmaktan çıkar**. İkisini birden tutmak build'de çift sembol üretir, dolayısıyla
biri devre dışı kalır — ve devre dışı kalan dosya artık hiçbir şey tarafından
doğrulanmadığı için sessizce eskir. Doğrulanmayan bir referans, referans değildir.

Orijinal kodu kaybetmezsin: kaynak zaten ROM'un kendisidir.

```sh
make disasm FUNC=WriteU32LE
```

Bu çıktı doğrudan `baserom.gba`'dan üretilir, bakım gerektirmez ve her zaman
doğrudur. Silinen assembly dosyaları ayrıca git geçmişinde durur.

**İstisna:** orijinalinde de elle assembly yazılmış olan kod — ARM modundaki
başlangıç ve IRQ dağıtıcısı (`src/bootstrap/agb_main.s`, `src/bootstrap/intr_main.s`)
— kalıcı olarak assembly kalır. Bunlar C'ye taşınmaz.

## Etkileşimli decomp haritası

`dashboard/`, decomp.dev benzeri yerel bir treemap sunar. Her dikdörtgen bir fonksiyondur; alanı fonksiyonun byte büyüklüğünü, rengi ise çalışma durumunu gösterir.

Renk anlamları decomp.dev ile hizalıdır:

| Renk | Anlam |
|---|---|
| Yeşil | Byte eşleşiyor |
| Mavi | Kaynak yazıldı, henüz eşleşmiyor (`decompiled`) |
| Turuncu | Belgelendi |
| Mor | Keşfedildi |
| Koyu gri | Henüz dokunulmadı (`candidate`) |

Gruplama üç şekilde yapılabilir:

- **Bitişik blok (varsayılan):** ROM'da art arda gelen fonksiyonlar aynı çeviri biriminden derlenmiş kabul edilerek kümelenir (`CLUSTER_GAP = 512` byte). Sembolü olmayan bir ROM'da decomp.dev'in dosya bazlı hiyerarşisinin en yakın dürüst karşılığıdır; sınıflandırılmamış 1451 fonksiyon 65 bloğa ayrılır.
- **Modül:** doğrulanmış `module` sütununa göre.
- **ROM bankı:** 64 KiB'lık adres blokları.

Sınıflandırılmış fonksiyonlar her zaman kendi modül grubunda kalır; kümeleme yalnızca yapısı henüz bilinmeyen bölgeye sınır getirir.

Renk ayrımı önemli: **"C'den eşleşiyor"** ile **"Assembly eşleşiyor"** farklı
tonlardadır. İkisi de ROM'u birebir üretir, ama yalnızca ilki okunabilir kaynak
üretir. Assembly transkripsiyonu ölçüyü yeşile boyar, projeyi ilerletmez;
gerçek hedef C'den eşleşmedir. `make c-status` bu ayrımı ölçer ve
[c_sources.csv](data/c_sources.csv) dosyasını üretir.

Fonksiyon başına eşleşme yüzdesi, fonksiyon aralığının [matching_regions.csv](data/matching_regions.csv) ile kesişen byte oranından hesaplanır — durum etiketinden türetilmez. Bugün her fonksiyon ya tamamen bir doğrulanmış bölgenin içinde ya da tamamen dışında olduğu için değerler %0 veya %100'dür; kısmi eşleşme çıktığında ara değerler kendiliğinden görünür.

İlk kurulumdan sonra haritayı güncel veriyle açmak için:

```sh
make dashboard-dev
```

### Çalışırken canlı izleme

İkinci bir terminalde izleyiciyi başlat:

```sh
make dashboard-watch
```

`data/*.csv` veya veri üreteci her değiştiğinde JSON yeniden üretilir. Sayfa
JSON'u statik olarak import ettiği için Vite HMR haritayı **sayfa yenilenmeden**
günceller: bir fonksiyon eşleştiğinde veya adlandırıldığında dikdörtgen anında
rengini değiştirir.

Dağıtım derlemesini doğrulamak için:

```sh
make dashboard-build
```

Harita verisi [functions.csv](data/functions.csv) ve [matching_regions.csv](data/matching_regions.csv) dosyalarından otomatik üretilir. Decomp durumu değiştikçe `make dashboard-data` JSON anlık görüntüsünü yeniler; `dashboard-dev` ve `dashboard-build` bunu zaten otomatik yapar.
