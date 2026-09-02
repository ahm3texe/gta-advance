# GBA başlangıç zinciri

Bu belge ilk otomatik analizden çıkarılan, henüz büyüyen başlangıç haritasıdır.

## `AgbMain` — `0x080000C0`

ROM başlığındaki ARM dalı bu adrese gelir. Fonksiyon:

1. CPU'yu IRQ moduna (`0x12`) geçirir ve IRQ stack pointer'ını `0x03007FA0` yapar.
2. CPU'yu System moduna (`0x1F`) geçirir ve ana stack pointer'ını `0x03007E00` yapar.
3. `IntrMain` adresini (`0x08000104`) GBA BIOS'unun kullanıcı IRQ işaretçisi olan `0x03007FFC` adresine yazar.
4. `0x08000431` işaretçisi üzerinden Thumb modundaki `GameInit` (`0x08000430`) fonksiyonuna dallanır.
5. `GameInit` geri dönerse başlangıca dönerek sistemi yeniden başlatır.

`src/bootstrap/agb_main.s` yeniden derlendiğinde fonksiyon gövdesi ve bitişik literal havuzu dahil ROM'daki `0x0000C0–0x000103` aralığıyla **68/68 byte eşleşir**. `make bootstrap-match` bu sonucu otomatik doğrular.

## `IntrMain` — `0x08000104`

ARM durumundaki kullanıcı interrupt dispatcher'ıdır. GBA interrupt bayraklarını okuyup uygun handler'a dallanır. Gövde ve literal havuzu byte-matching hale getirildi; ayrıntılı öncelik sırası [IRQ_DISPATCH.md](IRQ_DISPATCH.md) dosyasındadır.

## `GameInit` — `0x08000430`

Thumb durumundaki yüksek seviyeli başlangıç fonksiyonudur. Ghidra'nın ilk sınır tahmini 652 byte'tır; bu sınır manuel olarak doğrulanacaktır.

İlk decompile çıktısına ve literal sabitlere göre:

- `REG_WAITCNT` (`0x04000204`) yapılandırılıyor.
- DMA3 register bloğu (`0x040000D4`) kullanılarak EWRAM (`0x02000000`), IWRAM (`0x03000000`), VRAM (`0x06000000`) ve OAM (`0x07000000`) başlangıçta dolduruluyor/temizleniyor.
- BIOS VBlank interrupt bayrağı (`0x03007FF8`) etkinleştiriliyor.
- Başlangıçtan sonra 44 farklı alt fonksiyona ulaşan yüksek seviyeli oyun döngüsü kuruluyor.
- Döngü içinde giriş, grafik, ses, varlıklar ve oyun durumu olduğu düşünülen alt sistemler her karede çağrılıyor; kesin isimler dinamik test ve register erişimlerine göre verilecek.

İlk ham C-benzeri çıktı `analysis/decompiler/GameInit.c` dosyasındadır. Okunup adlandırılmış assembly karşılığı `src/bootstrap/game_init.s` içinde bulunur ve literal havuzlarıyla beraber `0x08000430–0x0800072F` aralığında **768/768 byte matching** durumundadır.

Başlangıç, IRQ, ekran sıfırlama, kayıt, serileştirme ve ilk UI fonksiyonlarının birleşmesiyle ROM'un `0x080000C0–0x08001457` aralığı kesintisiz **5016/5016 byte** yeniden üretilmektedir.

## Güven düzeyi

- Adresler ve ARM/Thumb modları: yüksek güven.
- `AgbMain` davranışı: yüksek güven ve byte-matching.
- `IntrMain` ve `GameInit` isimleri: işlevsel/geçici isimler; orijinal semboller değildir.
- `IntrMain` sınırı ve davranışı: yüksek güven ve byte-matching.
- `GameInit` sınırı ve makine kodu: yüksek güven ve byte-matching; içindeki henüz adlandırılmamış alt çağrıların rolleri geçicidir.
