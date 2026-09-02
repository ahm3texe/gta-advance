# ROM içi izler ve dış kaynaklar

## Standart kütüphane: ROM agbcc'nin newlib'ine linkleniyor

`0x00BD3450` civarında bir kütüphane imza bloğu var:

```
0x0BD3450  MultiSioSync020820
0x0BD3470  ASSERTION FAILED  FILE=[%s] LINE=[%d]  EXP=[%s]
0x0BD34A4  WARING FILE=[%s] LINE=[%d]  EXP=[%s]
0x0BD34D4  EEPROM_V124
0x0BD3644  0123456789abcdef
0x0BD3658  (null)
0x0BD3674  bug in vfprintf: bad base
0x0BD3698  Infinity
0x0BD3810  _sbrk: Heap and stack collision
```

Son üç dize **`tools/agbcc/lib/libc.a` içinde birebir mevcut.** Yani ROM'un
kod kuyruğu tersine mühendislik gerektirmeyen standart kütüphane kodudur ve
kaynağı zaten elimizde.

`MultiSioSync020820` Nintendo'nun seri iletişim kütüphanesinin sürüm damgası
(20 Ağustos 2002). `EEPROM_V124` zaten biliniyordu.

### Doğrulanan eşleşmeler

```sh
make scan-libc
```

libc.a'daki fonksiyon gövdelerini ROM içinde arar:

| Adres | Fonksiyon | Boyut | Not |
|---|---|---|---|
| `0x0806DE84` | `_toupper` | 28 B | maskeli; `toupper` ile belirsiz |
| `0x08070CE4` | `_mbtowc_r` | 42 B | |
| `0x08070DF8` | `_Bfree` | 24 B | |
| `0x08070F34` | `_hi0bits` | 88 B | |
| `0x08070F8C` | `_lo0bits` | 130 B | |
| `0x080716A4` | `isinf` | 36 B | **Ghidra kaçırmış** |
| `0x080716C8` | `isnan` | 32 B | **Ghidra kaçırmış** |
| `0x080717EC` | `findslot` | 30 B | maskeli |
| `0x0807180C` | `remap_handle` | 76 B | maskeli |
| `0x08071B9C` | `_exit` | 32 B | **Ghidra kaçırmış**; `_kill` ile belirsiz |
| `0x08071D8C` | `abort` | 32 B | |

### Maskeli arama

Dış çağrı içeren fonksiyonlar linklenmeden ROM byte'larıyla eşleşmez: `bl`
hedefi ve literal havuzdaki adresler bağlamaya göre değişir. Tarama bu yüzden
**maskeli** yapılıyor — yer değiştirmenin dokunduğu byte'lar joker sayılıp
geri kalan gövde birebir aranıyor. Böylece adres bilinmeden fonksiyon bulunur.

Yöntem `remap_handle` (`0x0807180C`, 76 B) üzerinde gözle doğrulandı: 37
komutun tamamı birebir aynı, farklı görünen tek şey literal havuz sözcükleri —
yani tam olarak maskelenen byte'lar.

Belirsizlik dürüstçe işaretleniyor: `toupper`/`_toupper` ve `_exit`/`_kill`
çiftlerinin gövdeleri birbirinin aynısı olduğu için hangisinin o adreste
durduğu byte'lardan anlaşılamıyor. Bunlar `documented` değil `discovered`
olarak kaydediliyor ve alternatif isim nota yazılıyor.

`isinf` ve `isnan` Ghidra'nın fonksiyon haritasında hiç yok — yani bu yöntem
yalnızca isim vermiyor, **kaçırılmış fonksiyonları da keşfediyor.**

Kod kuyruğu (`0x08070000` sonrası) 69 fonksiyon / 6870 byte, toplam kod
gövdesinin %2.4'ü. `0x0806F000-0x08071E00` bandında 74 fonksiyon / 10478 byte.

## Geliştirici tanımlayıcı tablosu

`0x03CEF38–0x03E30A4` arasında **5032 tanımlayıcı** var — geliştiricinin kendi
isimleri, oyun metni değil:

```
brief_multi3_5c    g_manana4_port1    start_asuka7
e_wantedlevel_last3    l_vehicletest3_rn1    grptrafficpolicelevel2
```

Önek dağılımı: `e_` 1119, `l_` 661, `brief_` 321, `grp` 12, `loc` 6, diğer 2913.
`brief_` bölgesinde kayıtlar 16 byte aralıklı — sabit boyutlu bir dizi.

Bu tablo görev/varlık arama sistemini adlandırmaya ve ona başvuran kod
tablolarını tanımaya yarar.

## Crawfish soy bağı — retail ROM devralınmış bir kod tabanı

Retail oyunu **Digital Eclipse** yaptı, ama ROM iki ayrı **Crawfish Interactive**
izi taşıyor:

1. **`CRAWSAVE` imzası.** `InitSaveSystem` (`0x0800082C`) EEPROM metadata'sının
   ilk 8 byte'ını karakter karakter `'C' 'R' 'A' 'W' 'S' 'A' 'V' 'E'` ile
   karşılaştırıyor. Dize ROM'da bitişik olarak *saklanmıyor* — karşılaştırma
   komutlarında gömülü sabitler hâlinde. (Bu yüzden düz metin araması bulamaz.)
