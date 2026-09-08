#ifndef TWINROVACONTROLLERH
#define TWINROVACONTROLLERH

#include <cstring>

#include <soh/Enhancements/game-interactor/GameInteractor_Hooks.h>
#include "../AbstractBossController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Tw/z_boss_tw.h"
#include "objects/object_tw/object_tw.h"
#include "assets/textures/boss_title_cards/object_tw.h"

void BossTw_TurnToPlayer(BossTw*, PlayState* play);
void BossTw_FlyTo(BossTw*, PlayState* play);
void BossTw_ShootBeam(BossTw*, PlayState* play);
void BossTw_FinishBeamShoot(BossTw*, PlayState* play);
void BossTw_HitByBeam(BossTw*, PlayState* play);
void BossTw_Laugh(BossTw*, PlayState* play);
void BossTw_Spin(BossTw*, PlayState* play);
void BossTw_MergeCS(BossTw*, PlayState* play);
void BossTw_Wait(BossTw*, PlayState* play);
void BossTw_DeathCS(BossTw*, PlayState* play);
void BossTw_CSWait(BossTw*, PlayState* play);

void BossTw_TwinrovaMergeCS(BossTw*, PlayState* play);
void BossTw_TwinrovaIntroCS(BossTw*, PlayState* play);
void BossTw_TwinrovaDeathCS(BossTw*, PlayState* play);
void BossTw_TwinrovaArriveAtTarget(BossTw*, PlayState* play);
void BossTw_TwinrovaChargeBlast(BossTw*, PlayState* play);
void BossTw_TwinrovaShootBlast(BossTw*, PlayState* play);
void BossTw_TwinrovaDoneBlastShoot(BossTw*, PlayState* play);
void BossTw_TwinrovaStun(BossTw*, PlayState* play);
void BossTw_TwinrovaGetUp(BossTw*, PlayState* play);
void BossTw_TwinrovaFly(BossTw*, PlayState* play);
void BossTw_TwinrovaSpin(BossTw*, PlayState* play);
void BossTw_TwinrovaLaugh(BossTw*, PlayState* play);

void BossTw_BlastFire(BossTw*, PlayState* play);
void BossTw_BlastIce(BossTw*, PlayState* play);
void BossTw_DeathBall(BossTw*, PlayState* play);

void BossTw_SetupFlyTo(BossTw*, PlayState* play);
void BossTw_SetupCSWait(BossTw*, PlayState* play);
void BossTw_SetupWait(BossTw*, PlayState* play);
void BossTw_TwinrovaSetupIntroCS(BossTw*, PlayState* play);
void BossTw_SetupDeathCS(BossTw*, PlayState* play);
void BossTw_TwinrovaSetupDeathCS(BossTw*, PlayState* play);

void BossTw_UpdateEffects(PlayState* play);
s32 BossTw_BlastShieldCheck(BossTw*, PlayState* play);
void BossTw_TwinrovaUpdate(Actor* thisx, PlayState* play2);
void BossTw_TwinrovaDraw(Actor* thisx, PlayState* play2);
void BossTw_DrawDeathBall(Actor* thisx, PlayState* play2);
void BossTw_BlastDraw(Actor* thisx, PlayState* play2);
void BossTw_BlastUpdate(Actor* thisx, PlayState* play);

u16 func_800FA0B4(u8 seqPlayerIndex);

extern BossTw** gBossTwKotake;
extern BossTw** gBossTwKoume;
extern BossTw** gBossTwTwinrova;

extern u8* gBossTwShieldFireCharge;
extern u8* gBossTwShieldIceCharge;
extern u8* gBossTwFreezeState;
extern u8* gBossTwBlastType;
extern u8* gBossTwGroundBlastType;
extern u8* gBossTwBeamDivertTimer;

extern ColliderCylinderInit* gBossTwCylBlasts;
extern ColliderCylinderInit* gBossTwCylKoumeKotake;
extern ColliderCylinderInit* gBossTwCylTwinrova;

