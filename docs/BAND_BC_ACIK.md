# Band B/C — kapanan is (tarihce)

**DURUM: HER IKI MADDE DE KAPANDI (2026-09-07).**

- `0x08031844` -> `src/video/blit_strip_clip_left.c`, BYTE-MATCHING.
  Cozum: eslesen kardesi `0x08031A1C` ile ROM govdelerini diff'lemek.
  235 komutun 235'i ayniydi; fark yalnizca sutun testinin kutbuydu
  (`if (col++ >= 0)` -> `if (col++ < 0)`). Yontem docs/WORKFLOW.md §10.
- `0x0803173E` sinir hatasi duzeltildi; yerine gelen `0x08031684` ve
  `0x080316B0` -> `src/video/blit_strip_plain.c`, ikisi de BYTE-MATCHING.

Asagisi, cozumden ONCEKI ajan notlaridir; yontem dersi icin birakildi:
yazmac oncelikleri dogru olculmustu ama YANLIS SORU soruluyordu.

---

## 0x08031844 — 472 bayt, 8x48 4bpp serit cizici (ESLESMEDI)

226/235 komut ayni. Kalan fark tek bir uc yazmacli dongusel yer
degistirme (`{p0, under, mask}` -> r3/r4/r5) ve fazladan bir
`mov ip,r6`. `tools/dump_alloc.py` olcumu: biriktiricinin onceligi
2.263'un uzerinde olmali ama 1.751 (referans 62 / omur 177);
gereken ya omur <= 136 ya da referans >= 67, ve KAYNAK DUZEYINDE
bunu veren bir yazim bulunamadi (~40 yazim elendi).

Ajanin dosya ici notu ve calisan govde asagida; yeni bir denemeye
buradan baslanmali.

