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
