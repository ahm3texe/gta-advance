# Çalışma günlüğü

## 2026-09-02 — İlk büyük tersine mühendislik geçişi

### Ortam ve koruma

- Avrupa ROM'u SHA-1 ile doğrulandı; ROM ve save/state çıktıları Git dışında tutuldu.
- Ghidra 12.1.3, OpenJDK 21, mGBA 0.10.5 ve ARM GNU 16.2 araç zinciri hazırlandı.
- Ghidra ARMv4T projesi, otomatik fonksiyon haritası, decompile dışa aktarımı ve CSV senkronizasyonu kuruldu.

### Haritalanan modüller

- ARM giriş, IRQ dispatcher, VBlank/VCount ve interrupt tablosu.
- `GameInit` başlangıç/ana döngü iskeleti.
- `CRAWSAVE` metadata katmanı ve Nintendo `EEPROM_V124` kullanan düşük/yüksek seviyeli save fonksiyonları.
- Üç oyun kayıt slotu, marker/tümleyen checksum düzeni ve little-endian serileştirme yardımcıları.
- İlk UI menü öğesi süzme, çizim, ekran başlatma ve grafik yükleme yardımcıları.
- Büyük `RunMenuScreen` giriş/alt menü akışı ilk kez belgelendi; henüz matching değil.

### Doğrulanmış ölçüm

- Ghidra fonksiyon adayı: 1497.
- Byte-matching fonksiyon: 42.
- Matching fonksiyon gövdesi: 4660 / 290837 byte (`1,60%`).
- Literal/padding dahil matching ROM alanı: 5340 benzersiz byte.
- En büyük kesintisiz matching aralık: `0x080000C0–0x08001457`, 5016 byte.
- İkinci matching UI aralığı: `0x08001DC0–0x08001F03`, 324 byte.

### Otomatik doğrulama

- `make progress`: fonksiyon ve ROM-bölgesi metriklerini gösterir.
- `make matching`: 21 ikili parçayı yeniden derler, her parçayı ROM ile karşılaştırır ve birleşik aralık raporu üretir.
- `make doctor`: ROM hash'ini ve gerekli araçların kurulu olduğunu denetler.
- `make dashboard-dev`: 1497 fonksiyonun büyüklük, durum ve modül bilgilerini etkileşimli treemap üzerinde gösterir; arama, filtreleme ve ayrıntı paneli sağlar.
- `make dashboard-build`: güncel CSV verisini üretip dashboard'un dağıtım derlemesini doğrular.

### Sıradaki teknik hedef

1. `RunMenuScreen` (`0x08001458`) kontrol akışını ve input/action tablolarını kesinleştirmek.
2. `0x08001F04` sonrası UI yardımcılarını sınıflandırmak.
3. mGBA breakpoint/watchpoint oturumuyla menü değişkenlerini dinamik olarak doğrulamak.
4. Compiler parmak izini belirleyip uygun matching assembly parçalarını okunabilir C'ye taşımak.

## 2026-09-03 — Derleyici kimliği çözüldü, C'ye geçiş başladı

### Bulgu

ROM'un **`old_agbcc`** ile derlendiği byte düzeyinde kanıtlandı. Bu, projenin
semantik yeniden inşaya mecbur olmadığı, C'den byte-matching hedefleyebileceği
anlamına gelir.

Kanıt zinciri:

1. **Kod kalıpları.** Doğrulanmış 42 fonksiyonda register kopyalama 84 kez
   `adds rX, rY, #0` (agbcc kalıbı), 0 kez `movs rX, rY` (modern kalıp);
   fonksiyondan dönüş 28 kez `pop {rN}` + `bx rN`, 0 kez `pop {..., pc}`.
2. **Byte doğrulaması.** `src/save/save_helpers.c` yazıldı; 6 fonksiyonun 5'i
   ROM ile birebir eşleşti. `WriteU32LE`'nin 28 byte'ı, maskeyi literal
   havuzdan okumak yerine iki kez `mov`+`lsl` ile kurma gibi ayırt edici bir
   tercihle birlikte tam eşleşti.
