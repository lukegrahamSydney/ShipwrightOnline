#ifndef BONGOFLOORCONTROLLERH
#define BONGOFLOORCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Sst_Floor/z_bg_sst_floor.h"
void BossSst_SetFloor(BgSstFloor* floor);
}

namespace ZeldaOnline {

class BongoFloorController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgSstFloor* Typed() const {
        return reinterpret_cast<BgSstFloor*>(m_actor);
    }

  protected:
    void OnActorInit() override {
        BossSst_SetFloor(Typed());
        GoLocal();
    }
};

}

#endif