# Interrupt dispatcher haritası

## `IntrMain` — `0x08000104`

`AgbMain`, bu ARM fonksiyonunun adresini BIOS kullanıcı IRQ vektörü `0x03007FFC` konumuna yazar. Dispatcher toplam 276 byte kod ve 8 byte literal havuzundan oluşur.

İşlem sırası:

1. `REG_IE` ve `REG_IF` değerlerini `0x04000200` üzerinden birlikte okur.
2. `REG_IME` (`0x04000208`) değerini saklayıp interrupt işlemeyi geçici olarak sınırlar.
3. Etkin ve bekleyen interrupt maskesini `IE & IF` olarak hesaplar.
4. Aşağıdaki öncelik sırasındaki ilk kaynağı seçer.
5. Seçilen IF bitini acknowledge eder.
6. Handler ofsetini `0x02000284` adresine kaydeder.
7. `0x02000170 + handler_offset` tablosundaki ARM/Thumb handler işaretçisini çağırır.
8. CPU, IE/IF/IME, register ve SPSR durumunu geri yükler.

## Öncelik ve tablo ofsetleri

| Öncelik | IF biti | Kaynak | Handler ofseti |
|---:|---:|---|---:|
| 1 | `0x0001` | VBlank | `0x04` |
| 2 | `0x0004` | VCount | `0x0C` |
| 3 | `0x0002` | HBlank | `0x10` |
| 4 | `0x0008` | Timer 0 | `0x14` |
| 5 | `0x0100` | DMA 0 | `0x18` |
| 6 | `0x0200` | DMA 1 | `0x1C` |
| 7 | `0x0400` | DMA 2 | `0x20` |
| 8 | `0x0800` | DMA 3 | `0x24` |
| 9 | `0x1000` | Keypad | `0x28` |
| 10 | `0x2000` | GamePak | `0x2C`; özel durum |

GamePak interrupt algılanırsa `SOUNDCNT_X` (`0x04000084`) sıfırlanır ve kod sonsuz döngüye girer; normal handler çağrısı yapılmaz.

Timer 1–3 ve Serial bitleri bu tarama zincirinde doğrudan seçilmiyor. `0x20C0` maskesinin IE geri-yazımındaki etkisi sonraki handler tablosu analiziyle netleştirilecek.

## `InitInterrupts` — `0x0800038C`

Başlangıçta handler tablosunu ve donanımı şu şekilde kurar:

- `REG_IME` kapatılır.
- `0x02000170` adresindeki 13 girişlik tablo varsayılan `0x08000735` Thumb handler'ıyla doldurulur.
- VBlank slotuna `VBlankIntr` (`0x08000221`) yazılır.
- Timer 0 slotuna `0x0800079D` Thumb handler'ı yazılır.
- DMA3, `IntrMain`in ROM'daki `0x08000104` adresinden EWRAM `0x020004D0` adresine kopyalanması için kullanılır.
- BIOS IRQ vektörü `0x03007FFC`, EWRAM kopyasına yönlendirilir.
- `REG_DISPSTAT = 0x3228`, `REG_IE = 0x0005` ve son olarak `REG_IME = 1` ayarlanır.

Okunabilir kaynak `src/bootstrap/init_interrupts.s` içindedir. `make init-interrupts-match` komutu ROM'daki `0x00038C–0x00042F` aralığıyla **164/164 byte MATCH** sonucunu verir.

## Yardımcı handler'lar — `0x08000730–0x080007B3`

- `NoOpVBlankFinalize` ve `DummyIntr`: iki byte'lık dönüş fonksiyonları.
- `RunVBlankTransfers`: VBlank içinde görülen grafik/aktarım alt çağrılarını ortak bir yoldan çalıştırır; IWRAM kare/gecikme sayacını sınırlar.
- `NoOpInterruptHelper`: iki byte'lık ikinci boş yardımcı.
- `VCountIntr`: `0x080327C8` alt rutinini çağırır ve `REG_IF` üzerindeki VCount bitini acknowledge eder.

Bu blok `src/interrupt/irq_helpers.s` ile **132/132 byte MATCH**.

## `ResetDisplayAndInterrupts` — `0x080007B4`

DMA3 ile VRAM ve OAM'i sıfırlar, VBlank/display durumunu hazırlar, BIOS IRQ bayrağını günceller ve `InitInterrupts`i yeniden çağırır. `src/bootstrap/reset_display_interrupts.s` ile gövde ve literal havuzu dahil **120/120 byte MATCH**.

## Matching durumu

Okunabilir kaynak `src/bootstrap/intr_main.s` içindedir. `make intr-match` komutu fonksiyon gövdesi ve literal havuzunu ROM'daki `0x000104–0x00021F` aralığıyla karşılaştırır; sonuç **284/284 byte MATCH**.
