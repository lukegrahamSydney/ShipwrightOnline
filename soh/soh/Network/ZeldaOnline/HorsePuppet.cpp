
#include "HorsePuppet.hpp"
#include "ZeldaOnlineClient.hpp"
#include "ActorControllers/PlayerPuppetController.hpp"
extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Horse/z_en_horse.h"
extern PlayState* gPlayState;

extern SkeletonHeader* sSkeletonHeaders[];
extern AnimationHeader** sAnimationHeaders[];

}

static const int HORSE_ANIM_COUNT = 9;

void HorsePuppet_Init(Actor* actor, PlayState* play) {
    EnHorse* horse = (EnHorse*)actor;

    u8 type = (actor->params & 0x8000) ? HORSE_HNI : HORSE_EPONA;
    actor->params &= ~0x8000;

    horse->type = type;
    Actor_SetScale(actor, 0.01f);
    Skin_Init(play, &horse->skin, sSkeletonHeaders[type], sAnimationHeaders[type][ENHORSE_ANIM_IDLE]);
    horse->animationIdx = ENHORSE_ANIM_IDLE;
    Animation_PlayLoop(&horse->skin.skelAnime, sAnimationHeaders[type][ENHORSE_ANIM_IDLE]);

    actor->room = -1;
}

void HorsePuppet_SetAnimation(Actor* actor, u8 animIndex) {
    EnHorse* horse = (EnHorse*)actor;
    if (animIndex >= HORSE_ANIM_COUNT || animIndex == horse->animationIdx) {
        return;
    }
    horse->animationIdx = animIndex;
    Animation_Change(&horse->skin.skelAnime, sAnimationHeaders[horse->type][animIndex], 0.0f, 0.0f,
                     Animation_GetLastFrame(sAnimationHeaders[horse->type][animIndex]), ANIMMODE_LOOP, -3.0f);
}

void HorsePuppet_SetAnimFrame(Actor* actor, u8 frame) {
    if (frame == 0xFF) {
        return;
    }
    EnHorse* horse = (EnHorse*)actor;
    f32 length = Animation_GetLastFrame(sAnimationHeaders[horse->type][horse->animationIdx]);
    f32 f = (f32)frame;
    if (f > length) {
        f = length;
    }
    horse->skin.skelAnime.curFrame = f;
}

void HorsePuppet_Update(Actor* actor, PlayState* play) {
    EnHorse* horse = (EnHorse*)actor;
    actor->shape.shadowAlpha = 255;

    SkelAnime_Update(&horse->skin.skelAnime);
}

void HorsePuppet_Destroy(Actor* actor, PlayState* play) {
    if (actor->parent)
    {
        auto playerActor = reinterpret_cast<Player*>(actor->parent);
        if (playerActor->actor.zoController) {
            auto playerController = reinterpret_cast<ZeldaOnline::PlayerPuppetController*>(playerActor->actor.zoController);
            if (playerController->SatelliteHorse() == actor) {
                playerController->ClearSatelliteHorse();
            }
        }
    }

    EnHorse* horse = (EnHorse*)actor;
    Skin_Free(play, &horse->skin);
}
