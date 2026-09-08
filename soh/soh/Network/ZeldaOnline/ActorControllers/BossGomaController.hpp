#ifndef BOSSGOMACONTROLLERH
#define BOSSGOMACONTROLLERH

#include "../AbstractBossController.hpp"
extern "C" {
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#include "objects/object_goma/object_goma.h"
#include "assets/textures/boss_title_cards/object_goma.h"
        
void BossGoma_SetupDefeated(BossGoma* boss, PlayState* play);

u16 func_800FA0B4(u8 seqPlayerIndex);
void func_80064534(PlayState* play, CutsceneContext* csCtx);

void BossGoma_UpdateTailLimbsScale(BossGoma* boss);

void BossGoma_Encounter(BossGoma* boss, PlayState* play);
void BossGoma_CeilingIdle(BossGoma* boss, PlayState* play);
void BossGoma_CeilingMoveToCenter(BossGoma* boss, PlayState* play);
void BossGoma_CeilingPrepareSpawnGohmas(BossGoma* boss, PlayState* play);
void BossGoma_CeilingSpawnGohmas(BossGoma* boss, PlayState* play);
void BossGoma_WallClimb(BossGoma* boss, PlayState* play);
void BossGoma_FallJump(BossGoma* boss, PlayState* play);
void BossGoma_FallStruckDown(BossGoma* boss, PlayState* play);
void BossGoma_FloorLand(BossGoma* boss, PlayState* play);
void BossGoma_FloorLandStruckDown(BossGoma* boss, PlayState* play);
void BossGoma_FloorIdle(BossGoma* boss, PlayState* play);
void BossGoma_FloorMain(BossGoma* boss, PlayState* play);
void BossGoma_FloorStunned(BossGoma* boss, PlayState* play);
void BossGoma_FloorAttackPosture(BossGoma* boss, PlayState* play);
void BossGoma_FloorPrepareAttack(BossGoma* boss, PlayState* play);
void BossGoma_FloorAttack(BossGoma* boss, PlayState* play);
void BossGoma_FloorDamaged(BossGoma* boss, PlayState* play);
void BossGoma_Defeated(BossGoma* boss, PlayState* play);
}

namespace ZeldaOnline {

class BossGomaController : public AbstractBossController {
  public:
    using AbstractBossController::AbstractBossController;

