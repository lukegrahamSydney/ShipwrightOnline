#include "HorsePuppet.hpp"
#include "ZeldaOnlineClient.hpp"
#include "ActorControllers/PlayerPuppetController.hpp"
extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "z64collision_check.h"

void EnHorse_Draw(Actor* thisx, PlayState* play);
extern PlayState* gPlayState;

extern SkeletonHeader* sSkeletonHeaders[];
extern AnimationHeader** sAnimationHeaders[];
}

static const int HORSE_ANIM_COUNT = 9;

static ColliderCylinderInit sPuppetCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_NONE,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1 | OC2_UNK1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0xFFCFFFFF, 0x00, 0x00 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 20, 70, 0, { 0, 0, 0 } },
};

static ColliderCylinderInit sPuppetTrampleInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1 | OC2_UNK1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x20000000, 0x00, 0x08 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 20, 70, 0, { 0, 0, 0 } },
};

static const f32 HORSE_TRAMPLE_SPEED = 8.0f;

void HorsePuppet_Init(Actor* actor, PlayState* play) {
    EnHorse* horse = (EnHorse*)actor;

    u8 type = (actor->params & 0x8000) ? HORSE_HNI : HORSE_EPONA;
    actor->params &= ~0x8000;

    horse->type = type;
    Actor_SetScale(actor, 0.01f);
    Skin_Init(play, &horse->skin, sSkeletonHeaders[type], sAnimationHeaders[type][ENHORSE_ANIM_IDLE]);
    horse->animationIdx = ENHORSE_ANIM_IDLE;
    Animation_PlayLoop(&horse->skin.skelAnime, sAnimationHeaders[type][ENHORSE_ANIM_IDLE]);
    SkelAnime_Update(&horse->skin.skelAnime);
    actor->room = -1;

    Collider_InitCylinder(play, &horse->cyl1);
    Collider_SetCylinder(play, &horse->cyl1, actor, &sPuppetTrampleInit);
    Collider_InitCylinder(play, &horse->cyl2);
    Collider_SetCylinder(play, &horse->cyl2, actor, &sPuppetCylinderInit);
    actor->colChkInfo.mass = MASS_IMMOVABLE;
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

    auto playSpeed = horse->skin.skelAnime.playSpeed;
    playSpeed = 0.0f;
    SkelAnime_Update(&horse->skin.skelAnime);
    horse->skin.skelAnime.playSpeed = playSpeed;

    Collider_UpdateCylinder(actor, &horse->cyl1);
    Collider_UpdateCylinder(actor, &horse->cyl2);

    horse->cyl1.dim.pos.x += (s16)(Math_SinS(actor->shape.rot.y) * 11.0f);
    horse->cyl1.dim.pos.z += (s16)(Math_CosS(actor->shape.rot.y) * 11.0f);
    horse->cyl2.dim.pos.x += (s16)(Math_SinS(actor->shape.rot.y) * -18.0f);
    horse->cyl2.dim.pos.z += (s16)(Math_CosS(actor->shape.rot.y) * -18.0f);

    CollisionCheck_SetOC(play, &play->colChkCtx, &horse->cyl1.base);
    CollisionCheck_SetOC(play, &play->colChkCtx, &horse->cyl2.base);

    if (horse->cyl1.base.atFlags & AT_HIT) {
        horse->cyl1.base.atFlags &= ~AT_HIT;

        func_8002F6D4(play, actor, 6.0f, actor->shape.rot.y, 2.0f, 4);
        Player_PlaySfx(&GET_PLAYER(play)->actor, NA_SE_PL_BODY_HIT);
    }

    if (fabsf(actor->speedXZ) >= HORSE_TRAMPLE_SPEED) {
        horse->cyl1.base.atFlags |= AT_ON;
        horse->cyl1.info.toucherFlags |= TOUCH_ON;
        CollisionCheck_SetAT(play, &play->colChkCtx, &horse->cyl1.base);
    } else {
        horse->cyl1.base.atFlags &= ~AT_ON;
    }

    if (IS_CUTSCENE_LAYER)
        horse->actor.draw = NULL;
    else
        horse->actor.draw = EnHorse_Draw;
}

void HorsePuppet_Destroy(Actor* actor, PlayState* play) {
    if (actor->parent) {
        auto playerActor = reinterpret_cast<Player*>(actor->parent);
        if (playerActor->actor.zoController) {
            auto playerController =
                reinterpret_cast<ZeldaOnline::PlayerPuppetController*>(playerActor->actor.zoController);
            if (playerController->SatelliteHorse() == actor) {
                playerController->ClearSatelliteHorse();
            }
        }
    }

    EnHorse* horse = (EnHorse*)actor;
    Collider_DestroyCylinder(play, &horse->cyl1);
    Collider_DestroyCylinder(play, &horse->cyl2);
    Skin_Free(play, &horse->skin);
}