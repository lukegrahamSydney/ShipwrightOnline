#ifndef CARPENTERCONTROLLERH
#define CARPENTERCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Daiku_Kakariko/z_en_daiku_kakariko.h"

void EnDaikuKakariko_Wait(EnDaikuKakariko* dk, PlayState* play);
void EnDaikuKakariko_Run(EnDaikuKakariko* dk, PlayState* play);
void EnDaikuKakariko_StopRunning(EnDaikuKakariko* dk, PlayState* play);
void EnDaikuKakariko_Talk(EnDaikuKakariko* dk, PlayState* play);

void EnDaikuKakariko_ChangeAnim(EnDaikuKakariko* dk, s32 index, s32* currentIndex);
}

namespace ZeldaOnline {

class CarpenterController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDaikuKakariko* Typed() const {
        return reinterpret_cast<EnDaikuKakariko*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u16 NPCFLAG_WIDE_COLLIDER = 4;
    static constexpr u16 NPCFLAG_TRACK_HEAD = 0x100;
    static constexpr u16 NPCFLAG_TRACK_BODY = 0x200;
    static constexpr u16 NPCFLAG_TRACKING_ON = 0x1000;

    using CarpenterActionFunc = void (*)(EnDaikuKakariko*, PlayState*);
    static const CarpenterActionFunc* ActionTable(size_t* count) {
        static const CarpenterActionFunc sTable[] = {
            EnDaikuKakariko_Wait,
            EnDaikuKakariko_Run,
            EnDaikuKakariko_StopRunning,
            EnDaikuKakariko_Talk,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const CarpenterActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ANIM_INDEX,
        PROP_PATH,
        PROP_TIMER,
        PROP_RUN,
        PROP_TALK_STATE,
        PROP_NPC_FLAGS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnDaikuKakariko* dk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ANIM_INDEX, PackedUInt1((u8)(dk->currentAnimIndex)), out);
        PackProperty(PROP_PATH,
                     ByteStream() << PackedInt2((s16)(dk->waypoint))
                                  << PackedInt2((s16)(dk->pathContinue)),
                     out);
        PackProperty(PROP_TIMER, PackedInt4(dk->timer), out);
        PackProperty(PROP_RUN,
                     ByteStream() << PackedInt4(dk->run) << PackedUInt2(dk->runFlag) << PackedFloat4(dk->runSpeed),
                     out);
        PackProperty(PROP_TALK_STATE, PackedInt4(dk->talkState), out);
        PackProperty(PROP_NPC_FLAGS, PackedUInt2(dk->flags), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDaikuKakariko* dk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const CarpenterActionFunc* table = ActionTable(&count);
                if (id < count)
                    dk->actionFunc = table[id];
                break;
            }
            case PROP_ANIM_INDEX: {
                s32 id = (s32)(data.Read<PackedUInt1>().value());
                if (!m_animIssued || id != dk->currentAnimIndex) {
                    m_animIssued = true;
                    EnDaikuKakariko_ChangeAnim(dk, id, &dk->currentAnimIndex);
                }
                break;
            }
            case PROP_PATH:
                dk->waypoint = (s32)(data.Read<PackedInt2>().value());
                dk->pathContinue = (s32)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER:
                dk->timer = data.Read<PackedInt4>().value();
                break;
            case PROP_RUN:
                dk->run = data.Read<PackedInt4>().value();
                dk->runFlag = (u16)(data.Read<PackedUInt2>().value());
                dk->runSpeed = data.Read<PackedFloat4>().value();
                break;
            case PROP_TALK_STATE:
                dk->talkState = data.Read<PackedInt4>().value();
                break;
            case PROP_NPC_FLAGS:
                dk->flags = (u16)(data.Read<PackedUInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnDaikuKakariko* dk = Typed();

        UpdateAnimation(&dk->skelAnime, LOCK_CUR_FRAME);

        if (dk->actor.xzDistToPlayer < 150.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Collider_UpdateCylinder(&dk->actor, &dk->collider);
        if (dk->flags & NPCFLAG_WIDE_COLLIDER) {
            dk->collider.dim.pos.x -= 27;
            dk->collider.dim.pos.z -= 27;
            dk->collider.dim.radius = 63;
        }
        RegisterColliderBase(play, &dk->collider.base, COLL_OC);

        dk->interactInfo.trackPos = GET_PLAYER(play)->actor.focus.pos;

        if (dk->flags & NPCFLAG_TRACK_HEAD) {
            dk->neckAngleTarget.x = 5900;
            dk->flags |= NPCFLAG_TRACKING_ON;
            Npc_TrackPoint(&dk->actor, &dk->interactInfo, 0, NPC_TRACKING_HEAD_AND_TORSO);
        } else if (dk->flags & NPCFLAG_TRACK_BODY) {
            dk->neckAngleTarget.x = 5900;
            dk->flags |= NPCFLAG_TRACKING_ON;
            Npc_TrackPoint(&dk->actor, &dk->interactInfo, 0, NPC_TRACKING_FULL_BODY);
        }

        Math_SmoothStepToS(&dk->neckAngle.x, dk->neckAngleTarget.x, 1, 1820, 0);
    }

  private:
    bool m_animIssued = false;
};

}

#endif
