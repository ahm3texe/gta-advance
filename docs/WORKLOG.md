# Çalışma günlüğü

## 2026-09-04 — 53 sınır bulgusunun kapatılması

- Her bulgu özyinelemeli Thumb akışı ve ROM disassembly'siyle incelendi;
  karar kanıtları `data/boundary_review.csv` içine yazıldı.
- 52 bulgu, `split_at_calls.py` aracının literal havuzları ve fonksiyon içi
  ortak blokları doğrusal `bl` hedefi sanarak oluşturduğu sahte bölmelerdi.
- `0x08053FF2`, `0x1C03FFFF` literalinin üst yarısındaki sahte başlangıçtı;
  üç gerçek `bl` çağrısı ve `push {r4-r7,lr}` prologu bulunan
  `0x08053FF4` ayrı fonksiyon olarak korundu.
- Kısa sınır borcu 53'ten 0'a indi. ARM aralığındaki 18 kayıt ve 4096 bayt
  eşiğini aşan 4 kayıt farklı inceleme sınıfları olarak açık tutuldu.
- `ScanAllEntries`, ROM'un yalnız 0 numaralı girdiyi işleyen gerçek davranışını
  yansıtacak biçimde `ProcessFirstEntry` olarak yeniden adlandırıldı.
- `gRam02000F10`, `gRam02001140` ve `gRam02025810` için çelişkili extern
  türleri ortak ham depolama bildirimlerinde birleştirildi. Dokuz matching
  fonksiyonun bayt çıktısı değişmedi; tutarlılık kapısı yeni tür çelişkilerini
  artık otomatik reddediyor.
- Dashboard'daki kullanılmayan 57 UI scaffold/hook dosyası ve sekiz gereksiz
  çalışma zamanı bağımlılığı kaldırıldı. Tam depo dashboard lint'i ve production
  build artık geçiyor; kilometre taşı kapısı daraltılmış lint yerine tam lint
  çalıştırıyor.
- Dört aşırı büyüme kaydı ARM/Thumb disassembly ile kapatıldı. Üç büyük
  fonksiyonun eksik kuyrukları uzatıldı; `0x0802BDF0`–`0x0802DE70` arası
  8.320 baytın tek stack frame olduğu doğrulandı. Sahte girişlerin yeniden
  keşfedilmesini önleyen 57 adreslik negatif bilgi tablosu eklendi.
- ARM overlay'deki 18 kaydın tamamı incelendi: 14 stack-frame fonksiyonu ve
  ebeveyn register/frame durumunu kullanan 4 yerel BL rutini. Dört eksik boyut
  düzeltildi; `0x08067E04–0x0806B84C` aralığı boşluksuz doğrulandı. Sınır
  baseline'ındaki açık/atlanan bütün sınıflar sıfıra indi.

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

## 2026-09-03 (devam 7) — Dört blok denendi, üçü bitti

`make matching` 21/21 bozulmadan duruyor.

### Biten üç blok

**menu_graphics** (212 B, 4 fonksiyon) — **ilk denemede 4/4**. Birikmiş
kurallar (IME `volatile`, DMA3 `volatile`, sabit cast) doğrudan işe yaradı.

**save_slots** (228 B, 2 fonksiyon) — ilk denemede 120/120 baytın 119'u
tuttu. Tek fark `ble` ↔ `bls`: `length` parametresi `u32` olunca eşleşti.

**read_eeprom_bytes** (208 B) — ROM'un açılmış sekizli byte kopyası
`-funroll-loops` ile üretilemedi (136B/127 fark → 256B/239, daha kötü).
Sekiz kopya `COPY_EEPROM_BYTE` makrosuyla açık yazılınca tam eşleşme.

### Yarım kalan: init_save_system

240 baytın 197'si tutuyor, yapı doğru. İki küme fark direniyor:

1. Slot bayrağı temizleme döngüsünde ROM işaretçiyi +31'den aşağı yürütüyor,
   bizimki +16'dan yukarı. Sayaç aynı. Altı farklı döngü biçimi denendi;
   en iyisi 41 bayt fark.