3. **Varyant ayrımı.** Altı kombinasyon denendi: `agbcc -O2/-O1` 3/6,
   **`old_agbcc -O2/-O1` 5/6**, her ikisi `-O0` 0/6. Aynı C kaynağı, tek satır
   değişiklik olmadan.

Bayraklar: `old_agbcc -mthumb-interwork -O2 -fhex-asm`.

### Açık kalan

`WriteU16LE` (0x08001124): ROM girişte anlamsal olarak gereksiz bir 16-bit
kırpma yapıyor, `old_agbcc` bunu eliyor. Dokuz farklı C biçimi denendi,
hiçbiri tutmadı; denenenler kaynak dosyada listeli. Assembly kaynağı geçerli
kalıyor.

### Eklenen araçlar

- `make agbcc` — derleyiciyi yerelde üretir (`tools/setup_agbcc.sh`).
  agbcc 1998 dönemi C kaynağı olduğu için modern clang uyumluluk sarmalayıcısı
  gerekiyor; betik bunu kuruyor.
- `make c-match FILE=...` — C dosyasındaki her fonksiyonu ROM ile karşılaştırır.
- `make diff FILE=... FUNC=...` — tek fonksiyonun ROM halini derlenmiş haliyle
  yan yana, komut komut gösterir. Eşleşmeyen fonksiyonu düzeltirken "nerede
  sapıyor" sorusunun cevabı.
- `make doctor` artık derleyiciyi de denetliyor.

objdiff değerlendirildi ama kurulmadı: iki *nesne dosyası* karşılaştırıyor,
yani ROM tarafında da hedef `.o` üreten bir splat/dtk boru hattı gerektiriyor.
Bu kurulana kadar `make diff` aynı işi bizim veri modelimizle yapıyor.

### Depo hijyeni

İlk commit atıldı (154 dosya). Git dışında tutulanlar: ROM, Ghidra projesi,
Ghidra decompiler çıktısı (`analysis/decompiler/`), üretilen dashboard verisi
ve agbcc ikilileri. Decompiler çıktısı ROM'dan türetilmiş materyal olduğu için
deponun kendi yayın politikasına uygun biçimde yerelde bırakıldı.

### Sıradaki teknik hedef

1. `save_helpers` bloğundaki kalan `EraseSaveSlot`, `GetSaveSlotHeader` ve
   `WriteU16LE`'yi C'ye taşımak; blok tamamlanınca `matching_regions.csv`'yi
   `.c` build'ine çevirip `save_helpers.s`'i kaldırmak.
2. Yaprak fonksiyonlardan devam ederek save ve ui modüllerini C'ye taşımak.
3. mGBA yamalama/çalıştırma döngüsü — byte-matching olmayan fonksiyonlar için
   davranışsal doğrulama.
4. `RunMenuScreen` kontrol akışı.

## 2026-09-03 (devam) — Linkleme boru hatti ve save_helpers bloğu

### Kritik altyapı: linkleme

Yaprak olmayan hiçbir fonksiyon linklenmeden doğrulanamaz — `bl` komutları
hedef adres çözülmeden doğru byte üretmez. `tools/agbcc_build.py` eklendi:
C kaynağını derler, blokun ROM taban adresine linkler, dış sembolleri
`data/functions.csv`'den çözer. `verify_c_function.py` ve `diff_function.py`
artık bu ortak katmanı kullanıyor.

**Yakalanan tuzak:** agbcc `.text` bölümünü 8'e hizalıyor. Taban adres 8'in
katı değilse (0x08001094 gibi) linker bölümü 4 byte ileri itiyor ve *önceden
eşleşen fonksiyonlar dahil her ölçüm kayıyor*. Link betiğinde bölüm adresi
artık açıkça sabitleniyor (`SUBALIGN(1)`). Bu hata sessiz: her şey "eşleşmiyor"
görünür ve sebep kodda sanılır.

### save_helpers bloğu: 8 fonksiyonun 5'i C'den byte-matching

Eklenenler: `EraseSaveSlot`, `GetSaveSlotHeader`. İkisi de eşleşmedi ama
yapıları doğru — tüm komutlar var, fark sıralamada.

