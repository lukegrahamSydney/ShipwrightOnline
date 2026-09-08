#ifndef MORIHASHIRA4CONTROLLERH
#define MORIHASHIRA4CONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Mori_Hashira4/z_bg_mori_hashira4.h"

void BgMoriHashira4_WaitForMoriTex(BgMoriHashira4* h4, PlayState* play);
void BgMoriHashira4_PillarsRotate(BgMoriHashira4* h4, PlayState* play);
void BgMoriHashira4_GateWait(BgMoriHashira4* h4, PlayState* play);
void BgMoriHashira4_GateOpen(BgMoriHashira4* h4, PlayState* play);

void BgMoriHashira4_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class MoriHashira4Controller : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgMoriHashira4* Typed() const {
        return reinterpret_cast<BgMoriHashira4*>(m_actor);
    }


  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using H4ActionFunc = void (*)(BgMoriHashira4*, PlayState*);
    static const H4ActionFunc* ActionTable(size_t* count) {
        static const H4ActionFunc sTable[] = {
            BgMoriHashira4_WaitForMoriTex,
            BgMoriHashira4_PillarsRotate,
            BgMoriHashira4_GateWait,
            BgMoriHashira4_GateOpen,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const H4ActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_GATE_TIMER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgMoriHashira4* h4 = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_GATE_TIMER, PackedInt2(h4->gateTimer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgMoriHashira4* h4 = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const H4ActionFunc* table = ActionTable(&count);
                if (id < count)
                    h4->actionFunc = table[id];
                break;
            }
            case PROP_GATE_TIMER:
                h4->gateTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgMoriHashira4* h4 = Typed();

        if (h4->dyna.actor.draw == nullptr && h4->moriTexObjIndex >= 0 &&
            Object_IsLoaded(&play->objectCtx, h4->moriTexObjIndex)) {
            h4->dyna.actor.draw = BgMoriHashira4_Draw;
        }

    }
};

}

#endif