2. ROM `&gSavePayloadSize`'ı bölme çağrısından önce callee-saved register'a
   alıyor. Yerel işaretçi denendi: fonksiyon başında 220, kullanım yerinde 80.

Denenenler kaynak dosyanın başındaki yoruma yazıldı. `init_save_system.s`
geçerli build kaynağı olarak kalıyor.

### Durum

- Emekli assembly dosyası: 8
- C kaynağı: 28 fonksiyon, 27'si byte-matching
- Matching byte'ların **%29.06**'sı C'den

## 2026-09-03 (gece) — Doğrulanmış ROM alanı büyüdü

Bu oturumda ilk kez **yeni ROM alanı doğrulandı** — şimdiye kadarki iş zaten
eşleşen bölgeleri assembly'den C'ye taşımaktı; kapsam artmıyordu.

### libc bölgeleri build'e bağlandı

ROM'un agbcc newlib'ine linklendiği daha önce tespit edilmişti ama bu yalnızca
bir tarama sonucuydu. Artık `data/libc_regions.csv` + `make libc-verify` ile
her giriş `libc.a`'dan çıkarılıp ROM ile karşılaştırılıyor ve `make matching`
bunu otomatik çalıştırıyor.

**9/9 parça, 448 byte.** Doğrulanmış toplam ROM alanı 5340 → **5788 byte**.

Bunlar tersine mühendislik ürünü değil; kaynağı elimizde olan kütüphane
kodunun ROM'daki byte'larla aynı olduğunun kanıtı.

### Yerleşim argümanı

`_exit` ve `_kill` gövdeleri birebir aynı olduğu için byte karşılaştırması
hangisinin nerede olduğunu söyleyemiyordu. Çözüm byte'larda değil yerleşimde:
bu gövdeden ROM'da **tam iki adet** var ve araları 32 byte — `syscalls.o`
içindeki mesafenin aynısı (`_exit` ofset 892, `_kill` 924). İkili ancak bu
sırayla yerleşebilir.

Aynı gövdeli sembol çiftleri için genellenebilir bir yöntem. `toupper` /
`_toupper` çiftine uygulanamadı: ikisi de yer değiştirme içeriyor, bu yüzden
ROM'da düz byte araması sıfır sonuç veriyor. O çift belirsiz kalıyor.

### game_init taslağı

En büyük blok (768 byte). Kontrol akışı tam çıkarıldı ve fonksiyonun baş kısmı
birebir eşleşiyor; 772 baytın ~508'i tutuyor.

Ölçülen: yığın değişkeni tipi büyük fark yaratıyor — `u16` kaynaklar 673 fark,
`volatile u16` 590, **`u32` 264**, `u16` dizi 304, union `.half` 739,
`u32` yuva + cast yazım 739. ROM'un 16 baytlık yığın çerçevesi `u32`'lerle
yakalandı.

Kalan bilinen sapma: ROM DMA kaynağına halfword yazıyor (`strh`), bizimki
word (`str`). Yuva 4 byte aralıklı olmalı ama yazım 16 bit — denenen beş biçim
bu ikisini aynı anda vermedi.

### Paralel çalışma altyapısı

Ultracode ile 12 ajanlık workflow başlatıldı. Öncesinde üç yarış koşulu
kapatıldı: `diff_function.py`'nin paylaşılan geçici dosyası çağrıya özel
yapıldı, kalan assembly'de geçen 20 RAM adresi `ram_map.csv`'ye tek seferde
eklendi (ajanlar o dosyaya yazmıyor), ve ajanlara `make` tamamen yasaklandı.
Emeklilik kararı ve son doğrulama ana süreçte kalıyor.

## 2026-09-03 (gece, workflow) — On blok birden C'ye taşındı

12 ajanlık paralel workflow tamamlandı (0 hata, ~33 dakika). Her sonuç ana
süreçte bağımsız olarak ROM'a karşı yeniden doğrulandı; ajan raporuna
güvenilmedi.

### Sonuç

**Kalan taşınabilir assembly bitti.** `src/` altında yalnızca üç `.s` kaldı:
`agb_main.s` ve `intr_main.s` (ARM modunda, kalıcı olarak assembly) ve
`game_init.s` (henüz eşleşmeyen taslağın yedeği).

