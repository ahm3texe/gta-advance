# Yol haritası ve ilerleme ölçümü

## Başarı tanımı

Uzun vadeli teknik hedef, kullanıcının yerel `baserom.gba` girdisinden çalışan ve doğrulanabilir bir ROM üretmektir. “Tamamlandı” ölçütü yalnızca okunabilir C miktarı değildir: yeniden derlenen kodun davranışsal olarak doğru, ideal olarak da orijinal makine koduyla byte-eşleşmeli olmasıdır.

## Aşamalar

| Aşama | Çıktı | Ölçüm | Durum |
|---|---|---|---|
| 0. Temel | Hash, Git koruması, araç raporu | ROM doğrulanıyor | Tamamlandı |
| 1. Haritalama | ARM/Thumb fonksiyon ve veri sınırları | Keşfedilen fonksiyon sayısı | Devam ediyor; 1934 fonksiyon |
| 2. İskelet | Linker script, assembly kaynakları, yeniden derleme | ROM boyutu/yerleşimi | Tamamlandı |
| 3. Modül analizi | Grafik, giriş, dünya, görev, ses, kayıt alt sistemleri | Belgelenen fonksiyonlar | Devam ediyor; IRQ ve save haritalandı |
| 4. Matching decomp | C/assembly kaynak ve compiler bayrakları | Matching/toplam fonksiyon ve byte | Devam ediyor; **419/1934 fonksiyon, 33.554/454.072 bayt (%7,39)** |
| 5. Doğrulama | Otomatik ROM diff + mGBA testleri | Hash/davranış testleri | `make rom` kaynak bölgelerini hibrit görüntüye yerleştirip SHA-1 doğruluyor; tam kaynak build'i ve mGBA davranış testi hâlâ eksik |

## İlk çalışma oturumu

1. `make prepare-rom ...` ile yerel ROM'u hazırla ve hash'i doğrula.
2. Ghidra + Java 21, mGBA ve devkitPro `gba-dev` araçlarını kur.
3. Ghidra'ya raw binary olarak yükle: ARM little-endian, ARMv4T/ARM7TDMI, base address `0x08000000`.
4. `0x080000C0` giriş noktasını doğrula; ARM/Thumb geçişlerini ve ilk çağrı grafiğini çıkar.
5. Her doğrulanan fonksiyonu `data/functions.csv` dosyasına ekle.
6. İlk küçük modülü seç; assembly çıktısı, C karşılığı ve diff döngüsünü kur.

## Ölçümler

- **Discovery coverage:** keşfedilen fonksiyon sayısı. İlk fonksiyon haritası oluşana kadar yüzdelik verilmez.
- **Documentation coverage:** `documented + decompiled + matching` / toplam keşfedilen.
- **Decomp coverage:** `decompiled + matching` / toplam keşfedilen.
- **Matching coverage:** `matching` / toplam keşfedilen.
- **Byte coverage:** fonksiyon boyutları güvenilir olduğunda matching byte / toplam kod byte. Bu, fonksiyon sayısından daha anlamlı ana metriktir.

Tahmini yüzdeleri kesin ilerleme gibi göstermemek için boyutu bilinmeyen fonksiyonlar byte metriğine dahil edilmez.

`data/matching_regions.csv`, literal havuzları ve padding dahil gerçek ROM aralıklarını tutar. `make matching` her üretilen parçayı ROM'a karşı denetler; `make progress` hem fonksiyon gövdesi metriğini hem de benzersiz ROM bölgesi metriğini gösterir.

## Nerede duruyoruz (2026-09-06)

**%7,39** — 419/1934 fonksiyon, 33.554/454.072 bayt. Kalan iş üç banda ayrılıyor
ve bantların maliyeti çok farklı:

| bant | fonksiyon | bayt | ROM payı | not |
|---|---|---|---|---|
| < 120 bayt | 756 | 44.858 | %9,9 | çoğu saplama/sarmalayıcı, ucuz ama küçük |
| 120–560 bayt | 558 | 141.418 | %31,1 | asıl verimli bant |
| ≥ 560 bayt | 183 | 219.322 | **%48,3** | gerçek duvar |

