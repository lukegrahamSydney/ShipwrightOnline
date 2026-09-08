//Many things taken from Anchor
#include "PlayerPuppet.hpp"
#include "soh/Enhancements/nametag.h"
#include "ActorControllers/PlayerPuppetController.hpp"
#include <soh/ResourceManagerHelpers.h>
#include "ZeldaOnlineClient.hpp"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "z64collision_check.h"
#include <cmath>

extern PlayState* gPlayState;

void Player_UseItem(PlayState* play, Player* player, s32 item);
void Player_Draw(Actor* actor, PlayState* play);

extern Vec3f D_808547A4;
extern Vec3f D_808547B0;
extern Color_RGBA8 D_808547BC;
extern Color_RGBA8 D_808547C0;
extern Gfx** sPlayerDListGroups[];
}

namespace ZeldaOnline {


static PlayerPuppetState* PuppetState(Actor* actor) {
    auto* controller = static_cast<AbstractActorController*>(actor->zoController);
    if (controller == nullptr) {
        return nullptr;
    }
    return static_cast<PlayerPuppetController*>(controller)->PuppetState();
}

void PlayerPuppet_Init(Actor* actor, PlayState* play) {
    Player* player = (Player*)actor;
    PlayerPuppetController* controller = reinterpret_cast<PlayerPuppetController*>(actor->zoController);

    u8 linkAge = controller->LinkAge();

    s32 originalAge = gSaveContext.linkAge;
    gSaveContext.linkAge = linkAge;

    actor->room = -1;
    player->itemAction = player->heldItemAction = -1;
    player->heldItemId = ITEM_NONE;
    Player_UseItem(play, player, ITEM_NONE);
    Player_SetModelGroup(player, Player_ActionToModelGroup(player, player->heldItemAction));

    play->playerInit(player, play, gPlayerSkelHeaders[((void)0, linkAge)]);

    Effect_Delete(play, player->meleeWeaponEffectIndex);
    player->meleeWeaponEffectIndex = TOTAL_EFFECT_COUNT;

    play->func_11D54(player, play);

    player->cylinder.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    player->cylinder.base.ocFlags2 = OC2_TYPE_1;
    player->cylinder.info.bumperFlags = BUMP_ON | BUMP_HOOKABLE | BUMP_NO_HITMARK;
    player->actor.flags |= ACTOR_FLAG_HOOKSHOT_PULLS_PLAYER;

    player->actor.flags |= ACTOR_FLAG_CAN_PRESS_SWITCHES;

    player->cylinder.dim = GET_PLAYER(play)->cylinder.dim;

    gSaveContext.linkAge = originalAge;
}

static void Puppet_Vec3sCopy(Vec3s* dest, const Vec3s* src) {
    dest->x = src->x;
    dest->y = src->y;
    dest->z = src->z;
}

void PlayerPuppet_Update(Actor* actor, PlayState* play) {
    Player* player = (Player*)actor;
    PlayerPuppetState* st = PuppetState(actor);
    if (st == nullptr) {
        return;
    }

    actor->shape.shadowAlpha = 255;

    player->skelAnime.movementFlags = st->movementFlags;
    Puppet_Vec3sCopy(&player->unk_3BC, &st->unk_3BC);
    Puppet_Vec3sCopy(&player->skelAnime.prevTransl, &st->prevTransl);
    player->currentBoots = st->currentBoots;
    player->currentShield = st->currentShield;
    player->heldItemId = st->buttonItem0;
    player->currentTunic = st->currentTunic;
    player->stateFlags1 = st->stateFlags1;
    player->stateFlags2 = st->stateFlags2 & ~PLAYER_STATE2_DISABLE_DRAW;
    player->csAction = st->csAction;
    player->itemAction = st->itemAction;
    player->heldItemAction = player->itemAction = st->heldItemAction;
    player->unk_85C = st->unk_85C;
    player->invincibilityTimer = st->invincibilityTimer;
    player->unk_862 = (st->heldGetItemId > (s16)GID_MAXIMUM) ? (s16)GID_STONE_OF_AGONY : st->heldGetItemId;
    player->av1.actionVar1 = st->actionVar1;
    player->unk_860 = st->unk860;

    Vec3f diff;
    SkelAnime_UpdateTranslation(&player->skelAnime, &diff, player->actor.shape.rot.y);

    if (player->modelGroup != st->modelGroup) {
        s32 originalAge = gSaveContext.linkAge;
        gSaveContext.linkAge = st->linkAge;
        u8 originalButtonItem0 = gSaveContext.equips.buttonItems[0];
        gSaveContext.equips.buttonItems[0] = st->buttonItem0;
        Player_SetModelGroup(player, st->modelGroup);
        gSaveContext.linkAge = originalAge;
        gSaveContext.equips.buttonItems[0] = originalButtonItem0;
    }

    player->leftHandType = st->modelLeftHandType;
    player->rightHandType = st->modelRightHandType;
    player->sheathType = st->modelSheathType;
    if ((u32)st->modelLeftHandType < PLAYER_MODELTYPE_MAX) {
        player->leftHandDLists = &sPlayerDListGroups[st->modelLeftHandType][st->linkAge];
    }
    if ((u32)st->modelRightHandType < PLAYER_MODELTYPE_MAX) {
        player->rightHandDLists = &sPlayerDListGroups[st->modelRightHandType][st->linkAge];
    }
    if ((u32)st->modelSheathType < PLAYER_MODELTYPE_MAX) {
        player->sheathDLists = &sPlayerDListGroups[st->modelSheathType][st->linkAge];
    }

    actor->flags |= ACTOR_FLAG_LOCK_ON_DISABLED;

    Collider_UpdateCylinder(&player->actor, &player->cylinder);
    if (player->actor.velocity.y > 0.0f) {
        player->actor.velocity.y = 0.0f;
    }

    Vec3f syncedPos = player->actor.world.pos;
    Actor_UpdateBgCheckInfo(play, &player->actor, 26.0f, 6.0f, player->ageProperties->ceilingCheckHeight, 7);

    if (!st->onPlatform)
        player->actor.world.pos = syncedPos;

    if (!(player->stateFlags2 & PLAYER_STATE2_FROZEN)) {
        if (!(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_HANGING_OFF_LEDGE |
                                     PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_ON_HORSE))) {
            CollisionCheck_SetOC(play, &play->colChkCtx, &player->cylinder.base);
        }

        if (!(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_DAMAGED)) &&
            (player->invincibilityTimer <= 0)) {
            CollisionCheck_SetAC(play, &play->colChkCtx, &player->cylinder.base);

            if (player->invincibilityTimer < 0) {
                CollisionCheck_SetAT(play, &play->colChkCtx, &player->cylinder.base);
            }
        }
    }

    if (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE)) {
        player->actor.colChkInfo.mass = MASS_IMMOVABLE;
    } else {
        player->actor.colChkInfo.mass = 50;
    }

    if (player->heldItemAction == PLAYER_IA_DEKU_STICK && player->unk_860 > 0) {
        f32 flameScale = 1.0f;
        if (player->unk_860 > 200) {
            flameScale = (210 - player->unk_860) / 10.0f;
        } else if (player->unk_860 < 20) {
            flameScale = player->unk_860 / 20.0f;
            player->unk_85C = flameScale;
        }
        func_8002836C(play, &player->meleeWeaponInfo[0].tip, &D_808547A4, &D_808547B0, &D_808547BC, &D_808547C0,
                      (s16)(flameScale * 200.0f), 0, 8);
    }

    Collider_ResetCylinderAC(play, &player->cylinder.base);
}