- `make matching` 21/21 + libc 9/9
- C kaynağı: 40 fonksiyon, **39'u byte-matching**
- Matching byte'ların **%78.97**'si artık okunabilir C'den (oturum başı: %0)
- 19/19 C dosyası okunabilirlik denetiminden geçiyor

### İki direnen fonksiyon da çözüldü

**`WriteU16LE`** — cevap parametrenin **işaretli dar tip** olmasıydı (`s16`).
Bütün oturum boyunca daha *geniş* tipler denemiştim; yön tersmiş. ROM'daki
`lsls #16`/`lsrs #16` çifti semantik olarak gereksiz, bu yüzden `u16` ile hiç
üretilmiyor: işaretsiz HImode parametre çağırandan zaten sıfır-genişletilmiş
gelir. `s16` yazılınca değer işaret-genişletilmiş kabul edilir ve agbcc üst
yarıyı temizlemek zorunda kalır. **Yani o dört bayt, özgün kaynakta
parametrenin işaretli olduğunun kanıtıdır.**

**`InitSaveSystem`** — iki ajan bağımsız olarak 240/240 buldu. Kuyruk bölümü
üç yazım tercihinin *birlikte* uygulanmasıyla tuttu; hiçbiri tek başına
yetmiyor (kurallar 16, 17, 18).

Ayrıca ajanlardan biri kaynak dosyadaki yorumumun bayat olduğunu bayt kanıtıyla
gösterdi: "çözülemeyen döngü" olarak işaretlediğim birinci küme aslında zaten
eşleşiyordu — ilk farklı bayt döngünün *sonrasındaydı*. Kural 8 böylece
ölçülerek teyit edildi.

### Araç hatası düzeltildi

`diff_function.py` ROM tarafını `functions.csv`'deki boyutla kırpıyordu. O
boyut Ghidra'nın gövde tahmini ve literal havuzu dışarıda bırakabiliyor;
sonuç olarak **tam eşleşen bir fonksiyonda bile** sahte "ROM da YOK" satırları
çıkıyordu. Bu beni de yanıltmıştı. Artık iki taraftan büyüğü alınıyor.

### Kural seti 19'a çıktı

Beş yeni kural (15-19) ve önemli bir üst-kural: **kurallar birbirine bağlı.**
18. kural tek başına etkisizdi ama 16 ve 17 uygulandıktan sonra belirleyici
oldu. "Denendi, tutmadı" kaydı tek başına değerlendirilmemeli.

### Süreç notu

Ajanlar çalışırken `rm -rf build` çalıştırdım ve `build/variants/` altındaki
dört varyant dosyası silindi. Bir ajan bunu fark edip yedek bırakmıştı;
`InitSaveSystem` çözümü oradan kurtarıldı, `WriteU16LE` çözümü ise ajanın
rapor metninden geri yazıldı. Paralel çalışmada ortak dizinlere dokunmamak
gerekiyor.

## 2026-09-03 (gece, 2. workflow) — GameInit eşleşti: taşınabilir assembly bitti

Son blok. Altı ajan, altı farklı açı. **Dokuzu bağımsız olarak 0 farka ulaştı**
(bazı ajanlar birden fazla çözüm üretti) — güçlü çapraz doğrulama. En açıklamalı
olanı seçildi: ROM'dan sıfırdan yazılmış, %39 yorum oranı, 768/768 byte.

### Asıl düğümün çözümü

Oturum boyunca takıldığım nokta şuydu: ROM 16 baytlık yığın çerçevesi kullanıyor,
yuvalar 4 bayt aralıklı, ama sp+4 ve sp+8'e **halfword** yazıyor. `u16` skaler
çerçeveyi 12 bayta düşürüyordu (673 fark), `u32` yazımı word yapıyordu (264).

Cevap: yuvaları **`u16 x[2]` dizisi** yapmak. Dizi BLKmode olduğu için agbcc onu
bildirim sırasında ve 4 bayta hizalı yerleştiriyor; `x[0] = 0` yine `strh`
üretiyor ve `(u32)x` adresi tek komutta veriyor. Word yuvası da dizi olmalı —
skaler bırakılırsa dizilerden sonra yerleşip `sp+0`'ı kaybediyor.