**Kardeş bandı** — eşleşmiş bir fonksiyonun 1,6 KB komşuluğundaki 120–560 baytlık
işler — bu oturumdaki hızın kaynağıydı ve **84 fonksiyon / 18.818 bayt (%4,1)**
kaldı. Orada isabet oranı %90'ın üstünde çünkü ajan struct'ları, çağrılan
imzalarını ve RAM sembollerini kardeşten hazır alıyor.

**Bu tempo aynen sürmez.** Kardeş bandı bitince 120–560 bandının geri kalanına
geçilecek; orada kardeş bitişik değil, fonksiyon başına maliyet artar.

## Sıradaki adımlar

**1. Kardeş bandını bitir (%4,1).** Kanıtlanmış yol. Her tur kural kütüphanesini
de büyütüyor, yani büyük fonksiyonlar için hazırlık.

**2. Giriş / link / istatistik bölgesini al.** 0x08065000–0x08067400 arası 54
fonksiyonun 34'ü yazılmamış (~8,6 KB). Bu bölge artık **ne iş yaptığı bilinen**
bir alt sistem: giriş okuma (KEYINPUT), seri iletişim (SIOCNT) ve istatistik
sayaçları. Ayrıntı [HARICI_BELGELER.md](HARICI_BELGELER.md).

**3. Script yorumlayıcısını bul.** Görevler ROM'da derlenmiş bayt kodu; onları
yürüten yorumlayıcı büyük olasılıkla ≥560 bayt bandındaki fonksiyonlardan biri.
Bulunması o bandı açacak ilk somut hedef.

**4. Adlandırma borcunu kapat.** 100 eşleşmiş fonksiyon hâlâ `FUN_` yer tutucu
adı taşıyor, 172 RAM sembolünün 139'u `provisional`. Bir kısmı bilerek adsız
(boş saplama, kör sarmalayıcı) ama hepsi değil. Bu, yüzdeyi değil belge
kalitesini artırır ve `check_consistency` her koşuda sayıyı basıyor.

**5. Yazmaç dağıtımı sınıfını sistemleştir.** `dump_alloc.py --rom` artık
"sorun dağıtım mı değil mi" ayrımını tek bakışta veriyor (COMPILER.md kural 50).
Bu ayrım yapılmadan kurcalanan üç dosya günlerce boşa gitmişti.

**Ne YAPMAYACAĞIZ:** mGBA ile oynayıp eşleşme aramak. Ölçüldü — tıkanmaların
tamamı derleyici kalıbı, anlambilim değil. mGBA'nın yeri madde 4: RAM alanlarının
ve istatistik ofsetlerinin anlamını çözmek.

## Araçlar

- **Ghidra:** statik analiz, ARM/Thumb disassembly, decompiler, sembol ve çağrı grafiği.
- **mGBA:** oyunu çalıştırma, breakpoint/watchpoint, GDB remote ve davranış testi.
- **devkitARM (`gba-dev`):** ARM assembler/linker/objdump ve modern GBA derleme araç zinciri.
- **agbcc / old_agbcc:** byte-matching C derlemesi. Parmak izi doğrulandı ve `src/save/save_helpers.c` üzerinde 6 fonksiyonun 5'i C'den birebir üretildi; ayrıntı [COMPILER.md](COMPILER.md).
- **Git:** her keşfi, sembol adını ve matching dönüşümü geri alınabilir biçimde izleme.
- **Python araçları:** ROM ayrıştırma, varlık çıkarma, tablo üretimi ve otomatik diff.

## Hukuki/dağıtım sınırı

Yalnızca yasal olarak edinilmiş ROM kullanılmalı. ROM, çıkarılmış sanat/ses/veri varlıkları ve yayınlanması uygun olmayan üretici materyalleri depoya commit edilmemeli. Paylaşılabilir proje; özgün analiz, yeniden yazılmış kod, araçlar ve kullanıcının kendi ROM'undan yerelde veri çıkaran betiklerden oluşmalı.