    BossGoma* Typed() const {
        return reinterpret_cast<BossGoma*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 SETUP_ID_FLOOR_IDLE = 10;
    static constexpr u8 SETUP_ID_FLOOR_ATTACK = 15;
    static constexpr u8 SETUP_ID_DEFEATED = 17;

    static const BossGomaActionFunc* ActionTable(size_t* count) {
        static const BossGomaActionFunc sTable[] = {
            BossGoma_Encounter,
            BossGoma_CeilingIdle,
            BossGoma_CeilingMoveToCenter,
            BossGoma_CeilingPrepareSpawnGohmas,
            BossGoma_CeilingSpawnGohmas,
            BossGoma_WallClimb,
            BossGoma_FallJump,
            BossGoma_FallStruckDown,
            BossGoma_FloorLand,
            BossGoma_FloorLandStruckDown,
            BossGoma_FloorIdle,
            BossGoma_FloorMain,
            BossGoma_FloorStunned,
            BossGoma_FloorAttackPosture,
            BossGoma_FloorPrepareAttack,
            BossGoma_FloorAttack,
            BossGoma_FloorDamaged,
            BossGoma_Defeated,
        };

        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    const char* GetTitleCard() const override {
        return gGohmaTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return &Typed()->subCameraId;
    }

    Vec3f_* GetCameraAt() override {
        return &Typed()->subCameraAt;
    }

    Vec3f_* GetCameraEye() override {
        return &Typed()->subCameraEye;
    }

    void OnActorInit() override {
        if (!IsLeader())
            EndCutsceneCamera();
    }

    u8 CurrentSetupIndex() const {
        size_t count;
        const BossGomaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] != nullptr && table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId != ACTOR_ITEM_B_HEART && actorId != ACTOR_DOOR_WARP1 && actorId != ACTOR_DOOR_SHUTTER;
    }

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gGohmaHangAnim;
            case 1:
                return gGohmaIdleCrouchedAnim;
            case 2:
                return gGohmaWalkAnim;
            case 3:
                return gGohmaWalkCrouchedAnim;
            case 4:
                return gGohmaStandAnim;
            case 5:
                return gGohmaClimbAnim;
            case 6:
                return gGohmaLandAnim;
            case 7:
                return gGohmaInitialLandingAnim;
            case 8:
                return gGohmaCrashAnim;
            case 9:
                return gGohmaStunnedAnim;
            case 10:
                return gGohmaPrepareAttackAnim;
            case 11:
                return gGohmaAttackAnim;
            case 12:
                return gGohmaRecoverAfterAttackAnim;
            case 13:
                return gGohmaRestAfterAttackAnim;
            case 14:
                return gGohmaDamageAnim;
            case 15:
                return gGohmaPrepareEggsAnim;
            case 16:
                return gGohmaLayEggsAnim;
            case 17:
                return gGohmaEyeRollAnim;
            case 18:
                return gGohmaDeathAnim;
            default:
                return gGohmaHangAnim;
        }
    }
    static constexpr int ANIM_COUNT = 19;

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelanime.animation;
        if (cur == nullptr)
            return ID_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ID_UNKNOWN;
    }

    void OnBecomeLeader() override {
        BossGoma* boss = Typed();

        if (boss->skelanime.playSpeed <= 0.0f)
            boss->skelanime.playSpeed = 1.0f;
    }


    static constexpr s16 ENCOUNTER_STATE_AWAIT_LOOK = 3;

    bool CurrentSetupIsEncounter() const {
        return Typed()->actionFunc == BossGoma_Encounter;
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_HEALTH,
        PROP_DISABLE_GAMEPLAY_LOGIC,
        PROP_ACTION_STATE,
        PROP_FRAMES_UNTIL_NEXT_ACTION,
        PROP_TIMER,
        PROP_FRAME_COUNT,
        PROP_PATIENCE_TIMER,
        PROP_INVINCIBILITY_FRAMES,
        PROP_EYE_CLOSED_TIMER,
        PROP_SPAWN_GOHMAS_TIMER,
        PROP_CURRENT_ANIM_FRAME_COUNT,
        PROP_CHILDREN_GOHMA_STATE,
        PROP_EYE_LID_BOTTOM_ROT_X,
        PROP_EYE_LID_TOP_ROT_X,
        PROP_EYE_IRIS_ROT_X,
        PROP_EYE_IRIS_ROT_Y,
        PROP_EYE_IRIS_SCALE,
        PROP_EYE_STATE,
        PROP_MAIN_ENV_COLOR,
        PROP_EYE_ENV_COLOR,
        PROP_COLLIDER_FLAGS,
        PROP_TAIL_SCALE_TIMERS,
        PROP_JOINT_TABLE
    };

    void BuildCustomProperties(ByteStream& out) override {
        BossGoma* boss = Typed();

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        PackProperty(PROP_ACTION, PackedUInt1(CurrentSetupIndex()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(boss->skelanime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &boss->skelanime), out);
        PackProperty(PROP_HEALTH, PackedUInt1(boss->actor.colChkInfo.health), out);
        PackProperty(PROP_DISABLE_GAMEPLAY_LOGIC, PackedInt2(boss->disableGameplayLogic), out);

        PackProperty(PROP_ACTION_STATE, PackedInt2(boss->actionState), out);
        PackProperty(PROP_FRAMES_UNTIL_NEXT_ACTION, PackedInt2(boss->framesUntilNextAction), out);
        PackProperty(PROP_TIMER, PackedInt2(boss->timer), out);
        PackProperty(PROP_FRAME_COUNT, PackedInt2(boss->frameCount), out);
        PackProperty(PROP_PATIENCE_TIMER, PackedInt2(boss->patienceTimer), out);
        PackProperty(PROP_INVINCIBILITY_FRAMES, PackedInt2(boss->invincibilityFrames), out);
        PackProperty(PROP_EYE_CLOSED_TIMER, PackedInt2(boss->eyeClosedTimer), out);
        PackProperty(PROP_SPAWN_GOHMAS_TIMER, PackedInt2(boss->spawnGohmasActionTimer), out);
        PackProperty(PROP_CURRENT_ANIM_FRAME_COUNT, PackedFloat4(boss->currentAnimFrameCount), out);

        {
            ByteStream kids;
            for (int i = 0; i < 3; i++)
                kids << PackedInt2(boss->childrenGohmaState[i]);
            PackProperty(PROP_CHILDREN_GOHMA_STATE, kids, out);
        }

        PackProperty(PROP_EYE_LID_BOTTOM_ROT_X, PackedInt2(boss->eyeLidBottomRotX), out);
        PackProperty(PROP_EYE_LID_TOP_ROT_X, PackedInt2(boss->eyeLidTopRotX), out);
        PackProperty(PROP_EYE_IRIS_ROT_X, PackedInt2(boss->eyeIrisRotX), out);
        PackProperty(PROP_EYE_IRIS_ROT_Y, PackedInt2(boss->eyeIrisRotY), out);
        PackProperty(PROP_EYE_IRIS_SCALE,
                     ByteStream() << PackedFloat4(boss->eyeIrisScaleX) << PackedFloat4(boss->eyeIrisScaleY), out);
        PackProperty(PROP_EYE_STATE, PackedInt2(boss->eyeState), out);

        PackProperty(PROP_MAIN_ENV_COLOR,
                     ByteStream() << PackedFloat4(boss->mainEnvColor[0]) << PackedFloat4(boss->mainEnvColor[1])
                                  << PackedFloat4(boss->mainEnvColor[2]),
                     out);
        PackProperty(PROP_EYE_ENV_COLOR,
                     ByteStream() << PackedFloat4(boss->eyeEnvColor[0]) << PackedFloat4(boss->eyeEnvColor[1])
                                  << PackedFloat4(boss->eyeEnvColor[2]),
                     out);

        PackProperty(PROP_COLLIDER_FLAGS, PackedUInt1(PackColliderFlags(boss->collider.base)), out);

        {
            ByteStream tail;
            for (int i = 0; i < 4; i++)
                tail << PackedUInt1((u8)(boss->tailLimbsScaleTimers[i]));
            PackProperty(PROP_TAIL_SCALE_TIMERS, tail, out);
        }

        BuildBossProperty(PROP_BOSS_CAMERA, out);
        BuildBossProperty(PROP_BOSS_BGM, out);
        BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
        BuildBossProperty(PROP_BOSS_LIGHTING, out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        if (false) {
            ByteStream pose;
            pose.Write(reinterpret_cast<const char*>(boss->skelanime.jointTable),
                       sizeof(Vec3s) * boss->skelanime.limbCount);
            PackProperty(PROP_JOINT_TABLE, pose, out);
        }
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossGoma* boss = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 setupIndex = (u8)(data.Read<PackedUInt1>().value());
                if (setupIndex != ID_UNKNOWN) {
                    m_currentSetupIndex = setupIndex;
                    size_t count;
                    const BossGomaActionFunc* table = ActionTable(&count);
                    if (setupIndex < count && table[setupIndex] != nullptr)
                        boss->actionFunc = table[setupIndex];
                }
                break;
            }
            case PROP_ANIM_CUR_FRAME:
                boss->skelanime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (ai != ID_UNKNOWN && ai < ANIM_COUNT) ? AnimForIndex(ai) : nullptr;
                ApplyAnimProperty((void*)a, &boss->skelanime, LOCK_CUR_FRAME ? boss->skelanime.curFrame : 0.0f, data);
                break;
            }
            case PROP_HEALTH:
                boss->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_DISABLE_GAMEPLAY_LOGIC:
                boss->disableGameplayLogic = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ACTION_STATE:
                boss->actionState = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FRAMES_UNTIL_NEXT_ACTION:
                boss->framesUntilNextAction = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER:
                boss->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FRAME_COUNT:
                boss->frameCount = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_PATIENCE_TIMER:
                boss->patienceTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_INVINCIBILITY_FRAMES:
                boss->invincibilityFrames = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_EYE_CLOSED_TIMER:
                boss->eyeClosedTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SPAWN_GOHMAS_TIMER:
                boss->spawnGohmasActionTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_CURRENT_ANIM_FRAME_COUNT:
                boss->currentAnimFrameCount = data.Read<PackedFloat4>().value();
                break;
            case PROP_CHILDREN_GOHMA_STATE:
                for (int i = 0; i < 3; i++)
                    boss->childrenGohmaState[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_EYE_LID_BOTTOM_ROT_X:
                boss->eyeLidBottomRotX = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_EYE_LID_TOP_ROT_X:
                boss->eyeLidTopRotX = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_EYE_IRIS_ROT_X:
                boss->eyeIrisRotX = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_EYE_IRIS_ROT_Y:
                boss->eyeIrisRotY = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_EYE_IRIS_SCALE:
                boss->eyeIrisScaleX = data.Read<PackedFloat4>().value();
                boss->eyeIrisScaleY = data.Read<PackedFloat4>().value();
                break;
            case PROP_EYE_STATE:
                boss->eyeState = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_MAIN_ENV_COLOR:
                boss->mainEnvColor[0] = data.Read<PackedFloat4>().value();
                boss->mainEnvColor[1] = data.Read<PackedFloat4>().value();
                boss->mainEnvColor[2] = data.Read<PackedFloat4>().value();
                break;
            case PROP_EYE_ENV_COLOR:
                boss->eyeEnvColor[0] = data.Read<PackedFloat4>().value();
                boss->eyeEnvColor[1] = data.Read<PackedFloat4>().value();
                boss->eyeEnvColor[2] = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLLIDER_FLAGS:
                ApplyColliderFlags(boss->collider.base, (u8)(data.Read<PackedUInt1>().value()));
                break;
            case PROP_TAIL_SCALE_TIMERS:
                for (int i = 0; i < 4; i++)
                    boss->tailLimbsScaleTimers[i] = (s16)(data.Read<PackedUInt1>().value());
                break;

            case PROP_JOINT_TABLE: {
                unsigned int cap = sizeof(Vec3s) * boss->skelanime.limbCount;
                unsigned int n = data.BytesLeft() < cap ? data.BytesLeft() : cap;
                ReadBlob(data, boss->skelanime.jointTable, n);
                break;
            }
            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == BossGoma_Defeated) {
            EndCutsceneCamera();
            BossGoma_SetupDefeated(Typed(), gPlayState);
            GoLocal();
        }
    }

    static constexpr int COLLIDER_ELEM_EYE = 0;

    bool EyeHitWouldReact() const {
        BossGoma* boss = Typed();
        return boss->invincibilityFrames == 0 && boss->eyeClosedTimer == 0 &&
               boss->actionFunc != BossGoma_CeilingSpawnGohmas &&
               (boss->collider.elements[COLLIDER_ELEM_EYE].info.bumperFlags & BUMP_HIT);
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);

        BossGoma* boss = Typed();

        if (boss->actionFunc == BossGoma_Defeated && !Flags_GetClear(play, play->roomCtx.curRoom.num))
            Flags_SetClear(play, play->roomCtx.curRoom.num);
    }

    void UpdatePuppet(PlayState* play) override {
        BossGoma* boss = Typed();

        UpdateAnimation(&boss->skelanime, LOCK_CUR_FRAME);

        if (EyeHitWouldReact()) {
            if (ClaimLeadership(CLAIM_REASON_COOLDOWN)) {
                UpdateLeader(play);
                return;
            }
        }


        if (boss->actionFunc == BossGoma_Encounter && boss->actionState == 0 && IsLocalPlayerClosest())
        {
            if (ClaimLeadership(CLAIM_REASON_COOLDOWN))
            {
                UpdateLeader(play);
                return;
            }
        }

        if (boss->actionState == 3 && boss->actionFunc == BossGoma_Encounter) {
            if (fabsf(boss->actor.projectedPos.x) < 150.0f && fabsf(boss->actor.projectedPos.y) < 250.0f &&
                boss->actor.projectedPos.z < 800.0f && boss->actor.projectedPos.z > 0.0f) {
                m_lookedAtFrames++;
            } else {
                m_lookedAtFrames = 0;
            }

            if (m_lookedAtFrames > 15) {
                m_lookedAtFrames = 0;
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }
        }

        boss->collider.base.acFlags &= ~AC_HIT;
        for (int i = 0; i < boss->collider.count; i++)
            boss->collider.elements[i].info.bumperFlags &= ~BUMP_HIT;

        bool floorPhase = m_currentSetupIndex >= SETUP_ID_FLOOR_IDLE && m_currentSetupIndex <= SETUP_ID_FLOOR_ATTACK;
        if (floorPhase && boss->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        BossGoma_UpdateTailLimbsScale(boss);

        if (!boss->disableGameplayLogic) {
            bool stunOrDamaged = m_currentSetupIndex == 13 || m_currentSetupIndex == 17;
            RegisterColliderBase(play, &boss->collider.base,
                                 stunOrDamaged ? (COLL_AC | COLL_OC) : (COLL_AT | COLL_AC | COLL_OC));
        }
    }

  private:
    s16 m_lookedAtFrames = 0;
    u8 m_currentSetupIndex = ID_UNKNOWN;
};

} // namespace ZeldaOnline

#endif