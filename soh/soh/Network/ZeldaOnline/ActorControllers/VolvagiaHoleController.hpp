#ifndef VOLVAGIAHOLECONTROLLERH
#define VOLVAGIAHOLECONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Fd2/z_boss_fd2.h"
#include "src/overlays/actors/ovl_Boss_Fd/z_boss_fd.h"
#include "objects/object_fd2/object_fd2.h"

void BossFd2_Emerge(BossFd2* fd2, PlayState* play);
void BossFd2_Idle(BossFd2* fd2, PlayState* play);
void BossFd2_Burrow(BossFd2* fd2, PlayState* play);
void BossFd2_BreatheFire(BossFd2* fd2, PlayState* play);
void BossFd2_ClawSwipe(BossFd2* fd2, PlayState* play);
void BossFd2_Vulnerable(BossFd2* fd2, PlayState* play);
void BossFd2_Damaged(BossFd2* fd2, PlayState* play);
void BossFd2_Death(BossFd2* fd2, PlayState* play);
void BossFd2_Wait(BossFd2* fd2, PlayState* play);
void BossFd2_SetupDeath(BossFd2* thisx, PlayState* play);
void BossFd2_UpdateFace(BossFd2* fd2, PlayState* play);
void BossFd2_SpawnFireBreath(PlayState* play, BossFdEffect* effect, Vec3f* position, Vec3f* velocity,
                             Vec3f* acceleration, f32 scale, s16 alpha, s16 kbAngle);
void BossFd2_SpawnEmber(PlayState* play, BossFdEffect* effect, Vec3f* position, Vec3f* velocity, Vec3f* acceleration,
                        f32 scale);
}

namespace ZeldaOnline {

class VolvagiaHoleController : public AbstractActorController {
    typedef enum {
        /* 0 */ DEATH_START,
        /* 1 */ DEATH_RETREAT,
        /* 2 */ DEATH_HANDOFF,
        /* 3 */ DEATH_FD_BODY,
        /* 4 */ DEATH_FD_SKULL,
        /* 5 */ DEATH_FINISH
    } BossFd2CutsceneState;

  public:
    using AbstractActorController::AbstractActorController;

