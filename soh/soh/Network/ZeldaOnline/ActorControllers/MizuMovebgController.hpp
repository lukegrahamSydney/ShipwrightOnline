#ifndef MIZUMOVEBGCONTROLLERH
#define MIZUMOVEBGCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Mizu_Movebg/z_bg_mizu_movebg.h"
}

namespace ZeldaOnline {


class MizuMovebgController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgMizuMovebg* Typed() const {
        return reinterpret_cast<BgMizuMovebg*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return (((u16)(params) >> 0xC) & 0xF) == 7;
    }

  protected:
    enum {
        PROP_WAYPOINT = PROP_CUSTOM_START,
    };

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_WAYPOINT, PackedInt4(Typed()->waypointId), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        switch (index) {
            case PROP_WAYPOINT:
                Typed()->waypointId = data.Read<PackedInt4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgMizuMovebg* platform = Typed();


        if (platform->sfxFlags & 1) {
            func_8002F948(&platform->dyna.actor, NA_SE_EV_ROLL_STAND_2 - SFX_FLAG);
        }
    }
};

}

#endif
