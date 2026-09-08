#ifndef HIDANHROCKCONTROLLERH
#define HIDANHROCKCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Hrock/z_bg_hidan_hrock.h"

void func_808894A4(BgHidanHrock* rock, PlayState* play);
void func_808894B0(BgHidanHrock* rock, PlayState* play);
void func_8088960C(BgHidanHrock* rock, PlayState* play);
void func_808896B8(BgHidanHrock* rock, PlayState* play);
}

namespace ZeldaOnline {

class HidanHrockController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanHrock* Typed() const {
        return reinterpret_cast<BgHidanHrock*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using HrockActionFunc = decltype(&func_808894A4);
    static const HrockActionFunc* ActionTable(size_t* count) {
        static const HrockActionFunc sTable[] = {
            func_808894A4,   //settled, does nothing
            func_808894B0,   //shaking
            func_8088960C,   //falling
            func_808896B8,   //waiting for the hammer
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HrockActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_HOME_Y,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHidanHrock* rock = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, PackedInt2(rock->unk_168), out);


        PackProperty(PROP_HOME_Y, PackedFloat4(rock->dyna.actor.home.pos.y), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanHrock* rock = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const HrockActionFunc* table = ActionTable(&count);
                if (id < count)
                    rock->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                rock->unk_168 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HOME_Y:
                rock->dyna.actor.home.pos.y = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanHrock* rock = Typed();

        if (rock->actionFunc != func_808896B8)
            return;


        if (rock->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }


        RegisterColliderBase(play, &rock->collider.base, COLL_AC);


        if (DynaPolyActor_IsPlayerOnTop(&rock->dyna)) {
            Math_StepToF(&rock->dyna.actor.world.pos.y, rock->dyna.actor.home.pos.y - 5.0f, 1.0f);
        }
    }
};

}

#endif
