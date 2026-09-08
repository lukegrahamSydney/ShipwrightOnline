#ifndef GANONDORFCONTROLLERH
#define GANONDORFCONTROLLERH

#include <cstring>

#include "../AbstractBossController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Ganon/z_boss_ganon.h"
#include "src/overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.h"
#include "objects/object_ganon_anime1/object_ganon_anime1.h"
#include "objects/object_ganon_anime2/object_ganon_anime2.h"
#include "objects/object_ganon_anime3/object_ganon_anime3.h"
#include "assets/textures/boss_title_cards/object_ganon.h"
void BossGanon_SetupIntroCutscene(BossGanon*, PlayState* play);
void BossGanon_IntroCutscene(BossGanon*, PlayState* play);
void BossGanon_SetupTowerCutscene(BossGanon*, PlayState* play);
void BossGanon_DeathAndTowerCutscene(BossGanon*, PlayState* play);
void BossGanon_Wait(BossGanon*, PlayState* play);
void BossGanon_ChargeLightBall(BossGanon*, PlayState* play);
void BossGanon_PlayTennis(BossGanon*, PlayState* play);
void BossGanon_PoundFloor(BossGanon*, PlayState* play);
void BossGanon_ChargeBigMagic(BossGanon*, PlayState* play);
void BossGanon_Block(BossGanon*, PlayState* play);
void BossGanon_HitByLightBall(BossGanon*, PlayState* play);
void BossGanon_Vulnerable(BossGanon*, PlayState* play);
void BossGanon_Damaged(BossGanon*, PlayState* play);
void BossGanon_SetupDeathCutscene(BossGanon*, PlayState* play);
void BossGanon_SetColliderPos(Vec3f* pos, ColliderCylinder* collider);
void BossGanon_UpdateEffects(PlayState* play);

extern BossGanon* sBossGanonGanondorf;
extern EnGanonMant* sBossGanonCape;

u16 func_800FA0B4(u8 seqPlayerIndex);
void func_80064534(PlayState* play, CutsceneContext* csCtx);
}

namespace ZeldaOnline {

class GanondorfController : public AbstractBossController {
  public:
    using AbstractBossController::AbstractBossController;

