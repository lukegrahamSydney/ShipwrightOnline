#ifndef BOMBCONTROLLERH
#define BOMBCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bom/z_en_bom.h"

void EnBom_Move(EnBom* bom, PlayState* play);
void EnBom_WaitForRelease(EnBom* bom, PlayState* play);
void EnBom_Explode(EnBom* bom, PlayState* play);
void EnBom_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class BombController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBom* Typed() const {
        return reinterpret_cast<EnBom*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_EXPLODE = 2;

    using BomActionFunc = void (*)(EnBom*, PlayState*);
    static const BomActionFunc* ActionTable(size_t* count) {
        static const BomActionFunc sTable[] = {
            EnBom_Move,
            EnBom_WaitForRelease,
            EnBom_Explode,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BomActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool HitWouldReact() const {
        EnBom* bom = Typed();
        if (bom->actor.params != BOMB_BODY)
            return false;
        if (bom->bombCollider.base.acFlags & AC_HIT)
            return true;
        return (bom->bombCollider.base.ocFlags1 & OC1_HIT) && bom->bombCollider.base.oc != nullptr &&
               bom->bombCollider.base.oc->category == ACTORCAT_ENEMY;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
        PROP_PARAMS,
        PROP_BUMP_ON,
        PROP_FLASH_SPEED,
        PROP_WORLD_ROT_Y,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBom* bom = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(bom->timer), out);
        PackProperty(PROP_PARAMS, PackedInt2(bom->actor.params), out);
        PackProperty(PROP_BUMP_ON, PackedUInt1(bom->bumpOn), out);
        PackProperty(PROP_FLASH_SPEED, PackedInt2(bom->flashSpeedScale), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBom* bom = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;

                size_t count;
                const BomActionFunc* table = ActionTable(&count);
                if (id < count)
                    bom->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                bom->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_PARAMS:
                bom->actor.params = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BUMP_ON:
                bom->bumpOn = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FLASH_SPEED:
                bom->flashSpeedScale = (s16)(data.Read<PackedInt2>().value());
                break;

            default:
                return false;
        }
        return true;
    }

    //Hide it on creation for puppets (its positioned wrong, take a while to get the update)
    void OnActorInit() override {
        if (!IsRunningLocally() && !IsCreator()) {
            Typed()->actor.draw = nullptr;
        }
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
        if (Typed()->actionFunc == EnBom_Explode && !IsRunningLocally()) {
            SendUpdate();
            GoLocal();
        }
    }

    void OnPropertiesApplied(u64 changed) override {
        if (m_currentActionIndex == ID_EXPLODE && !IsRunningLocally())
            GoLocal();
        if (Typed()->actor.draw == nullptr) {
            Typed()->actor.draw = EnBom_Draw;
        }
    }

    static void DummyAction(EnBom* bom, PlayState* play) {
    }

    void UpdatePuppet(PlayState* play) override {
        EnBom* bom = Typed();

        if (bom->actor.parent == &GET_PLAYER(play)->actor && !IsRunningLocally()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        bom->bombCollider.base.acFlags &= ~AC_HIT;
        bom->bombCollider.base.ocFlags1 &= ~OC1_HIT;

        if (bom->actor.parent != nullptr) {
            bom->actor.room = -1;
        } else if (bom->timer >= 4 && (bom->actor.bgCheckFlags & 1)) {
            Actor_OfferCarry(&bom->actor, play);
        }

        EnBomActionFunc saved = bom->actionFunc;
        bom->actionFunc = DummyAction;
        m_originalUpdate(m_actor, play);
        if (bom->actionFunc == DummyAction) {
            bom->actionFunc = saved;
        }
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

}

#endif
