#ifndef MORIRAKKATENJOCONTROLLERH
#define MORIRAKKATENJOCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Mori_Rakkatenjo/z_bg_mori_rakkatenjo.h"

void BgMoriRakkatenjo_WaitForMoriTex(BgMoriRakkatenjo* rt, PlayState* play);
void BgMoriRakkatenjo_Wait(BgMoriRakkatenjo* rt, PlayState* play);
void BgMoriRakkatenjo_Fall(BgMoriRakkatenjo* rt, PlayState* play);
void BgMoriRakkatenjo_Rest(BgMoriRakkatenjo* rt, PlayState* play);
void BgMoriRakkatenjo_Rise(BgMoriRakkatenjo* rt, PlayState* play);

void BgMoriRakkatenjo_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class MoriRakkatenjoController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgMoriRakkatenjo* Typed() const {
        return reinterpret_cast<BgMoriRakkatenjo*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using RtActionFunc = void (*)(BgMoriRakkatenjo*, PlayState*);
    static const RtActionFunc* ActionTable(size_t* count) {
        static const RtActionFunc sTable[] = {
            BgMoriRakkatenjo_WaitForMoriTex, BgMoriRakkatenjo_Wait, BgMoriRakkatenjo_Fall,
            BgMoriRakkatenjo_Rest,           BgMoriRakkatenjo_Rise,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const RtActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgMoriRakkatenjo* rt = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(rt->timer), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgMoriRakkatenjo* rt = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const RtActionFunc* table = ActionTable(&count);
                if (id < count)
                    rt->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                rt->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgMoriRakkatenjo* rt = Typed();


        if ((rt->actionFunc == BgMoriRakkatenjo_Wait || rt->actionFunc == BgMoriRakkatenjo_Rest) &&
            rt->dyna.actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest()) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
        }


        if (rt->dyna.actor.draw == nullptr && rt->moriTexObjIndex >= 0 &&
            Object_IsLoaded(&play->objectCtx, rt->moriTexObjIndex)) {
            rt->dyna.actor.draw = BgMoriRakkatenjo_Draw;
        }
    }
};

} // namespace ZeldaOnline

#endif