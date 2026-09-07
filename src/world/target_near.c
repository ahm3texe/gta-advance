/* Hedefe yakinlik sinamasi -- 0x08045AA4-0x08045CBF (540 bayt)
 *
 * DURUM: ESLESTI -- 0/540 fark.  Onceki durum 12/540 idi.
 *
 * KALAN 12 BAYT NEYDI
 *   ROM  : ldr r0,[pc]+4 / mov ip,r0 / ldr r7,[pc]+8   sonra ldr r1,[r0] / ldr r0,[r7]
 *   bizim: ldr r7,[pc]+4 / ldr r0,[pc]+8 / mov ip,r0   sonra ldr r1,[r7] / ldr r0,[r0]
 * Havuz sirasi iki tarafta da ayni (3D8, 224, 3D0, 3DC); yalnizca
 * gRam02000224 ile gRam0202F3D0 ADRESLERI ters yazmaclara dusuyordu:
 * ROM ip<-224 / r7<-3D0, bizde r7<-224 / ip<-3D0.
 *
 * OLCULEN MEKANIZMA (yeni kural adayi -- rapora da yazildi)
 *   Bu iki adres, gcse'nin urettigi iki pseudo (bizde 147 = &gRam02000224,
 *   148 = &gRam0202F3D0).  .greg dokumu:
 *       Register 147, refs = 5, live_length = 484
 *       Register 148, refs = 5, live_length = 484
 *   Kural 50 onceligi IKISINDE DE ESIT: floor_log2(5)*5/484 -> 206.
 *   Esitlikte global.c kucuk allocno'yu (= kucuk pseudo numarasini) once
 *   isler, ilk islenen en kucuk bos yazmaci (r7) alir, ikincisi r12'ye
 *   duser.  Yani bu farkta OMUR/REFS kaldiraci YOK; is pseudo NUMARASINDA.
 *
 *   148'in omru 147'ninkinden ASLA kisa olamaz (147 `ldr`de, 148 bir sonraki
 *   `str`de oluyor), refs de esit; dolayisiyla 148 onceligi ile one gecemez.
 *   Geriye tek yol kaliyor: pseudo NUMARALARINI takas etmek.
 *
 *   Numaralari gcse'nin ifade tablosundaki KOVA (bucket) sirasi belirliyor.
 *   Kova, adresin literal havuzu etiketinin (.LCn) adindan hesaplaniyor ve
 *   OLCULDU (probe ile n=7..34 tarandi):
 *       tek haneli n : kova = n + 100      (.LC7=107 .LC8=108 .LC9=109)
 *       iki haneli n : kova = n + 11       (.LC10=21 .LC11=22 ... .LC34=45)
 *   Yani .LCn -> .LC(n+1) her zaman kovayi +1 yapar, TEK ISTISNA .LC9->.LC10
 *   (109 -> 21).  Kucuk kova once islenir, once islenen kucuk pseudo alir.
 *
 *   Bizim TU'da nearest() satir disi da derlendigi icin (baslikta static
 *   __inline__; .rtl dokumunde ";; Function nearest" var) .LC0-.LC6'yi o
 *   yiyor, IsTargetNear .LC7'den basliyordu:
 *       .LC7=gGameState .LC8=gRam02000F10 .LC9=gRam0202F3D8
 *       .LC10=gRam02000224(kova 21) .LC11=gRam0202F3D0(kova 22)
 *   21 < 22 -> 224 once -> 224 r7'yi kapiyor.  ROM'un istedigi tersi.
 *
 * COZUM: gRam02000224'un havuz etiketini ONE ALMAK.  Fonksiyonun basinda
 * OLU bir gonderme, RTL uretimi sirasinda .LC7'yi (kova 107) ona verdiriyor;
 * gRam0202F3D0 .LC11'de (kova 22) kaliyor.  22 < 107 oldugu icin bu kez 3D0
 * once isleniyor ve r7'yi aliyor -- ROM ile birebir.  `if(z)` govdesi
 * jump/cse tarafindan silindigi icin TEK BAYT kod uretmiyor: boyut 540'ta
 * kaldi, 12 fark 0'a indi.
 *
 * ELENEN -- olu gondermenin CALISMAYAN bicimleri:
 *   (void)gRam02000224;        -> parse'ta katlaniyor, .LC hic uretilmiyor,
 *                                 12 fark aynen kaliyor
 *   u32 pre=gRam02000224;      -> yuk DCE'ye takiliyor ama ADRES yasiyor,
 *                                 cse sonraki kullanimlarla birlestiriyor,
 *                                 adres bastan r4'te duruyor: 538 bayt/286 fark
 *   return'den SONRA olu blok  -> .LC etiketi gec uretiliyor, sira degismiyor
 *   fonksiyondan ONCE olu statik yardimci (1..5 adet denendi) -> etiketleri
 *                                 YUKARI kaydiriyor, gereken YON ASAGI:
 *                                 12 -> 14 fark
 * CALISAN bicimlerin hepsi ayni (0 fark): `if(z) separation=gRam02000224;`,
 * `if(z) tick=&gRam02000224;`, `separation=0; if(separation) ...`.  Adres mi
 * deger mi okundugu fark etmiyor; onemli olan .LC'nin ERKEN uretilmesi.
 *
 * ELENEN -- kural 50'nin normal kaldiraclari (hepsi ETKISIZ, 12'de kaldi):
 *   yerel bildirim sirasini takas, result'i fonksiyon basina alma,
 *   result'i u32 yapma, if(result)/if(!result) takasi, 4==kind,
 *   GetBaseAlt()==0, GetBaseAlt()>=2, target==0, if/else kolunu cevirme,
 *   result'i else ile yazma.  Hepsi ayni RTL'i uretiyor: 12/540.
 *   distance()'i if(!target)'tan ONCE cagirma -> 40 fark.
 *   `separation<=DIST_NEAR goto yes` yerine `>DIST_NEAR goto no` -> 528 bayt.
 *
 * PAYLASILAN BASLIK DOGRU -- kanit: ayni satir-ici nearest() kodu
 * target_follow.c (0x0804B088) ve target_repeat.c (0x08047600) icinde
 * BIREBIR tutuyor.  Baslik DEGISTIRILMEDI.
 *
 * ELENEN -- baslik degisiklikleri (hepsi diger IKI eslesmeyi KIRDI):
 *   karsilastirma operandlarini takas  -> 45aa4=11 ama 4b088/47600=13
 *   karsilastirma sirasini degistir    -> ucu de 36
 *   damga icin yerel degisken          -> 45aa4=11 ama digerleri=3
 *   tick icin yerel degisken           -> degisiklik yok
 *
 * ELENEN -- bu dosyanin yeniden yapilandirilmasi (hepsi DAHA KOTU):
 *   GetBaseAlt() yereline alma  -> 528 bayt, 240 fark
 *   olu `result` yapisini kaldirma -> 528 bayt, 75 fark
 *   kind'i canli tutma / target on-atamasi -> 528 bayt, 75 fark
 * Olu `result` yapisi YUK TASIYOR: fonksiyonu tam 540 bayta getiriyor.
 *
 */

#include "target_common.h"
u32 IsTargetNear(TargetActor *self)
{
 TargetActor *target;
 int separation;
 int z=0;
 if(z) separation=gRam02000224;
 if(!GetBaseAlt()) goto no;
 target=nearest(self);
 if(!target) goto no;
 separation=distance(self,target);
 if(GetBaseAlt()>1) {
  if(separation<=DIST_NEAR) goto yes;
  goto no;
 } else {
  if(target) {
   int result=0;
   if(target->kind==4) result=1;
   if(result) { if(separation>DIST_NEAR) goto no; }
   else { if(separation>DIST_NEAR) goto no; }
  } else { if(separation>DIST_NEAR) goto no; }
 }
yes:
 return 1;
no:
 return 0;
}