2. **Araç fizik debug menüsü.** TCRF'in Crawfish prototipi için belgelediği
   "Press A and B to toggle car physics test" özelliği retail ROM'da hâlâ
   duruyor (aşağıdaki tablo).

TCRF'e göre proje Crawfish'ten devralındı; Crawfish Kasım 2002'de kapandı.
Bu iki iz, Digital Eclipse'in sıfırdan başlamak yerine **Crawfish'in kod
tabanını devraldığını** gösteriyor. Yani 16 Nisan 2002 tarihli prototip,
"farklı bir oyun" olsa da aynı motor soyundan geliyor olabilir.

## Debug izleri

`0x03E3554`: `!!! ASSERT cam(0x%x 0x%x 0x%x) %s : %d` — kamera debug'ı.

**TCRF'in adresleri ABD sürümüne ait; Avrupa ROM'unda kaymış hâlde.**
Avrupa karşılıkları:

| İçerik | TCRF (ABD) | Avrupa (bizim) |
|---|---|---|
| Debug menü başlangıcı | `0x3E31A8` | `0x3E40BC` |
| `CAR PHYSICS TEST` | — | `0x3E4184` |
| `PLACEHOLDER` | `0x7C9624` | `0x7C99D4` |
| `FLAKEY CHECK ON` | `0x7C76DC` | `0x7C7A8C` |
| `SAY HELLO TO MR PAGER` | `0x3BC6B5` | `0x7C909C` |
| `KILL FRENZY` | `0x7C7CA4` | `0x7C8054` |
| `MULTI PLAYER` | `0x7C93AC` | `0x7C975C` |

### Araç fizik alan adları — struct için hazır isimler

`0x03E4198`'den itibaren **8 byte aralıklı sabit dizi**. Bunlar geliştiricinin
araç yapısı için kullandığı kendi alan adları; araç alt sistemine gelindiğinde
`unk_14` yerine gerçek isimler kullanılabilir:

| Adres | Etiket | Adres | Etiket |
|---|---|---|---|
| `0x03E4198` | `SPEED` | `0x03E41D8` | `ROLL` |
| `0x03E41A0` | `ACCEL` | `0x03E41E0` | `TILT` |
| `0x03E41A8` | `FACE` | `0x03E41E8` | `VEL` |
| `0x03E41B0` | `DIR` | `0x03E41F0` | `POS` |
| `0x03E41B8` | `ROTSP` | `0x03E41F8` | `HBRAKE` |
| `0x03E41C0` | `RADIUS` | `0x03E4200` | *(boş)* |
| `0x03E41C8` | `FCOLL` | `0x03E4208` | `HORN` |
| `0x03E41D0` | `ACOLL` | | |

`0x03E40BC`'de ayrıca `CUSTOM 0` – `CUSTOM 10`, 15 byte aralıklı.

Oyunda erişilebilir debug özellikleri de var (TCRF):

- **Cheat Mode:** oyun sırasında A + B + Start → "CHEAT MODE ON" ve ekranda
  karakter koordinatları. Koordinat göstergesi dinamik doğrulama için kullanışlı.
- **Level Select:** ana menüde Sol, Sağ, Yukarı, Aşağı, L, R, sonra Start basılı
  tutup A. Üçüncü menü seçeneğinin altında ok belirir.

## Kaynak dosya adları: yok

`FILE=[%s]` biçimi `__FILE__` geçirildiğini gösteriyor, ama ROM'da hiçbir
kaynak dosya yolu veya uzantısı yok. Assert'ler yayın derlemesinde muhtemelen
devre dışı bırakılmış ve dize argümanları elenmiş. Çeviri birimi adları bu
yoldan kurtarılamıyor.

## Diğer veri adresleri (TCRF)

| Adres | İçerik |
|---|---|
| `0x349104` | Kullanılmayan görev metni |
| `0x3BC6B5` | "SAY HELLO TO MR PAGER" |
| `0x3CDB9C` | Kesilen çok oyunculu mod metni |
| `0x7C7490` | Araç adları (kesilenler dahil) |
| `0x7C7D48` | Kesilen görev metni |
| `0x7C88E4` | Çok oyunculu görev adları |
| `0x7C949C` | Acil durum araç adları |

## Prototip — sınırlı değer

TCRF'in belgelediği prototip **16 Nisan 2002 tarihli, Crawfish Interactive'in
teknoloji demosu**: tek küçük alan, tek taksi, tuning ekranı ve araç fizik testi.
GTA III portu olarak planlanan ilk aşamadan geliyor; retail oyunu Digital Eclipse
yaptı ve oyun içeriği tamamen farklı.

Yine de yukarıdaki soy bağı nedeniyle motor kodu ortak olabilir. Prototipin
sembol tablosu taşıyıp taşımadığı bilinmiyor — Hidden Palace'ta dump'lanmış
durumda. İncelenirse öncelik sırası: sembol/debug bölümü var mı, `CRAWSAVE`
imzası ve araç fizik menüsü aynı mı, ortak fonksiyon gövdeleri var mı.

Not: prototip ROM'u bu depoya girmez ve yasal edinim kullanıcının sorumluluğundadır.
