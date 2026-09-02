# VBlank interrupt analizi

## `VBlankIntr` — `0x08000220`

Bu Thumb fonksiyonu her dikey boşluk interrupt'ında çalışır. Ghidra'nın fonksiyon gövdesi ölçümü 312 byte; aralara yerleşmiş literal havuzlarıyla birlikte yeniden üretilen sürekli ROM aralığı 364 byte'tır.

Doğrulanan yüksek seviyeli akış:

1. `0x02035CA8` ve IWRAM `0x03000004` kare sayaçlarını artırır.
2. `0x02000EB8` interrupt derinlik/yeniden giriş sayacını artırır.
3. Sayaç `1` değilse ağır güncellemeleri atlayarak çıkış yoluna gider.
4. En az iki sabit kare-başı alt sistemi çağırır; `0x02000D08` etkinse üçüncü isteğe bağlı alt sistemi çalıştırır.
5. `REG_VCOUNT` (`0x04000006`) ve `0x02000130` durum bayrağına göre grafik/aktarımı yöneten iki güncelleme yolundan birini seçer.
6. IWRAM'deki kare/gecikme değerini belirli koşullarda `5` ile sınırlar.
7. Ortak bitiş fonksiyonunu çağırır, yeniden giriş sayacını azaltır ve BIOS IRQ bayrağında VBlank bitini `0x03007FF8` üzerinden işaretler.

Fonksiyon içindeki 10 benzersiz alt çağrının gerçek isimleri henüz bilinmiyor; donanım register erişimleri ve mGBA gözlemleriyle isimlendirilecek.

## Matching durumu

Okunabilir Thumb kaynağı `src/interrupt/vblank_intr.s` içindedir. `make vblank-match` komutu ROM'daki `0x000220–0x00038B` aralığıyla **364/364 byte MATCH** sonucu verir.