Her ikisinde de **aynı sistematik fark**: ROM taban adresini indeks
hesabından önce yüklüyor, agbcc sonra. `GetSaveSlotHeader` için beş farklı
yerel değişken dizilimi, işaretçi aritmetiği, ters koşul, `void*` dönüş,
extern dizi sembolü ve iki derleyici varyantı denendi — **beşi de byte-byte
aynı çıktıyı verdi.** agbcc bu fonksiyonda C biçimine duyarsız, dolayısıyla
C'yi kurcalayarak çözülecek bir sorun değil. Kalan olasılıklar: pret/agbcc'nin
yeniden kurulmuş sürümü ile orijinal SDK sürümü arasındaki fark, veya henüz
bulunmamış bir derleyici bayrağı.

`WriteU16LE` de açık (dokuz C biçimi denendi, kayıtlı).

### Memset adlandırıldı

`FUN_0806dcc0` incelendi ve `Memset` olarak doğrulandı: (dest, value, count)
alıyor, baytı 4 byte'lık desene yayıp `stmia` ile 16'şar byte yazıyor, kalanı
bayt bayt bitiriyor, `dest` döndürüyor. `function_overrides.csv`'ye
`documented`/`sdk` olarak işlendi.

### Eklenen: make disasm

`make disasm FUNC=...` bir fonksiyonun disassembly'sini doğrudan ROM'dan
üretir. Assembly kaynakları C'ye taşındıkça silinecek; orijinal koda erişim
bu araçla korunuyor, bakım gerektirmeyen ve eskiyemeyen biçimde.

### Durum

`make matching` 21/21 bozulmadı. `save_helpers.s` hâlâ geçerli build kaynağı;
blok 8/8 olana kadar öyle kalacak.

## 2026-09-03 (devam 2) — İlk assembly dosyası emekli oldu

### Asıl bulgu: RAM adresleri extern sembol olmalı

`EraseSaveSlot` ve `GetSaveSlotHeader`'ın eşleşmemesinin sebebinin derleyici
sürümü olduğunu düşünmüştüm. **Yanlıştı.** Sebep C tarafındaydı:

```c
#define gSaveSlotHeaders ((SaveSlotHeader *)0x02000460)   /* katlaniyor */
extern SaveSlotHeader gSaveSlotHeaders[3];                /* dogru      */
```

Adres derleme-zamanı sabiti olunca agbcc `base + 16`'yı ayrı bir literal
hâline getiriyor ve tabanı register'da tutmuyor; ROM ise tabanı bir kez
yükleyip saklıyor. Extern sembole çevrilince `EraseSaveSlot` anında eşleşti,
`GetSaveSlotHeader` da doğrudan üye erişimine geçirilince eşleşti.

Önceki oturumda "derleyici hipotezi tükendi, bu üç fonksiyon kapanmıyor"
diye kaydedilen sonuç bu yüzden hatalıydı; `docs/COMPILER.md` düzeltildi.
Bayrak taraması ve `release` sürümü ölçümleri kayıt olarak duruyor — ikisi de
gerçekten etkisizdi, ama asıl değişken başka yerdeydi.

### save_wrappers: ilk tam blok

`IsSaveSlotValid`, `ReadSaveMetadata`, `WriteSaveMetadata` — 3/3 byte-matching,
ve 68 byte'lık bölgenin tamamı birebir. `src/save/save_wrappers.s` ve
`config/save_wrappers.ld` silindi; bölge artık C'den üretiliyor.
`make matching` 21/21 bozulmadan geçiyor.

save_helpers 7/8: yalnızca `WriteU16LE` açık.

### Yol boyunca düzeltilen üç tuzak

1. **Dış semboller `.equ` ile verilir.** Linker'a bırakılınca mutlak sembolü
   Thumb fonksiyonu saymıyor ve araya interworking veneer'i sokuyor;
   `bl` hedefi yanlış çıkıyor.
