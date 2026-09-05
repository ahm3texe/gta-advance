/* Hedefe yakinlik sinamasi — 0x08045AA4-0x08045CBF (540 bayt)
 *
 * DURUM: PARK — 12/540 fark, BOYUT DOGRU.  Kalan fark saf yazmac atamasi:
 *     ROM  : ldr r0,[pc] / mov ip,r0 / ldr r7,[pc]   sonra ldr r1,[r0] / ldr r0,[r7]
 *     bizim: ldr r7,[pc] / ldr r0,[pc] / mov ip,r0   sonra ldr r1,[r7] / ldr r0,[r0]
 * Yani gRam02000224 ve gRam0202F3D0 ters yazmaclara dusuyor.
 *
 * PAYLASILAN BASLIK DOGRU — kanit: ayni satir-ici nearest() kodu
 * target_follow.c (0x0804B088) ve target_repeat.c (0x08047600) icinde
 * BIREBIR tutuyor.  Fark bu fonksiyonun kendi yazmac baskisindan geliyor.
 *
 * ELENEN — baslik degisiklikleri (hepsi diger IKI eslesmeyi KIRDI):
 *   karsilastirma operandlarini takas  -> 45aa4=11 ama 4b088/47600=13
 *   karsilastirma sirasini degistir    -> ucu de 36
 *   damga icin yerel degisken          -> 45aa4=11 ama digerleri=3
 *   tick icin yerel degisken           -> degisiklik yok
 *
 * ELENEN — bu dosyanin yeniden yapilandirilmasi (hepsi DAHA KOTU):
 *   GetBaseAlt() yereline alma  -> 528 bayt, 240 fark
 *   olu `result` yapisini kaldirma -> 528 bayt, 75 fark
 *   kind'i canli tutma / target on-atamasi -> 528 bayt, 75 fark
 * Olu `result` yapisi YUK TASIYOR: fonksiyonu tam 540 bayta getiriyor.
 *
 */

#include "target_common.h"
u32 FUN_08045aa4(TargetActor *self)
{
 TargetActor *target;
 int separation;
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
