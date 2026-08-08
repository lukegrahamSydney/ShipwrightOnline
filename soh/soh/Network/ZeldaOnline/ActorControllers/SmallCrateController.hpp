#ifndef SMALLCRATECONTROLLERH
#define SMALLCRATECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Kibako/z_obj_kibako.h"

void ObjKibako_Idle(ObjKibako* box, PlayState* play);
void ObjKibako_Held(ObjKibako* box, PlayState* play);
void ObjKibako_Thrown(ObjKibako* box, PlayState* play);

void ObjKibako_AirBreak(ObjKibako* box, PlayState* play);
void ObjKibako_WaterBreak(ObjKibako* box, PlayState* play);
}

namespace ZeldaOnline {

class SmallCrateController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    ObjKibako* Typed() const {
        return reinterpret_cast<ObjKibako*>(m_actor);
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_IDLE = 0;

    using KibakoActionFunc = void (*)(ObjKibako*, PlayState*);
    static const KibakoActionFunc* ActionTable(size_t* count) {
        static const KibakoActionFunc sTable[] = {
            ObjKibako_Idle,
            ObjKibako_Held,
            ObjKibako_Thrown,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const KibakoActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_MASS,
        PROP_OC_ARMED,
    };

    void BuildCustomProperties(ByteStream& out) override {
        ObjKibako* box = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_MASS, PackedUInt1(box->actor.colChkInfo.mass), out);
        PackProperty(PROP_OC_ARMED, PackedUInt1((box->collider.base.ocFlags1 & OC1_TYPE_PLAYER) ? 1 : 0), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjKibako* box = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const KibakoActionFunc* table = ActionTable(&count);
                if (id < count)
                    box->actionFunc = table[id];
                break;
            }
            case PROP_MASS:
                box->actor.colChkInfo.mass = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_OC_ARMED: {
                u8 armed = (u8)(data.Read<PackedUInt1>().value());
                if (armed)
                    box->collider.base.ocFlags1 |= OC1_TYPE_PLAYER;
                else
                    box->collider.base.ocFlags1 &= ~OC1_TYPE_PLAYER;
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
        if (Typed()->actor.update == nullptr)
            m_brokeLocally = true;
    }

    void OnServerDestroy() override {
        ObjKibako* box = Typed();
        if (m_brokeLocally || gPlayState == nullptr || box == nullptr)
            return;

        if ((box->actor.bgCheckFlags & 0x40) || ((box->actor.bgCheckFlags & 0x20) && box->actor.yDistToWater > 19.0f))
            ObjKibako_WaterBreak(box, gPlayState);
        else
            ObjKibako_AirBreak(box, gPlayState);

        SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &box->actor.world.pos, 20, NA_SE_EV_WOODBOX_BREAK);
    }

    void UpdatePuppet(PlayState* play) override {
        ObjKibako* box = Typed();

        if (box->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }

        if (box->actor.parent == &GET_PLAYER(play)->actor) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        box->collider.base.atFlags &= ~AT_HIT;

        if (m_currentActionIndex == ID_IDLE) {
            if (!(box->collider.base.ocFlags1 & OC1_TYPE_PLAYER) && box->actor.xzDistToPlayer > 28.0f)
                box->collider.base.ocFlags1 |= OC1_TYPE_PLAYER;

            Actor_MoveXZGravity(&box->actor);
            Actor_UpdateBgCheckInfo(play, &box->actor, 19.0f, 20.0f, 0.0f, 5);
            if (box->actor.xzDistToPlayer < 600.0f) {
                Collider_UpdateCylinder(&box->actor, &box->collider);
                RegisterColliderBase(play, &box->collider.base, COLL_AC);
                if (box->actor.xzDistToPlayer < 180.0f)
                    RegisterColliderBase(play, &box->collider.base, COLL_OC);
            }

            if (box->actor.xzDistToPlayer < 100.0f)
                Actor_OfferCarry(&box->actor, play);
        }
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    bool m_brokeLocally = false;
};

}

#endif