extern u8* gBossTwInitalized;
extern s8* gBossTwEnvType;
extern u8* gBossTwFixedBlastType;
extern u8* gBossTwFixedBlatSeq;
extern f32* gBossTwD854;
extern f32* gBossTwD858;
extern u8* gBossTwD86F;
extern u8* gBossTwD870;
extern s16* gBossTwD872;
extern s16* gBossTwD874;
extern s16* gBossTwD876;
extern u8* gBossTwD878;
extern s16* gBossTwD87A;
extern s16* gBossTwD87C;
extern u8* gBossTwD87E;

extern BossTwEffect sTwEffects[150];
}

namespace ZeldaOnline {

class TwinrovaController : public AbstractBossController {
    typedef enum {
        /* 0x00 */ TW_KOTAKE,
        /* 0x01 */ TW_KOUME,
        /* 0x02 */ TW_TWINROVA,
        /* 0x64 */ TW_FIRE_BLAST = 0x64,
        /* 0x65 */ TW_FIRE_BLAST_GROUND,
        /* 0x66 */ TW_ICE_BLAST,
        /* 0x67 */ TW_ICE_BLAST_GROUND,
        /* 0x68 */ TW_DEATHBALL_KOTAKE,
        /* 0x69 */ TW_DEATHBALL_KOUME
    } TwinrovaType;

    static constexpr bool LOCK_CUR_FRAME = true;

  public:
    using AbstractBossController::AbstractBossController;

    BossTw* Typed() const {
        return reinterpret_cast<BossTw*>(m_actor);
    }

    const char* GetTitleCard() const override {
        return gTwinrovaTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return &Typed()->subCamId;
    }

    Vec3f_* GetCameraAt() override {
        auto tw = Typed();
        return (tw->actionFunc == BossTw_TwinrovaMergeCS && tw->unk_5F9 != 0) ? &tw->subCamAt2 : &tw->subCamAt;
    }

    Vec3f_* GetCameraEye() override {
        auto tw = Typed();
        return (tw->actionFunc == BossTw_TwinrovaMergeCS && tw->unk_5F9 != 0) ? &tw->subCamEye2 : &tw->subCamEye;
    }

    bool IsBlast() const {
        return Typed()->actor.params >= TW_FIRE_BLAST;
    }

    bool IsTwinrova() const {
        return Typed()->actor.params == TW_TWINROVA;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId != ACTOR_ITEM_B_HEART && actorId != ACTOR_DOOR_WARP1;
    }

    static void RegisterHooks(s16 actorID, bool enabled) {
        COND_ID_HOOK(ShouldActorInit, actorID, enabled, [&](void* actorRef, bool*) {
            Actor* actor = (Actor*)actorRef;

            AbstractActorController::InstallCustomInit(actor, TwinrovaController::ReplacementInit);
        });
    }

