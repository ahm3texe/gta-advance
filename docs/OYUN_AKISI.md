# Oyun akisi (gozlem)

Kaynak: kullanicinin 2026-09-05 tarihli oynanis anlatimi (Europe ROM).
Bu bilgi ROM baytlarindan CIKARILAMAZ; fonksiyon adlandirmada ve mod/durum
makinesini eslestirmede dayanak olarak kullaniliyor.

## Acilis sirasi

1. **Dil ekrani** — secenekler arasinda Ingilizce.  (Europe surumu cok dilli;
   secim muhtemelen kalici bir ayar baytina yaziliyor.)
2. **Rockstar logosu** ve devaminda birkac tanitim ekrani.
3. **Press Start ekrani**
   - Arka planda menu gorseli
   - Ortada "GTA Advance" logo sprite'i
   - Solda Rockstar Games, sagda Digital Eclipse sprite'i
   - Ortada yanip sonen "PRESS START" -> ZAMANLAYICI ile yanip sonme
   - Muzik caliyor
4. **X** -> ana menu: `NEW GAME` / `LOAD GAME` / `ERASE GAME`
   - Sprite'lar kayarak giriyor (gecis animasyonu)
   - Arka planda karakterler var
5. **NEW GAME** -> **3 kayit yuvasi**.  Ilk yuva secildi.
   - Isim girisi: "aaa"
   - 3 yuva, `gSaveSlotHeaders` ve `gSaveBuffer` ile ilgili olabilir
6. **"Jump Start" bolumu** basliyor, **muzik degisiyor**.
7. **Karakter/diyalog ekrani**
   - Solda Vinnie, sagda Mike portresi
   - Altta metin kutusu
   - **X** ile ilerleniyor (birkac basis)
8. **Oyun basliyor**
   - NPC'ler yuruyor, korna sesleri
   - Karakter yuruyebiliyor
9. Bir sure yurudukten sonra **sinematik ekran**
10. Altta hedef metni: **"ENTER THE CAR"**

## Kare eslestirmesi (izleme logundan, DOGRULANDI)

tools/trace.lua ile kaydedilen oturum, yukaridaki anlatimla birebir ortusuyor.
Cozumleme: `python3 tools/analyze_trace.py`

| Kare | Olay | Degisen semboller |
|------|------|-------------------|
| f4-f5 | Acilis, kesme kurulumu | gIrqVector, gIrqStack, gSystemStack, gIrqHandlerTable, gIntrMainEwram, gActiveIrqSlot (her biri BIR KEZ) |
| f4 | **Dil ekrani** | gActiveMenuItemCount 0->**5** (bes dil secenegi), gActiveMenuItems, gMenuPositionX |
| f19-f20 | Metin cizim kurulumu | gFontIndex, gGlyphWidths, gTextVramBase, gTextRowStride |
| f52+ | Kare sayaci donuyor | gGameState[0] her kare +1 |
| f684 | **Kayit kontrolu** | gCartFlag 0->1, gAddrTable, gRam02027310 |
| f936-f940 | **Ana menu** (NEW/LOAD/ERASE) | gFontIndex 192->128, gMenuPaletteSource (palet tamponu) |
| **f1066-f1069** | **NEW GAME + 1. yuva** | gCartFlag 1->0, **player_health 0->100**, **gSessionPtr 0->0x02000F10**, gSlotIds 0->42, gJobTable, gNodeListHead, player_health_copy |
| f1099-f1101 | Bolum yukleme | gCartFlag 0->1, gRam02000F10, gFrameCounterEwram, gRam02011030, gUnk0202F310 |
| **f1319** | **Diyalog kutusu** (Vinnie/Mike) | gHalfLineSpacing 0->1, gFontIndex 160->240 |
| f1384-f1402 | Diyalogu ilerletme | gFontIndex 240->242->240, tuş **[A]** ile |
| **f1507-f1510** | **Oyun dunyasi doguyor** | gNodePool, gListHead02016280, gUnk02028270, gFrameDelay -- bagli liste sistemi devreye giriyor |
| f1783-f1801 | **Yuruyus** | gDistanceAccum, ardindan mission_timer 0x100er artiyor, tuş **[Up]** ile |
| f2273-f2754 | **Sinematik** | mission_timer 481 kare boyunca HIC artmadi |
| f2303 / f2427 | Diyalog kapandi / acildi | gHalfLineSpacing 1->0 / 0->1 |

## Bu logdan cikan kesin sonuclar

- **player_health** adi ve anlami DOGRULANDI: yeni oyunda 100 oluyor.
- **gSessionPtr** gercekten **gRam02000F10**'u gosteriyor -- iki ayri sembolun
  ayni yapiyi isaret ettigi kanitlandi.
- **gActiveMenuItemCount** acilista 5; dil ekraninin bes secenegi.
- **gNodePool + gListHead02016280** oyun dunyasi dogdugunda (f1508) birlikte
  aktiflesiyor: bagli liste sistemi varlik (entity) yonetimi icin.
- **gMenuPaletteSource** ISARETCI DEGIL: degerleri BGR555 renk (0x7C1F, 0x7FFF).
  Palet tamponu; u32 adres olarak okunmamali.
- **mission_timer** adi DOGRU (dorduncu oturumda kanitlandi).  Oyuncu hic
  tusa basmadan 30 kez tam 0x100 artti, ~64 karede bir.  Onceki "mesafe
  sayaci" iddiasi CURUTULDU: Up surekli basili oldugu icin artislar tusla
  ayni anda gorunuyordu, nedensellik degil rastlanti.

## Ilk cikarimlar (DOGRULANMADI)