    BossFd2* Typed() const {
        return reinterpret_cast<BossFd2*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using Fd2ActionFunc = decltype(&BossFd2_Idle);
    static const Fd2ActionFunc* ActionTable(size_t* count) {
        static const Fd2ActionFunc sTable[] = {
            BossFd2_Emerge,     BossFd2_Idle,    BossFd2_Burrow, BossFd2_BreatheFire, BossFd2_ClawSwipe,
            BossFd2_Vulnerable, BossFd2_Damaged, BossFd2_Death,  BossFd2_Wait,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const Fd2ActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 10;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gHoleVolvagiaEmergeAnim;
            case 1:
                return gHoleVolvagiaIdleAnim;
            case 2:
                return gHoleVolvagiaBurrowAnim;
            case 3:
                return gHoleVolvagiaBreatheFireAnim;
            case 4:
                return gHoleVolvagiaClawSwipeAnim;
            case 5:
                return gHoleVolvagiaVulnerableAnim;
            case 6:
                return gHoleVolvagiaDamagedAnim;
            case 7:
                return gHoleVolvagiaHitAnim;
            case 8:
                return gHoleVolvagiaKnockoutAnim;
            case 9:
                return gHoleVolvagiaTurnAnim;
            default:
                return nullptr;
        }
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    void DragFlyingHalf(PlayState* play) {
        BossFd* bossFd = (BossFd*)m_actor->parent;

        if (bossFd) {
            auto* ctl = reinterpret_cast<VolvagiaController*>(bossFd->actor.zoController);
            if (!ctl->IsLeader())
                ctl->ClaimLeadership(CLAIM_REASON_NOW);
        }
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WORK,
        PROP_TIMERS,
        PROP_FWORK,
        PROP_STATE,
        PROP_HEAD,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void OnActorInit() override {
        if (Flags_GetClear(gPlayState, gPlayState->roomCtx.curRoom.num))
            Actor_Kill(m_actor);
    }

    void BuildCustomProperties(ByteStream& out) override {
        BossFd2* fd2 = Typed();
        Actor* parent = m_actor->parent;
        BossFd* bossFd = nullptr;

        if (parent != nullptr && parent->id == ACTOR_BOSS_FD)
            bossFd = (BossFd*)parent;

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        ByteStream work;
        for (s32 i = 0; i < FD2_SHORT_COUNT; i++)
            work << PackedInt2(fd2->work[i]);
        PackProperty(PROP_WORK, work, out);

        ByteStream timers;
        for (s32 i = 0; i < 5; i++)
            timers << PackedInt2(fd2->timers[i]);
        PackProperty(PROP_TIMERS, timers, out);

        ByteStream fwork;
        for (s32 i = 0; i < FD2_FLOAT_COUNT; i++)
            fwork << PackedFloat4(fd2->fwork[i]);
        PackProperty(PROP_FWORK, fwork, out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(fd2->disableAT) << PackedUInt1(fd2->eyeState)
                                  << PackedFloat4(fd2->jawOpening) << PackedInt2(fd2->deathState)
                                  << PackedUInt1(fd2->actor.colChkInfo.health)
                                  << PackedUInt1(bossFd != nullptr ? bossFd->faceExposed : 0)
                                  << PackedUInt1(bossFd != nullptr ? bossFd->actor.colChkInfo.health : 0),
                     out);

        PackProperty(PROP_HEAD,
                     ByteStream() << PackedFloat4(fd2->headPos.x) << PackedFloat4(fd2->headPos.y)
                                  << PackedFloat4(fd2->headPos.z) << PackedInt2(fd2->headRot.x)
                                  << PackedInt2(fd2->headRot.y) << PackedInt2(fd2->headRot.z),
                     out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(fd2->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &fd2->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossFd2* fd2 = Typed();
        Actor* parent = m_actor->parent;
        BossFd* bossFd = nullptr;

        if (parent != nullptr && parent->id == ACTOR_BOSS_FD)
            bossFd = (BossFd*)parent;

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const Fd2ActionFunc* table = ActionTable(&count);
                if (id < count)
                    fd2->actionFunc = table[id];
                break;
            }
            case PROP_WORK:
                for (s32 i = 0; i < FD2_SHORT_COUNT; i++)
                    fd2->work[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 5; i++)
                    fd2->timers[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FWORK:
                for (s32 i = 0; i < FD2_FLOAT_COUNT; i++)
                    fd2->fwork[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_STATE: {
                fd2->disableAT = (u8)(data.Read<PackedUInt1>().value());
                fd2->eyeState = (u8)(data.Read<PackedUInt1>().value());
                fd2->jawOpening = data.Read<PackedFloat4>().value();
                fd2->deathState = (s16)(data.Read<PackedInt2>().value());
                fd2->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());

                u8 faceExposed = (u8)(data.Read<PackedUInt1>().value());
                u8 parentHealth = (u8)(data.Read<PackedUInt1>().value());

                if (bossFd != nullptr) {
                    bossFd->faceExposed = faceExposed;
                    bossFd->actor.colChkInfo.health = parentHealth;
                }
                break;
            }
            case PROP_HEAD:
                fd2->headPos.x = data.Read<PackedFloat4>().value();
                fd2->headPos.y = data.Read<PackedFloat4>().value();
                fd2->headPos.z = data.Read<PackedFloat4>().value();
                fd2->headRot.x = (s16)(data.Read<PackedInt2>().value());
                fd2->headRot.y = (s16)(data.Read<PackedInt2>().value());
                fd2->headRot.z = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                fd2->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &fd2->skelAnime, LOCK_CUR_FRAME ? fd2->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BossFd2* fd2 = Typed();

        if (fd2->actionFunc != BossFd2_Death || fd2->deathState == DEATH_FINISH)
            return;

        BossFd2_SetupDeath(fd2, gPlayState);
        fd2->work[FD2_DAMAGE_FLASH_TIMER] = 10;
        fd2->work[FD2_INVINC_TIMER] = 30000;
        Audio_QueueSeqCmd(0x1 << 28 | SEQ_PLAYER_BGM_MAIN << 24 | 0x100FF);
        Audio_PlayActorSound2(&fd2->actor, NA_SE_EN_VALVAISA_DEAD);
        Enemy_StartFinishingBlow(gPlayState, &fd2->actor);
        GameInteractor_ExecuteOnBossDefeat(&fd2->actor);

        Actor* parent = m_actor->parent;

        if (parent != nullptr && parent->id == ACTOR_BOSS_FD) {
            BossFd* bossFd = (BossFd*)parent;
            auto* ctl = reinterpret_cast<VolvagiaController*>(bossFd->actor.zoController);

            if (ctl != nullptr)
                ctl->PrepareForDeath();
        }

        GoLocal();
    }

    void OnBecomeLeader() override {
        if (gPlayState != nullptr)
            DragFlyingHalf(gPlayState);
    }

    void UpdateLeader(PlayState* play) override {
        DragFlyingHalf(play);
        AbstractActorController::UpdateLeader(play);

        BossFd2* fd2 = Typed();

        if (fd2->actionFunc == BossFd2_Death && !Flags_GetClear(play, play->roomCtx.curRoom.num))
            Flags_SetClear(play, play->roomCtx.curRoom.num);
    }

    void SpawnFireBreath(PlayState* play) {
        BossFd2* fd2 = Typed();
        BossFd* bossFd = (BossFd*)fd2->actor.parent;
        s16 breathOpacity = 0;
        s16 i;
        f32 tempX;
        f32 tempY;

        if (bossFd == nullptr || fd2->actionFunc != BossFd2_BreatheFire)
            return;

        if ((25.0f <= fd2->skelAnime.curFrame) && (fd2->skelAnime.curFrame < 70.0f)) {
            if (fd2->skelAnime.curFrame > 50) {
                breathOpacity = (s16)((70.0f - fd2->skelAnime.curFrame) * 12.0f);
            } else {
                breathOpacity = 255;
            }
        }

        if (breathOpacity == 0)
            return;

        f32 breathScale;
        Vec3f spawnSpeed = { 0.0f, 0.0f, 0.0f };
        Vec3f spawnVel;
        Vec3f spawnAccel = { 0.0f, 0.0f, 0.0f };
        Vec3f spawnPos;

        bossFd->fogMode = 2;
        spawnSpeed.z = 30.0f;
        spawnPos = fd2->headPos;

        tempY = ((fd2->actor.shape.rot.y + fd2->headRot.y) / (f32)0x8000) * (f32)M_PI;
        tempX = ((fd2->headRot.x / (f32)0x8000) * (f32)M_PI) + 1.0f / 2;
        Matrix_RotateY(tempY, MTXMODE_NEW);
        Matrix_RotateX(tempX, MTXMODE_APPLY);
        Matrix_MultVec3f(&spawnSpeed, &spawnVel);

        breathScale = 300.0f + 50.0f * Math_SinS(fd2->work[FD2_VAR_TIMER] * 0x2000);
        BossFd2_SpawnFireBreath(play, bossFd->effects, &spawnPos, &spawnVel, &spawnAccel, breathScale, breathOpacity,
                                fd2->actor.shape.rot.y + fd2->headRot.y);

        spawnPos.x += spawnVel.x * 0.5f;
        spawnPos.y += spawnVel.y * 0.5f;
        spawnPos.z += spawnVel.z * 0.5f;

        breathScale = 300.0f + 50.0f * Math_SinS(fd2->work[FD2_VAR_TIMER] * 0x2000);
        BossFd2_SpawnFireBreath(play, bossFd->effects, &spawnPos, &spawnVel, &spawnAccel, breathScale, breathOpacity,
                                fd2->actor.shape.rot.y + fd2->headRot.y);

        spawnSpeed.x = 0.0f;
        spawnSpeed.y = 17.0f;
        spawnSpeed.z = 0.0f;

        for (i = 0; i < 6; i++) {
            tempY = Rand_ZeroFloat(2.0f * (f32)M_PI);
            tempX = Rand_ZeroFloat(2.0f * (f32)M_PI);
            Matrix_RotateY(tempY, MTXMODE_NEW);
            Matrix_RotateX(tempX, MTXMODE_APPLY);
            Matrix_MultVec3f(&spawnSpeed, &spawnVel);

            spawnAccel.x = (spawnVel.x * -10.0f) / 100.0f;
            spawnAccel.y = (spawnVel.y * -10.0f) / 100.0f;
            spawnAccel.z = (spawnVel.z * -10.0f) / 100.0f;

            BossFd2_SpawnEmber(play, bossFd->effects, &fd2->headPos, &spawnVel, &spawnAccel,
                               Rand_ZeroFloat(2.0f) + 8.0f);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        BossFd2* fd2 = Typed();

        UpdateAnimation(&fd2->skelAnime, LOCK_CUR_FRAME);

        if ((fd2->collider.base.acFlags & AC_HIT) || (fd2->collider.elements[0].info.bumperFlags & BUMP_HIT)) {
            if (ClaimLeadership(CLAIM_REASON_COOLDOWN))
            {
                UpdateLeader(play);
                return;
            }
        }
        fd2->collider.base.acFlags &= ~AC_HIT;
        if (fd2->deathState == DEATH_START) {
            Collider_UpdateSpheres(0, &fd2->collider);

            u8 roles = COLL_AC | COLL_OC;
            if (!fd2->disableAT)
                roles |= COLL_AT;

            RegisterColliderBase(play, &fd2->collider.base, roles);
        }

        SpawnFireBreath(play);

        BossFd2_UpdateFace(fd2, play);

        if (fd2->actor.focus.pos.y < 90.0f) {
            fd2->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
        } else {
            fd2->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
        }

        fd2->fwork[FD2_TEX1_SCROLL_X] += 4.0f;
        fd2->fwork[FD2_TEX1_SCROLL_Y] = 120.0f;
        fd2->fwork[FD2_TEX2_SCROLL_X] += 3.0f;
    }
};

} // namespace ZeldaOnline

#endif