```c
/* ---- 0x08031844 — 470 bayt, ESLESMEDI ------------------------------------
 *
 * 8 piksel genisliginde, 48 satirlik bir 4bpp seridi olusturuyor.  Her satir
 * icin: satir numarasi [0, height) disindaysa sekiz kaynak baytini oldugu gibi
 * aliyor; icindeyse her piksel icin sutun numarasi negatifse yine kaynagi,
 * degilse `(under[k] & mask[k]) | src[k]` harmanini aliyor.  Sekiz deger dort
 * bitlik alanlara paketlenip iki yarim soz olarak `out`a yaziliyor.  Satir
 * sonunda `mask` degisken adimla (stride), `under` ve `src` 40 bayt ilerliyor
 * (sekiz piksel zaten teker teker ilerletildigi icin satir adimi 48).
 * Parametre adlari ROLE TAHMINIDIR; kanit yalnizca ilerleme adimlari.
 *
 * DURUM: YAZMAC ADLARI NORMALIZE EDILDIGINDE 235 KOMUTUN 226'SI BIREBIR AYNI.
 * Ham olcum 160/236 komut, 329/472 bayt; ham sayilar yaniltici cunku farkin
 * neredeyse tamami TEK bir yazmac permutasyonundan geliyor (ROM'un r3/r4/r5
 * uclusunu {p0, under, mask} olarak yeniden adlandirip karsilastirinca geriye
 * yalnizca asagidaki iki madde kaliyor).
 *
 * KALAN FARK TAM OLARAK IKI SEY:
 *  1) UC YAZMACLIK DONGUSEL PERMUTASYON.
 *     ROM:  r3 = p0 (paket biriktiricisi), r4 = under (arg5), r5 = mask (arg4)
 *     Biz:  r5 = p0,                       r3 = under,        r4 = mask
 *     tools/dump_alloc.py ile olculen global dagitici oncelikleri:
 *         pseudo 27 (under) refs 89 omur 236 oncelik 2.263  -> r3
 *         pseudo 26 (mask)  refs 89 omur 237 oncelik 2.253  -> r4
 *         pseudo 33 (p0)    refs 62 omur 177 oncelik 1.751  -> r5
 *     ROM'un dagilimi icin p0'in onceligi 2.263'un USTUNE cikmali.  Oncelik
 *     `floor_log2(refs) * refs / omur` oldugu icin bunun iki yolu var:
 *         (a) refs 62 sabitken omur <= 136 (su an 177), ya da
 *         (b) omur 177 sabitken refs >= 67 (su an 62).
 *     Kaynak duzeyinde ikisini de saglayacak bir kaldirac BULUNAMADI.
 *  2) Atlanan satir yolunda fazladan bir `mov ip, r6` (2 bayt; 472 vs 470).
 *     p1'in global yazmaci ROM'da r6, bizde ip; bu da (1)'in yan urunu.
 *
 * OLCULUP ELENEN YAZIMLAR (tekrar denemeyin — hicbiri (1)'i degistirmedi):
 *   bildirim sirasi: p'lerin i/x/y'ye gore 10 permutasyonu -> hepsi ayni
 *   tipler: p0..p3 icin u8 / s32 / int / u16 -> hepsi ayni
 *   kapsam: p0..p3'u dongu govdesinde bildirmek -> ayni
 *   `register` anahtar sozcugu (p0'a ve dordune birden) -> ayni
 *   paketleme: ayri `w` degiskeni, `p0 |= ...` biriktirme, ters sirali OR,
 *     ikili gruplama, `p0 = PACK(...)` geri atama, `(a) | ((a) & 0)`
 *   omur no-op'lari (kural 50): `mask++; mask--;`, `under++; under--;`,
 *     `src++; src--;`
 *   sekiz ayri deger degiskeni (ikinci grup icin q0..q3) -> 97/238, cok kotu
 *   son store'u iki dala da kopyalamak (cross-jump umuduyla) -> 476 bayt
 *   `out` icin yerel kopya, `out[0] = ...; out++;` -> ayni ya da kotu
 *   `x` icin `if (x < 0) ... x++;` (artirimi sona almak) -> ayni
 *   atlanan yolda `mask += 8; under += 8;` ciftini basa/ortaya almak: bayt
 *     sayisi 470'e iniyor ama komut SIRASI ROM'dan sapiyor (218-219/235);
 *     ROM'daki yer sekiz okumadan SONRA, asagidaki gibi.
 *   isaretci artirimlarini if/else'ten SONRA tek yere almak: agbcc onlari
 *     birlestiriyor, cikti 8 komut kisaliyor (117/235).  ROM ikiye kopyaladigi
 *     icin kaynakta da IKI dalda ayri ayri yazilmalari gerekiyor.
 *
 * COZULEN YAPISAL AYRINTILAR (bunlari degistirmeyin):
 *   - Dis dongu GERIYE sayan bir sayac olmali (`for (i = ROWS; i != 0; i--)`)
 *     ve satir numarasi `y0 + (ROWS - i)` olarak yazilmali.  Artan `for` +
 *     `y = y0 + i` yazimi ROM'un `ldr y0 / adds #48 / subs sayac` uclusunu
 *     vermiyor; parantezleme de onemli: `y0 + ROWS - i` ve `y0 - i + ROWS`
 *     baska komut sirasi uretiyor, yalnizca `y0 + (ROWS - i)` (ve esdegeri
 *     `y0 - (i - ROWS)`) ROM'unkini veriyor.
 *   - Sutun testi `if (x++ < 0)` bicimi: ROM `adds r0,r6,#0 / adds r6,#1 /
 *     cmp r0,#0` yani ONCE artirim.
 *   - Satir testi `||` ile tek ifade (kural 60): `if (y >= height || y < 0)`.
 *     Atlanan govde ONCE gelmeli.
 *   - Grup 1'in paketleme+store'u IKI dalda da ayri yazilmali; yalnizca grup
 *     2'ninki paylasilan kuyruk (agbcc cross-jump ile kendisi birlestiriyor).
 *   - `mask[0] & under[0]` sirasi: agbcc bunu ters cevirip once `under`i
 *     okuyor, ROM da once arg5'i okuyor.  Artirim siralari her dalda ayri
 *     olculdu (negatif dalda src/mask/under, harman dalinda under/mask/src).
 */

