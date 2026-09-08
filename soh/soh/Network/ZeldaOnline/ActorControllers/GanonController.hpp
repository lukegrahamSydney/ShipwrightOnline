#ifndef GANONCONTROLLERH
#define GANONCONTROLLERH

#include <cstring>

#include "../AbstractBossController.hpp"
#include "soh/Enhancements/PlayerSkin/PlayerSkin.h"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.h"
#include "objects/object_ganon2/object_ganon2.h"
#include "objects/object_ganon_anime3/object_ganon_anime3.h"
#include "assets/textures/boss_title_cards/object_ganon2.h"

void func_808FD5C4(BossGanon2*, PlayState* play);
void func_808FD5F4(BossGanon2*, PlayState* play);
void func_808FFDB0(BossGanon2*, PlayState* play);
void func_808FFEBC(BossGanon2*, PlayState* play);
void func_808FFFE0(BossGanon2*, PlayState* play);
void func_80900104(BossGanon2*, PlayState* play);
void func_8090026C(BossGanon2*, PlayState* play);
void func_809002CC(BossGanon2*, PlayState* play);
void func_80900344(BossGanon2*, PlayState* play);
void func_80900580(BossGanon2*, PlayState* play);
void func_80900650(BossGanon2*, PlayState* play);
void func_80900890(BossGanon2*, PlayState* play);
void func_8090120C(BossGanon2*, PlayState* play);
void func_80901020(BossGanon2*, PlayState* play);
void BossGanon2_UpdateEffects(BossGanon2*, PlayState* play);
void BossGanon2_SetObjectSegment(BossGanon2*, PlayState* play, s32 objectId, u8 setRSPSegment);
void func_808FFC84(BossGanon2*);

u16 func_800FA0B4(u8 seqPlayerIndex);
void func_80064534(PlayState* play, CutsceneContext* csCtx);
void func_80846720(PlayState* play, Player* thisx, s32 arg2);

extern EnGanonMant* sBossGanonCape;
}

namespace ZeldaOnline {

class GanonController : public AbstractBossController {
  public:
    using AbstractBossController::AbstractBossController;

