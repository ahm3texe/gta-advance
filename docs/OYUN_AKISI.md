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
- **mission_timer** adi SUPHELI (kaynagi libretro_cheat).  Artislar sadece Up
  basiliyken oluyor ve sinematik boyunca 481 kare hic artmadi -- zaman degil
  mesafe sayaci gibi duruyor.  Kesinlesmedi.

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
