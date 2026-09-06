# Harici belgelerden türetilen bulgular

## Bu dosya nedir, ne değildir

2003 tarihli iki geliştirme belgesi incelendi (bir tasarım/planlama dosyası ve
bir bütçe/kilometre taşı dosyası). Bu dosya **belgelerin kendisi değil**, onlardan
çıkardığımız ve mümkün olan yerde **ROM'a karşı doğruladığımız** sonuçları tutar.

Belgeler üreticiye ait, dağıtılamaz materyaldir. `docs/ROADMAP.md`'deki
hukuki sınır gereği belgeler, sayfa görüntüleri veya uzun alıntılar depoya
**girmez**. Buradaki her madde kendi cümlelerimizle yazılmış çıkarımdır.

Belgelerdeki banka/imza/kişisel bilgiler hiçbir yere aktarılmadı.

## ROM'a karşı DOĞRULANAN

**Kartuş 128 Mbit.** Belge böyle diyor, `baserom.gba` tam 16.777.216 bayt.

**Link kablosu kodu ROM'da var.** Belge 2 oyunculu link oyununu planlıyor.
Seri yazmaç sabitleri havuzda bulundu: `SIOCNT` (0x04000128) altı yerde,
`SIODATA32` (0x04000120) ve `RCNT` (0x04000134) birer yerde. Taşıyan
fonksiyonlar: FUN_080657d8 (2374 B), FUN_08066904, FUN_0806660c,
FUN_08066568, FUN_0806686c, FUN_08066a54. Toplam ~3,4 KB, hiçbiri yazılmamış.

**Giriş okuma iki fonksiyonda.** `KEYINPUT` (0x04000130) havuzda yedi kez;
sahipleri FUN_080656f4 (228 B) ve FUN_0806620c (320 B). İkisi de yazılmamış.

## ÇIKARIM — kod tabanı iki ekipten geliyor

Faturalar ve kilometre taşı formları, projenin 2002'de **Crawfish Interactive**
tarafından başlatıldığını (dosyalarda proje adı önce "Gang Wars", sonra
"GTA Advanced"), Aralık 2002'de **Digital Eclipse**'e devredildiğini gösteriyor.
İkinci ekibin belgesi "final ürün için planlanan iyileştirmeler" başlığını
taşıyor ve o tarihte oyunu %50 tamamlanmış sayıyor.

**Bizim için anlamı:** ROM tek bir ekibin tutarlı kod tabanı değil. Bu oturumda
ölçtüğümüz bir bulmacaya makul açıklama veriyor — `FUN_080543D0` ile
`FUN_08054744` neredeyse ikiz fonksiyonlar ama biri giriş korumalı `do/while`,
öteki döndürülmüş `for` derlenmiş (bkz. COMPILER.md kural 49 yan bulgusu).
Kuralı değiştirmez: **döngü biçimi her fonksiyon için ROM'dan okunmalı**,
kardeşten kopyalanmamalı.

Ayrıca ikinci ekibin kendi GBA kütüphane bileşenlerini projeye kattığı, bu
bileşenlerin telifinin onlarda kaldığı ve teslim edilen kaynaktan çıkarılacağı
yazılı. Yani ROM'un bir kısmı oyuna özgü değil, yeniden kullanılabilir kütüphane
kodu olabilir — `libc` bölgesine benzer üçüncü bir sınıf. HENÜZ DOĞRULANMADI.

## ÇIKARIM — istatistik sayaçları kümesi çözüldü

Belge, duraklama menüsündeki istatistik ekranı için yaklaşık 35 istatistik
öngörüldüğünü, kat edilen mesafe / gizli paket / cephane / can gibi değerlerin
tutulacağını söylüyor.

ROM'da 0x080671B8–0x080673E0 arasında zaten eşleştirdiğimiz bir küme var:
`BumpCount64`, `BumpCount68`, `BumpCount6A`, `BumpCount70`, `BumpCount7A`,
`BumpCount80`, `BumpCount84`, `BumpSaveCounter`, `AddDistance`,
`AccumulateDistance`, `ResetDistanceAccum`. Bunları "bir sayacı artırıyor"
diye adlandırmıştık; **ne sayacı olduğunu bilmiyorduk**.

Belge kümenin ne olduğunu veriyor: istatistik ekranı sayaçları. Tek tek hangi
ofsetin hangi istatistik olduğu HÂLÂ BİLİNMİYOR — onu oyun oturumu (mGBA
izleme) çözer, belge değil.

## ÇIKARIM — görevler derlenmiş bayt kodu

Belge, ikinci ekibin görev script'lerini tersine mühendislikle çözüp bir
derleyici yazdığını söylüyor. Yani görevler ROM'da **veri**, ve onları yürüten
bir **yorumlayıcı** var.

Bu, 560 bayt üstü 183 fonksiyonluk duvarda somut bir hedef: yorumlayıcı büyük
olasılıkla onlardan biri. HENÜZ ARANMADI.

## ÇIKARIM — ses sürücüsü hızlı RAM'de

Belge "ses belleği normalde hızlı RAM'e taşınır" diyor ve 3D işlemenin tüm
hızlı RAM'i kullanıyor olabileceğinden endişe ediyor. GBA'de hızlı RAM = IWRAM
(0x03000000). Ses sürücüsü aranacaksa bakılacak yer orası.

## ÇIKARIM — 3D motor ve sabit nokta

Belge tekrar tekrar "3d motor", "3d koordinatlar" ve sabit nokta aritmetiğindeki
yuvarlama hatalarından söz ediyor. Bu, `BuildVolumePlanes` (0x0800AB88) için
tahmin ettiğimiz kimliği destekliyor: sekiz köşeli hacimden altı yüzey düzlemi
kuran fonksiyon 3D çarpışma tespitidir. Projede ölçtüğümüz 20.12 ve 16.16
sabit nokta biçimleriyle tutarlı.

Araç fiziği için de bir ipucu var: o tarihte arabaların dönüş ekseni aracın
merkezindeymiş, ön tekerler arasına alınması planlanmış. Araç direksiyon kodu
aranırken işaret bu. Çıkan oyunda hangisi olduğu BİLİNMİYOR.

## Kontrol şeması — DİKKATLİ KULLAN

Belge yaya ve araç için tuş eşlemesi veriyor: yayada L araca binme, R zıplama,
A vuruş/ateş, B koşma; araçta L inme, R ateş, A gaz, B fren/geri, A+B el freni,
Select+Start araç görevi başlat/iptal.

**UYARI:** bu Ocak 2003'ün "önerilen revize şeması". Belge yaya yön hareketinin
değiştiğini ve strafe kipinin kaldırıldığını açıkça söylüyor, yani o an oyunda
olan şema bu DEĞİL. Çıkan oyunla aynı olduğu varsayılmamalı; giriş fonksiyonları
yazılırken maskeler ROM'dan okunmalı, bu tablo yalnızca ADLANDIRMA için ipucu.

## BELGELERDE OLMAYAN — beklentiyi buraya kadar tut

Umulan ama bulunmayanlar:
- struct yerleşimi, alan anlamı, sembol adı
- durum kodu / sayı sabiti tabloları
- **kartuş bellek haritası** — belgenin kendi kilometre taşı listesinde
  Milestone 3 teslimatı olarak geçiyor ama o belge bunların içinde DEĞİL

Yani bu belgeler eşleşme yüzdesine doğrudan katkı vermiyor. Katkıları
adlandırma ve alt sistem tanımlama tarafında.
