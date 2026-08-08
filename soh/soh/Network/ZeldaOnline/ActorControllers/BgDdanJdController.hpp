#ifndef BGDDANJDCONTROLLERH
#define BGDDANJDCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Ddan_Jd/z_bg_ddan_jd.h"

void BgDdanJd_Idle(BgDdanJd* platform, PlayState* play);
void BgDdanJd_Move(BgDdanJd* platform, PlayState* play);
}

namespace ZeldaOnline {

class BgDdanJdController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

  protected:
    static constexpr u8 ID_IDLE = 0;
    static constexpr u8 ID_MOVE = 1;

    u8 CurrentActionIndex() const {
        BgDdanJd* p = Typed();
        if (p->actionFunc == BgDdanJd_Move) {
            return ID_MOVE;
        }
        return ID_IDLE;
    }

    void OnBecomeLeader() override {

        BgDdanJd* p = Typed();
        if (m_currentActionIndex == ID_MOVE) {
            p->actionFunc = BgDdanJd_Move;
        } else {
            p->actionFunc = BgDdanJd_Idle;
        }
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_YSPEED,
        PROP_IDLE_TIMER,
        PROP_TARGET_Y,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgDdanJd* p = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, PackedUInt1(p->state), out);
        PackProperty(PROP_YSPEED, PackedUInt1(p->ySpeed), out);
        PackProperty(PROP_IDLE_TIMER, PackedInt2(p->idleTimer), out);
        PackProperty(PROP_TARGET_Y, PackedFloat4(p->targetY), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgDdanJd* p = Typed();

        switch (index) {
            case PROP_ACTION: m_currentActionIndex = (u8)(data.Read<PackedUInt1>().value()); break;
            case PROP_STATE: p->state = (u8)(data.Read<PackedUInt1>().value()); break;
            case PROP_YSPEED: p->ySpeed = (u8)(data.Read<PackedUInt1>().value()); break;
            case PROP_IDLE_TIMER: p->idleTimer = (s16)(data.Read<PackedInt2>().value()); break;
            case PROP_TARGET_Y: p->targetY = data.Read<PackedFloat4>().value(); break;
            default: return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
    }

  private:
    BgDdanJd* Typed() const {
        return reinterpret_cast<BgDdanJd*>(m_actor);
    }

    u8 m_currentActionIndex = ID_IDLE;
};

}

#endif
