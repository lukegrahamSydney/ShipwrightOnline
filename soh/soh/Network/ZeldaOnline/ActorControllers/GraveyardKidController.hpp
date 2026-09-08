#ifndef GRAVEYARDKIDCONTROLLERH
#define GRAVEYARDKIDCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Cs/z_en_cs.h"

void EnCs_Walk(EnCs* cs, PlayState* play);
void EnCs_Talk(EnCs* cs, PlayState* play);
void EnCs_Wait(EnCs* cs, PlayState* play);

void EnCs_HandleTalking(EnCs* cs, PlayState* play);
void EnCs_ChangeAnim(EnCs* cs, s32 index, s32* currentIndex);
}

namespace ZeldaOnline {

class GraveyardKidController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnCs* Typed() const {
        return reinterpret_cast<EnCs*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using CsActionFunc = void (*)(EnCs*, PlayState*);
    static const CsActionFunc* ActionTable(size_t* count) {
        static const CsActionFunc sTable[] = {
            EnCs_Walk,
            EnCs_Talk,
            EnCs_Wait,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }


    u8 CurrentActionIndex() const {
        size_t count;
        const CsActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsTalkingLocally() const {
        return Typed()->talkState != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WALK,
        PROP_WAYPOINT,
        PROP_ANIM_INDEX,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnCs* cs = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_WALK,
                     ByteStream() << PackedFloat4(cs->walkAngle) << PackedFloat4(cs->walkDist)
                                  << PackedFloat4(cs->walkSpeed),
                     out);
        PackProperty(PROP_WAYPOINT,
                     ByteStream() << PackedInt4(cs->waypoint) << PackedInt4(cs->animLoopCount) << PackedInt4(cs->path),
                     out);
        PackProperty(PROP_ANIM_INDEX, PackedUInt1((u8)(cs->currentAnimIndex)), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnCs* cs = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const CsActionFunc* table = ActionTable(&count);
                if (id < count)
                    cs->actionFunc = table[id];
                break;
            }
            case PROP_WALK:
                cs->walkAngle = data.Read<PackedFloat4>().value();
                cs->walkDist = data.Read<PackedFloat4>().value();
                cs->walkSpeed = data.Read<PackedFloat4>().value();
                break;
            case PROP_WAYPOINT:
                cs->waypoint = data.Read<PackedInt4>().value();
                cs->animLoopCount = data.Read<PackedInt4>().value();
                cs->path = data.Read<PackedInt4>().value();
                break;
            case PROP_ANIM_INDEX: {
                s32 id = (s32)(data.Read<PackedUInt1>().value());
                if (id != cs->currentAnimIndex) {
                    EnCs_ChangeAnim(cs, id, &cs->currentAnimIndex);
                }
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnCs* cs = Typed();

        if(UpdateAnimation(&cs->skelAnime, LOCK_CUR_FRAME))
            EnCs_ChangeAnim(cs, cs->currentAnimIndex, &cs->currentAnimIndex);

        Collider_UpdateCylinder(&cs->actor, &cs->collider);
        RegisterColliderBase(play, &cs->collider.base, COLL_OC);

        if (GET_PLAYER(play)->talkActor == &cs->actor) {
            if (!m_talkClaimed) {
                m_talkClaimed = true;
                ClaimLeadership(CLAIM_REASON_NOW);
            }
        } else {
            m_talkClaimed = false;
        }

        EnCs_HandleTalking(cs, play);
        m_conversationHandled = true;

        cs->eyeBlinkTimer--;
        if (cs->eyeBlinkTimer < 0) {
            static const s32 eyeBlinkFrames[] = { 70, 1, 1 };
            cs->eyeIndex++;
            if (cs->eyeIndex >= 3) {
                cs->eyeIndex = 0;
            }
            cs->eyeBlinkTimer = eyeBlinkFrames[cs->eyeIndex];
        }
    }

  private:
    bool m_talkClaimed = false;
};

} // namespace ZeldaOnline

#endif