/* Select the current target and advance the two-phase target action. */
#include "target_common.h"
u32 AdvanceTargetAction(TargetActor *self)
{
 TargetContext *context=self->context;
 TargetActor *candidate=context->details->target;
 TargetActor *target;
 u16 *phase;
 if(candidate) target=linked(candidate); else target=nearest(self);
 phase=&context->details->phase;
 if(self->flags&8) goto done;
 if(self) { int result=0; if(self->pose->state==2) result=1; if(result) goto done; }
 if(target) { int result=0; if(target->pose->state==2) result=1; if(result) goto done; }
 if(!IsActorUsable(target)) goto done;
 if(target->flags&1) {
  switch(*phase) {
  case 0:
   if(self->task) {self->task->kind=0x4003;self->task->target=target;}
   FUN_0803f78c(self->action,target);
   *phase=1;
   break;
  case 1:
   if(actionReady(self->action)) {
    if(self->task) self->task->state=99;
    goto done;
   }
  }
 }
 goto pending;
done:
 return 1;
pending:
 return 0;
}
