#ifndef PHASE1_TYPES_H
#define PHASE1_TYPES_H
#include "gba_types.h"
/* Iki hedefli eylem kaydinin ikiz kurulum/adim fonksiyonlarinda olculen alanlari. */
typedef struct Phase1Action Phase1Action;
struct Phase1Action {
    void *owner;
    u32 pad04;
    u32 (*step)(Phase1Action *);
    u8 pad0c[32];
    void *target;
    u16 result;
    u8 pad32[78];
    u8 state;
    u8 done;
};
typedef struct Phase1ReleasePair {
    u8 objects[168];
    u16 active;
} Phase1ReleasePair;
typedef struct Phase1Resource {
    u8 pad00[12];
    u32 length;
    u8 data[1];
} Phase1Resource;
typedef struct Phase1Model {
    u8 pad00[22];
    u16 value;
    u8 pad18[54];
    u16 alternate;
} Phase1Model;
typedef struct Phase1ActorDesc {
    u8 pad00[6]; u16 kind; u8 pad08[32]; u32 id;
} Phase1ActorDesc;
typedef struct Phase1Actor {
    u8 pad00[8]; u8 kind; u8 pad09[19]; Phase1ActorDesc *desc;
} Phase1Actor;
typedef struct Phase1RomNode {
    u32 pad00; struct Phase1RomNode **slots; u8 pad08[12]; u32 value;
} Phase1RomNode;
typedef struct Phase1ListNode {
    u32 pad00[2]; s32 id; u32 pad0c[2];
    struct Phase1ListNode *next, *previous;
} Phase1ListNode;
typedef struct Phase1NodePool {
    Phase1ListNode nodes[32]; Phase1ListNode *free, *used;
} Phase1NodePool;
typedef struct Phase1StatTable { u16 ids[12]; u8 values[12]; } Phase1StatTable;
typedef struct Phase1RecordGroup { u8 pad00[13]; u8 count; u16 pad0e; u16 *ids; } Phase1RecordGroup;
typedef struct Phase1GroupedActor { u8 pad00[24]; u32 flags; u16 pad1c; u16 state; u8 pad20[12]; Phase1RecordGroup *group; } Phase1GroupedActor;
typedef struct Phase1SoundState { u32 pad00[2]; u32 handle; } Phase1SoundState;
typedef struct Phase1SoundActor { u8 pad00[52]; Phase1SoundState *sound; } Phase1SoundActor;
typedef struct Phase1SlotContext {
    u8 pad00[28]; u8 *current; u8 pad20[142]; u16 flags;
} Phase1SlotContext;
typedef struct Phase1SlotHead { void *entry; u32 kind; } Phase1SlotHead;
typedef struct Phase1Entry {
    u8 active,pad01; u16 timer; u8 object[136]; u32 state; u32 key;
} Phase1Entry;
typedef struct Phase1EntryOwner { u8 pad00[28]; u8 *data; } Phase1EntryOwner;
/* Soft-float unpacker output: kind, sign, exponent and fraction words. */
typedef struct Phase1FloatParts { u32 kind,sign; s32 exponent; u32 fraction; } Phase1FloatParts;
typedef struct Phase1DoubleParts { u32 kind,sign; s32 exponent; u32 fraction[2]; } Phase1DoubleParts;
typedef struct Phase1NotifyConfig { u8 pad00[204]; u32 enabled; } Phase1NotifyConfig;
typedef struct Phase1NotifyContext { u32 pad00; Phase1NotifyConfig *config; u8 pad08[32]; u32 flags; } Phase1NotifyContext;
typedef struct Phase1Heading { s32 x,y,z,yaw,targetYaw; } Phase1Heading;
typedef struct Phase1NotifyGate { u8 pad00[39]; u8 enabled; } Phase1NotifyGate;
typedef struct Phase1NotifyTask { u8 pad00[60]; Phase1NotifyGate *gate; } Phase1NotifyTask;
typedef struct Phase1SpriteDesc {
    u32 pad00; struct Phase1SpriteDesc **slots; u32 pad08;
    u8 count; u8 pad0d[3]; const u8 **pieces;
} Phase1SpriteDesc;
typedef struct Phase1TextState {
    u8 pad00[4412]; void *current,*previous; u8 pad1144[584]; u8 buffer[1];
} Phase1TextState;
typedef struct Phase1Readiness { u8 pad00[52]; s16 value; } Phase1Readiness;
typedef struct Phase1ReadinessActor { u8 pad00[24]; Phase1Readiness *state; } Phase1ReadinessActor;
typedef struct Phase1LinkedActor { u8 pad00[44]; Phase1GroupedActor *linked; } Phase1LinkedActor;

/* --- src/world/band_0803afbc.c'den BIREBIR kopyalanan govdeler ---------
 * gRam02000F08 orada `Ctx *` olarak bildirili; tutarlilik kapisi ayni
 * sembol icin ayni struct govdesini sart kostugu icin govdeler aynen
 * alindi. (Faz 3'te bu tipler tek bir paylasilan baslikta toplanacak.) */
typedef struct Vec3 {
    u32 x;
    u32 y;
    u32 z;
} Vec3;
typedef struct Node {
    u8  state;                  /* +0x00 */
    u8  pad01[1];
    u16 pending;                /* +0x02 */
} Node;
typedef struct AltBody {
    u32  pad00;
    Vec3 pos;                   /* +0x04 */
} AltBody;
typedef struct Item {
    u8       pad00[8];
    u8       flags;             /* +0x08 */
    u8       pad09[15];
    Vec3    *pos;               /* +0x18 */
    u8       pad1c[4];
    AltBody *alt;               /* +0x20 */
} Item;
typedef struct NodeSlot {
    u8 raw[8];
} NodeSlot;
typedef struct Ctx {
    u8       pad00[6];
    u16      timer;             /* +0x06 */
    u8       busy;              /* +0x08 */
    u8       pad09[2];
    u8       phase;             /* +0x0B */
    u8       pad0c[4];
    Vec3     pos;               /* +0x10 */
    Node    *node;              /* +0x1C */
    u8       pad20[4];
    NodeSlot slots[17];         /* +0x24 */
    u8       padac[2];
    u16      flags;             /* +0xAE */
    u8       padb0[4];
    Item    *item;              /* +0xB4 */
    u8       step;              /* +0xB8 */
    u8       padb9[3];
    u32      unkBC;             /* +0xBC */
} Ctx;

#endif