Denenip tutmayanlar: `struct{u16 h; u16 pad;}` (agbcc SImode sayıp `ldr`/`and`/
`str` üretiyor), tek büyük struct, union, cast'lar.

### Üç yeni kural (20-22)

Ayrıca ölçülmüş bir mekanizma açıklaması: **yığın yerleşimini belirleyen şey
bildirim sırası değil, tipin BLKmode olup olmadığıdır.** Bir ajan 24 bildirim
sırası permütasyonu deneyip yerleşimin hiç değişmediğini gösterdi — bu, daha
önce "bildirim sırası etkisiz" diye kaydettiğim gözlemin *nedenini* veriyor.

### Nihai durum

```
make matching        21/21 + libc 9/9
C kaynağı            40 fonksiyon, 40'ı byte-matching
C'den matching byte  4332/4660  (%92.96)
kalan assembly       agb_main.s (52 B) + intr_main.s (276 B) = 328 B
```

**Kalan %7.04 tam olarak o iki dosyadır** (4660 - 4332 = 328). Yani taşınabilir
her şey taşındı: geriye yalnızca ARM modundaki başlangıç kodu ve IRQ dispatcher
kaldı, ki bunlar özgün kaynakta da assembly'ydi ve öyle kalacak.

19/19 C dosyası okunabilirlik denetiminden geçiyor.

---

## 2026-09-03 — Harita revizyonu, hasat, 32 kural

### Harita: 1.466 → 1.978 fonksiyon (+%35)

Uc is birlikte fonksiyon haritasini bastan kurdu:

1. **Sinir denetimi** (`audit_boundaries.py`): ozyinelemeli inisle 647
   sinir duzeltildi, 43 sahte kayit silindi. Ilk yaklasim (dogrusal
   disassembly) 998 fonksiyonu 8 KB'a "buyutuyordu" — yalniz-rapor
   kipinde yakalandi.
2. **Eksik fonksiyon kesfi** (`discover_functions.py`, uc yontem):
   `bl` cagri hedefi (kesin, 51+49+3), ROM verisindeki fonksiyon
   isaretcileri (prolog sartiyla 46; ham tarama 1.538 aday veriyordu,
   cogu grafik verisinde rastlanti), prolog deseni (358). Yakinsayana
   kadar tekrarlandi.
3. **Kuyruk cagrisi bolmesi** (`split_at_calls.py`): yurutucu kosulsuz
   `b`yi fonksiyon ici akis sayiyordu; GCC bunu kuyruk cagrisi icin de
   kullaniyor. 52 `bl` hedefi bilinen kayitlarin ICINE dusuyordu —
   32 kayit bolundu, 54 fonksiyon ayrildi. Sonrasinda kesif 0 veriyor.

**Olcut uc kez asagi duzeltildi** (payda 291K → 338K → 414K → 433K;
oran %2.34 → %2.28 gorunumu). Kapsama hic dusmedi; payda gercege
yaklasti. Ders: harita isi kapsama isinden ONCE bitmeliydi.

### Hasat: 7.268 → 11.264 dogrulanmis ROM bayti

~30 yeni bolge, cogu 2-6 fonksiyonluk yaprak kumeleri. Kanitlanmis
deyimlerin (tasma korumali sayac, cift bagli liste, DMA blogu, karo
isaretci aritmetigi, 148/180 baytlik tablo girisleri) tekrar kullanimi
cogu kumede ILK denemede tam eslesme verdi.

### Kurallar: 27 → 32

- 28: isaretci aritmetigi != dizi indeksi (olcekleme sirasi)
- 29: iki ayni dal → erken return + ortak kuyruk (cross-jump engeli)
- 30: seyrek case → `||` zinciri (atlama tablosu felaketi: 64 B yerine 212)
- 31: dongu sayacinin isaretliligi `bls`/`ble` secimini belirler
- 32: ardisik kelime kopyasi struct atamayla (`ldmia/stmia` tetigi)
- Register dagitim onceligi mekanizma bolumu olarak belgelendi
  (`oncelik = floor_log2(ref) × ref / omur`).