    static void ReplacementInit(Actor* thisx, PlayState* play) {
        BossTw* self = reinterpret_cast<BossTw*>(thisx);
        s16 i;

        thisx->targetMode = 5;
        thisx->gravity = 0.0f;
        thisx->targetArrowOffset = 0.0f;
        ActorShape_Init(&thisx->shape, 0.0f, NULL, 0.0f);

        if (thisx->params >= TW_FIRE_BLAST) {
            Actor_SetScale(thisx, 0.01f);
            thisx->update = BossTw_BlastUpdate;
            thisx->draw = BossTw_BlastDraw;
            thisx->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;

            Collider_InitCylinder(play, &self->collider);
            Collider_SetCylinder(play, &self->collider, thisx, gBossTwCylBlasts);

            if (thisx->params == TW_FIRE_BLAST || thisx->params == TW_FIRE_BLAST_GROUND) {
                self->actionFunc = BossTw_BlastFire;
                self->collider.info.toucher.effect = 1;
            } else if (thisx->params == TW_ICE_BLAST || thisx->params == TW_ICE_BLAST_GROUND) {
                self->actionFunc = BossTw_BlastIce;
            } else if (thisx->params >= TW_DEATHBALL_KOTAKE) {
                self->actionFunc = BossTw_DeathBall;
                thisx->draw = BossTw_DrawDeathBall;
                self->workf[TAIL_ALPHA] = 128.0f;

                if (*gBossTwTwinrova != nullptr) {
                    if (thisx->params == TW_DEATHBALL_KOTAKE) {
                        thisx->world.rot.y = (*gBossTwTwinrova)->actor.world.rot.y + 0x4000;
                    } else {
                        thisx->world.rot.y = (*gBossTwTwinrova)->actor.world.rot.y - 0x4000;
                    }
                }
            }

            self->timers[1] = 150;
            return;
        }

        Actor_SetScale(thisx, 2.5f * 0.01f);
        thisx->colChkInfo.mass = 255;
        thisx->colChkInfo.health = 0;
        Collider_InitCylinder(play, &self->collider);

        if (!*gBossTwInitalized) {
            *gBossTwInitalized = true;
            play->envCtx.unk_BF = 1;
            play->envCtx.unk_BE = 1;
            play->envCtx.unk_BD = 1;
            play->envCtx.unk_D8 = 0.0f;

            *gBossTwD874 = *gBossTwD876 = *gBossTwD87A = *gBossTwD87C = *gBossTwD872 = 0;
            *gBossTwD878 = *gBossTwD87E = *gBossTwD870 = *gBossTwD86F = 0;
            *gBossTwBeamDivertTimer = *gBossTwGroundBlastType = *gBossTwFreezeState = *gBossTwBlastType = 0;
            *gBossTwFixedBlatSeq = *gBossTwShieldFireCharge = *gBossTwShieldIceCharge = 0;
            *gBossTwEnvType = 0;

            *gBossTwD858 = *gBossTwD854 = 0.0f;
            *gBossTwFixedBlastType = (u8)Rand_ZeroFloat(1.99f);
            play->specialEffects = sTwEffects;

            for (i = 0; i < 150; i++) {
                sTwEffects[i].type = TWEFF_NONE;
                sTwEffects[i].epoch++;
            }
        }

        if (thisx->params == TW_KOTAKE) {
            Collider_SetCylinder(play, &self->collider, thisx, gBossTwCylKoumeKotake);
            thisx->naviEnemyId = 0x33;
            SkelAnime_InitFlex(play, &self->skelAnime, (FlexSkeletonHeader*)gTwinrovaKotakeSkel,
                               (AnimationHeader*)gTwinrovaKotakeKoumeFlyAnim, NULL, NULL, 0);

            if (Flags_GetEventChkInf(EVENTCHKINF_BEGAN_TWINROVA_BATTLE)) {
                BossTw_SetupFlyTo(self, play);
                thisx->world.pos.x = -600.0f;
                thisx->world.pos.y = 400.0f;
                thisx->world.pos.z = 0.0f;
                Audio_QueueSeqCmd(SEQ_PLAYER_BGM_MAIN << 24 | NA_BGM_BOSS);
            } else {
                BossTw_SetupCSWait(self, play);
            }

            Animation_MorphToLoop(&self->skelAnime, (AnimationHeader*)gTwinrovaKotakeKoumeFlyAnim, -3.0f);
            self->visible = true;
        } else if (thisx->params == TW_KOUME) {
            Collider_SetCylinder(play, &self->collider, thisx, gBossTwCylKoumeKotake);
            thisx->naviEnemyId = 0x32;
            SkelAnime_InitFlex(play, &self->skelAnime, (FlexSkeletonHeader*)gTwinrovaKoumeSkel,
                               (AnimationHeader*)gTwinrovaKotakeKoumeFlyAnim, NULL, NULL, 0);

            if (Flags_GetEventChkInf(EVENTCHKINF_BEGAN_TWINROVA_BATTLE)) {
                BossTw_SetupFlyTo(self, play);
                thisx->world.pos.x = 600.0f;
                thisx->world.pos.y = 400.0f;
                thisx->world.pos.z = 0.0f;
            } else {
                BossTw_SetupCSWait(self, play);
            }

            Animation_MorphToLoop(&self->skelAnime, (AnimationHeader*)gTwinrovaKotakeKoumeFlyAnim, -3.0f);
            self->visible = true;
        } else {
            Collider_SetCylinder(play, &self->collider, thisx, gBossTwCylTwinrova);
            thisx->naviEnemyId = 0x5B;
            thisx->colChkInfo.health = 24;
            thisx->update = BossTw_TwinrovaUpdate;
            thisx->draw = BossTw_TwinrovaDraw;
            SkelAnime_InitFlex(play, &self->skelAnime, (FlexSkeletonHeader*)gTwinrovaSkel,
                               (AnimationHeader*)gTwinrovaTPoseAnim, NULL, NULL, 0);
            Animation_MorphToLoop(&self->skelAnime, (AnimationHeader*)gTwinrovaTPoseAnim, -3.0f);

            if (Flags_GetEventChkInf(EVENTCHKINF_BEGAN_TWINROVA_BATTLE)) {
                BossTw_SetupWait(self, play);
            } else {
                BossTw_TwinrovaSetupIntroCS(self, play);
                thisx->world.pos.x = 0.0f;
                thisx->world.pos.y = 1000.0f;
                thisx->world.pos.z = 0.0f;
            }

            thisx->params = TW_TWINROVA;
            *gBossTwTwinrova = self;

            if (Flags_GetClear(play, play->roomCtx.curRoom.num)) {
                Actor_Kill(thisx);
                if (GameInteractor_Should(VB_SPAWN_BLUE_WARP, true, self)) {
                    Actor_SpawnAsChild(&play->actorCtx, thisx, play, ACTOR_DOOR_WARP1, 600.0f, 230.0f, 0.0f, 0, 0, 0,
                                       WARP_DUNGEON_ADULT);
                }

                if (GameInteractor_Should(VB_SPAWN_HEART_CONTAINER, true)) {
                    Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_B_HEART, -600.0f, 230.0f, 0.0f, 0, 0, 0, 0);
                }
            } else {

                BossTw* kotake =
                    (BossTw*)Actor_SpawnAsChild(&play->actorCtx, thisx, play, ACTOR_BOSS_TW, thisx->world.pos.x,
                                                thisx->world.pos.y, thisx->world.pos.z, 0, 0, 0, TW_KOTAKE);
                BossTw* koume =
                    (BossTw*)Actor_SpawnAsChild(&play->actorCtx, thisx, play, ACTOR_BOSS_TW, thisx->world.pos.x,
                                                thisx->world.pos.y, thisx->world.pos.z, 0, 0, 0, TW_KOUME);
                if (kotake != nullptr)
                    *gBossTwKotake = kotake;
                if (koume != nullptr)
                    *gBossTwKoume = koume;

                if (kotake != nullptr && koume != nullptr) {
                    kotake->actor.parent = &koume->actor;
                    koume->actor.parent = &kotake->actor;
                }
            }
        }

