# EEPROM kayıt sistemi

ROM'daki `0x0800082C–0x08000C27` aralığı oyunun yüksek seviyeli kayıt katmanıdır. İsimler davranışa göre verilmiş geçici sembollerdir; orijinal kaynak sembolleri değildir.

## Veri düzeni

- Metadata RAM tamponu: `0x02000ED0`, 32 byte.
- İlk 8 byte: ASCII `CRAWSAVE` imzası.
- Byte `8`: yapılandırılan slot sayısı.
- Byte `16 + slot`: ilgili kayıt slotunun geçerli olduğunu gösteren bayrak.
- EEPROM metadata alanı: ilk dört 8-byte blok (`0–3`).
- Slot verisi: blok `4 + slot * (payload_size / 8)` adresinden başlar.

## Fonksiyon haritası

| Adres | Geçici ad | Rol |
|---|---|---|
| `0x0800082C` | `InitSaveSystem` | EEPROM kitaplığını başlatır, slot sayısını `1..16` aralığına sınırlar, metadata imzasını doğrular/oluşturur ve kayıt boyutunu 8 byte'a hizalar. |
| `0x0800091C` | `ReadEepromBytes` | Nintendo EEPROM rutininden 8-byte bloklar okur ve byte sırasını hedef tampona aktarır. |
| `0x080009EC` | `WriteEepromBytes` | 8-byte blok yazar, karşılaştırır ve hata halinde en fazla 20 yeniden deneme yapar. |
| `0x08000B00` | `WriteSaveSlot` | Slot/boyut sınırlarını uygular, veriyi yazar ve metadata geçerlilik bayrağını set eder. |
| `0x08000B78` | `ReadSaveSlot` | Geçerlilik bayrağını kontrol eder ve slot verisini okur. |
| `0x08000BE4` | `IsSaveSlotValid` | Metadata içinden slot geçerlilik baytını döndürür. |
| `0x08000C00` | `ReadSaveMetadata` | Sabit 32-byte metadata alanını okur. |
| `0x08000C14` | `WriteSaveMetadata` | Sabit 32-byte metadata alanını yazar. |
| `0x08000C28` | `InitSaveManager` | Üç oyun kayıt başlığını yükler ve marker/checksum çiftlerini doğrular. |
| `0x08000D20` | `LoadSaveSlot` | 160-byte oyun kaydını yükler ve checksum kontrolü yapar. |
| `0x08000D80` | `WriteGameSaveSlot` | Marker ve tümleyeniyle checksum oluşturup 160-byte slot yazar. |
| `0x08000DDC` | `ReadEepromRange` | Hizalanmamış herhangi bir EEPROM byte aralığını okur. |
| `0x08000F1C` | `WriteEepromRange` | Hizalanmamış aralığı yazar; blok başına sınırlı yeniden deneme yapar. |
| `0x08001094` | `EraseSaveSlot` | Slot başlığını ve ilk EEPROM bloğunu temizler. |
| `0x080010D4` | `GetSaveSlotHeader` | Geçerli slot başlığı işaretçisini veya null döndürür. |

Save katmanı ve bitişiğindeki sekiz küçük little-endian serileştirme yardımcısı, literal havuzları ve padding dahil `0x0800082C–0x0800114B` boyunca kesintisiz **2336/2336 byte matching** durumundadır. Kaynaklar `src/save/` altındadır; `make matching` tüm parçaları ROM'a karşı doğrular.

Nintendo EEPROM kitaplığı ROM'un ilerleyen bölümünde `EEPROM_V124` sürüm etiketiyle bulunuyor. Yüksek seviye fonksiyon adları doğrulanmış davranışı anlatır; kütüphane fonksiyonları ayrıca sınırlandırılıp adlandırılacaktır.