### Park korpusu: 16 dosya

Yedisi ≤5 bayt uzaklikta (ClearTextArea 1, MaybeAdvance 1, QueryEntity 2,
ProcessFirstEntry 2, GetInnerId 4, IsRamModeWanted 4, ProbeObject 5).
Engel siniflari tanimlandi: register dagitim sirasi, dal yonu
normalizasyonu, taban kopyalama/iki-taban, havuz yerlesimi, -O0 sinifi,
carpim faktorizasyonu. Bunlar Faz 2'nin test korpusu.

### Diger

- BIOS yuzeyi kapandi: oyunun tum `swi` temasi 10 thunk, hepsi eslesti.
- libc: `findslot`/`remap_handle` maskeli eslesme ile kimliklendirildi
  (bayt-birebir olmadigi icin bolge sayilmadi); `identify_libc_at.py`.
- Dashboard: bayat JSON (make check artik yeniliyor), palet ton ayrimi,
  panel artik Ghidra yerine bizim kaynagi gosteriyor.
- Yeniden adlandirma YEDI kez baska dosyayi kirdi → rename araci gerek.

## 2026-09-04 — ProcessFirstEntry byte eşleşmesi

- `src/world/scan_all.c` içindeki son iki baytlık fark kapatıldı. İşaretçi
  hesabını `(u32)i * sizeof(Entry) + (u32)tbl` sırasıyla ifade etmek,
  old_agbcc'nin ROM'daki `adds r1, r0, r5` kodlamasını üretmesini sağladı.
- Fonksiyon 62/62 bayt eşleşiyor; iki baytlık hizalama dolgusu ile birlikte
  `0x08029014–0x08029054` aralığı kalıcı matching zincirine eklendi.
- Park korpusundaki `scan_all` engeli kapandı; bu ifade sırası benzer
  register-dağıtımı farkları için yeniden kullanılabilir bir adaydır.

## 2026-09-04 — MaybeAdvance semantik düzeltmesi

- Tek baytlık `bls`/`bhi` farkının derleyici tercihi olmadığı kanıtlandı:
  önceki C ve yorum ROM dal hedefini ters okuyordu.
- Gerçek davranış: yalnız `gVBlankEnabled == 2` ve sayaç `> 1` iken 0;
  diğer tüm durumlarda 1 döndürür.
- Düzeltilen doğal C 42/42 bayt eşleşti. İki bayt hizalamayla
  `0x080664F0–0x0806651C` bölgesi kalıcı matching zincirine eklendi.

## 2026-09-04 — QueryEntity byte eşleşmesi

- `flags & 3` ifadesindeki iki operand aynı değeri verse de agbcc sonucu
  ROM'dan farklı register'da tutuyordu.
- `mask = 3; mask &= flags` biçimi sonucu sabitin register'ında tuttu ve
  kalan iki opcode baytını kapattı. Fonksiyon 60/60 bayt eşleşiyor.
- `0x08055AF8–0x08055B34` kalıcı matching zincirine eklendi; derleyici
  davranışı `COMPILER.md` kural 33 olarak kaydedildi.

## 2026-09-04 — Object query ikilisi byte eşleşmesi

- `ProbeObject` (58/58) ve `GetInnerId` (20/20), iç içe null kontrollerini
  açık erken `return 0` kontrollerine çevirince ROM blok sırasına oturdu.
- Aralarında başka bir doğrulanmış fonksiyon bulunduğundan dosya iki gerçek
  ROM bölgesine ayrıldı: `object_query.c` ve `get_inner_id.c`.
- `0x080381F8–0x08038234` ile `0x0803824C–0x08038260` kalıcı matching
  zincirine eklendi; desen `COMPILER.md` kural 34 olarak kaydedildi.

## 2026-09-04 — IsRamModeWanted byte eşleşmesi

- İç içe ilk koşul açık `if (!active) return 0;` biçimine çevrildi.
  Bu, ortak sıfır bloğunu literal havuzundan önce yerleştirerek kalan dört
  baytlık kontrol-akışı farkını kapattı.
