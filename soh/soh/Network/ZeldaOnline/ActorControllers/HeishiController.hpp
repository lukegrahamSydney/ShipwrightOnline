#ifndef HEISHICONTROLLERH
#define HEISHICONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Heishi1/z_en_heishi1.h"
#include "objects/object_sd/object_sd.h"

void EnHeishi1_SetupWait(EnHeishi1* hs, PlayState* play);
void EnHeishi1_Wait(EnHeishi1* hs, PlayState* play);
void EnHeishi1_SetupWalk(EnHeishi1* hs, PlayState* play);
void EnHeishi1_Walk(EnHeishi1* hs, PlayState* play);
void EnHeishi1_SetupMoveToLink(EnHeishi1* hs, PlayState* play);
void EnHeishi1_MoveToLink(EnHeishi1* hs, PlayState* play);
void EnHeishi1_SetupTurnTowardLink(EnHeishi1* hs, PlayState* play);
void EnHeishi1_TurnTowardLink(EnHeishi1* hs, PlayState* play);
void EnHeishi1_SetupKick(EnHeishi1* hs, PlayState* play);
void EnHeishi1_Kick(EnHeishi1* hs, PlayState* play);
void EnHeishi1_SetupWaitNight(EnHeishi1* hs, PlayState* play);
void EnHeishi1_WaitNight(EnHeishi1* hs, PlayState* play);
}

namespace ZeldaOnline {

class HeishiController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnHeishi1* Typed() const {
        return reinterpret_cast<EnHeishi1*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_SETUP_WALK = 2;

    using HeishiActionFunc = void (*)(EnHeishi1*, PlayState*);
    static const HeishiActionFunc* ActionTable(size_t* count) {
        static const HeishiActionFunc sTable[] = {
            EnHeishi1_SetupWait,
            EnHeishi1_Wait,
            EnHeishi1_SetupWalk,
            EnHeishi1_Walk,
            EnHeishi1_SetupMoveToLink,
            EnHeishi1_MoveToLink,
            EnHeishi1_SetupTurnTowardLink,
            EnHeishi1_TurnTowardLink,
            EnHeishi1_SetupKick,
            EnHeishi1_Kick,
            EnHeishi1_SetupWaitNight,
            EnHeishi1_WaitNight,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HeishiActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsCatching() const {
        EnHeishi1* hs = Typed();
        return hs->actionFunc == EnHeishi1_SetupMoveToLink || hs->actionFunc == EnHeishi1_MoveToLink ||
               hs->actionFunc == EnHeishi1_SetupTurnTowardLink || hs->actionFunc == EnHeishi1_TurnTowardLink ||
               hs->actionFunc == EnHeishi1_SetupKick || hs->actionFunc == EnHeishi1_Kick;
    }

    bool IsPatrolling() const {
        EnHeishi1* hs = Typed();
        return hs->actionFunc == EnHeishi1_Walk || hs->actionFunc == EnHeishi1_Wait;
    }

    static constexpr u8 ANIM_IDLE = 0;
    static constexpr u8 ANIM_WALK = 1;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 i) {
        return (i == ANIM_WALK) ? gEnHeishiWalkAnim : gEnHeishiIdleAnim;
    }
    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        return (strcmp(cur, gEnHeishiWalkAnim) == 0) ? ANIM_WALK : ANIM_IDLE;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WAYPOINT,
        PROP_TIMERS,
        PROP_HEAD,
        PROP_SPEEDS,
        PROP_ANIM_PARAMS,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnHeishi1* hs = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(IsCatching() ? ID_SETUP_WALK : CurrentActionIndex()), out);
        PackProperty(PROP_WAYPOINT, ByteStream() << PackedInt2(hs->waypoint) << PackedInt2(hs->waypointTimer), out);
        PackProperty(
            PROP_TIMERS,
            ByteStream() << PackedInt2(hs->headTimer) << PackedInt2(hs->waitTimer) << PackedInt2(hs->kickTimer), out);
        PackProperty(PROP_HEAD,
                     ByteStream() << PackedFloat4(hs->headAngle) << PackedFloat4(hs->headAngleTarget)
                                  << PackedInt2(hs->headDirection) << PackedInt2(hs->headBehaviorDecided),
                     out);
        PackProperty(PROP_SPEEDS, ByteStream() << PackedFloat4(hs->moveSpeed) << PackedFloat4(hs->bodyTurnSpeed), out);

        {
            ByteStream params;
            for (int i = 0; i < 8; i++)
                params << PackedFloat4(hs->animParams[i]);
            PackProperty(PROP_ANIM_PARAMS, params, out);
        }

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(hs->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &hs->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnHeishi1* hs = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const HeishiActionFunc* table = ActionTable(&count);
                if (id < count)
                    hs->actionFunc = table[id];
                break;
            }
            case PROP_WAYPOINT:
                hs->waypoint = (s16)(data.Read<PackedInt2>().value());
                hs->waypointTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                hs->headTimer = (s16)(data.Read<PackedInt2>().value());
                hs->waitTimer = (s16)(data.Read<PackedInt2>().value());
                hs->kickTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEAD:
                hs->headAngle = data.Read<PackedFloat4>().value();
                hs->headAngleTarget = data.Read<PackedFloat4>().value();
                hs->headDirection = (s16)(data.Read<PackedInt2>().value());
                hs->headBehaviorDecided = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SPEEDS:
                hs->moveSpeed = data.Read<PackedFloat4>().value();
                hs->bodyTurnSpeed = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_PARAMS:
                for (int i = 0; i < 8; i++)
                    hs->animParams[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_CUR_FRAME:
                hs->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                ApplyAnimProperty((void*)AnimForIndex(id), &hs->skelAnime, LOCK_CUR_FRAME ? hs->skelAnime.curFrame : 0.0f,
                                 data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdateLeader(PlayState* play) override {
        bool wasCalm = !IsCatching();

        AbstractActorController::UpdateLeader(play);

        if (wasCalm && IsCatching()) {
            GoLocal();
        }
    }

    void OnBecomeLeader() override {
        if (IsCatching())
            EnHeishi1_SetupWalk(Typed(), gPlayState);
    }

    void UpdatePuppet(PlayState* play) override {
        EnHeishi1* hs = Typed();

        UpdateAnimation(&hs->skelAnime, LOCK_CUR_FRAME);

        if (!IsCatching() && hs->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        hs->activeTimer++;

        hs->actor.uncullZoneForward = 550.0f;
        hs->actor.uncullZoneScale = 350.0f;
        hs->actor.uncullZoneDownward = 700.0f;

        if (IsPatrolling()) {
            Vec3f ballVel;
            Vec3f ballAccel = { 0.0f, 0.0f, 0.0f };
            Vec3f ballMult = { 0.0f, 0.0f, 30.0f };
            Vec3f ballPos;
            s16 detectedSink = false;

            ballPos.x = hs->actor.world.pos.x;
            ballPos.y = hs->actor.world.pos.y + 60.0f;
            ballPos.z = hs->actor.world.pos.z;

            Matrix_Push();
            Matrix_RotateY(((hs->actor.shape.rot.y + hs->headAngle) / 32768.0f) * 3.14159265f, MTXMODE_NEW);
            Matrix_MultVec3f(&ballMult, &ballVel);
            Matrix_Pop();

            EffectSsSolderSrchBall_Spawn(play, &ballPos, &ballVel, &ballAccel, 2, &detectedSink);
        }
    }
};

}

#endif
