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
| `0x08070CE4` | `_mbtowc_r` | 42 B | |
| `0x080716A4` | `isinf` | 36 B | **Ghidra kaçırmış** |
| `0x080716C8` | `isnan` | 32 B | **Ghidra kaçırmış** |
| `0x08071D8C` | `abort` | 32 B | |

Tarama şu an yalnızca yer değiştirmesiz fonksiyonları bulabiliyor: 362
fonksiyonun 324'ü dış çağrı içeriyor ve hedef adrese linklenmeden ROM
byte'larıyla eşleşmiyor. Bunlar için adres tahmini + `tools/agbcc_build.py`
linkleme katmanı gerekiyor.

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

## Debug izleri

`0x03E3554`: `!!! ASSERT cam(0x%x 0x%x 0x%x) %s : %d` — eski bir sürümden
kalan kamera debug menüsünün parçası. TCRF `0x03E31A8`'den itibaren `CUSTOM 0-10`,
`CAR PHYSICS TEST`, `SPEED/ACCEL/FACE/DIR/ROTSP/RADIUS/FCOLL/ACOLL/ROLL/TILT/VEL/POS/HBRAKE/HORN`
alanlarını belgeliyor.

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

## Prototip

TCRF'te GTA Advance için ayrı bir **prototip sayfası** var. Prototip
derlemeler bazen sembol tablosu veya debug bilgisi taşır; taşıyorsa çeviri
birimi yapısı doğrudan kurtarılabilir. Henüz incelenmedi.
