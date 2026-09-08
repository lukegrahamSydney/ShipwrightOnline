#ifndef BUBBLECONTROLLERH
#define BUBBLECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bubble/z_en_bubble.h"

void EnBubble_Wait(EnBubble* bb, PlayState* play);
void EnBubble_Pop(EnBubble* bb, PlayState* play);

u32 func_809CBCBC(EnBubble* bb);
}

namespace ZeldaOnline {

class BubbleController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBubble* Typed() const {
        return reinterpret_cast<EnBubble*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using BubbleActionFunc = void (*)(EnBubble*, PlayState*);
    static const BubbleActionFunc* ActionTable(size_t* count) {
        static const BubbleActionFunc sTable[] = {
            EnBubble_Wait,
            EnBubble_Pop,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BubbleActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        return (Typed()->actionFunc == EnBubble_Wait) ? (COLL_AC | COLL_OC) : 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_POP_TIMERS,
        PROP_VISUAL,
        PROP_BOUNCE,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBubble* bb = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(bb->actor.colChkInfo.health), out);
        PackProperty(PROP_POP_TIMERS,
                     ByteStream() << PackedInt2(bb->explosionCountdown) << PackedInt2(bb->unk_208), out);
        PackProperty(PROP_VISUAL, ByteStream() << PackedFloat4(bb->graphicRotSpeed)
                                               << PackedFloat4(bb->graphicEccentricity)
                                               << PackedFloat4(bb->expansionWidth)
                                               << PackedFloat4(bb->expansionHeight), out);
        PackProperty(PROP_BOUNCE, ByteStream() << PackedUInt1(bb->bounceCount)
                                               << PackedFloat4(bb->bounceDirection.x)
                                               << PackedFloat4(bb->bounceDirection.y)
                                               << PackedFloat4(bb->bounceDirection.z)
                                               << PackedFloat4(bb->velocityFromBounce.x)
                                               << PackedFloat4(bb->velocityFromBounce.y)
                                               << PackedFloat4(bb->velocityFromBounce.z)
                                               << PackedFloat4(bb->velocityFromBump.x)
                                               << PackedFloat4(bb->velocityFromBump.y)
                                               << PackedFloat4(bb->velocityFromBump.z)
                                               << PackedFloat4(bb->normalizedBumpVelocity.x)
                                               << PackedFloat4(bb->normalizedBumpVelocity.y)
                                               << PackedFloat4(bb->normalizedBumpVelocity.z)
                                               << PackedFloat4(bb->sinkSpeed), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBubble* bb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BubbleActionFunc* table = ActionTable(&count);
                if (id < count)
                    bb->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                bb->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_POP_TIMERS:
                bb->explosionCountdown = (s16)(data.Read<PackedInt2>().value());
                bb->unk_208 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_VISUAL:
                bb->graphicRotSpeed = data.Read<PackedFloat4>().value();
                bb->graphicEccentricity = data.Read<PackedFloat4>().value();
                bb->expansionWidth = data.Read<PackedFloat4>().value();
                bb->expansionHeight = data.Read<PackedFloat4>().value();
                break;
            case PROP_BOUNCE:
                bb->bounceCount = (u8)(data.Read<PackedUInt1>().value());
                bb->bounceDirection.x = data.Read<PackedFloat4>().value();
                bb->bounceDirection.y = data.Read<PackedFloat4>().value();
                bb->bounceDirection.z = data.Read<PackedFloat4>().value();
                bb->velocityFromBounce.x = data.Read<PackedFloat4>().value();
                bb->velocityFromBounce.y = data.Read<PackedFloat4>().value();
                bb->velocityFromBounce.z = data.Read<PackedFloat4>().value();
                bb->velocityFromBump.x = data.Read<PackedFloat4>().value();
                bb->velocityFromBump.y = data.Read<PackedFloat4>().value();
                bb->velocityFromBump.z = data.Read<PackedFloat4>().value();
                bb->normalizedBumpVelocity.x = data.Read<PackedFloat4>().value();
                bb->normalizedBumpVelocity.y = data.Read<PackedFloat4>().value();
                bb->normalizedBumpVelocity.z = data.Read<PackedFloat4>().value();
                bb->sinkSpeed = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default: return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnBubble* bb = Typed();
        if (bb->actionFunc == EnBubble_Pop) {
            bb->explosionCountdown = (s16)(func_809CBCBC(bb));
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnBubble* bb = Typed();

        if ((bb->colliderSphere.base.acFlags & AC_HIT) || (bb->colliderSphere.base.ocFlags2 & OC2_HIT_PLAYER)) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        if (bb->actionFunc != EnBubble_Pop && bb->actor.colChkInfo.health > 0 && bb->actor.xzDistToPlayer < 300.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        bb->actor.shape.yOffset = (bb->expansionHeight + 1.0f) * 16.0f;
        Actor_SetFocus(&bb->actor, bb->actor.shape.yOffset);

        if (m_roles != 0)
            RegisterColliderBase(play, &bb->colliderSphere.base, m_roles);
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