2. **Bölüm sonu dolgusu.** `as` Thumb bölümlerini NOP (`0x46C0`) ile
   dolduruyor, ROM sıfırla. Üretilen assembly'nin sonuna `.align 2, 0` eklendi.
3. **Bölüm hizalaması** (önceki oturumdan): agbcc `.text`'i 8'e hizalıyor,
   taban 8'in katı değilse her ölçüm kayıyor.

### Eklenenler

- `tools/build_c.py` — C kaynağından ROM adresine linklenmiş `.bin` üretir;
  Makefile bölge kuralları artık bunu kullanabiliyor.
- `data/ram_map.csv` artık `agbcc_build` tarafından okunuyor; `gSaveMetadata`
  (`0x02000ED0`) ve `gSaveSlotHeaders` (`0x02000460`) doğrulanmış olarak eklendi.

### Sıradaki

`WriteU16LE`; ardından `menu_helpers` (112 byte) ve
`reset_display_interrupts` (63 satır) gibi küçük blokları C'ye taşımak.
`agb_main.s` ve `intr_main.s` kalıcı olarak assembly kalır.

## 2026-09-03 (devam 3) — İkinci blok C'ye taşındı

`ResetDisplayAndInterrupts` (`0x080007B4`, 120 byte bölge) C'den byte-matching.
`src/bootstrap/reset_display_interrupts.s` ve link betiği silindi.
`make matching` 21/21; iki bölge artık C'den üretiliyor.

### İki yeni kural

**Yığındaki geçici tampon `volatile` olmalı.** DMA kaynağı olarak kullanılan
yığın değişkeni `volatile` yapılmadan agbcc `mov r0, sp` ile `movs r2, #0`'ı
ters sırada üretiyordu. Fark 57 bayttan 7 bayta düştü.

**Donanım/BIOS değişkeni `volatile` OLMAMALI.** Kalan 7 bayt `gBiosIrqFlags`
erişimindeydi; `volatile` kaldırılınca tam eşleşti. İkisi zıt görünüyor ama
`volatile` burada semantik değil, sıralama düğmesi.

Tüm kurallar `docs/COMPILER.md` içinde tablo hâlinde.

### Doğrulanmamış isimler benimsenmedi

Assembly kaynağı üç dış fonksiyonu `WaitForDma3`, `InitSubsystem`,
`WaitForVBlank` diye etiketlemişti. Disassembly bunları desteklemiyor:
`FUN_08063b74` DMA döngüsü değil, dört donanım register'ına sabit yazıyor;
`FUN_0800cae4` VBlank beklemiyor, iki fonksiyon çağırıyor. C dosyasında
Ghidra adları kullanıldı ve gerekçe yorumda yazıldı.

### RAM haritası

`gVBlankState` (0x02000130), `gDisplayState` (0x020004BC, provisional),
`gBiosIrqFlags` (0x03007FF8) eklendi.

## 2026-09-03 (devam 4) — C metriği ve üçüncü blok

### Ölçüm ayrıldı

