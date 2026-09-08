#ifndef MORIELEVATORCONTROLLERH
#define MORIELEVATORCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Mori_Elevator/z_bg_mori_elevator.h"

void BgMoriElevator_WaitAfterInit(BgMoriElevator* el, PlayState* play);
void BgMoriElevator_SetPosition(BgMoriElevator* el, PlayState* play);
void BgMoriElevator_MoveIntoGround(BgMoriElevator* el, PlayState* play);
void BgMoriElevator_MoveAboveGround(BgMoriElevator* el, PlayState* play);

void BgMoriElevator_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class MoriElevatorController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgMoriElevator* Typed() const {
        return reinterpret_cast<BgMoriElevator*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using ElActionFunc = void (*)(BgMoriElevator*, PlayState*);
    static const ElActionFunc* ActionTable(size_t* count) {
        static const ElActionFunc sTable[] = {
            BgMoriElevator_WaitAfterInit,
            BgMoriElevator_SetPosition,
            BgMoriElevator_MoveIntoGround,
            BgMoriElevator_MoveAboveGround,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ElActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsMoving() const {
        BgMoriElevator* el = Typed();
        return el->actionFunc == BgMoriElevator_MoveIntoGround ||
               el->actionFunc == BgMoriElevator_MoveAboveGround;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgMoriElevator* el = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedFloat4(el->targetY) << PackedInt4(el->unk_16C)
                                  << PackedUInt1(el->unk_170) << PackedInt2(el->unk_172),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgMoriElevator* el = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const ElActionFunc* table = ActionTable(&count);
                if (id < count)
                    el->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                el->targetY = data.Read<PackedFloat4>().value();
                el->unk_16C = data.Read<PackedInt4>().value();
                el->unk_170 = (u8)(data.Read<PackedUInt1>().value());
                el->unk_172 = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgMoriElevator* el = Typed();

        // WaitAfterInit installs the draw once the texture object lands.
        if (el->dyna.actor.draw == nullptr && el->moriTexObjIndex >= 0 &&
            Object_IsLoaded(&play->objectCtx, el->moriTexObjIndex)) {
            el->dyna.actor.draw = BgMoriElevator_Draw;
        }

        if (!IsMoving() && el->dyna.actor.xzDistToPlayer >= 40.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
    }
};

}

#endif