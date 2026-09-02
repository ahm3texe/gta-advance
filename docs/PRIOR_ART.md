# Ön araştırma: mevcut çalışmalar ve yeniden kullanılabilir izler

Son kontrol: 2026-09-02

## Sonuç

Açık web, GitHub depo araması, GitLab/web indeksleri, Data Crystal/TCRF ve ROM-hacking aramalarında **Grand Theft Auto Advance için yayımlanmış bir matching decompilation, disassembly deposu, Ghidra projesi veya kapsamlı ROM haritası bulunamadı**.

Bulunan GTA Advance projelerinin çoğu tarayıcı emülatörü/ROM paketi, başka bir GTA motorunda remake veya mod niteliğinde. Bunlar bu ROM'un ARM kaynak kodunu ya da sembol haritasını sağlamıyor.

Bu negatif sonuç mutlak yokluk kanıtı değildir; özel Discord sunucuları, indekslenmeyen forum ekleri veya kişisel arşivler bulunabilir. Yeni kaynak bulunduğunda lisansı ve kökeni doğrulanmadan projeye alınmamalıdır.

## Kullanılabilir açık bilgiler

- Fiziksel Avrupa kartuş kaydı `AGB-BGTP-EUR`, ROM parçası `MX23L12806-12C` ve EEPROM parçası bildiriyor.
- Açık cheat veritabanında oyuncu sağlığı, zırh, para, silahlar, aranan seviyesi ve bazı görev sayaçları için RAM adresleri var. Bunlar dinamik analiz başlangıç noktalarıdır.
- Toplulukta diyalog/tek-satır metin dökümleri var; kod haritası değildir ama metin tablolarını doğrulamada kullanılabilir.
- Bir yapım röportajı, oyunun Digital Eclipse tarafından yeniden yapıldığını ve önceki iptal edilmiş GBA projelerinden kod kullanılmadığını belirtiyor. Bu bir röportaj iddiasıdır, teknik olarak ROM üzerinde ayrıca sınanmalıdır.

## ROM içinden doğrulanan izler

| ROM ofseti | GBA adresi | Bulgu | Anlamı |
|---:|---:|---|---|
| `0x000000` | `0x08000000` | ARM branch | Hedef `0x080000C0` |
| `0x06BCF0` | `0x0806BCF0` | Thumb fonksiyon başlangıcı | Assertion/log çağrı zincirinin parçası |
| `0x06BD18` | `0x0806BD18` | `0x08BD3470` literal referansı | Assertion biçim metnine kod referansı |
| `0xBD3450` | `0x08BD3450` | `MultiSioSync020820` | Nintendo MultiSio kütüphane imzası |
| `0xBD3470` | `0x08BD3470` | `ASSERTION FAILED ...` | SDK assertion biçim metni |
| `0xBD34D4` | `0x08BD34D4` | `EEPROM_V124` | Nintendo EEPROM kütüphane sürüm imzası |

`0x0806BCF0` çevresindeki kod Thumb komutlarıdır. Fonksiyon sınırları ve isimler disassembler analizi tamamlanana kadar geçici kabul edilmelidir.

## Güvenlik ve lisans yaklaşımı

Sızdırıldığı iddia edilen özel Rockstar/Digital Eclipse kaynak kodu kullanılmayacak. Projeye yalnızca kendi ROM'umuzdan üretilen analiz, özgün yeniden yazım, açıkça lisanslanmış araç/kod ve doğrulanabilir kamusal teknik bilgi alınacak.

## Kaynaklar

- Fiziksel kartuş kaydı: <https://gbhwdb.gekkio.fi/cartridges/AGB-BGTP-0/kurodo-1.html>
- Açık libretro cheat verisi: <https://github.com/libretro/libretro-database/blob/master/cht/Nintendo%20-%20Game%20Boy%20Advance/Grand%20Theft%20Auto%20Advance%20%28USA%2C%20Europe%29%20%28Code%20Breaker%29.cht>
- Yapım röportajı: <https://www.timeextension.com/features/the-making-of-grand-theft-auto-advance-the-gta-iii-prequel-yourve-probably-never-heard-of>
- Diyalog verisi: <https://github.com/ThirteenAG/GTA-One-Liners>
