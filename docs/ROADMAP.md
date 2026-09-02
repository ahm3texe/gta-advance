# Yol haritası ve ilerleme ölçümü

## Başarı tanımı

Uzun vadeli teknik hedef, kullanıcının yerel `baserom.gba` girdisinden çalışan ve doğrulanabilir bir ROM üretmektir. “Tamamlandı” ölçütü yalnızca okunabilir C miktarı değildir: yeniden derlenen kodun davranışsal olarak doğru, ideal olarak da orijinal makine koduyla byte-eşleşmeli olmasıdır.

## Aşamalar

| Aşama | Çıktı | Ölçüm | Durum |
|---|---|---|---|
| 0. Temel | Hash, Git koruması, araç raporu | ROM doğrulanıyor | Tamamlandı |
| 1. Haritalama | ARM/Thumb fonksiyon ve veri sınırları | Keşfedilen fonksiyon sayısı | Devam ediyor |
| 2. İskelet | Linker script, assembly kaynakları, yeniden derleme | ROM boyutu/yerleşimi | Tamamlandı |
| 3. Modül analizi | Grafik, giriş, dünya, görev, ses, kayıt alt sistemleri | Belgelenen fonksiyonlar | Devam ediyor; IRQ ve save haritalandı |
| 4. Matching decomp | C/assembly kaynak ve compiler bayrakları | Matching/toplam fonksiyon ve byte | Devam ediyor |
| 5. Doğrulama | Otomatik ROM diff + mGBA testleri | Hash/davranış testleri | ROM diff etkin; dinamik test bekliyor |

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

## Araçlar

- **Ghidra:** statik analiz, ARM/Thumb disassembly, decompiler, sembol ve çağrı grafiği.
- **mGBA:** oyunu çalıştırma, breakpoint/watchpoint, GDB remote ve davranış testi.
- **devkitARM (`gba-dev`):** ARM assembler/linker/objdump ve modern GBA derleme araç zinciri.
- **agbcc:** compiler parmak izi uyarsa byte-matching C derlemesi. Başta varsaymayacağız; üretilen assembly kalıplarıyla doğrulayacağız.
- **Git:** her keşfi, sembol adını ve matching dönüşümü geri alınabilir biçimde izleme.
- **Python araçları:** ROM ayrıştırma, varlık çıkarma, tablo üretimi ve otomatik diff.

## Hukuki/dağıtım sınırı

Yalnızca yasal olarak edinilmiş ROM kullanılmalı. ROM, çıkarılmış sanat/ses/veri varlıkları ve yayınlanması uygun olmayan üretici materyalleri depoya commit edilmemeli. Paylaşılabilir proje; özgün analiz, yeniden yazılmış kod, araçlar ve kullanıcının kendi ROM'undan yerelde veri çıkaran betiklerden oluşmalı.