#define ROWS       48
#define ROW_STRIDE 48
#define GROUP      8

/* Tek piksel: sutun negatifse yalnizca kaynak, degilse harman.  Iki dal da
   uc isaretciyi kendi icinde ilerletir (ROM ikisini de kopyalamis). */
#define FETCH(v)                                    \
    if (x++ < 0) {                                  \
        v = src[0];                                 \
        src++;                                      \
        mask++;                                     \
        under++;                                    \
    } else {                                        \
        v = (mask[0] & under[0]) | src[0];          \
        under++;                                    \
        mask++;                                     \
        src++;                                      \
    }

#define PACK(a, b, c, d)  ((a) | ((b) << 4) | ((c) << 8) | ((d) << 12))

/* 0x08031844 */
void BlitStripClipLeft4bpp(s32 y0, s32 x0, s32 height, s32 stride,
                  const u8 *mask, const u8 *under, u16 *out, const u8 *src)
{
    s32 i;
    s32 x;
    s32 y;
    u32 p0;
    u32 p1;
    u32 p2;
    u32 p3;

    for (i = ROWS; i != 0; i--) {
        x = x0;
        y = y0 + (ROWS - i);
        if (y >= height || y < 0) {
            p0 = *src++;
            p1 = *src++;
            p2 = *src++;
            p3 = *src++;
            *out++ = PACK(p0, p1, p2, p3);
            p0 = *src++;
            p1 = *src++;
            p2 = *src++;
            p3 = *src++;
            mask += GROUP;
            under += GROUP;
        } else {
            FETCH(p0)
            FETCH(p1)
            FETCH(p2)
            FETCH(p3)
            *out++ = PACK(p0, p1, p2, p3);
            FETCH(p0)
            FETCH(p1)
            FETCH(p2)
            FETCH(p3)
        }
        *out++ = PACK(p0, p1, p2, p3);
        mask += stride;
        under += ROW_STRIDE - GROUP;
        src += ROW_STRIDE - GROUP;
    }
}
```

## 0x0803173E — SINIR YANLIS, FONKSIYON DEGIL

0x0803173E — SINIR YANLIS, FONKSIYON DEGIL (olculdu, C yazilmadi)
---------------------------------------------------------------------
Kanit:
1. 0x0803173E'deki ilk komut `adds r4,#1`; prolog yok.
2. 0x080317DC'deki `b.n 0x80316CE` GERIYE, kayitli baslangictan ONCEYE
daliyor -- yani govde 0x0803173E'den once basliyor.
3. 0x080317DE'deki epilog `pop {r3,r4,r5} / mov r8..sl / pop {r4-r7} /
pop {r0} / bx r0`; buna karsilik gelen prolog 0x080316B0'da:
`push {r4,r5,r6,r7,lr} / mov r7,sl / mov r6,r9 / mov r5,r8 /
push {r5,r6,r7} / sub sp,#4`.
4. 0x08031684-0x080316AF arasi AYRI ve tam bir fonksiyon
(`push {r4,lr}` ... `bx r0`, 0x080316AC'de havuz kelimesi
0x02025810) -- data/functions.csv'de hic kayitli degil.
Sonuc: 186 baytlik "bosluk" aslinda iki fonksiyon; gercek sinir
0x080316B0-0x080317EE (318 bayt) ve 0x0803173E onun govde ortasi.
0x0803173E icin C yazilmadi.  (Gercek fonksiyon 0x08031A1C'nin
kardesi: ayni sekiz-nibble maskeli serit cizici, tek fark satir
sayisi ve parametre yerlesimi.)
