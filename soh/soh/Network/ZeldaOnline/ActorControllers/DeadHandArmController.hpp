#ifndef DEADHANDARMCONTROLLERH
#define DEADHANDARMCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Dha/z_en_dha.h"
#include "src/overlays/actors/ovl_En_Dh/z_en_dh.h"

void EnDha_Wait(EnDha* dha, PlayState* play);
void EnDha_TakeDamage(EnDha* dha, PlayState* play);
void EnDha_Die(EnDha* dha, PlayState* play);

void EnDha_SetupDeath(EnDha* dha);
}

namespace ZeldaOnline {

class DeadHandArmController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDha* Typed() const {
        return reinterpret_cast<EnDha*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using DhaActionFunc = void (*)(EnDha*, PlayState*);
    static const DhaActionFunc* ActionTable(size_t* count) {
        static const DhaActionFunc sTable[] = {
            EnDha_Wait,
            EnDha_TakeDamage,
            EnDha_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DhaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsHoldingSomeone() const {
        return Typed()->unk_1CC != 0;
    }

    bool AnotherHandIsHolding(PlayState* play) const {
        EnDha* self = Typed();
        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].head; it != nullptr; it = it->next) {
            if (it->id != ACTOR_EN_DHA || it == &self->actor)
                continue;
            if (it->parent != self->actor.parent)
                continue;
            if (reinterpret_cast<EnDha*>(it)->unk_1CC != 0)
                return true;
        }
        return false;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_GRAB,
        PROP_TIMERS,
        PROP_LIMBS,
        PROP_HEALTH,
        PROP_HOME_ROT_Z,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnDha* dha = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_GRAB, ByteStream() << PackedUInt1(dha->unk_1CC) << PackedUInt1(dha->unk_1C0), out);
        PackProperty(PROP_TIMERS, ByteStream() << PackedInt2(dha->actionTimer) << PackedInt2(dha->timer), out);
        PackProperty(PROP_LIMBS,
                     ByteStream() << PackedInt2(dha->limbAngleX[0]) << PackedInt2(dha->limbAngleX[1])
                                  << PackedInt2(dha->limbAngleY) << PackedInt2(dha->handAngle.x)
                                  << PackedInt2(dha->handAngle.y) << PackedInt2(dha->handAngle.z),
                     out);
        PackProperty(PROP_HEALTH, PackedUInt1(dha->actor.colChkInfo.health), out);
        PackProperty(PROP_HOME_ROT_Z, PackedInt2(dha->actor.home.rot.z), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDha* dha = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const DhaActionFunc* table = ActionTable(&count);
                if (id < count)
                    dha->actionFunc = table[id];
                break;
            }
            case PROP_GRAB:
                dha->unk_1CC = (u8)(data.Read<PackedUInt1>().value());
                dha->unk_1C0 = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                dha->actionTimer = (s16)(data.Read<PackedInt2>().value());
                dha->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_LIMBS:
                dha->limbAngleX[0] = (s16)(data.Read<PackedInt2>().value());
                dha->limbAngleX[1] = (s16)(data.Read<PackedInt2>().value());
                dha->limbAngleY = (s16)(data.Read<PackedInt2>().value());
                dha->handAngle.x = (s16)(data.Read<PackedInt2>().value());
                dha->handAngle.y = (s16)(data.Read<PackedInt2>().value());
                dha->handAngle.z = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                dha->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HOME_ROT_Z:
                dha->actor.home.rot.z = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnBecomeLeader() override {
        EnDha* dha = Typed();
        if (dha->unk_1CC == 0) {
            return;
        }
        dha->unk_1CC = 0;
        dha->timer = 0;
        dha->limbAngleY = 0;
    }

    void UpdateLeader(PlayState* play) override {
        EnDha* dha = Typed();

        AbstractActorController::UpdateLeader(play);

        bool holding = dha->unk_1CC != 0;
        if (holding && !m_wasHolding && dha->actor.parent != nullptr && dha->actor.parent->zoController != nullptr &&
            !AnotherHandIsHolding(play)) {
            auto* head = static_cast<AbstractActorController*>(dha->actor.parent->zoController);
            if (!head->IsLeader()) {
                head->ClaimLeadership(CLAIM_REASON_HIT);
            }
        }
        m_wasHolding = holding;
    }

    void UpdatePuppet(PlayState* play) override {
        EnDha* dha = Typed();

        if (dha->actor.parent == NULL) {
            dha->actor.parent = Actor_FindNearby(play, &dha->actor, ACTOR_EN_DH, ACTORCAT_ENEMY, 10000.0f);
        }

        UpdateAnimation(&dha->skelAnime, false);

        {
            Player* localPlayer = GET_PLAYER(play);
            if (!dha->unk_1CC && (localPlayer->stateFlags2 & PLAYER_STATE2_GRABBED_BY_ENEMY) &&
                &dha->actor == localPlayer->actor.parent) {
                localPlayer->stateFlags2 &= ~PLAYER_STATE2_GRABBED_BY_ENEMY;
                localPlayer->actor.parent = NULL;
                localPlayer->av2.actionVar2 = 200;
            }
        }

        if (!IsHoldingSomeone()) {
            if (dha->collider.base.acFlags & AC_HIT) {
                ClaimLeadership(CLAIM_REASON_HIT);
                UpdateLeader(play);
                return;
            }

            if (dha->actionFunc != EnDha_Die && dha->actor.xzDistToPlayer < 200.0f && IsLocalPlayerClosest())
                ClaimLeadership(CLAIM_REASON_PROXIMITY);
        }
        dha->collider.base.acFlags &= ~AC_HIT;

        RegisterColliderBase(play, &dha->collider.base, COLL_AC | COLL_OC);
    }

  private:
    bool m_wasHolding = false;
};

}

#endif
