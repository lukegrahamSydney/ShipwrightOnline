#ifndef LIFTCONTROLLERH
#define LIFTCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Lift/z_obj_lift.h"

void ObjLift_Wait(ObjLift* lift, PlayState* play);
void ObjLift_Shake(ObjLift* lift, PlayState* play);
void ObjLift_Fall(ObjLift* lift, PlayState* play);

void ObjLift_SpawnFragments(ObjLift* lift, PlayState* play);
}

namespace ZeldaOnline {

class LiftController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    ObjLift* Typed() const {
        return reinterpret_cast<ObjLift*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_WAIT = 0;

    using LiftActionFunc = void (*)(ObjLift*, PlayState*);
    static const LiftActionFunc* ActionTable(size_t* count) {
        static const LiftActionFunc sTable[] = {
            ObjLift_Wait,
            ObjLift_Shake,
            ObjLift_Fall,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const LiftActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
        PROP_SHAKE_ORIENT,
    };

    void BuildCustomProperties(ByteStream& out) override {
        ObjLift* lift = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(lift->timer), out);
        PackProperty(PROP_SHAKE_ORIENT,
                     ByteStream() << PackedInt2(lift->shakeOrientation.x) << PackedInt2(lift->shakeOrientation.y)
                                  << PackedInt2(lift->shakeOrientation.z),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjLift* lift = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const LiftActionFunc* table = ActionTable(&count);
                if (id < count)
                    lift->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                lift->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SHAKE_ORIENT:
                lift->shakeOrientation.x = (s16)(data.Read<PackedInt2>().value());
                lift->shakeOrientation.y = (s16)(data.Read<PackedInt2>().value());
                lift->shakeOrientation.z = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        ObjLift* lift = Typed();
        if (gPlayState == nullptr || lift == nullptr)
            return;

        ObjLift_SpawnFragments(lift, gPlayState);
        SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &lift->dyna.actor.world.pos, 20, NA_SE_EV_BOX_BREAK);
    }

    void UpdatePuppet(PlayState* play) override {
        ObjLift* lift = Typed();

        if (m_currentActionIndex == ID_WAIT && DynaPolyActor_IsPlayerOnTop(&lift->dyna))
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

}

#endif