void PlayerPuppet_Draw(Actor* actor, PlayState* play) {
    Player* player = (Player*)actor;

    PlayerPuppetState* st = PuppetState(actor);
    if (st == nullptr) {
        return;
    }

    if (st->onPlatform) {
        CollisionPoly* poly = NULL;
        s32 bgId = BGCHECK_SCENE;
        Vec3f from = player->actor.world.pos;
        from.y += 50.0f;

        f32 floorY = BgCheck_EntityRaycastFloor4(&play->colCtx, &poly, &bgId, &player->actor, &from);
        if (bgId != BGCHECK_SCENE) {
            player->actor.world.pos.y = floorY;
        }
    }

    Puppet_Vec3sCopy(&player->upperLimbRot, &st->upperLimbRot);
    for (s32 i = 0; i < PLAYER_LIMB_BUF_COUNT; i++) {
        player->skelAnime.jointTable[i] = st->jointTable[i];
    }

    s32 originalAge = gSaveContext.linkAge;
    gSaveContext.linkAge = st->linkAge;
    u8 originalButtonItem0 = gSaveContext.equips.buttonItems[0];
    gSaveContext.equips.buttonItems[0] = st->buttonItem0;

    Player_Draw((Actor*)player, play);

    gSaveContext.linkAge = originalAge;
    gSaveContext.equips.buttonItems[0] = originalButtonItem0;

}

void PlayerPuppet_Destroy(Actor* actor, PlayState* play) {
    actor->id = ACTOR_PLAYER;
}
}
