/* Reissue a target action whenever its action controller becomes ready. */
#include "target_common.h"
u32 FUN_08047600(TargetActor *self)
{
 TargetContext *context=self->context;
 TargetActor *candidate=context->details->target;
 TargetActor *target;
 u16 *phase;
 if(candidate) target=linked(candidate); else target=nearest(self);
 phase=&context->details->phase;
 if(!target) goto done;
 if(self) { int result=0; if(self->pose->state==2) result=1; if(result) goto done; }
 if(target) { int result=0; if(target->pose->state==2) result=1; if(result) goto done; }
 if(!IsActorUsable(target)) goto done;
 goto usable;
done:
 return 1;
usable:
 if(target->flags&1) {
  if(!*phase) {
   FUN_0803f78c(self->action,target);
   *phase=1;
  } else if(actionReady(self->action)) FUN_0803f78c(self->action,target);
 }
 return 0;
}
