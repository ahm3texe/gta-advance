/* The target proximity test -- 0x08045AA4-0x08045CBF (540 bytes)
 *
 * STATUS: MATCHED -- 0/540 off.  It was previously 12/540.
 *
 * WHAT THE REMAINING 12 BYTES WERE
 *   the ROM: ldr r0,[pc]+4 / mov ip,r0 / ldr r7,[pc]+8   then ldr r1,[r0] / ldr r0,[r7]
 *   ours   : ldr r7,[pc]+4 / ldr r0,[pc]+8 / mov ip,r0   then ldr r1,[r7] / ldr r0,[r0]
 * The pool order is the same on both sides (3D8, 224, 3D0, 3DC); only the
 * ADDRESSES of gRam02000224 and gRam0202F3D0 landed in the opposite registers:
 * the ROM has ip<-224 / r7<-3D0, we had r7<-224 / ip<-3D0.
 *
 * THE MEASURED MECHANISM (a candidate for a new rule -- also written up in the
 * report)
 *   These two addresses are two pseudos produced by gcse (147 = &gRam02000224
 *   and 148 = &gRam0202F3D0 for us).  The .greg dump:
 *       Register 147, refs = 5, live_length = 484
 *       Register 148, refs = 5, live_length = 484
 *   The rule 50 priority is EQUAL FOR BOTH: floor_log2(5)*5/484 -> 206.
 *   On a tie, global.c processes the smaller allocno (= the smaller pseudo
 *   number) first, and whichever is processed first takes the lowest free
 *   register (r7) while the second falls to r12.  So there is NO
 *   LIFETIME/REFS lever in this difference; it is all in the pseudo NUMBER.
 *
 *   148's lifetime can NEVER be shorter than 147's (147 dies at an `ldr`, 148
 *   at the following `str`) and the refs are equal, so 148 cannot get ahead on
 *   priority.  One route remains: swapping the pseudo NUMBERS.
 *
 *   The numbers are decided by the BUCKET order in gcse's expression table.
 *   The bucket is computed from the name of the address's literal pool label
 *   (.LCn) and was MEASURED (probed for n=7..34):
 *       single-digit n : bucket = n + 100      (.LC7=107 .LC8=108 .LC9=109)
 *       two-digit n    : bucket = n + 11       (.LC10=21 .LC11=22 ... .LC34=45)
 *   So .LCn -> .LC(n+1) always adds 1 to the bucket, with the SINGLE EXCEPTION
 *   .LC9->.LC10 (109 -> 21).  The smaller bucket is processed first, and
 *   whichever is processed first gets the smaller pseudo.
 *
 *   Because nearest() is also compiled out of line in our TU (it is
 *   static __inline__ in the header; the .rtl dump has ";; Function nearest"),
 *   it eats .LC0-.LC6 and IsTargetNear started at .LC7:
 *       .LC7=gGameState .LC8=gRam02000F10 .LC9=gRam0202F3D8
 *       .LC10=gRam02000224(bucket 21) .LC11=gRam0202F3D0(bucket 22)
 *   21 < 22 -> 224 first -> 224 takes r7.  The reverse of what the ROM wants.
 *
 * THE SOLUTION: MOVE gRam02000224's pool label EARLIER.  A DEAD reference at
 * the top of the function makes RTL generation give it .LC7 (bucket 107),
 * while gRam0202F3D0 stays at .LC11 (bucket 22).  Since 22 < 107, 3D0 is now
 * processed first and takes r7 -- exactly as in the ROM.  Because the `if(z)`
 * body is deleted by jump/cse it produces NOT ONE BYTE of code: the size
 * stayed at 540 and the 12 differences went to 0.
 *
 * RULED OUT -- forms of the dead reference that DO NOT WORK:
 *   (void)gRam02000224;        -> folded at parse time, no .LC is ever
 *                                 produced, the 12 differences remain
 *   u32 pre=gRam02000224;      -> the load is caught by DCE but the ADDRESS
 *                                 survives, cse merges it with the later uses
 *                                 and the address sits in r4 from the start:
 *                                 538 bytes/286 off
 *   a dead block AFTER the return -> the .LC label is produced late, the order
 *                                 does not change
 *   a dead static helper BEFORE the function (1..5 of them tried) -> it shifts
 *                                 the labels UP, and the direction needed is
 *                                 DOWN: 12 -> 14 off
 * All the forms that DO work are equivalent (0 off):
 * `if(z) separation=gRam02000224;`, `if(z) tick=&gRam02000224;`,
 * `separation=0; if(separation) ...`.  Whether the address or the value is
 * read makes no difference; what matters is that the .LC is produced EARLY.
 *
 * RULED OUT -- rule 50's usual levers (all INEFFECTIVE, it stayed at 12):
 *   swapping the local declaration order, moving result to the top of the
 *   function, making result u32, swapping if(result)/if(!result), 4==kind,
 *   GetBaseAlt()==0, GetBaseAlt()>=2, target==0, inverting the if/else branch,
 *   writing result with an else.  They all produce the same RTL: 12/540.
 *   Calling distance() BEFORE if(!target) -> 40 off.
 *   `>DIST_NEAR goto no` instead of `separation<=DIST_NEAR goto yes` -> 528
 *   bytes.
 *
 * THE SHARED HEADER IS RIGHT -- the evidence: the same inlined nearest() code
 * matches EXACTLY in target_follow.c (0x0804B088) and target_repeat.c
 * (0x08047600).  The header was NOT CHANGED.
 *
 * RULED OUT -- header changes (all of them BROKE the other TWO matches):
 *   swapping the comparison operands  -> 45aa4=11 but 4b088/47600=13
 *   changing the comparison order     -> all three at 36
 *   a local variable for the stamp    -> 45aa4=11 but the others=3
 *   a local variable for the tick     -> no change
 *
 * RULED OUT -- restructuring this file (all WORSE):
 *   taking GetBaseAlt() into a local  -> 528 bytes, 240 off
 *   removing the dead `result` structure -> 528 bytes, 75 off
 *   keeping kind live / pre-assigning target -> 528 bytes, 75 off
 * The dead `result` structure CARRIES WEIGHT: it brings the function to
 * exactly 540 bytes.
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