- Fonksiyon 28/28 bayt eşleşti ve `0x08062530–0x0806254C` kalıcı matching
  zincirine eklendi. Kural 34 böylece üçüncü fonksiyonda doğrulandı.

## 2026-09-04 — CallWithOffset imza düzeltmesi

- ROM epilogu çağrı sonucundaki r0'ı `pop {r0}` ile eziyordu; bu kanıt
  sarmalayıcının önceki `u32` imzasının yanlış olduğunu gösterdi.
- Dönüş tipi `void` yapılınca register dağıtımı ve epilog dahil fonksiyon
  24/24 bayt eşleşti. `0x080509C4–0x080509DC` kalıcı matching zincirine
  eklendi; çıkarım `COMPILER.md` kural 35 olarak kaydedildi.

## 2026-09-04 — InitActor byte eşleşmesi

- Döngü sayacını son kuyruk sıfır yazımlarında yeniden kullanmak farkı
  14 bayttan tek taşınmış komuta indirdi.
- `tail = &actor->unk90` adresini sıfır atamasından önce açıkça hesaplamak
  ROM'un komut sırasını üretti; fonksiyon 192/192 bayt eşleşti.
- `0x080154D8–0x08015598` kalıcı matching zincirine eklendi ve desen
  `COMPILER.md` kural 36 olarak kaydedildi.

## 2026-09-04 — HasWantedEntry byte eşleşmesi

- `base`, `kind` ve `cur` işaretçilerini ayrı yaşam aralıkları olarak ifade
  etmek ROM'daki r0/r1/r2 register dağıtımını geri getirdi.
- Eksik kopya komutu geri gelince literal havuzu ve döngü hedefi de doğru
  konuma oturdu; fonksiyon 48/48 bayt eşleşti.
- `0x08028E3C–0x08028E6C` kalıcı matching zincirine eklendi; desen
  `COMPILER.md` kural 37 olarak kaydedildi.

## 2026-09-04 — PushHistory byte eşleşmesi

- Genel taban, okunan `current` değeri ve yazım tabanı karşılaştırmadan önce
  ayrı yaşam aralıklarına ayrıldı.
- Bu biçim agbcc'nin eşitlik yolunu sondaki store ile birleştirmesini önledi;
  register dağıtımı ve döngü hedefiyle birlikte fonksiyon 48/48 eşleşti.
- `0x08008064–0x08008094` kalıcı matching zincirine eklendi; desen
  `COMPILER.md` kural 38 olarak kaydedildi.

## 2026-09-04 — AddDistance byte eşleşmesi

- `gDistanceAccum` adresi ayrı işaretçiye alınarak ROM'daki erken r4 taban
  yüklemesi ve tek taban kullanımı üretildi.
- Yalnız son karşılaştırmadaki `distance` okumasına dar volatile görünümü
  verilerek ROM'un store sonrası `ldrh` yeniden okuması korundu.
- Fonksiyon 68/68 bayt eşleşti; `0x08067274–0x080672B8` kalıcı matching
  zincirine eklendi ve desen `COMPILER.md` kural 39 olarak kaydedildi.

## 2026-09-04 — BumpOrReset byte eşleşmesi

- Yapısal `if/else` yerine ROM'un üç bloğu `reset`, `increment` ve ortak
  `store` C etiketleriyle açıkça ifade edildi.
- agbcc böylece iki dalda ayrı sayaç adresi yükleyip tek `strb` paylaştı;
  fonksiyon 56/56 bayt eşleşti.
- `0x0805AC50–0x0805AC88` kalıcı matching zincirine eklendi ve desen
  `COMPILER.md` kural 40 olarak kaydedildi.

## 2026-09-04 — ReleaseSlot byte eşleşmesi

- Struct indekslemesi kaldırılıp bir kez hesaplanan `scaled` ofset ile
  `heldBase` ve `extraBase` alan tabanları ayrı yerellerde kuruldu.
- Çarpımı iki deyimden tek `scaled = index * 180` atamasına indirmek son
  r3/r4 takasını düzeltti; fonksiyon 64/64 bayt eşleşti.
- `0x080308AC–0x080308EC` kalıcı matching zincirine eklendi ve desen
  `COMPILER.md` kural 41 olarak kaydedildi.
