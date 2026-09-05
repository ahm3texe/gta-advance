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
(izleme logu: `gActiveMenuItemCount` 0->5, bkz. docs/OYUN_AKISI.md), yani
618 metin dizesi x 5 dil.

Blok duzeni kanit: her 618'lik dilimin en dusuk isaretcisi, o dilin adinin
hemen ARDINA dusuyor.

| Dilim | En dusuk isaretci | Hemen oncesindeki dil adi |
|-------|-------------------|---------------------------|
| 0 | `0x07C6E18` | `ITALIANO` @ `0x07C6DF4` |
| 1 | `0x07C44F4` | `FRANCAIS` @ `0x07C44D4` |
| 3 | `0x07BF31C` | `DEUTSCH`  @ `0x07BF2E4` |

`ENGLISH` @ `0x07C9A00`, `ESPANOL` duz ASCII olarak YOK (aksanli
karakterler yuzunden farkli kodlanmis olabilir -- DOGRULANMADI).

## Kodlama

Metin **duz ASCII**, sifirla sonlandirilmis, sikistirilmamis.  Menu
etiketleri dogrudan okunabiliyor (`PRESS START` @ `0x07C6E70`,
`NEW GAME` @ `0x07C9748`, `LOAD GAME`, `ERASE`).

ROM'da 5046 aday LZ77 blogu var ama metin bolgesi bunlarin disinda;
sikistirma grafik/harita verisi icin kullaniliyor gorunuyor.

## ACIK SORULAR

- Dilimlerin ust siniri ic ice geciyor (hepsi ~`0x07C99xx`'e ulasiyor).
  Bazi dizeler diller arasinda PAYLASILIYOR olabilir; dogrulanmadi.
- 618 dizenin hangisinin hangi ekrana ait oldugu bilinmiyor.  Izleme
  scriptiyle (tools/trace.lua) diyalog acilirken hangi indeksin
  okundugu yakalanabilir.
- `ESPANOL` bulunamadi; besinci dilin ne oldugu dogrulanmadi.
- Seviye ad tablosundaki (`0x03D0000`) sonekler bolge kodu gibi duruyor
  (`a1`-`a7`, `c1`-`c3`, `k4`, `v1`); onekler nesne turu (`brief`,
  `briefing`, `pager`, `mafia`, `guard`, `ambush`, `playerstart`).
  Hikayenin brifing ve cagri cihazi mesajlari olarak orgutlendigini
  gosteriyor.

## Okuma

`tools/dump_text.py` yerelde calisir, ciktisi `build/` altina yazilir ve
depoya GIRMEZ.
