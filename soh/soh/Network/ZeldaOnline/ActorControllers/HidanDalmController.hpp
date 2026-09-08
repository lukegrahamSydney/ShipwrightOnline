#ifndef HIDANDALMCONTROLLERH
#define HIDANDALMCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Dalm/z_bg_hidan_dalm.h"

void BgHidanDalm_Wait(BgHidanDalm* dalm, PlayState* play);
void BgHidanDalm_Shrink(BgHidanDalm* dalm, PlayState* play);
}

namespace ZeldaOnline {

class HidanDalmController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanDalm* Typed() const {
        return reinterpret_cast<BgHidanDalm*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using DalmActionFunc = decltype(&BgHidanDalm_Wait);
    static const DalmActionFunc* ActionTable(size_t* count) {
        static const DalmActionFunc sTable[] = {
            BgHidanDalm_Wait,
            BgHidanDalm_Shrink,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DalmActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }


    bool HitWouldReact() const {
        BgHidanDalm* dalm = Typed();
        if (dalm->actionFunc != BgHidanDalm_Wait || !(dalm->collider.base.acFlags & AC_HIT))
            return false;

        if (gPlayState == nullptr)
            return false;

        Player* player = GET_PLAYER(gPlayState);
        return !Player_InCsMode(gPlayState) &&
               (player->meleeWeaponAnimation == 22 || player->meleeWeaponAnimation == 23);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
    };

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanDalm* dalm = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const DalmActionFunc* table = ActionTable(&count);
                if (id < count)
                    dalm->actionFunc = table[id];
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanDalm* dalm = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        dalm->collider.base.acFlags &= ~AC_HIT;


        if (dalm->actionFunc == BgHidanDalm_Wait) {
            RegisterColliderBase(play, &dalm->collider.base, COLL_AC);
        }
    }
};

}

#endif
