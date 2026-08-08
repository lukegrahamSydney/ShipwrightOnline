#ifndef HORSEPUPPET_H
#define HORSEPUPPET_H

#include <z64.h>

#ifdef __cplusplus
extern "C" {
#endif

void HorsePuppet_Init(Actor* actor, PlayState* play);
void HorsePuppet_Update(Actor* actor, PlayState* play);
void HorsePuppet_Destroy(Actor* actor, PlayState* play);

void HorsePuppet_SetAnimation(Actor* actor, u8 animIndex);
void HorsePuppet_SetAnimFrame(Actor* actor, u8 frame);
#ifdef __cplusplus
}
#endif

#endif