    BossGanon2* Typed() const {
        return reinterpret_cast<BossGanon2*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId != ACTOR_ITEM_B_HEART && actorId != ACTOR_DOOR_WARP1;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 23;

    using Ganon2ActionFunc = void (*)(BossGanon2*, PlayState*);
    static const Ganon2ActionFunc* ActionTable(size_t* count) {
        static const Ganon2ActionFunc sTable[] = {
            func_808FD5C4, func_808FD5F4, func_808FFDB0, func_808FFEBC, func_808FFFE0, func_80900104, func_8090026C,
            func_809002CC, func_80900344, func_80900580, func_80900650, func_80900890, func_8090120C,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    const char* GetTitleCard() const override {
        return gGanonTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return &Typed()->subCamId;
    }

    Vec3f_* GetCameraAt() override {
        return &Typed()->subCamAt;
    }

    Vec3f_* GetCameraEye() override {
        return &Typed()->subCamEye;
    }

    void OnActorInit() override {

        // Typed()->actor.colChkInfo.health = 2;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const Ganon2ActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gGanonDamageAnim,
            gGanonDeadLoopAnim,
            gGanonDeadStartAnim,
            gGanonDownedLoopAnim,
            gGanonDownedStartAnim,
            gGanonFinalBlowAnim,
            gGanonGetUpAnim,
            gGanonGuardIdleAnim,
            gGanonGuardWalkAnim,
            gGanonLeftSwordSwingAnim,
            gGanonRightSwordSwingAnim,
            gGanonRoarAnim,
            gGanonStunEndAnim,
            gGanonStunLoopAnim,
            gGanonStunStartAnim,
            gGanonUncurlAndFlailAnim,
            gGanonWalkAnim,
            gGanondorfBurstOutAnim,
            gGanondorfFloatingHeavyBreathingAnim,
            gGanondorfShowTriforceLoopAnim,
            gGanondorfShowTriforceStartAnim,
            gGanondorfTransformEndAnim,
            gGanondorfTransformStartAnim,
        };
        if (i >= ANIM_COUNT)
            return nullptr;
        return sAnims[i];
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++)
            if (strcmp(AnimForIndex(i), cur) == 0)
                return i;
        return ANIM_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_STATE,
        PROP_TIMERS,
        PROP_MOTION,
        PROP_TAIL,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_HAS_SWORD,
        PROP_SWORD_EFFECT,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BossGanon2* gn = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(gn->csState) << PackedUInt4(gn->csTimer) << PackedInt2(gn->unk_19C)
                                  << PackedInt2(gn->unk_1AC) << PackedUInt1(gn->unk_310) << PackedUInt1(gn->unk_311)
                                  << PackedUInt1(gn->unk_314) << PackedUInt1(gn->unk_334)
                                  << PackedUInt1(gn->unk_335) << PackedUInt1(gn->unk_336) << PackedUInt1(gn->unk_337)
                                  << PackedUInt1(gn->unk_338) << PackedInt1(gn->unk_339) << PackedInt2(gn->unk_328)
                                  << PackedInt2(gn->unk_330) << PackedInt2(gn->unk_332) << PackedInt2(gn->unk_340)
                                  << PackedInt1(gn->actor.colChkInfo.health),
                     out);

        ByteStream timers;
        for (s32 i = 0; i < 5; i++)
            timers << PackedInt2(gn->unk_1A2[i]);
        timers << PackedInt2(gn->unk_316) << PackedInt2(gn->unk_318) << PackedInt2(gn->unk_31A)
               << PackedInt2(gn->unk_31C) << PackedInt2(gn->unk_342) << PackedInt2(gn->unk_344)
               << PackedInt2(gn->unk_346) << PackedInt2(gn->unk_390) << PackedInt2(gn->unk_392);
        PackProperty(PROP_TIMERS, timers, out);

        PackProperty(PROP_MOTION,
                     ByteStream() << PackedFloat4(gn->unk_194) << PackedFloat4(gn->unk_198) << PackedFloat4(gn->unk_1B0)
                                  << PackedFloat4(gn->unk_1B4) << PackedFloat4(gn->unk_224) << PackedFloat4(gn->unk_228)
                                  << PackedFloat4(gn->unk_30C) << PackedFloat4(gn->unk_320) << PackedFloat4(gn->unk_324)
                                  << PackedFloat4(gn->unk_32C) << PackedFloat4(gn->unk_33C) << PackedFloat4(gn->unk_35C)
                                  << PackedFloat4(gn->unk_36C) << PackedFloat4(gn->unk_37C) << PackedFloat4(gn->unk_380)
                                  << PackedFloat4(gn->unk_384) << PackedFloat4(gn->unk_388) << PackedFloat4(gn->unk_38C)
                                  << PackedFloat4(gn->unk_394) << PackedFloat4(gn->unk_41C)
                                  << PackedFloat4(gn->unk_420),
                     out);

        PackProperty(
            PROP_TAIL,
            ByteStream() << PackedFloat4(gn->unk_360.x) << PackedFloat4(gn->unk_360.y) << PackedFloat4(gn->unk_360.z)
                         << PackedFloat4(gn->unk_370.x) << PackedFloat4(gn->unk_370.y) << PackedFloat4(gn->unk_370.z)
                         << PackedFloat4(gn->unk_410.x) << PackedFloat4(gn->unk_410.y) << PackedFloat4(gn->unk_410.z),
            out);


        BuildBossProperty(PROP_BOSS_CAMERA, out);
        BuildBossProperty(PROP_BOSS_BGM, out);
        BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
        BuildBossProperty(PROP_BOSS_LIGHTING, out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(gn->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &gn->skelAnime), out);

        PackProperty(PROP_HAS_SWORD, PackedUInt1(gSaveContext.equips.buttonItems[0] != ITEM_NONE ? 1 : 0), out);

        {
            BossGanon2Effect* effect = (BossGanon2Effect*)gPlayState->specialEffects;

            PackProperty(PROP_SWORD_EFFECT,
                         ByteStream() << PackedUInt1(effect->type) << PackedUInt1(effect->unk_01)
                                      << PackedInt2(effect->unk_2E) << PackedFloat4(effect->position.x)
                                      << PackedFloat4(effect->position.y) << PackedFloat4(effect->position.z)
                                      << PackedFloat4(effect->velocity.x) << PackedFloat4(effect->velocity.y)
                                      << PackedFloat4(effect->velocity.z) << PackedFloat4(effect->accel.x)
                                      << PackedFloat4(effect->accel.y) << PackedFloat4(effect->accel.z)
                                      << PackedFloat4(effect->scale) << PackedFloat4(effect->unk_38.x)
                                      << PackedFloat4(effect->unk_38.y) << PackedFloat4(effect->unk_38.z),
                         out);
        }

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossGanon2* gn = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const Ganon2ActionFunc* table = ActionTable(&count);
                if (id < count)
                    gn->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                gn->csState = (s16)(data.Read<PackedInt2>().value());
                gn->csTimer = data.Read<PackedUInt4>().value();
                gn->unk_19C = (s16)(data.Read<PackedInt2>().value());
                gn->unk_1AC = (s16)(data.Read<PackedInt2>().value());
                gn->unk_310 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_311 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_314 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_334 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_335 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_336 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_337 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_338 = (u8)(data.Read<PackedUInt1>().value());
                gn->unk_339 = (s8)(data.Read<PackedInt1>().value());
                gn->unk_328 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_330 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_332 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_340 = (s16)(data.Read<PackedInt2>().value());
                gn->actor.colChkInfo.health = (s8)(data.Read<PackedInt1>().value());
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 5; i++)
                    gn->unk_1A2[i] = (s16)(data.Read<PackedInt2>().value());
                gn->unk_316 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_318 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_31A = (s16)(data.Read<PackedInt2>().value());
                gn->unk_31C = (s16)(data.Read<PackedInt2>().value());
                gn->unk_342 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_344 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_346 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_390 = (s16)(data.Read<PackedInt2>().value());
                gn->unk_392 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_MOTION:
                gn->unk_194 = data.Read<PackedFloat4>().value();
                gn->unk_198 = data.Read<PackedFloat4>().value();
                gn->unk_1B0 = data.Read<PackedFloat4>().value();
                gn->unk_1B4 = data.Read<PackedFloat4>().value();
                gn->unk_224 = data.Read<PackedFloat4>().value();
                gn->unk_228 = data.Read<PackedFloat4>().value();
                gn->unk_30C = data.Read<PackedFloat4>().value();
                gn->unk_320 = data.Read<PackedFloat4>().value();
                gn->unk_324 = data.Read<PackedFloat4>().value();
                gn->unk_32C = data.Read<PackedFloat4>().value();
                gn->unk_33C = data.Read<PackedFloat4>().value();
                gn->unk_35C = data.Read<PackedFloat4>().value();
                gn->unk_36C = data.Read<PackedFloat4>().value();
                gn->unk_37C = data.Read<PackedFloat4>().value();
                gn->unk_380 = data.Read<PackedFloat4>().value();
                gn->unk_384 = data.Read<PackedFloat4>().value();
                gn->unk_388 = data.Read<PackedFloat4>().value();
                gn->unk_38C = data.Read<PackedFloat4>().value();
                gn->unk_394 = data.Read<PackedFloat4>().value();
                gn->unk_41C = data.Read<PackedFloat4>().value();
                gn->unk_420 = data.Read<PackedFloat4>().value();
                break;
            case PROP_TAIL:
                gn->unk_360.x = data.Read<PackedFloat4>().value();
                gn->unk_360.y = data.Read<PackedFloat4>().value();
                gn->unk_360.z = data.Read<PackedFloat4>().value();
                gn->unk_370.x = data.Read<PackedFloat4>().value();
                gn->unk_370.y = data.Read<PackedFloat4>().value();
                gn->unk_370.z = data.Read<PackedFloat4>().value();
                gn->unk_410.x = data.Read<PackedFloat4>().value();
                gn->unk_410.y = data.Read<PackedFloat4>().value();
                gn->unk_410.z = data.Read<PackedFloat4>().value();
                break;
           
