/* Secilen hedefe yonel ve yakinlik bildirimlerini gonder — 0x080480F4.
 * Faz 1; ROM ikizleri karsilastirildi. Olcum: data/phase1_evidence/0x080480F4.json. */
#include "gba_types.h"
#include "target_common.h"
#include "phase1_types.h"

#define CONTEXT_FLAG_ACTIVE 0x02000000

extern u32 GetOwnerSlot(TargetActor *);
extern s32 FUN_0800c180(s32,s32);
extern void FUN_080426ec(Phase1NotifyContext *,u32,TargetActor *);
extern void FUN_080502f8(u32,TargetActor *);
static inline void NotifyContext(TargetActor *self,TargetActor *candidate)
{
    Phase1NotifyContext *context = (Phase1NotifyContext *)self->context;
    if (context) {
        u32 flags = CONTEXT_FLAG_ACTIVE;
        flags |= context->flags;
        context->flags = flags;
        if (context->config->enabled) FUN_080426ec(context,25,candidate);
    }
}
static inline u32 CanNotify(TargetActor *self)
{
    Phase1NotifyTask *task = (Phase1NotifyTask *)self->task;
    u32 result;
    if (task) {
        if (!task->gate) return 0;
        result = task->gate->enabled;
    } else result = 1;
    return result;
}
static inline u32 IsKindFour(TargetActor *actor)
{
    u32 result = 0;
    if (actor) { if (actor->kind == 4) result = 1; }
    return result;
}
u32 FUN_080480f4(TargetActor *self)
{
    TargetActor *candidate = self->context->details->target;
    TargetActor *target;
    TargetPose *a,*b;
    s32 dx,dy;
    u32 heading;
    if (candidate) target = linked(candidate); else target = nearest(self);
    if (!(self->flags & 0x10000) || !GetOwnerSlot(target)) return 1;
    a = self->pose; b = target->pose;
    dx = a->x-b->x; if (dx < 0) dx = -dx;
    if (dx > 0xEFFFFF) goto far;
    dy = a->y-b->y; if (dy < 0) dy = -dy;
    if (dy > 0x9FFFFF) goto far;
    if (self->kind & 0x30) a = (TargetPose *)(self->alt+4);
    if (target->kind & 0x30) b = (TargetPose *)(target->alt+4);
    heading = FUN_0800c180(b->x-a->x,b->y-a->y) & 1023;
    if (!IsKindFour(self)) {
        ((Phase1Heading *)self->pose)->yaw = heading << 16;
        ((Phase1Heading *)self->pose)->targetYaw = heading << 16;
    }
    /* ROM evaluates the distance here; its result is not consumed. */
    distance(self,target);
    NotifyContext(self,candidate);
    if (GetOwnerSlot(target) && CanNotify(self)) {
        FUN_080502f8(33,target);
        NotifyContext(self,candidate);
    } else {
        if (GetBaseAlt()) {
            NotifyContext(self,candidate);
            FUN_080502f8(42,target);
        } else {
            FUN_080502f8(42,target);
        }
    }
    return 1;
far:
    FUN_080502f8(42,target);
    return 1;
}
