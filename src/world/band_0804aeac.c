/* Hedef secimi + yakin-tehlike bildirimi — FUN_0804aeac @ 0x0804AEAC, 476 bayt
 *
 * Kardes fonksiyon AdvanceTargetAction (0x0804B088, eslesmis) ile ayni aileden:
 * `include/target_common.h` icindeki `linked` / `nearest` / `distance` satir ici
 * yardimcilarini oldugu gibi kullaniyor, struct'lar ve stil oradan geliyor.
 *
 * Yapi:
 *   1. context->details->target doluysa hedef, o adayin link/owner/context
 *      zinciri (`linked`); bos ise iki global aday arasindan sekizgen yaklasik
 *      uzakliga gore en yakini (`nearest`).
 *   2. Binek (self->link) 0x100000 bayragini tasimiyorsa VE ilerleme sayaci
 *      (details +0x0A) sifirsa ve GetOwnerSlot hedefi dogruluyorsa FUN_080502f8
 *      31 koduyla cagriliyor (kilitlenme/uyari bildirimi).
 *   3. Her durumda FUN_0804b2bc(self) sonucu donuyor.
 *
 * OLCULEN IKI KAYNAK KALDIRACI (ikisi de byte-matching icin sart):
 *
 * (a) `actor = self` kopyasi. ROM girisde `adds r6,r0,#0 / adds r5,r6,#0` ile
 *     parametrenin IKI kopyasini tutuyor: r6 context/link/son cagri icin, r5
 *     `nearest` govdesindeki uzaklik olcumu ve gRam0202F3D8 yazimi icin.
 *     Tek degiskenle yazildiginda iki pseudo ayni yazmaca birlesiyor, kopya
 *     komutu kayboluyor ve kayma butun fonksiyona yayiliyor: 152/227.
 *     Ayri `actor` yereliyle 213/227. (Kardes AdvanceTargetAction'da bu kopya
 *     YOK — orada `self` tek pseudo; kural 37'nin "kopya yasamaz" siniri
 *     yazmac baskisina bagli, mutlak degil.)
 *
 * (b) `node` yerelinin YENIDEN KULLANIMI. ROM `adds r2,r0,#0 / adds r2,#10`
 *     ile sayac isaretcisini gercekten kuruyor. Duz `phase=&details->phase`
 *     yazildiginda agbcc'nin cse gecisi adresi kullanim yerine katliyor
 *     (`ldrh r0,[r2,#10]`) ve iki bayt eksiliyor. Kok neden olculdu:
 *     `-fno-cse-skip-blocks` ile fark kayboluyor, yani cse binek kontrolunun
 *     blogunu ATLAYIP taban pseudo'yu hala canli goruyor. Kaynaktaki tek
 *     kaldirac, taban pseudo'yu ARADA YENIDEN ATAMAK: `node` once details'i,
 *     sonra binegi tutuyor; ikinci atama cse'nin denkligini gecersiz kiliyor
 *     ve ROM'un iki komutlu adres kurulumu geri geliyor.
 *
 * ELENEN YAZIMLAR (hepsi olculdu, hicbiri (b)'yi acmadi — 213/227'de kaldi):
 *   phase'i erken/gec hesaplamak; `details` adinda ayri yerel; `&d->phase`
 *   yerine `(u16 *)((u8 *)d+10)`; ic ice / `||` / bayrak yereli ile yazilmis
 *   binek kontrolu; kuyrugu ayri bir `static __inline__` fonksiyona almak;
 *   phase'i `volatile u16 *` yapmak; iki ayri atama; dizi/alt-struct gorunumu;
 *   `*phase`i yerele okumak; bildirim sirasi degisimleri; `*phase=1` ile ikinci
 *   kullanim eklemek (cse yine katliyor, yani sorun kullanim SAYISI degil).
 *   `s16 *phase` (215/227) fold'u kirdi ama `ldrsh r0,[r2,r1]` uretti — yanlis
 *   komut; yine de geri kalan blogun ROM ile birebir oldugunu gosterdi.
 *   `phase=(u16 *)context->details; phase+=5;` (217/227) sihirli ofset, elendi.
 *   `node=(TargetNode *)self->link` ile binegi gec okumak: 168/227 (yukleme
 *   fonksiyon basindan kayiyor, ROM `mov r0,sl` ile onbellekten okuyor).
 *
 * Derleyici: old_agbcc -mthumb-interwork -O2 -fhex-asm  (docs/COMPILER.md)
 * Dogrulama: make c-match FILE=src/world/band_0804aeac.c  -> BYTE-MATCHING
 */

#include "target_common.h"

#define MOUNT_BUSY  (0x80 << 13)    /* 0x00100000 */
#define NOTIFY_KIND 31

/* details (+0x24) ve binek (+0x2C) icin ortak gorunum: her ikisi de +0x0A'da
 * yarim kelime, +0x18'de tam kelime tasiyor. */
typedef struct TargetNode {
    u8  pad0[10];
    u16 phase;                  /* +0x0A */
    u8  pad12[12];
    u32 flags;                  /* +0x18 */
} TargetNode;

extern u32  GetOwnerSlot(TargetActor *actor);
extern void FUN_080502f8(u32 kind, TargetActor *actor);
extern u32  FUN_0804b2bc(TargetActor *self);

u32 FUN_0804aeac(TargetActor *self)
{
 TargetActor *actor=self;
 TargetContext *context=self->context;
 TargetActor *mount=self->link;
 TargetActor *candidate=context->details->target;
 TargetActor *target;
 TargetNode *node;
 u16 *phase;
 if(candidate) target=linked(candidate); else target=nearest(actor);
 node=(TargetNode *)context->details;
 phase=&node->phase;
 node=(TargetNode *)mount;
 if(node && (node->flags&MOUNT_BUSY)) goto done;
 if(*phase) goto done;
 if(!GetOwnerSlot(target)) goto done;
 FUN_080502f8(NOTIFY_KIND,target);
done:
 return FUN_0804b2bc(self);
}
