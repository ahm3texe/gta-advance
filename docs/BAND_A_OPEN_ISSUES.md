# band_a — yazilamayan fonksiyonlarin gerekceleri

Ajanin olcumleri. Semboller artik data/ram_map.csv'ye eklendi;
bu fonksiyonlar simdi yazilabilir durumda.

---

## 0x0803308EC

```
/* 0x080308EC -- 72 bayt -- YAZILMADI: gRam02025810 yasak.
 *
 * Havuzda tek bir literal var: 0x0202585C = gRam02025810 + 0x4C, yani
 * src/world/release_slot.c'nin (0x080308AC, eslesiyor) isledigi 24 x 180
 * baytlik yuva dizisinin ta kendisi. Bu fonksiyon o dizinin TAMAMINI
 * gezip her dolu yuvanin nesnesinden 0x02000000 bayragini siliyor, yuva
 * ve +4 alanini sifirliyor, 1 donduruyor -- ReleaseSlot'un "hepsi"
 * surumu. 0x0202585C data/ram_map.csv'de ayri bir sembol DEGIL; tek yol
 * gRam02025810'u kullanmak, o da bu gorevde acikca yasaklandi.
 * (Gorev notu "dokuzun hicbiri 0x02025810'a dokunmuyor" diyordu; ROM'a
 * gore bu DOGRU DEGIL, tek havuz kelimesi tam olarak o blogun +0x4C'si.)
 *
 * Yazilirsa cozulmesi gereken tek ilginc ayrinti: dongu icindeki
 * `cmp r2,r5 / bhi` (isaretsiz) ile dongu sonundaki `cmp r2,ip / ble`
 * (isaretli) AYNI siniri iki farkli isaretlilikte test ediyor ve sinir
 * degeri iki ayri yazmacta (r5 ve ip) tutuluyor -- yani kaynakta ayni
 * ifade iki kez, iki farkli turde yaziliyor. */


```

---

## 0x080331294

```
/* 0x08031294 -- 132 bayt -- YAZILMADI: UC sembol data/ram_map.csv'de yok.
 *     0x02026D90  (4 bayt, u32; sifirsa fonksiyon hemen donuyor)
 *     0x02000008  (2 bayt, u16; son yazilan X degeri)
 *     0x0200000A  (2 bayt, u16; son yazilan Y degeri)
 * Kullanilan iki sembol ram_map'te VAR (gSlotSelector 0x02000D40) ya da
 * functions.csv'de (SelectSlotAB 0x0803C49C, FUN_0802A734, FUN_080625D0).
 *
 * Yapisi: FUN_080625D0'i cagirdiktan sonra, 0x02026D90 kuruluysa
 * SelectSlotAB(gSlotSelector + 1) ile secilen kaydin +0x18
 * isaretcisinden iki sabit noktali koordinati (>>22, u16'ya daraltilmis)
 * okuyor. Parametre sifirken ve iki koordinat da onbellektekiyle
 * ayniysa hicbir sey yapmiyor; degilse onbellegi tazeleyip
 * FUN_0802A734(x, 18, 10) ve FUN_0802A734(y, 22, 10) ile HUD alanlarini
 * yeniden ciziyor. */


```

---

## 0x080331388

```
/* 0x08031388 -- 88 bayt -- YAZILMADI: 0x02026DA0 data/ram_map.csv'de yok
 * (en yakin kayitlar 0x02026CD0 ve 0x02026DF0; ikisinin de araligi bu
 * adresi kapsamiyor).
 *
 * Yapisi: 0x02026DA0'daki nesneyi FUN_08014FFC(obj,0,0,0) ile kurup
 * FUN_08014040(obj, 0x083444E8, 512, 32, 32, 0) ile veriyi yukluyor,
 * FUN_08014EE4(obj, 0x08CA635C) ile paleti/ikinci kaynagi baglıyor,
 * ardindan obj+8'e 200 ve obj+10'a 8 (u16) yazip FUN_08015038(obj) ile
 * gonderiyor. `cmp r4,#0 / beq` -> iki u16 yazimi bir `if (obj != 0)`
 * korumasi altinda, yani nesne kaynakta isaretci olarak tutuluyor.
 * Iki ROM adresi (0x083444E8, 0x08CA635C) icin de birer veri sembolu
 * gerekiyor. */


```

---

## 0x080330F84

```
/* 0x08030F84, 0x08030B40 vb. bu bandin diger adaylaridir; bu gorevin
 * kapsaminda degiller. */

```
