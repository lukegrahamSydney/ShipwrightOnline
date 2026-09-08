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

        if (m_litTorchCount == sLitTorchCount)
            PackProperty(PROP_LIT_TORCH_COUNT, PackedInt1((s8)(m_litTorchCount)), out);
        else
            PackNullProperty(PROP_LIT_TORCH_COUNT, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjSyokudai* torch = Typed();

        switch (index) {
            case PROP_LIT_TIMER:
                torch->litTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_LIT_TORCH_COUNT:
                if (propLen != 0)
                    m_litTorchCount = sLitTorchCount = (s32)(data.Read<PackedInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdateLeader(PlayState* play) override {
        auto oldGlobalLitTorchCount = sLitTorchCount;
        AbstractActorController::UpdateLeader(play);

        if (oldGlobalLitTorchCount != sLitTorchCount)
            m_litTorchCount = sLitTorchCount;
    }

    void UpdatePuppet(PlayState* play) override {
        ObjSyokudai* torch = Typed();

        s16 litBefore = torch->litTimer;
        UpdateLeader(play);

        //Our local player lit it...
        if (litBefore == 0 && torch->litTimer != 0) {
            ClaimLeadership(CLAIM_REASON_NOW);
            m_litTorchCount = sLitTorchCount;
        }
    }

  private:
    s32 m_litTorchCount = -1;

    ObjSyokudai* Typed() const {
        return reinterpret_cast<ObjSyokudai*>(m_actor);
    }
};

} // namespace ZeldaOnline

#endif