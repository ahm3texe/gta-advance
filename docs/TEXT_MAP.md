# Metin ve dil verisi haritasi

Yapisal bulgular.  Metnin KENDISI bu depoya girmiyor (bkz. docs/ROADMAP.md:
ROM ve ondan cikarilan varliklar paylasilmaz).  Burada yalnizca nerede
oldugu, nasil duzenlendigi ve nasil okunacagi kayitli.

## Ozet

| Ne | Nerede |
|----|--------|
| Metin govdesi | `0x07B0FE0 - 0x07D7FC8` (~62 KB yazdirilabilir) |
| Isaretci tablosu | `0x0EC46D4 - 0x0EC771C` (**3090 giris**, hepsi gecerli) |
| Seviye/varlik ad tablosu | `0x03D0000 - 0x03DFFEC` (~4000 ad) |

## Isaretci tablosu

3090 giris, hepsi metin govdesine isaret ediyor ve **tam olarak 5'e
bolunuyor: 618 x 5**.  Oyunun acilistaki dil ekraninda BES secenek var
(izleme logu: `gActiveMenuItemCount` 0->5, bkz. docs/GAME_FLOW.md), yani
618 metin dizesi x 5 dil.

Dilim -> dil eslemesi DOGRULANDI (oturum 7): **dilim k = dil k**, dogrudan.
Ayni indeksin bes dilimdeki degeri okunarak sinandi:

| Dilim | Indeks 0 | Dil |
|-------|----------|-----|
| 0 | ENGLISH  | ingilizce |
| 1 | *(dilim 0 ile AYNI adres)* | ispanyolca |
| 2 | ANGLAIS  | fransizca |
| 3 | INGLESE  | italyanca |
| 4 | ENGLISCH | almanca |

Sira, izleme logundaki `gLanguage` ile birebir ortusuyor: oyuncu imleci
asagi gezdirdikce deger 0->1->2->3->4 ilerledi, menu sirasi da
ingilizce/ispanyolca/fransizca/italyanca/almanca.

GERI ALINAN CIKARIM: onceden "her dilimin en dusuk isaretcisi o dilin
adinin ardina dusuyor" denip dilim 0 italyanca, dilim 1 fransizca, dilim 3
almanca sanilmisti.  O bitisiklik TESADUFMUS; gercek esleme yukaridaki
gibi dogrudan.

Ispanyolca dil adlari indeks 0'da degil 1, 2, 3 ve 5'te; dilim 1'in
indeks 0'i dilim 0'inkiyle ayni adresi gosteriyor (kullanilmayan yuva).

## Kodlama

Metin **LATIN-1 (ISO-8859-1)**, sifirla sonlandirilmis, sikistirilmamis.
DUZELTME: once "duz ASCII" denmisti, YANLISTI.  Aksanli karakterler
0x80-0xFF araliginda: 0xC9 = E-akut, 0xD1 = N-tilde, 0xC1 = A-akut.
`ESPANOL` bulunamamasinin sebebi buydu; ROM'da ESPAN(0xD1)OL olarak duruyor.  Menu
etiketleri dogrudan okunabiliyor (`PRESS START` @ `0x07C6E70`,
`NEW GAME` @ `0x07C9748`, `LOAD GAME`, `ERASE`).

ROM'da 5046 aday LZ77 blogu var ama metin bolgesi bunlarin disinda;
sikistirma grafik/harita verisi icin kullaniliyor gorunuyor.

## ACIK SORULAR

- Bazi dizeler diller arasinda PAYLASILIYOR: dilim 1'in indeks 0'i dilim
  0'inkiyle ayni adresi gosteriyor.  Kac dizenin paylasildigi sayilmadi.
- 618 dizenin hangisinin hangi ekrana ait oldugu bilinmiyor.  Izleme
  scriptiyle (tools/trace.lua) diyalog acilirken hangi indeksin
  okundugu yakalanabilir.
- Seviye ad tablosundaki (`0x03D0000`) sonekler bolge kodu gibi duruyor
  (`a1`-`a7`, `c1`-`c3`, `k4`, `v1`); onekler nesne turu (`brief`,
  `briefing`, `pager`, `mafia`, `guard`, `ambush`, `playerstart`).
  Hikayenin brifing ve cagri cihazi mesajlari olarak orgutlendigini
  gosteriyor.

## Okuma

`tools/dump_text.py` yerelde calisir, ciktisi `build/` altina yazilir ve
depoya GIRMEZ.
