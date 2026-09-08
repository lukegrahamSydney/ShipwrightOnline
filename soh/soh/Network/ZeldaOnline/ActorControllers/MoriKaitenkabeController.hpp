#ifndef MORIKAITENKABECONTROLLERH
#define MORIKAITENKABECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Mori_Kaitenkabe/z_bg_mori_kaitenkabe.h"

void BgMoriKaitenkabe_WaitForMoriTex(BgMoriKaitenkabe* kk, PlayState* play);
void BgMoriKaitenkabe_Wait(BgMoriKaitenkabe* kk, PlayState* play);
void BgMoriKaitenkabe_Rotate(BgMoriKaitenkabe* kk, PlayState* play);

void BgMoriKaitenkabe_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class MoriKaitenkabeController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgMoriKaitenkabe* Typed() const {
        return reinterpret_cast<BgMoriKaitenkabe*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using KkActionFunc = void (*)(BgMoriKaitenkabe*, PlayState*);
    static const KkActionFunc* ActionTable(size_t* count) {
        static const KkActionFunc sTable[] = {
            BgMoriKaitenkabe_WaitForMoriTex,
            BgMoriKaitenkabe_Wait,
            BgMoriKaitenkabe_Rotate,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const KkActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsRotating() const {
        return Typed()->actionFunc == BgMoriKaitenkabe_Rotate;
    }


    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ROTATION,
        PROP_HOME_ROT_Y,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgMoriKaitenkabe* kk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ROTATION,
                     ByteStream() << PackedInt4(kk->timer) << PackedFloat4(kk->rotDirection)
                                  << PackedFloat4(kk->rotSpeed) << PackedFloat4(kk->rotYdeg),
                     out);


        PackProperty(PROP_HOME_ROT_Y, PackedInt2(kk->dyna.actor.home.rot.y), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgMoriKaitenkabe* kk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const KkActionFunc* table = ActionTable(&count);
                if (id < count)
                    kk->actionFunc = table[id];
                break;
            }
            case PROP_ROTATION:
                kk->timer = data.Read<PackedInt4>().value();
                kk->rotDirection = data.Read<PackedFloat4>().value();
                kk->rotSpeed = data.Read<PackedFloat4>().value();
                kk->rotYdeg = data.Read<PackedFloat4>().value();
                break;
            case PROP_HOME_ROT_Y:
                kk->dyna.actor.home.rot.y = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgMoriKaitenkabe* kk = Typed();

        //WaitForMoriTex installs the draw once the texture object lands.
        if (kk->dyna.actor.draw == nullptr && kk->moriTexObjIndex >= 0 &&
            Object_IsLoaded(&play->objectCtx, kk->moriTexObjIndex)) {
            kk->dyna.actor.draw = BgMoriKaitenkabe_Draw;
        }


        if (!IsRotating() && kk->dyna.unk_150 > 0.001f) {
            ClaimLeadership(CLAIM_REASON_NOW);
        }

        if (fabsf(kk->dyna.unk_150) > 0.001f) {
            kk->dyna.unk_150 = 0.0f;
            GET_PLAYER(play)->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;
        }
    }
};

}

#endif