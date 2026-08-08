#ifndef TORCHCONTROLLERH
#define TORCHCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Syokudai/z_obj_syokudai.h"

extern s32 sLitTorchCount;
}

namespace ZeldaOnline {

class TorchController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

  protected:
    enum {
        PROP_LIT_TIMER = PROP_CUSTOM_START,
        PROP_LIT_TORCH_COUNT,
    };

    void BuildCustomProperties(ByteStream& out) override {
        ObjSyokudai* torch = Typed();

        PackProperty(PROP_LIT_TIMER, PackedInt2(torch->litTimer), out);
        PackProperty(PROP_LIT_TORCH_COUNT, ByteStream() << PackedInt1((s8)(sLitTorchCount)), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjSyokudai* torch = Typed();

        switch (index) {
            case PROP_LIT_TIMER: torch->litTimer = (s16)(data.Read<PackedInt2>().value()); break;
            case PROP_LIT_TORCH_COUNT: sLitTorchCount = (s32)(data.Read<PackedInt1>().value()); break;
            default: return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        ObjSyokudai* torch = Typed();

        s16 litBefore = torch->litTimer;
        m_originalUpdate(m_actor, play);

        if (litBefore == 0 && torch->litTimer != 0) {
            ClaimLeadership(CLAIM_REASON_HIT);
            SendUpdate();
        }
    }

  private:
    ObjSyokudai* Typed() const {
        return reinterpret_cast<ObjSyokudai*>(m_actor);
    }
};

}

#endif
