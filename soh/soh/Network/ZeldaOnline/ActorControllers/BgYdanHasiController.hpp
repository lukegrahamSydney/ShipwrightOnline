#ifndef BGYDANHASICONTROLLERH
#define BGYDANHASICONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Ydan_Hasi/z_bg_ydan_hasi.h"

void func_8002F994(Actor* actor, s32 timer);

void BgYdanHasi_Draw(Actor*, PlayState* play);
}

namespace ZeldaOnline {

class BgYdanHasiController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

  protected:
    enum {
        PROP_TIMER = PROP_CUSTOM_START,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgYdanHasi* hasi = Typed();
        PackProperty(PROP_TIMER, PackedInt2(hasi->timer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgYdanHasi* hasi = Typed();

        switch (index) {
            case PROP_TIMER:
                hasi->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void EnsureDrawInstalled(PlayState* play) {
        BgYdanHasi* hasi = Typed();
        if (hasi->dyna.actor.params == HASI_THREE_BLOCKS && hasi->dyna.actor.draw == NULL &&
            Flags_GetSwitch(play, hasi->type)) {
            hasi->dyna.actor.draw = BgYdanHasi_Draw;
        }
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawInstalled(play);
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        BgYdanHasi* hasi = Typed();

        if (hasi->dyna.actor.params == HASI_WATER) {
            play->colCtx.colHeader->waterBoxes[1].ySurface = (s16)hasi->dyna.actor.world.pos.y;

            if (hasi->timer > 0) {
                func_8002F994(&hasi->dyna.actor, hasi->timer);
            }
        }

        EnsureDrawInstalled(play);
    }

  private:
    BgYdanHasi* Typed() const {
        return reinterpret_cast<BgYdanHasi*>(m_actor);
    }
};

}

#endif