        self->fogR = play->lightCtx.fogColor[0];
        self->fogG = play->lightCtx.fogColor[1];
        self->fogB = play->lightCtx.fogColor[2];
        self->fogNear = play->lightCtx.fogNear;
        self->fogFar = 1000.0f;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 29;

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gTwinrovaKotakeKoumeFlyAnim,
            gTwinrovaKotakeKoumeIdleLoopAnim,
            gTwinrovaKotakeKoumeIdleEndAnim,
            gTwinrovaKotakeKoumeAttackStartAnim,
            gTwinrovaKotakeKoumeAttackLoopAnim,
            gTwinrovaKotakeKoumeAttackEndAnim,
            gTwinrovaKotakeKoumeChargeUpAttackStartAnim,
            gTwinrovaKotakeKoumeChargeUpAttackLoopAnim,
            gTwinrovaKotakeKoumeDamageStartAnim,
            gTwinrovaKotakeKoumeDamageEndAnim,
            gTwinrovaKotakeKoumeFloatLookForwardAnim,
            gTwinrovaKotakeKoumeFloatLookUpAnim,
            gTwinrovaKotakeKoumeLaughAnim,
            gTwinrovaKotakeKoumeSpinAnim,
            gTwinrovaKotakeKoumeShakeHandAnim,
            gTwinrovaKotakeKoumeBickerAnim,
            gTwinrovaTPoseAnim,
            gTwinrovaHoverAnim,
            gTwinrovaIntroAnim,
            gTwinrovaLaughAnim,
            gTwinrovaWindUpAnim,
            gTwinrovaFireAttackAnim,
            gTwinrovaIceAttackAnim,
            gTwinrovaChargedAttackHitAnim,
            gTwinrovaDamageAnim,
            gTwinrovaStunStartAnim,
            gTwinrovaStunLoopAnim,
            gTwinrovaStunEndAnim,
            gTwinrovaDeathAnim,
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

