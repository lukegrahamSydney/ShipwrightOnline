#ifndef PLAYERPUPPET_H
#define PLAYERPUPPET_H

#include <z64.h>

namespace ZeldaOnline {
typedef struct PlayerPuppetState {
    Vec3f pos;

    u8 linkAge;
    char name[17];

    u8 currentTunic;
    u8 currentBoots;
    u8 currentShield;
    u8 buttonItem0;
    s32 modelGroup;

    Vec3s jointTable[24];
    Vec3s upperLimbRot;
    u8 movementFlags;
    Vec3s prevTransl;

    u32 stateFlags1;
    u32 stateFlags2;
    u8 csAction;
    s8 itemAction;
    s8 heldItemAction;
    s32 invincibilityTimer;
    s16 heldGetItemId;
    s32 leftHandType;
    f32 unk_85C;
    s32 actionVar1;
    s16 unk860;
    u8 modelLeftHandType;
    u8 modelRightHandType;
    u8 modelSheathType;

    Vec3s unk_3BC;
    u8 riding;
    u8 horseAnimIndex;
    s16 horseYaw;
    u8 horseType;
    Vec3f horsePos;
    Vec3s horseRot;
    u8 horseMounted;
    u8 horseAnimFrame;
    u32 heldActorId;

    f32 ocarinaModulator;
    s8 ocarinaBend;
    u8 ocarinaNote;
    u8 onPlatform;
} PlayerPuppetState;

#ifdef __cplusplus
extern "C" {
#endif

void PlayerPuppet_Init(Actor* actor, PlayState* play);
void PlayerPuppet_Update(Actor* actor, PlayState* play);
void PlayerPuppet_Draw(Actor* actor, PlayState* play);
void PlayerPuppet_Destroy(Actor* actor, PlayState* play);

#ifdef __cplusplus
}
#endif
}


#endif
