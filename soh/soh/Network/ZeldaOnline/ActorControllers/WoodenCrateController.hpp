#ifndef WOODENCRATECONTROLLERH
#define WOODENCRATECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Kibako2/z_obj_kibako2.h"

void ObjKibako2_Idle(ObjKibako2* box, PlayState* play);
void ObjKibako2_Kill(ObjKibako2* box, PlayState* play);

void ObjKibako2_Break(ObjKibako2* box, PlayState* play);
}

namespace ZeldaOnline {

class WoodenCrateController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    ObjKibako2* Typed() const {
        return reinterpret_cast<ObjKibako2*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using CrateActionFunc = void (*)(ObjKibako2*, PlayState*);
    static const CrateActionFunc* ActionTable(size_t* count) {
        static const CrateActionFunc sTable[] = {
            ObjKibako2_Idle,
            ObjKibako2_Kill,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const CrateActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool BreakWouldFire(PlayState* play) const {
        ObjKibako2* box = Typed();
        if (box->collider.base.acFlags & AC_HIT)
            return true;
        if (box->dyna.actor.home.rot.z != 0)
            return true;
        return func_80033684(play, &box->dyna.actor) != nullptr;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_PARAMS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        ObjKibako2* box = Typed();
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_PARAMS, PackedInt2(box->dyna.actor.params), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjKibako2* box = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const CrateActionFunc* table = ActionTable(&count);
                if (id < count)
                    box->actionFunc = table[id];
                break;
            }
            case PROP_PARAMS:
                box->dyna.actor.params = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        if (gPlayState == nullptr || Typed() == nullptr)
            return;
        ObjKibako2_Break(Typed(), gPlayState);
    }

    void UpdatePuppet(PlayState* play) override {
        ObjKibako2* box = Typed();

        if (BreakWouldFire(play)) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        box->collider.base.acFlags &= ~AC_HIT;

        if (box->dyna.actor.xzDistToPlayer < 200.0f) {
            Collider_UpdateCylinder(&box->dyna.actor, &box->collider);
            RegisterColliderBase(play, &box->collider.base, COLL_AC);
        }
    }
};

}

#endif