Assembly transkripsiyonu ile C'den byte-matching aynı metrikte görünüyordu.
`make c-status` (`tools/scan_c_sources.py`) artık `src/` altındaki C
kaynaklarını derleyip ROM ile karşılaştırıyor ve `data/c_sources.csv`
üretiyor. `make progress` iki yeni satır veriyor; dashboard'da ayrı bir
gösterim durumu (**C'den eşleşiyor**, daha parlak yeşil), özet kartında sayaç,
durum filtresinde seçenek ve detay panelinde kaynak dosya yolu var.

### menu_helpers C'ye taşındı

`ResetMenuState`, `IsMenuFlagSet`, `FinalizeMenuLayout` — 3/3, 112 baytlık
bölgenin tamamı. `src/ui/menu_helpers.s` ve link betiği silindi.
Üçüncü emekli assembly dosyası.

**İki yeni kural:** dizi temizleme döngüsü *ileriye* yazılmalı (agbcc onu
geriye giden işaretçi yürüyüşüne çeviriyor; elle geriye yazmak farklı kod
üretiyor) ve döngü indeksi *işaretli* olmalı (işaretçi karşılaştırması
işaretsiz dal üretiyor, ROM işaretli kullanıyor).

### Durum

- `make matching` 21/21; üç bölge C'den üretiliyor
- C kaynağı: 15 fonksiyon, 14'ü byte-matching
- Matching byte'ların %8.45'i artık C'den geliyor (394/4660)
- RAM haritası: menü sembolleri eklendi

`FUN_080512b0` ve `FUN_08004280` adlandırılmadı; doğrulanmadan isim verilmiyor.

## 2026-09-03 (devam 5) — irq_helpers C'ye taşındı

Dördüncü emekli assembly dosyası. `NoOpVBlankFinalize`, `DummyIntr`,
`RunVBlankTransfers`, `NoOpInterruptHelper`, `VCountIntr` — 5/5, 132 baytlık
bölgenin tamamı.

### İki kural düzeltildi

**Kural 1 evrensel değil.** RAM sembolleri `extern` olmalı, ama agbcc'nin
kaydırmayla üretebildiği adresler ROM'da sabit cast olarak yazılmış:
`0x03000000` ROM'da `movs #0xc0` + `lsls #18` ile hesaplanıyor, literal
havuzdan okunmuyor. Extern sembol yapınca 55 bayt sapma; sabit cast yapınca
8'e düştü. Diff hangi biçimin doğru olduğunu söylüyor.

**Kural 4 fazla genellenmişti.** Önceki oturumda "donanım değişkeni volatile
olmamalı" diye yazmıştım — tek örnekten. `REG_IF` (`0x04000202`) tam tersini
istiyor: `volatile` olmadan 7 bayt sapma, `volatile` ile tam eşleşme. Aynı
`x |= sabit` deyimi, zıt gereksinimler. Kural "her erişim için ayrı denenir"
olarak düzeltildi.

### Üçüncü bulgu

`gFrameDelay = counter = gIwramFrameCounter;` — zincirleme atama. Ayrı iki
satır yazınca agbcc adres hesabını ters sıraya koyuyordu (8 bayt fark);
zincirleme yazınca tam eşleşme.

### Durum

- `make matching` 21/21; dört bölge C'den üretiliyor
- C kaynağı: 20 fonksiyon, 19'u byte-matching
- Matching byte'ların **%10.73**'ü C'den (500/4660)

`FUN_08012b9c`, `FUN_080133a8`, `FUN_080130f4`, `FUN_08013900`,
`FUN_080101d8`, `FUN_080327c8` adlandırılmadı.

## 2026-09-03 (devam 6) — init_menu_screen C'ye taşındı

Beşinci emekli assembly dosyası. 172 baytlık bölgenin tamamı byte-matching.
Şimdiye kadarki en karmaşık blok: iki DMA aktarımı, iki IME kritik bölümü,
altı ardışık çağrı ve koşullu kuyruk.

### Üç yeni kural

**Kaydet/geri-yükle çifti olan register `volatile` olmalı.** `REG_IME`
`volatile` değilken agbcc iki kritik bölümün kaydetmelerini birleştirip
sıralamayı tamamen bozuyordu (136 bayt sapma). DMA3 ile birlikte `volatile`
yapılınca 46'ya düştü.

**Çağrılar boyunca yaşayan adres başta yerel değişkene alınır.** ROM palette
kaynağını fonksiyonun ilk komutunda `r5`'e yükleyip altı çağrı boyunca orada
tutuyor. Kullanıldığı yerde okununca derleyici hoist etmiyor:

```c
const u8 *palette = gMenuPaletteSource;   /* basta */
...
REG_DMA3.src = palette;                   /* sonra */
```

Bu tek değişiklik 46 bayt sapmayı sıfıra indirdi.

**Zincirleme atama** (önceki bloktan): `a = b = c` ayrı satırlardan farklı
kod üretiyor.

Kural sayısı 12'ye çıktı.

### Durum

- `make matching` 21/21; beş bölge C'den üretiliyor
- C kaynağı: 21 fonksiyon, 20'si byte-matching
- Matching byte'ların **%13.69**'u C'den (638/4660)
- Kalan assembly dosyası 16, ikisi kalıcı
