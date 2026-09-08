#ifndef HIDANFSLIFTCONTROLLERH
#define HIDANFSLIFTCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Fslift/z_bg_hidan_fslift.h"

void BgHidanFslift_Idle(BgHidanFslift* lift, PlayState* play);
void BgHidanFslift_Descend(BgHidanFslift* lift, PlayState* play);
void BgHidanFslift_Ascend(BgHidanFslift* lift, PlayState* play);

void BgHidanFslift_SetHookshotTargetPos(BgHidanFslift* lift);
}

namespace ZeldaOnline {

class HidanFsliftController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanFslift* Typed() const {
        return reinterpret_cast<BgHidanFslift*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using LiftActionFunc = void (*)(BgHidanFslift*, PlayState*);
    static const LiftActionFunc* ActionTable(size_t* count) {
        static const LiftActionFunc sTable[] = {
            BgHidanFslift_Idle,
            BgHidanFslift_Descend,
            BgHidanFslift_Ascend,
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

    bool IsMoving() const {
        BgHidanFslift* lift = Typed();
        return lift->actionFunc == BgHidanFslift_Ascend || lift->actionFunc == BgHidanFslift_Descend;
    }


    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHidanFslift* lift = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(lift->timer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanFslift* lift = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const LiftActionFunc* table = ActionTable(&count);
                if (id < count)
                    lift->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                lift->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanFslift* lift = Typed();

        BgHidanFslift_SetHookshotTargetPos(lift);

        if (DynaPolyActor_IsPlayerOnTop(&lift->dyna)) {
            if (lift->cameraSetting == 0)
                lift->cameraSetting = 3;
            Camera_ChangeSetting(play->cameraPtrs[MAIN_CAM], CAM_SET_FIRE_PLATFORM);
        } else {
            if (lift->cameraSetting != 0)
                Camera_ChangeSetting(play->cameraPtrs[MAIN_CAM], CAM_SET_DUNGEON0);
            lift->cameraSetting = 0;
        }

        if (!IsMoving() && DynaPolyActor_IsPlayerAbove(&lift->dyna))
            ClaimLeadership(CLAIM_REASON_NOW);
    }
};

}

#endif