    BossGanon* Typed() const {
        return reinterpret_cast<BossGanon*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    bool IsLightBall() const {
        return Typed()->actor.params >= 0x64;
    }

    static bool IsNetworkedVariant(s16 params) {
        return true;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId != ACTOR_ITEM_B_HEART && actorId != ACTOR_DOOR_WARP1;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 41;

    using GanonActionFunc = void (*)(BossGanon*, PlayState*);
    static const GanonActionFunc* ActionTable(size_t* count) {
        static const GanonActionFunc sTable[] = {
            BossGanon_SetupIntroCutscene,
            BossGanon_IntroCutscene,
            BossGanon_SetupTowerCutscene,
            BossGanon_DeathAndTowerCutscene,
            BossGanon_Wait,
            BossGanon_ChargeLightBall,
            BossGanon_PlayTennis,
            BossGanon_PoundFloor,
            BossGanon_ChargeBigMagic,
            BossGanon_Block,
            BossGanon_HitByLightBall,
            BossGanon_Vulnerable,
            BossGanon_Damaged,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gGanondorfBigMagicChargeHoldAnim,
            gGanondorfBigMagicChargeStartAnim,
            gGanondorfBigMagicHitAnim,
            gGanondorfBigMagicThrowAnim,
            gGanondorfBigMagicThrowEndAnim,
            gGanondorfBigMagicWindupAnim,
            gGanondorfBlockAnim,
            gGanondorfBlockReleaseAnim,
            gGanondorfChargeLightBallAnim,
            gGanondorfCollapseAnim,
            gGanondorfDamageAnim,
            gGanondorfDefeatedLoopAnim,
            gGanondorfDefeatedStartAnim,
            gGanondorfDownedAnim,
            gGanondorfFloatAnim,
            gGanondorfGetUp1Anim,
            gGanondorfGetUp2Anim,
            gGanondorfGetUp3Anim,
            gGanondorfLandAnim,
            gGanondorfLeanOnOrganAnim,
            gGanondorfLightArrowHitAnim,
            gGanondorfLightArrowWaitAnim,
            gGanondorfLightEnergyHitAnim,
            gGanondorfPlayOrganAnim,
            gGanondorfPoundAnim,
            gGanondorfPoundEndAnim,
            gGanondorfRaiseHandLoopAnim,
            gGanondorfRaiseHandStartAnim,
            gGanondorfStandBackwardsAnim,
            gGanondorfStandIdleAnim,
            gGanondorfStandUpFromOrganAnim,
            gGanondorfStopPlayingOrganAnim,
            gGanondorfThrowAnim,
            gGanondorfTurnAroundAnim,
            gGanondorfVolleyLeftAnim,
            gGanondorfVolleyRightAnim,
            gGanondorfVomitLoopAnim,
            gGanondorfVomitStartAnim,
            gGanondorfVulnerableAnim,
            gGanondorfYellLoopAnim,
            gGanondorfYellStartAnim,
        };
        if (i >= ANIM_COUNT)
            return nullptr;
        return sAnims[i];
    }

    const char* GetTitleCard() const override {
        return gGanondorfTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return &Typed()->csCamIndex;
    }

    Vec3f_* GetCameraAt() override {
        return &Typed()->csCamAt;
    }

    Vec3f_* GetCameraEye() override {
        return &Typed()->csCamEye;
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

    u8 CurrentActionIndex() const {
        size_t count;
        const GanonActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool InCutscene() const {
        BossGanon* gd = Typed();
        return gd->actionFunc == BossGanon_IntroCutscene || gd->actionFunc == BossGanon_DeathAndTowerCutscene;
    }

    void CurrentColliderRoles(u8& roles) const {
        BossGanon* gd = Typed();

        roles = 0;

        if (gd->csState != 0)
            return;

        roles |= COLL_OC;

        if (gd->unk_2D4 != 0)
            return;

        roles |= COLL_AC;

        if (gd->actionFunc != BossGanon_HitByLightBall && gd->actionFunc != BossGanon_Vulnerable &&
            gd->actionFunc != BossGanon_Damaged)
            roles |= COLL_AT;
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_STATE,
        PROP_TIMERS,
        PROP_FWORK,
        PROP_HAND,
        PROP_VISUAL,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_BALL,

    };

    void BuildCustomProperties(ByteStream& out) override {
        BossGanon* gd = Typed();

        if (IsLightBall()) {
            PackProperty(PROP_BALL,
                         ByteStream() << PackedInt2(gd->unk_1A2) << PackedInt2(gd->unk_1A8) << PackedInt2(gd->unk_1AA)
                                      << PackedInt2(gd->unk_1AC) << PackedInt2(gd->unk_1C2) << PackedInt2(gd->timers[0])
                                      << PackedInt2(gd->timers[1]) << PackedFloat4(gd->fwork[0])
                                      << PackedFloat4(gd->fwork[1]) << PackedFloat4(gd->unk_1F0.x)
                                      << PackedFloat4(gd->unk_1F0.y) << PackedFloat4(gd->unk_1F0.z)
                                      << PackedInt2(gd->collider.dim.radius) << PackedInt2(gd->collider.dim.height)
                                      << PackedInt2(gd->collider.dim.yShift) << PackedUInt1(gd->timers[1] == 0 ? 1 : 0),
                         out);

            BuildStandardExtendedProperty(PROP_VELOCITY, out);
            BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
            BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
            BuildStandardExtendedProperty(PROP_SCALE, out);
            BuildStandardExtendedProperty(PROP_FLAGS, out);
            return;
        }

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(gd->csState) << PackedUInt4(gd->csTimer) << PackedUInt1(gd->unk_198)
                                  << PackedUInt1(gd->unk_19A) << PackedInt2(gd->unk_19C) << PackedUInt1(gd->unk_19E)
                                  << PackedUInt1(gd->unk_19F) << PackedInt1(gd->envLightMode) << PackedInt2(gd->unk_1A2)
                                  << PackedInt2(gd->unk_1A4) << PackedInt2(gd->unk_1A6) << PackedInt2(gd->unk_1A8)
                                  << PackedInt2(gd->unk_1AA) << PackedInt2(gd->unk_1AC) << PackedInt2(gd->triforceType)
                                  << PackedUInt1(gd->startVolley) << PackedInt2(gd->unk_1C2)
                                  << PackedInt2(gd->screenFlashTimer) << PackedInt1(gd->actor.colChkInfo.health),
                     out);

        ByteStream timers;
        for (s32 i = 0; i < 5; i++)
            timers << PackedInt2(gd->timers[i]);
        timers << PackedInt2(gd->unk_2D4) << PackedInt2(gd->unk_2E6) << PackedInt2(gd->unk_2E8)
               << PackedInt2(gd->unk_26C);
        PackProperty(PROP_TIMERS, timers, out);

        ByteStream fwork;
        for (s32 i = 0; i < GDF_FWORK_MAX; i++)
            fwork << PackedFloat4(gd->fwork[i]);
        PackProperty(PROP_FWORK, fwork, out);

        PackProperty(PROP_HAND,
                     ByteStream() << PackedFloat4(gd->unk_260.x) << PackedFloat4(gd->unk_260.y)
                                  << PackedFloat4(gd->unk_260.z) << PackedFloat4(gd->unk_1FC.x)
                                  << PackedFloat4(gd->unk_1FC.y) << PackedFloat4(gd->unk_1FC.z)
                                  << PackedFloat4(gd->handLightBallScale) << PackedUInt1(gd->useOpenHand),
                     out);

        PackProperty(PROP_VISUAL,
                     ByteStream() << PackedFloat4(gd->whiteFillAlpha) << PackedInt2(gd->organAlpha)
                                  << PackedUInt1(gd->windowShatterState) << PackedUInt1(gd->shockGlow)
                                  << PackedUInt1(gd->lensFlareMode) << PackedInt2(gd->lensFlareTimer)
                                  << PackedFloat4(gd->lensFlareScale) << PackedUInt1(gd->legSwayEnabled),
                     out);

        {
            u8 roles;
            CurrentColliderRoles(roles);
            PackProperty(PROP_COLL_ROLES, PackedUInt1(roles), out);
        }

        BuildBossProperty(PROP_BOSS_CAMERA, out);
        BuildBossProperty(PROP_BOSS_BGM, out);
        BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
        BuildBossProperty(PROP_BOSS_LIGHTING, out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(gd->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &gd->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossGanon* gd = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const GanonActionFunc* table = ActionTable(&count);
                if (id < count)
                    gd->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                gd->csState = (s16)(data.Read<PackedInt2>().value());
                gd->csTimer = data.Read<PackedUInt4>().value();
                gd->unk_198 = (u8)(data.Read<PackedUInt1>().value());
                gd->unk_19A = (u8)(data.Read<PackedUInt1>().value());
                gd->unk_19C = (s16)(data.Read<PackedInt2>().value());
                gd->unk_19E = (u8)(data.Read<PackedUInt1>().value());
                gd->unk_19F = (u8)(data.Read<PackedUInt1>().value());
                gd->envLightMode = (s8)(data.Read<PackedInt1>().value());
                gd->unk_1A2 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1A4 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1A6 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1A8 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1AA = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1AC = (s16)(data.Read<PackedInt2>().value());
                gd->triforceType = (s16)(data.Read<PackedInt2>().value());
                gd->startVolley = (u8)(data.Read<PackedUInt1>().value());
                gd->unk_1C2 = (s16)(data.Read<PackedInt2>().value());
                gd->screenFlashTimer = (s16)(data.Read<PackedInt2>().value());
                gd->actor.colChkInfo.health = (s8)(data.Read<PackedInt1>().value());
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 5; i++)
                    gd->timers[i] = (s16)(data.Read<PackedInt2>().value());
                gd->unk_2D4 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_2E6 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_2E8 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_26C = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FWORK:
                for (s32 i = 0; i < GDF_FWORK_MAX; i++)
                    gd->fwork[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_HAND:
                gd->unk_260.x = data.Read<PackedFloat4>().value();
                gd->unk_260.y = data.Read<PackedFloat4>().value();
                gd->unk_260.z = data.Read<PackedFloat4>().value();
                gd->unk_1FC.x = data.Read<PackedFloat4>().value();
                gd->unk_1FC.y = data.Read<PackedFloat4>().value();
                gd->unk_1FC.z = data.Read<PackedFloat4>().value();
                gd->handLightBallScale = data.Read<PackedFloat4>().value();
                gd->useOpenHand = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_VISUAL:
                gd->whiteFillAlpha = data.Read<PackedFloat4>().value();
                gd->organAlpha = (s16)(data.Read<PackedInt2>().value());
                gd->windowShatterState = (u8)(data.Read<PackedUInt1>().value());
                gd->shockGlow = (u8)(data.Read<PackedUInt1>().value());
                gd->lensFlareMode = (u8)(data.Read<PackedUInt1>().value());
                gd->lensFlareTimer = (s16)(data.Read<PackedInt2>().value());
                gd->lensFlareScale = data.Read<PackedFloat4>().value();
                gd->legSwayEnabled = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            

            case PROP_ANIM_CUR_FRAME:
                gd->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &gd->skelAnime, LOCK_CUR_FRAME ? gd->skelAnime.curFrame : 0.0f, data);
                break;
            }
            case PROP_BALL:
                gd->unk_1A2 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1A8 = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1AA = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1AC = (s16)(data.Read<PackedInt2>().value());
                gd->unk_1C2 = (s16)(data.Read<PackedInt2>().value());
                gd->timers[0] = (s16)(data.Read<PackedInt2>().value());
                gd->timers[1] = (s16)(data.Read<PackedInt2>().value());
                gd->fwork[0] = data.Read<PackedFloat4>().value();
                gd->fwork[1] = data.Read<PackedFloat4>().value();
                gd->unk_1F0.x = data.Read<PackedFloat4>().value();
                gd->unk_1F0.y = data.Read<PackedFloat4>().value();
                gd->unk_1F0.z = data.Read<PackedFloat4>().value();
                gd->collider.dim.radius = (s16)(data.Read<PackedInt2>().value());
                gd->collider.dim.height = (s16)(data.Read<PackedInt2>().value());
                gd->collider.dim.yShift = (s16)(data.Read<PackedInt2>().value());
                m_ballColliderOn = (u8)(data.Read<PackedUInt1>().value()) != 0;
                break;

            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BossGanon* gd = Typed();

        if (gd->actionFunc == BossGanon_DeathAndTowerCutscene) {
            if (gd->csTimer < 30) {
                gPlayState->envCtx.unk_D8 = 0.0f;
            }

            if (gd->csTimer >= 2) {
                gPlayState->envCtx.fillScreen = false;
            }
        }
    }

    void OnActorInit() override {
        ReinstallUpdate();

        if (!IsLightBall())
            sBossGanonGanondorf = Typed();

        //Typed()->actor.colChkInfo.health = 2;
    }


    void DragCape() {
        if (sBossGanonCape == nullptr || sBossGanonCape->actor.zoController == nullptr)
            return;

        auto ctrl = reinterpret_cast<AbstractActorController*>(sBossGanonCape->actor.zoController);
        if (!ctrl->IsLeader())
            ctrl->ClaimLeadership(CLAIM_REASON_NOW);
    }

    void UpdateLeader(PlayState* play) override {
        ReinstallUpdate();

        if (!IsLightBall())
            DragCape();

        AbstractActorController::UpdateLeader(play);
    }

    void ReplicateIntroCape() {
        BossGanon* ganon = Typed();

        s16 csState = ganon->csState;
        s32 csTimer = ganon->csTimer;

        if (csState != m_prevCapeCsState) {
            m_prevCapeCsState = csState;
            m_prevCapeCsTimer = -1;
        }

        sBossGanonCape->backPush = -2.0f;
        sBossGanonCape->backSwayMagnitude = 0.25f;
        sBossGanonCape->sideSwayMagnitude = -1.0f;
        sBossGanonCape->minDist = 0.0f;
        sBossGanonCape->minY = 57.0f;

        if (csState == 17) {
            if (m_prevCapeCsTimer < 62 && csTimer >= 62)
                sBossGanonCape->attachRightArmTimer = 20.0f;
        } else if (csState == 22) {
            if (m_prevCapeCsTimer < 20 && csTimer >= 20)
                sBossGanonCape->attachShouldersTimer = 18.0f;

            if (csTimer >= 20) {
                sBossGanonCape->backPush = -3.0f;
                sBossGanonCape->backSwayMagnitude = 0.25f;
                sBossGanonCape->sideSwayMagnitude = -3.0f;
            }
        }

        m_prevCapeCsTimer = csTimer;
    }

    void ReplicateFightCape() {
        BossGanon* gd = Typed();

        sBossGanonCape->minY = 2.0f;

        if (m_prevActionFunc == nullptr) {
            sBossGanonCape->backPush = -3.0f;
            sBossGanonCape->backSwayMagnitude = 0.25f;
            sBossGanonCape->sideSwayMagnitude = -3.0f;
            sBossGanonCape->minDist = 20.0f;
        }

        if (gd->actionFunc != m_prevActionFunc) {
            m_prevActionFunc = gd->actionFunc;

            if (gd->actionFunc == BossGanon_Block) {
                sBossGanonCape->attachLeftArmTimer = 10.0f;
            } else if (gd->actionFunc == BossGanon_HitByLightBall) {
                sBossGanonCape->attachRightArmTimer = 0.0f;
                sBossGanonCape->attachLeftArmTimer = 0.0f;
            } else if (gd->actionFunc == BossGanon_Vulnerable) {
                sBossGanonCape->attachRightArmTimer = 0.0f;
                sBossGanonCape->attachLeftArmTimer = 0.0f;
                sBossGanonCape->backPush = -4.0f;
                sBossGanonCape->backSwayMagnitude = 0.75f;
                sBossGanonCape->sideSwayMagnitude = -3.0f;
                sBossGanonCape->minDist = 20.0f;
            }
        }

        if (gd->actionFunc == BossGanon_Wait) {
            sBossGanonCape->backPush = -3.0f;
            sBossGanonCape->backSwayMagnitude = 0.25f;
            sBossGanonCape->sideSwayMagnitude = -3.0f;
            sBossGanonCape->minDist = 20.0f;
        } else if (gd->actionFunc == BossGanon_ChargeLightBall) {
            sBossGanonCape->backPush = -3.0f;
            sBossGanonCape->backSwayMagnitude = 1.25f;
            sBossGanonCape->sideSwayMagnitude = -2.0f;
            sBossGanonCape->minDist = 10.0f;
        } else if (gd->actionFunc == BossGanon_Block) {
            sBossGanonCape->backPush = -9.0f;
            sBossGanonCape->backSwayMagnitude = 0.25f;
            sBossGanonCape->sideSwayMagnitude = -2.0f;
            sBossGanonCape->minDist = 13.0f;
        }
    }


    void UpdatePuppet(PlayState* play) override {
        BossGanon* gd = Typed();

        if (IsLightBall()) {
            if (gd->collider.base.acFlags & AC_HIT) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }

            gd->collider.base.acFlags &= ~AC_HIT;
            Collider_UpdateCylinder(&gd->actor, &gd->collider);

            if (m_ballColliderOn) {
                ApplyColliderFlags(gd->collider.base, RolesToColliderBits(COLL_AC));
                RegisterColliderBase(play, &gd->collider.base, COLL_AC);
            }
            return;
        }

        if (sBossGanonCape) {
            sBossGanonCape->gravity = -3.0f;

            if (gd->actionFunc == BossGanon_IntroCutscene)
                ReplicateIntroCape();
            else
                ReplicateFightCape();
        }

        if (gd->csState == 9)
        {
            if (gd->csTimer == 2) {
                Player_SetCsActionWithHaltedActors(play, &gd->actor, 0x39);
            }

            gd->unk_70C = Math_SinS(gd->csTimer * 0x6300) * gd->unk_710;

            if (gd->csTimer < 100) {
                gd->windowShatterState = GDF_WINDOW_SHATTER_PARTIAL;
                gd->envLightMode = 15;
            } else {
                gd->envLightMode = 16;
                gd->windowShatterState = GDF_WINDOW_SHATTER_FULL;
            }

            if (gd->csTimer >= 130) {
                Math_ApproachF(&gd->whiteFillAlpha, 255.0f, 1.0f, 5.0f);
            }

            if (gd->csTimer >= 170) {
                play->transitionTrigger = TRANS_TRIGGER_START;
                play->nextEntranceIndex = ENTR_GANONS_TOWER_COLLAPSE_EXTERIOR_0;
                play->transitionType = TRANS_TYPE_FADE_WHITE_FAST;
                return;
            }
       
        }
        UpdateAnimation(&gd->skelAnime, LOCK_CUR_FRAME);

        BossGanon_UpdateEffects(play);

        if (gd->unk_2E6 != 0)
            gd->unk_2E6--;
        if (gd->unk_19C != 0)
            gd->unk_19C--;

        if (gd->collider.base.acFlags & AC_HIT) {
            if (ClaimLeadership(CLAIM_REASON_COOLDOWN)) {
                UpdateLeader(play);
                return;
            }
        }

        gd->collider.base.acFlags &= ~AC_HIT;
        gd->collider.base.atFlags &= ~AT_HIT;

        BossGanon_SetColliderPos(&gd->unk_1FC, &gd->collider);

        ApplyColliderFlags(gd->collider.base, RolesToColliderBits(m_roles));

        if (m_roles != 0)
            RegisterColliderBase(play, &gd->collider.base, m_roles);
    }

  private:
    static u8 RolesToColliderBits(u8 roles) {
        return ((roles & COLL_AC) ? 1 : 0) | ((roles & COLL_AT) ? 2 : 0) | ((roles & COLL_OC) ? 4 : 0);
    }

    u8 m_roles = 0;
    bool m_ballColliderOn = false;
    s16 m_prevCapeCsState = -1;
    s32 m_prevCapeCsTimer = -1;
    GanonActionFunc m_prevActionFunc = nullptr;
};

} // namespace ZeldaOnline

#endif