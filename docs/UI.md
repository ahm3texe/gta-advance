# İlk kullanıcı arayüzü haritası

Save/serileştirme katmanından hemen sonra `0x0800114C` adresinde menü öğelerini süzen ve yerleştiren bir fonksiyon başlıyor.

## `BuildActiveMenuItems` — `0x0800114C`

- Girdi yapısındaki `+0x14` öğe sayısını okur.
- `+0x18` konumundan başlayan 32-byte öğe kayıtlarını gezer.
- İsteğe bağlı görünürlük maskesiyle etkin öğeleri süzer.
- Etkin öğe işaretçilerini `0x020011B0` dizisine, sayıyı `0x020011A0` adresine yazar.
- Menü genişliğini en fazla sekiz öğe üzerinden hesaplayıp yatay konumu `0x02001414` adresine yazar.
- Son yerleşim adımını `0x08001E1C` fonksiyonuna devreder.

İsim davranışa göre verilmiş geçici bir semboldür. Fonksiyon ve literal havuzu `0x0800114C–0x080011EB` boyunca 160/160 byte matching'dir.

## `DrawMenuItems` — `0x080011EC`

Etkin öğe listesini çizer, seçili öğe için farklı stil kullanır ve sayısal değerleri ondalık basamaklara ayırıp `$` önekli metne dönüştürür. Fonksiyon ve literal havuzları `0x080011EC–0x080013AB` boyunca 448/448 byte matching'dir.

## `InitMenuScreen` — `0x080013AC`

Blend register'larını ayarlar, palette verisini DMA3 ile palette RAM'e yollar, VRAM'i temizler ve menüyle ilişkili alt sistemleri başlatır. Fonksiyon ve literal havuzu `0x080013AC–0x08001457` boyunca 172/172 byte matching'dir.

## `RunMenuScreen` — `0x08001458`

İlk Ghidra analizi bu büyük fonksiyonun menü kimliğine göre menü ağacını kurduğunu, giriş bitlerini işlediğini, alt menülere girip çıktığını ve seçim değişince `DrawMenuItems` çağırdığını gösteriyor. İsim ve ayrıntılı sınırlar geçicidir; fonksiyon henüz matching değildir.

`ResetMenuState`, `IsMenuFlagSet` ve `FinalizeMenuLayout` yardımcıları `0x08001DC0–0x08001E2F` aralığında 112/112 byte matching'dir.

`LoadMenuGraphics`, `ClearMenuVram` ve iki küçük display-control sarmalayıcısı `0x08001E30–0x08001F03` aralığında 212/212 byte matching'dir. Bu blok menü grafik kaynağını açar, iki palette aralığını DMA3 ile yükler, VRAM'i doldurur ve `DISPCNT=0x0101` ayarını uygular.