    using TwActionFunc = decltype(&BossTw_Wait);
    static const TwActionFunc* ActionTable(size_t* count) {
        static const TwActionFunc sTable[] = {
            BossTw_TurnToPlayer,
            BossTw_FlyTo,
            BossTw_ShootBeam,
            BossTw_FinishBeamShoot,
            BossTw_HitByBeam,
            BossTw_Laugh,
            BossTw_Spin,
            BossTw_MergeCS,
            BossTw_Wait,
            BossTw_DeathCS,
            BossTw_CSWait,
            BossTw_TwinrovaMergeCS,
            BossTw_TwinrovaIntroCS,
            BossTw_TwinrovaDeathCS,
            BossTw_TwinrovaArriveAtTarget,
            BossTw_TwinrovaChargeBlast,
            BossTw_TwinrovaShootBlast,
            BossTw_TwinrovaDoneBlastShoot,
            BossTw_TwinrovaStun,
            BossTw_TwinrovaGetUp,
            BossTw_TwinrovaFly,
            BossTw_TwinrovaSpin,
            BossTw_TwinrovaLaugh,
            BossTw_BlastFire,
            BossTw_BlastIce,
            BossTw_DeathBall,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TwActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    void OnActorInit() override {
        ReinstallUpdate();
        if (IsLeader())
            EndCutsceneCamera();

        if (Flags_GetClear(gPlayState, gPlayState->roomCtx.curRoom.num)) {
            Actor_Kill(m_actor);
            return;
        }
        BossTw* tw = Typed();

        switch (tw->actor.params) {
            case TW_KOTAKE:
                *gBossTwKotake = tw;
                break;
            case TW_KOUME:
                *gBossTwKoume = tw;
                break;
            case TW_TWINROVA:
                *gBossTwTwinrova = tw;
                break;
        }

        if (*gBossTwKotake != nullptr && *gBossTwKoume != nullptr) {
            (*gBossTwKotake)->actor.parent = &(*gBossTwKoume)->actor;
            (*gBossTwKoume)->actor.parent = &(*gBossTwKotake)->actor;
        }
    }

    void DragOthers(PlayState* play) {
        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_BOSS].head; it != nullptr; it = it->next) {
            if (it == m_actor || it->id != ACTOR_BOSS_TW || it->zoController == nullptr)
                continue;

            auto* other = reinterpret_cast<AbstractActorController*>(it->zoController);
            if (!other->IsLeader())
                other->ClaimLeadership(CLAIM_REASON_NOW);
        }
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_WORK,
        PROP_TIMERS,
        PROP_FWORK,
        PROP_STATE,
        PROP_BEAM,
        PROP_POS,
        PROP_ALPHA,
        PROP_SHARED,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BossTw* tw = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        ByteStream work;
        for (s32 i = 0; i < WORK_MAX; i++)
            work << PackedInt2(tw->work[i]);
        PackProperty(PROP_WORK, work, out);

        ByteStream timers;
        for (s32 i = 0; i < 5; i++)
            timers << PackedInt2(tw->timers[i]);
        PackProperty(PROP_TIMERS, timers, out);

        ByteStream fwork;
        for (s32 i = 0; i < FWORK_MAX; i++)
            fwork << PackedFloat4(tw->workf[i]);
        PackProperty(PROP_FWORK, fwork, out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(tw->eyeTexIdx) << PackedInt2(tw->leftEyeTexIdx)
                                  << PackedUInt1(tw->visible) << PackedUInt1(tw->blastActive)
                                  << PackedInt2(tw->blastType) << PackedUInt1(tw->unk_5F8) << PackedUInt1(tw->unk_5F9)
                                  << PackedUInt1(tw->twinrovaStun) << PackedInt2(tw->beamShootState)
                                  << PackedInt2(tw->csState1) << PackedInt2(tw->csState2) << PackedInt2(tw->csSfxTimer)
                                  << PackedInt1(tw->actor.colChkInfo.health),
                     out);

        PackProperty(PROP_BEAM,
                     ByteStream() << PackedFloat4(tw->beamOrigin.x) << PackedFloat4(tw->beamOrigin.y)
                                  << PackedFloat4(tw->beamOrigin.z) << PackedFloat4(tw->beamReflectionOrigin.x)
                                  << PackedFloat4(tw->beamReflectionOrigin.y)
                                  << PackedFloat4(tw->beamReflectionOrigin.z) << PackedFloat4(tw->beamPitch)
                                  << PackedFloat4(tw->beamYaw) << PackedFloat4(tw->beamRoll)
                                  << PackedFloat4(tw->beamDist) << PackedFloat4(tw->beamScale)
                                  << PackedFloat4(tw->beamReflectionPitch) << PackedFloat4(tw->beamReflectionYaw)
                                  << PackedFloat4(tw->beamReflectionDist) << PackedInt2(tw->magicDir.x)
                                  << PackedInt2(tw->magicDir.y) << PackedInt2(tw->magicDir.z),
                     out);

        PackProperty(PROP_POS,
                     ByteStream() << PackedFloat4(tw->crownPos.x) << PackedFloat4(tw->crownPos.y)
                                  << PackedFloat4(tw->crownPos.z) << PackedFloat4(tw->leftScepterPos.x)
                                  << PackedFloat4(tw->leftScepterPos.y) << PackedFloat4(tw->leftScepterPos.z)
                                  << PackedFloat4(tw->rightScepterPos.x) << PackedFloat4(tw->rightScepterPos.y)
                                  << PackedFloat4(tw->rightScepterPos.z) << PackedFloat4(tw->targetPos.x)
                                  << PackedFloat4(tw->targetPos.y) << PackedFloat4(tw->targetPos.z)
                                  << PackedFloat4(tw->groundBlastPos.x) << PackedFloat4(tw->groundBlastPos.y)
                                  << PackedFloat4(tw->groundBlastPos.z) << PackedFloat4(tw->groundBlastPos2.x)
                                  << PackedFloat4(tw->groundBlastPos2.y) << PackedFloat4(tw->groundBlastPos2.z),
                     out);

        PackProperty(PROP_ALPHA,
                     ByteStream() << PackedFloat4(tw->scepterAlpha) << PackedFloat4(tw->flameAlpha)
                                  << PackedFloat4(tw->spawnPortalAlpha) << PackedFloat4(tw->spawnPortalScale)
                                  << PackedFloat4(tw->unk_4DC) << PackedFloat4(tw->rotateSpeed)
                                  << PackedFloat4(tw->flameRotation) << PackedFloat4(tw->portalRotation)
                                  << PackedFloat4(tw->updateRate1) << PackedFloat4(tw->updateRate2),
                     out);

        if (IsTwinrova()) {
            PackProperty(PROP_SHARED,
                         ByteStream() << PackedUInt1(*gBossTwBlastType) << PackedUInt1(*gBossTwGroundBlastType), out);
        }

        if (!IsBlast()) {
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(tw->skelAnime.curFrame), out);
            PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &tw->skelAnime), out);
        }

        if (IsTwinrova()) {
            BuildBossProperty(PROP_BOSS_CAMERA, out);
            BuildBossProperty(PROP_BOSS_BGM, out);
            BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
            BuildBossProperty(PROP_BOSS_LIGHTING, out);
        }

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossTw* tw = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TwActionFunc* table = ActionTable(&count);
                if (id < count)
                    tw->actionFunc = table[id];
                break;
            }
            case PROP_WORK:
                for (s32 i = 0; i < WORK_MAX; i++)
                    tw->work[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 5; i++)
                    tw->timers[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FWORK:
                for (s32 i = 0; i < FWORK_MAX; i++)
                    tw->workf[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_STATE:
                tw->eyeTexIdx = (s16)(data.Read<PackedInt2>().value());
                tw->leftEyeTexIdx = (s16)(data.Read<PackedInt2>().value());
                tw->visible = (u8)(data.Read<PackedUInt1>().value());
                tw->blastActive = (u8)(data.Read<PackedUInt1>().value());
                tw->blastType = (s16)(data.Read<PackedInt2>().value());
                tw->unk_5F8 = (u8)(data.Read<PackedUInt1>().value());
                tw->unk_5F9 = (u8)(data.Read<PackedUInt1>().value());
                tw->twinrovaStun = (u8)(data.Read<PackedUInt1>().value());
                tw->beamShootState = (s16)(data.Read<PackedInt2>().value());
                tw->csState1 = (s16)(data.Read<PackedInt2>().value());
                tw->csState2 = (s16)(data.Read<PackedInt2>().value());
                tw->csSfxTimer = (s16)(data.Read<PackedInt2>().value());
                tw->actor.colChkInfo.health = (s8)(data.Read<PackedInt1>().value());
                break;
            case PROP_BEAM:
                tw->beamOrigin.x = data.Read<PackedFloat4>().value();
                tw->beamOrigin.y = data.Read<PackedFloat4>().value();
                tw->beamOrigin.z = data.Read<PackedFloat4>().value();
                tw->beamReflectionOrigin.x = data.Read<PackedFloat4>().value();
                tw->beamReflectionOrigin.y = data.Read<PackedFloat4>().value();
                tw->beamReflectionOrigin.z = data.Read<PackedFloat4>().value();
                tw->beamPitch = data.Read<PackedFloat4>().value();
                tw->beamYaw = data.Read<PackedFloat4>().value();
                tw->beamRoll = data.Read<PackedFloat4>().value();
                tw->beamDist = data.Read<PackedFloat4>().value();
                tw->beamScale = data.Read<PackedFloat4>().value();
                tw->beamReflectionPitch = data.Read<PackedFloat4>().value();
                tw->beamReflectionYaw = data.Read<PackedFloat4>().value();
                tw->beamReflectionDist = data.Read<PackedFloat4>().value();
                tw->magicDir.x = (s16)(data.Read<PackedInt2>().value());
                tw->magicDir.y = (s16)(data.Read<PackedInt2>().value());
                tw->magicDir.z = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_POS:
                tw->crownPos.x = data.Read<PackedFloat4>().value();
                tw->crownPos.y = data.Read<PackedFloat4>().value();
                tw->crownPos.z = data.Read<PackedFloat4>().value();
                tw->leftScepterPos.x = data.Read<PackedFloat4>().value();
                tw->leftScepterPos.y = data.Read<PackedFloat4>().value();
                tw->leftScepterPos.z = data.Read<PackedFloat4>().value();
                tw->rightScepterPos.x = data.Read<PackedFloat4>().value();
                tw->rightScepterPos.y = data.Read<PackedFloat4>().value();
                tw->rightScepterPos.z = data.Read<PackedFloat4>().value();
                tw->targetPos.x = data.Read<PackedFloat4>().value();
                tw->targetPos.y = data.Read<PackedFloat4>().value();
                tw->targetPos.z = data.Read<PackedFloat4>().value();
                tw->groundBlastPos.x = data.Read<PackedFloat4>().value();
                tw->groundBlastPos.y = data.Read<PackedFloat4>().value();
                tw->groundBlastPos.z = data.Read<PackedFloat4>().value();
                tw->groundBlastPos2.x = data.Read<PackedFloat4>().value();
                tw->groundBlastPos2.y = data.Read<PackedFloat4>().value();
                tw->groundBlastPos2.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_ALPHA:
                tw->scepterAlpha = data.Read<PackedFloat4>().value();
                tw->flameAlpha = data.Read<PackedFloat4>().value();
                tw->spawnPortalAlpha = data.Read<PackedFloat4>().value();
                tw->spawnPortalScale = data.Read<PackedFloat4>().value();
                tw->unk_4DC = data.Read<PackedFloat4>().value();
                tw->rotateSpeed = data.Read<PackedFloat4>().value();
                tw->flameRotation = data.Read<PackedFloat4>().value();
                tw->portalRotation = data.Read<PackedFloat4>().value();
                tw->updateRate1 = data.Read<PackedFloat4>().value();
                tw->updateRate2 = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_CUR_FRAME:
                if (propLen == 0)
                    break;
                tw->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                if (propLen == 0)
                    break;
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &tw->skelAnime, LOCK_CUR_FRAME ? tw->skelAnime.curFrame : 0.0f, data);
                break;
            }
            
            case PROP_SHARED:
                if (propLen == 0)
                    break;
                *gBossTwBlastType = (u8)(data.Read<PackedUInt1>().value());
                *gBossTwGroundBlastType = (u8)(data.Read<PackedUInt1>().value());
                break;

            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BossTw* tw = Typed();

        if (tw->actionFunc == BossTw_TwinrovaDeathCS) {
            EndCutsceneCamera();
            BossTw_TwinrovaSetupDeathCS(tw, gPlayState);
            GoLocal();
        } else if (tw->actionFunc == BossTw_DeathCS) {
            EndCutsceneCamera();
            BossTw_SetupDeathCS(tw, gPlayState);
            GoLocal();
        }
    }



    void OnLoseLeadership() override {
        BossTw* tw = Typed();

        if (!IsTwinrova()) {
            return;
        }

        *gBossTwShieldFireCharge = 0;
        *gBossTwShieldIceCharge = 0;
        *gBossTwFreezeState = 0;
        *gBossTwBeamDivertTimer = 0;
        *gBossTwD854 = 0.0f;
        *gBossTwD858 = 0.0f;
        *gBossTwD86F = 0;
        *gBossTwD872 = 0;

        EndCutsceneCamera();
    }

    bool SistersIdleFlying() const {
        BossTw* kotake = *gBossTwKotake;
        BossTw* koume = *gBossTwKoume;

        if (kotake == nullptr || koume == nullptr)
            return false;

        if (!kotake->visible || !koume->visible)
            return false;

        return kotake->actionFunc == BossTw_FlyTo && koume->actionFunc == BossTw_FlyTo;
    }

    void UpdateLeader(PlayState* play) override {

        if (IsTwinrova())
            DragOthers(play);

        AbstractActorController::UpdateLeader(play);
        ReinstallUpdate();

        BossTw* tw = Typed();
        if (tw->actionFunc == BossTw_DeathCS && !Flags_GetClear(play, play->roomCtx.curRoom.num))
            Flags_SetClear(play, play->roomCtx.curRoom.num);
    }

    void ClaimLeadershipViaTwinRova() {
        if (*gBossTwTwinrova != nullptr && (*gBossTwTwinrova)->actor.zoController != nullptr) {
            auto rovaController = reinterpret_cast<AbstractActorController*>((*gBossTwTwinrova)->actor.zoController);
            rovaController->ClaimLeadership(CLAIM_REASON_NOW);
        }
    }
    void UpdatePuppet(PlayState* play) override {
        BossTw* tw = Typed();

        if (IsTwinrova())
            BossTw_UpdateEffects(play);

        if (!IsBlast())
            UpdateAnimation(&tw->skelAnime, LOCK_CUR_FRAME);

        if (IsBlast()) {
            tw->work[TAIL_IDX]++;
            if (tw->work[TAIL_IDX] >= 50)
                tw->work[TAIL_IDX] = 0;
            tw->blastTailPos[tw->work[TAIL_IDX]] = tw->actor.world.pos;

            tw->actor.focus.pos = tw->actor.world.pos;

            Collider_UpdateCylinder(&tw->actor, &tw->collider);

            tw->collider.base.acFlags &= ~AC_HIT;
            tw->collider.base.atFlags &= ~AT_HIT;

            RegisterColliderBase(play, &tw->collider.base, COLL_AC | COLL_AT);
            return;
        }

        if (tw->visible) {
            tw->actor.focus.pos = tw->actor.world.pos;

            if (tw->collider.base.acFlags & AC_HIT) {
                if (IsTwinrova()) {
                    if (ClaimLeadership(CLAIM_REASON_COOLDOWN)) {
                        UpdateLeader(play);
                        return;
                    }
                }

                if (tw->visible && tw->unk_5F8 == 0) {
                    ClaimLeadershipViaTwinRova();
                }

                return;
            }

            if (tw->actor.params <= TW_KOUME && SistersIdleFlying() && IsLocalPlayerClosest())
                ClaimLeadershipViaTwinRova();

            tw->collider.base.acFlags &= ~AC_HIT;
            tw->collider.base.atFlags &= ~AT_HIT;

            tw->collider.base.colType = COLTYPE_HIT3;
            tw->collider.dim.radius = (tw->actionFunc == BossTw_Spin) ? 90 : 45;
            tw->collider.dim.height = 120;
            tw->collider.dim.yShift = -30;

            Collider_UpdateCylinder(&tw->actor, &tw->collider);

            if (tw->work[INVINC_TIMER] == 0)
                RegisterColliderBase(play, &tw->collider.base, COLL_AC | COLL_AT | COLL_OC);
        }
    }

  private:
};

} // namespace ZeldaOnline

#endif