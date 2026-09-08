#ifndef VOLVAGIACONTROLLERH
#define VOLVAGIACONTROLLERH

#include "../AbstractBossController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Fd/z_boss_fd.h"
#include "src/overlays/actors/ovl_Boss_Fd2/z_boss_fd2.h"
#include "assets/textures/boss_title_cards/object_fd.h"
void BossFd_SetupFly(BossFd* fd, PlayState* play);
void BossFd_Fly(BossFd* fd, PlayState* play);
void BossFd_Wait(BossFd* fd, PlayState* play);

void BossFd_UpdateEffects(BossFd* fd, PlayState* play);
void BossFd_Effects(BossFd* fd, PlayState* play);

u16 func_800FA0B4(u8 seqPlayerIndex);
void func_80064534(PlayState* play, CutsceneContext* csCtx);
}

namespace ZeldaOnline {

class VolvagiaController : public AbstractBossController {
  public:
    using AbstractBossController::AbstractBossController;

    BossFd* Typed() const {
        return reinterpret_cast<BossFd*>(m_actor);
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {

        return actorId != ACTOR_ITEM_B_HEART && actorId != ACTOR_DOOR_WARP1;
    }

    void PrepareForDeath() {
        auto fd = Typed();

        fd->handoffSignal = FD2_SIGNAL_NONE;
        fd->actor.colChkInfo.health = 0;
        EndCutsceneCamera();
        GoLocal();
    }


  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using FdActionFunc = decltype(&BossFd_Fly);
    static const FdActionFunc* ActionTable(size_t* count) {
        static const FdActionFunc sTable[] = {
            BossFd_Fly,
            BossFd_Wait,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }


    const char* GetTitleCard() const override {
        return gVolvagiaBossTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return &Typed()->introCamera;
    }

    Vec3f_* GetCameraAt() override {
        return &Typed()->camData.at;
    }

    Vec3f_* GetCameraEye() override {
        return &Typed()->camData.eye;
    }


    u8 CurrentActionIndex() const {
        size_t count;
        const FdActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_INTRO,
        PROP_WORK,
        PROP_TIMERS,
        PROP_FWORK,
        PROP_STATE,
        PROP_TARGET,
        PROP_HOLE,
        PROP_HEAD_POS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BossFd* fd = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        ByteStream work;
        for (s32 i = 0; i < BFD_SHORT_COUNT; i++)
            work << PackedInt2(fd->work[i]);
        PackProperty(PROP_WORK, work, out);

        ByteStream timers;
        for (s32 i = 0; i < 6; i++)
            timers << PackedInt2(fd->timers[i]);
        PackProperty(PROP_TIMERS, timers, out);

        ByteStream fwork;
        for (s32 i = 0; i < BFD_FLOAT_COUNT; i++)
            fwork << PackedFloat4(fd->fwork[i]);
        PackProperty(PROP_FWORK, fwork, out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(fd->handoffSignal) << PackedInt2(fd->fireBreathTimer)
                                  << PackedInt2(fd->skinSegments) << PackedUInt1(fd->fogMode)
                                  << PackedUInt1(fd->holeIndex) << PackedUInt1(fd->eyeState)
                                  << PackedUInt1(fd->platformSignal) << PackedFloat4(fd->flattenMane)
                                  << PackedFloat4(fd->jawOpening),
                     out);


        PackProperty(PROP_TARGET,
                     ByteStream() << PackedFloat4(fd->targetPosition.x) << PackedFloat4(fd->targetPosition.y)
                                  << PackedFloat4(fd->targetPosition.z),
                     out);
        PackProperty(PROP_HOLE,
                     ByteStream() << PackedFloat4(fd->holePosition.x) << PackedFloat4(fd->holePosition.y)
                                  << PackedFloat4(fd->holePosition.z),
                     out);
        PackProperty(PROP_HEAD_POS,
                     ByteStream() << PackedFloat4(fd->headPos.x) << PackedFloat4(fd->headPos.y)
                                  << PackedFloat4(fd->headPos.z),
                     out);

        PackProperty(PROP_INTRO, ByteStream() << PackedInt2(fd->introFlyState) << PackedInt2(fd->introState), out);

        BuildBossProperty(PROP_BOSS_CAMERA, out);
        BuildBossProperty(PROP_BOSS_BGM, out);
        BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
        BuildBossProperty(PROP_BOSS_LIGHTING, out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossFd* fd = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const FdActionFunc* table = ActionTable(&count);
                if (id < count)
                    fd->actionFunc = table[id];
                break;
            }
            case PROP_WORK:
                for (s32 i = 0; i < BFD_SHORT_COUNT; i++) {
                    s16 value = (s16)(data.Read<PackedInt2>().value());

                    if (i == BFD_LEAD_BODY_SEG || i == BFD_LEAD_MANE_SEG)
                        continue;

                    fd->work[i] = value;
                }
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 6; i++)
                    fd->timers[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FWORK:
                for (s32 i = 0; i < BFD_FLOAT_COUNT; i++)
                    fd->fwork[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_STATE:
                fd->handoffSignal = (u8)(data.Read<PackedUInt1>().value());
                fd->fireBreathTimer = (s16)(data.Read<PackedInt2>().value());
                fd->skinSegments = (s16)(data.Read<PackedInt2>().value());
                fd->fogMode = (u8)(data.Read<PackedUInt1>().value());
                fd->holeIndex = (u8)(data.Read<PackedUInt1>().value());
                fd->eyeState = (u8)(data.Read<PackedUInt1>().value());
                fd->platformSignal = (u8)(data.Read<PackedUInt1>().value());
                fd->flattenMane = data.Read<PackedFloat4>().value();
                fd->jawOpening = data.Read<PackedFloat4>().value();
                break;
 

            case PROP_TARGET:
                fd->targetPosition.x = data.Read<PackedFloat4>().value();
                fd->targetPosition.y = data.Read<PackedFloat4>().value();
                fd->targetPosition.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_HOLE:
                fd->holePosition.x = data.Read<PackedFloat4>().value();
                fd->holePosition.y = data.Read<PackedFloat4>().value();
                fd->holePosition.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_HEAD_POS:
                fd->headPos.x = data.Read<PackedFloat4>().value();
                fd->headPos.y = data.Read<PackedFloat4>().value();
                fd->headPos.z = data.Read<PackedFloat4>().value();
                break;

            case PROP_INTRO:
                fd->introFlyState = (s16)(data.Read<PackedInt2>().value());
                fd->introState = (s16)(data.Read<PackedInt2>().value());
                break;

            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }



    void OnTrigger(const std::string& name, ByteStream& data) override {
        if (name != "fdhit")
            return;

        m_pendingDmgFlags = data.Read<PackedUInt4>().value();
        m_pendingHit = true;
    }

    void UpdateLeader(PlayState* play) override {
        BossFd* fd = Typed();

        if (m_pendingHit) {
            m_pendingHit = false;
            m_fakeHitInfo.toucher.dmgFlags = m_pendingDmgFlags;
            fd->collider.elements[0].info.acHitInfo = &m_fakeHitInfo;
            fd->collider.base.acFlags |= AC_HIT;
        }

        AbstractActorController::UpdateLeader(play);

        
    }

    void UpdatePuppet(PlayState* play) override {
        BossFd* fd = Typed();

        if (fd->actionFunc == BossFd_Fly) {
            if (!m_trailSeeded) {
                for (s32 i = 0; i < 100; i++) {
                    fd->bodySegsPos[i] = fd->actor.world.pos;
                    fd->bodySegsRot[i].x = (fd->actor.world.rot.x / (f32)0x8000) * (f32)M_PI;
                    fd->bodySegsRot[i].y = (fd->actor.world.rot.y / (f32)0x8000) * (f32)M_PI;
                    fd->bodySegsRot[i].z = (fd->actor.world.rot.z / (f32)0x8000) * (f32)M_PI;
                }
                for (s32 i = 0; i < 30; i++) {
                    fd->centerMane.pos[i] = fd->centerMane.head;
                    fd->rightMane.pos[i] = fd->rightMane.head;
                    fd->leftMane.pos[i] = fd->leftMane.head;
                }
                m_trailSeeded = true;
            }

            fd->work[BFD_LEAD_BODY_SEG]++;
            if (fd->work[BFD_LEAD_BODY_SEG] >= 100)
                fd->work[BFD_LEAD_BODY_SEG] = 0;

            s16 lead = fd->work[BFD_LEAD_BODY_SEG];
            fd->bodySegsPos[lead] = fd->actor.world.pos;
            fd->bodySegsRot[lead].x = (fd->actor.world.rot.x / (f32)0x8000) * (f32)M_PI;
            fd->bodySegsRot[lead].y = (fd->actor.world.rot.y / (f32)0x8000) * (f32)M_PI;
            fd->bodySegsRot[lead].z = (fd->actor.world.rot.z / (f32)0x8000) * (f32)M_PI;

            fd->work[BFD_LEAD_MANE_SEG]++;
            if (fd->work[BFD_LEAD_MANE_SEG] >= 30)
                fd->work[BFD_LEAD_MANE_SEG] = 0;

            s16 maneLead = fd->work[BFD_LEAD_MANE_SEG];

            fd->centerMane.scale[maneLead] = (Math_SinS(fd->work[BFD_MOVE_TIMER] * 5596) * 0.3f) + 1.0f;
            fd->rightMane.scale[maneLead] = (Math_SinS(fd->work[BFD_MOVE_TIMER] * 5496) * 0.3f) + 1.0f;
            fd->leftMane.scale[maneLead] = (Math_CosS(fd->work[BFD_MOVE_TIMER] * 5696) * 0.3f) + 1.0f;

            fd->centerMane.pos[maneLead] = fd->centerMane.head;
            fd->rightMane.pos[maneLead] = fd->rightMane.head;
            fd->leftMane.pos[maneLead] = fd->leftMane.head;

            fd->fireManeRot[maneLead].x = (fd->actor.world.rot.x / (f32)0x8000) * (f32)M_PI;
            fd->fireManeRot[maneLead].y = (fd->actor.world.rot.y / (f32)0x8000) * (f32)M_PI;
            fd->fireManeRot[maneLead].z = (fd->actor.world.rot.z / (f32)0x8000) * (f32)M_PI;
        } else {
            m_trailSeeded = false;
        }

        if (fd->fireBreathTimer != 0)
            fd->fireBreathTimer--;

        BossFd_Effects(fd, play);
        BossFd_UpdateEffects(fd, play);

        fd->fwork[BFD_TEX1_SCROLL_X] += 4.0f;
        fd->fwork[BFD_TEX1_SCROLL_Y] = 120.0f;
        fd->fwork[BFD_TEX2_SCROLL_X] += 3.0f;
        fd->fwork[BFD_TEX2_SCROLL_Y] -= 2.0f;

        if (fd->collider.base.acFlags & AC_HIT) {
            fd->collider.base.acFlags &= ~AC_HIT;

            ColliderInfo* hurtbox = fd->collider.elements[0].info.acHitInfo;
            SendTriggerToLeader("fdhit", ByteStream()
                                             << PackedUInt4(hurtbox != nullptr ? hurtbox->toucher.dmgFlags : 0u));
        }

        if (fd->work[BFD_ACTION_STATE] < BOSSFD_DEATH_START) {
            RegisterColliderBase(play, &fd->collider.base, COLL_AC | COLL_AT);
        }
    }

  private:
    bool m_pendingHit = false;
    u32 m_pendingDmgFlags = 0;
    ColliderInfo m_fakeHitInfo{};
    bool m_trailSeeded = false;
};

} // namespace ZeldaOnline

#endif