            case PROP_ANIM_CUR_FRAME:
                gn->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &gn->skelAnime, LOCK_CUR_FRAME ? gn->skelAnime.curFrame : 0.0f, data);
                break;
            }

            

            case PROP_SWORD_EFFECT: {
                u8 type = (u8)(data.Read<PackedUInt1>().value());
                u8 unk01 = (u8)(data.Read<PackedUInt1>().value());
                s16 unk2E = (s16)(data.Read<PackedInt2>().value());

                Vec3f position, velocity, accel, unk38;
                position.x = data.Read<PackedFloat4>().value();
                position.y = data.Read<PackedFloat4>().value();
                position.z = data.Read<PackedFloat4>().value();
                velocity.x = data.Read<PackedFloat4>().value();
                velocity.y = data.Read<PackedFloat4>().value();
                velocity.z = data.Read<PackedFloat4>().value();
                accel.x = data.Read<PackedFloat4>().value();
                accel.y = data.Read<PackedFloat4>().value();
                accel.z = data.Read<PackedFloat4>().value();
                f32 scale = data.Read<PackedFloat4>().value();
                unk38.x = data.Read<PackedFloat4>().value();
                unk38.y = data.Read<PackedFloat4>().value();
                unk38.z = data.Read<PackedFloat4>().value();

                if (gPlayState == nullptr)
                    break;

                BossGanon2Effect* effect = (BossGanon2Effect*)gPlayState->specialEffects;

                if (effect->type != type) {
                    if (type)
                    {
                        Player* player = GET_PLAYER(gPlayState);
                        player->heldItemAction = player->itemAction = PLAYER_IA_NONE;
                        player->heldItemId = ITEM_NONE;
                        player->modelGroup = player->nextModelGroup = Player_ActionToModelGroup(player, PLAYER_IA_NONE);
                        player->leftHandDLists = Player_GetSkin(player)->dlistGroups[PLAYER_MODELTYPE_LH_OPEN];

                        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
                        gSaveContext.equips.buttonItems[0] = ITEM_SWORD_MASTER;
                        Inventory_DeleteEquipment(gPlayState, EQUIP_TYPE_SWORD);

                    } else {

                        Player* player = GET_PLAYER(gPlayState);

                        Item_Give(gPlayState, ITEM_SWORD_MASTER);
                        func_80846720(gPlayState, player, 0);
                    }
                }

                effect->type = type;
                effect->unk_01 = unk01;
                effect->unk_2E = unk2E;
                effect->position = position;
                effect->velocity = velocity;
                effect->accel = accel;
                effect->scale = scale;
                effect->unk_38 = unk38;
                break;
            }
            case PROP_HAS_SWORD: {
                bool hasSword = (u8)(data.Read<PackedUInt1>().value()) != 0;
                if (hasSword) {

                } else {

                }
            
                break;
            }
            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BossGanon2* gn = Typed();

        if (gn->unk_337 != 0 && !m_swappedToGanon) {
            m_swappedToGanon = true;

            BossGanon2_SetObjectSegment(gn, gPlayState, OBJECT_GANON2, false);
            SkelAnime_Free(&gn->skelAnime, gPlayState);
            SkelAnime_InitFlex(gPlayState, &gn->skelAnime, (FlexSkeletonHeader*)&gGanonSkel, NULL, NULL, NULL, 0);
            BossGanon2_SetObjectSegment(gn, gPlayState, OBJECT_GANON_ANIME3, false);
            Animation_PlayOnce(&gn->skelAnime, (AnimationHeader*)&gGanonUncurlAndFlailAnim);
        }

        if (gn->actionFunc == func_8090120C && gn->csState >= 7) {
            EndCutsceneCamera();

            func_80064520(gPlayState, &gPlayState->csCtx);
            GameInteractor_ExecuteOnBossDefeat(&gn->actor);
            gn->subCamId = Play_CreateSubCamera(gPlayState);
            Play_ChangeCameraStatus(gPlayState, MAIN_CAM, CAM_STAT_WAIT);
            Play_ChangeCameraStatus(gPlayState, gn->subCamId, CAM_STAT_ACTIVE);
            gn->csState = 7;
            gn->csTimer = 0;
            Animation_MorphToPlayOnce(&gn->skelAnime, (AnimationHeader*)&gGanonFinalBlowAnim, 0.0f);
            gn->unk_194 = Animation_GetLastFrame((void*)&gGanonFinalBlowAnim);
            gPlayState->startPlayerCutscene(gPlayState, &gn->actor, 0x61);
            GoLocal();
        }
    }

    void DragZelda() {
        if (sBossGanon2Zelda == nullptr || sBossGanon2Zelda->actor.zoController == nullptr)
            return;

        auto* ctl = reinterpret_cast<AbstractActorController*>(sBossGanon2Zelda->actor.zoController);
        if (!ctl->IsLeader())
            ctl->ClaimLeadership(CLAIM_REASON_NOW);
    }

    void OnBecomeLeader() override {
        DragZelda();
    }

    void UpdateLeader(PlayState* play) override {
        BossGanon2* gn = Typed();

        if (gn->csState > 0 && sBossGanon2Zelda == nullptr)
            return;

        DragZelda();

        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        BossGanon2* gn = Typed();

        UpdateAnimation(&gn->skelAnime, LOCK_CUR_FRAME);


        //Z-targetting
        {
            func_808FFC84(gn);
        }

        // Whoever touches sword becomes leader
        {
            BossGanon2Effect* effect = (BossGanon2Effect*)play->specialEffects;
            bool swordWasUp = (effect->type == 1);

            BossGanon2_UpdateEffects(gn, play);

            if (swordWasUp && effect->type == 0 && effect->unk_2E != 0) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }
        }

        if ((gn->unk_424.base.acFlags & AC_HIT) || (gn->unk_444.base.acFlags & AC_HIT)) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        gn->unk_424.base.acFlags &= ~AC_HIT;
        gn->unk_444.base.acFlags &= ~AC_HIT;
        gn->unk_424.base.atFlags &= ~AT_HIT;
        gn->unk_444.base.atFlags &= ~AT_HIT;

        if (gn->unk_316 == 0) {
            RegisterColliderBase(play, &gn->unk_424.base, COLL_AC | COLL_AT | COLL_OC);
            RegisterColliderBase(play, &gn->unk_444.base, COLL_AC | COLL_AT | COLL_OC);
        }
    }

  private:
    bool m_swappedToGanon = false;
    bool m_wentLocalForCs = false;
    u8 m_csStep = 0;
};

} // namespace ZeldaOnline

#endif