- En az uc ayri "mod" var: acilis/menu, diyalog, serbest dolasim.  Bunlar
  bizim `gLoopState` / `gOuterState` / `gDisplayState` sembolleriyle
  ortusuyor olabilir -- ESLESTIRME ICIN IZLEME LOGU GEREKIYOR.
- Yanip sonen "PRESS START" ve NPC animasyonlari kare sayacina bagli;
  `gFrameCounterLate` / `gIwramFrameCounter` adaylari.
- Menu gecisleri (kayan sprite'lar) palet ve OAM guncellemesi gerektiriyor;
  `FlushPaletteQueue` (0x08013900) bu sinifta.
- Uc kayit yuvasi -> `gSaveSlotHeaders` (0x02000460) muhtemelen yuva
  basliklari dizisi.

## Sonraki adim

Izleme scripti (tools/trace.lua) calisir hale gelince ayni akis tekrar
oynanip her adimda hangi RAM sembolunun degistigi kaydedilecek.  Bu belge
o zaman "gozlem + adres" olarak guncellenecek.


## Araba turu (ikinci oturum)

Kullanici arabaya bindi; geri, ileri, sag, sol surdu.  Oncesinde yaya olarak
da sag/sol/geri hareket etti.  `tools/analyze_trace.py` ile yuruyus turuna
gore fark alindi.

**Sadece araba turunda degisen 4 sembol** (yuruyus turunda degisen ama araba
turunda degismeyen HIC sembol yok -- araba turu yuruyusun ustune ekleniyor):

| Sembol | Degisim | Ilk kare |
|--------|---------|----------|
| `gFocusPoint`  | 31 | f1345 |
| `gClipBounds`  | 31 | f1345 |
| `gRam020302E0` | 2  | f3815 |
| `gUnk02028290` | 1  | f2864 |

### ~~Kesin bulgu: odak noktasi IWRAM'e aynalaniyor~~ (GERI ALINDI -- asagi bak)

`gFocusPoint` (0x020004B0, EWRAM) ve `gClipBounds` (0x03000014, IWRAM) ayri
adresler -- aralarinda 16 MB var -- ama 31 gecisin HEPSINDE ayni degeri
tasidilar, ayni karelerde.  Tesadüf degil: odak noktasi IWRAM'deki calisma
kopyasina yazilıyor.

Bu, uzerinde zaten calistigimiz iki fonksiyonu birbirine bagliyor:
- `UpdateFocusPoint` (0x0800A9E4) -- PARK EDILMIS (4/88)
- `ClipBounds` -- ESLESMIS (src/core/clip_bounds.c)

Degerler yon tusuna gore sabit adimlarla artiyor ve 256'da sariyor:
Up +156, Right +104, Left +152.  Bu, cok baytli bir kamera koordinatinin
DUSUK BAYTI; tam deger icin genisletilmis izleme gerekiyor.

### Aracin kendi kor noktasi

Bu tur, izleme aracinin ciddi bir sinirini ortaya cikardi: 1/2/4 disindaki
boyutlar sessizce 1 bayta kirpiliyordu.  37 sembolun 19086 bayti icin sadece
37 bayt izleniyordu.  Uretec artik yapilari kelime kelime aciyor (275 giris)
ve buyuk dizileri ORNEKLEYIP izlenmeyen 18382 bayti raporluyor.


## Ucuncu tur: tam genislikte odak noktasi

Izleyici artik yapilari kelime kelime aciyor (275 giris), yani `gFocusPoint`in
8 bayti da `gClipBounds`in 12 bayti da tam gorunuyor.

### DUZELTME: "aynalama" sonucu yanlisti

Ikinci turda "gFocusPoint ile gClipBounds 31/31 ayni deger" demistik.  O
sonuc SADECE 0. BAYT okunarak cikarilmisti.  Tam genislikte durum farkli:

| Karsilastirma | Ortak kare | Ayni deger |
|---|---|---|
| `gFocusPoint+0x00` vs `gClipBounds+0x00` | 6  | 5 |
| `gFocusPoint+0x04` vs `gClipBounds+0x04` | 15 | **0** |

Y bileseni HIC uyusmuyor.  Iki yapi kopya DEGIL; rolleri yakin, degerleri
ayri.  Cogu zaman ayri karelerde de degisiyorlar.

Bu, izleyiciyi genisletmenin neden gerektigini gosteriyor: dar okuma kendi
sonucumuzu uydurmustu.

### KANITLANAN: 16.16 sabit nokta

Format tahmin degil, olculdu:

- Baslangic degerleri TAM sayi: `gFocusPoint` = (3360.000, 9568.000),
  `gClipBounds` = (3360.000, 9568.000, 164.000)
- Bazi farklar TAM 65536 (= 1.000) ve TAM 262144 (= 4.000)
- Up basiliyken `gFocusPoint+0x04` her seferinde TAM -374632 (-5.716)
  adimliyor: **29/29 sabit** -> sabit hizla hareket

Yuvarlak olmayan -5.716'lik adim, yon acisina bagli bir hiz bileseni
olabilir (arac yonelimi), ama bu DOGRULANMADI.

### gClipBounds+0x08: hiza gore acilan menzil

Odak noktasinda karsiligi olmayan ucuncu bilesen:

| Kare | Deger | Tus |
|---|---|---|
| f688  | 164.0 | (ilk deger) |
| f3457 | 167.2 | A+Left |
| f3471 | 189.6 | A+Left |
| f3492 | **220.0** | A |
| f3916+ | geri 164.0'a dogru | (gaz birakildi) |

Gaz basiliyken 164.0'dan 220.0'a buyuyup birakinca geri donuyor.  Hiza gore
acilan kamera menzili / on-bakis mesafesi gibi duruyor.
