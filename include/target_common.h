#include "gba_types.h"
#include "ram_symbols.h"
#define DIST_MAX 0x7fffffff
#define DIST_NEAR 0x00ffffff

typedef struct TargetActor TargetActor;
typedef struct TargetPose { s32 x,y; u8 pad8[40]; u8 state; } TargetPose;
typedef struct TargetAction { u8 pad0[129]; u8 state; } TargetAction;
typedef struct TargetTask { u8 pad0[8]; u8 state; u8 pad9[31]; u32 kind; TargetActor *target; } TargetTask;
typedef struct TargetDetails { u8 pad0[10]; u16 phase; u8 pad12[12]; TargetActor *target; } TargetDetails;
typedef struct TargetContext { u8 pad0[36]; TargetDetails *details; } TargetContext;
struct TargetActor {
 u8 pad0[8]; u8 kind; u8 pad9[3]; u32 flags; u8 pad16[8];
 TargetPose *pose; TargetTask *task; u8 *alt; TargetAction *action;
 TargetContext *context; TargetActor *link; TargetActor *owner;
};
extern u8 gGameState[];
extern u32 gRam02000224, gRam0202F3D0;
extern TargetActor *gRam0202F3D8, *gRam0202F3DC;
extern u32 IsActorUsable(TargetActor *actor);
extern void FUN_0803f78c(TargetAction *action, TargetActor *target);
extern u32 GetBaseAlt(void);

static __inline__ int distance(TargetActor *a,TargetActor *b)
{
 TargetPose *pa,*pb;
 int dx,dy,lo,result;
 u32 mask;
 if (!a || !b) return DIST_MAX;
 mask=0x30;mask &= a->kind;
 if(mask) pa=(TargetPose *)(a->alt+4); else pa=a->pose;
 mask=0x30;mask &= b->kind;
 if(mask) pb=(TargetPose *)(b->alt+4); else pb=b->pose;
 dx=pa->x-pb->x; if(dx<0) dx=-dx;
 dy=pa->y-pb->y; if(dy<0) dy=-dy;
 lo=dy; if(lo>dx) lo=dx;
 result=dx+dy-(lo>>1)-(lo>>2)+(lo>>4);
 if(result<0) result=-result;
 return result;
}
static __inline__ TargetActor *nearest(TargetActor *self)
{
 int first,second;
 if(!gGameState[12]) return *(TargetActor **)gRam02000F10;
 if(self==gRam0202F3D8 && gRam02000224==gRam0202F3D0) return gRam0202F3DC;
 first=distance(*(TargetActor **)gRam02000F10,self);
 second=distance(*(TargetActor **)gRam02001140,self);
 gRam0202F3D8=self;
 gRam0202F3D0=gRam02000224;
 if(first<second) gRam0202F3DC=*(TargetActor **)gRam02000F10;
 else gRam0202F3DC=*(TargetActor **)gRam02001140;
 return gRam0202F3DC;
}
static __inline__ TargetActor *linked(TargetActor *target)
{
 TargetActor *p=target->link;
 if(!p || !(p=p->owner) || !(p=(TargetActor *)p->context)) p=target;
 return p;
}
static __inline__ int special(TargetActor *actor)
{
 if(actor) return actor->pose->state==2;
 return 0;
}
static __inline__ int actionReady(TargetAction *action)
{
 return action->state==1;
}
