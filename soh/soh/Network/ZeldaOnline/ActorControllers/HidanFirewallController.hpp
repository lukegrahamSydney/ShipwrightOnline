#ifndef HIDANFIREWALLCONTROLLERH
#define HIDANFIREWALLCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Firewall/z_bg_hidan_firewall.h"

void BgHidanFirewall_Wait(BgHidanFirewall* wall, PlayState* play);
void BgHidanFirewall_Countdown(BgHidanFirewall* wall, PlayState* play);
void BgHidanFirewall_Erupt(BgHidanFirewall* wall, PlayState* play);

void BgHidanFirewall_Collide(BgHidanFirewall* wall, PlayState* play);
void BgHidanFirewall_ColliderFollowPlayer(BgHidanFirewall* wall, PlayState* play);
void BgHidanFirewall_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class HidanFirewallController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanFirewall* Typed() const {
        return reinterpret_cast<BgHidanFirewall*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using WallActionFunc = void (*)(BgHidanFirewall*, PlayState*);
    static const WallActionFunc* ActionTable(size_t* count) {
        static const WallActionFunc sTable[] = {
            BgHidanFirewall_Wait,
            BgHidanFirewall_Countdown,
            BgHidanFirewall_Erupt,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const WallActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
    };

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanFirewall* wall = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const WallActionFunc* table = ActionTable(&count);
                if (id < count)
                    wall->actionFunc = table[id];
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanFirewall* wall = Typed();

        wall->unk_150 = (wall->unk_150 + 1) % 8;   //fireball texture cycle

        if (wall->collider.base.atFlags & AT_HIT) {
            wall->collider.base.atFlags &= ~AT_HIT;
            BgHidanFirewall_Collide(wall, play);
        }

        if (wall->actionFunc == BgHidanFirewall_Wait) {
            wall->actor.draw = nullptr;
        } else {
            wall->actor.draw = BgHidanFirewall_Draw;
        }


        if (wall->actionFunc == BgHidanFirewall_Erupt) {
            BgHidanFirewall_ColliderFollowPlayer(wall, play);
            RegisterColliderBase(play, &wall->collider.base, COLL_AT | COLL_OC);
            func_8002F974(&wall->actor, NA_SE_EV_FIRE_PLATE - SFX_FLAG);
        }
    }
};

}

#endif
