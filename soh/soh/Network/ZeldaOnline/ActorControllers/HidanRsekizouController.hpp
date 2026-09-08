#ifndef HIDANRSEKIZOUCONTROLLERH
#define HIDANRSEKIZOUCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Rsekizou/z_bg_hidan_rsekizou.h"
}

namespace ZeldaOnline {


class HidanRsekizouController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanRsekizou* Typed() const {
        return reinterpret_cast<BgHidanRsekizou*>(m_actor);
    }

  protected:
    void BuildCustomProperties(ByteStream& out) override {
 
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanRsekizou* statue = Typed();

        statue->burnFrame = (statue->burnFrame + 1) % 8;

        if (statue->bendFrame != 0) {
            statue->bendFrame--;
        }
        if (statue->bendFrame == 0) {
            statue->bendFrame = 3;
        }

        f32 yawSine = Math_SinS(statue->dyna.actor.shape.rot.y);
        f32 yawCosine = Math_CosS(statue->dyna.actor.shape.rot.y);

        for (s32 i = 0; i < ARRAY_COUNT(statue->colliderItems); i++) {
            ColliderJntSphElement* sphere = &statue->collider.elements[i];

            sphere->dim.worldSphere.center.x =
                int16_t(statue->dyna.actor.home.pos.x + yawCosine * sphere->dim.modelSphere.center.x +
                        yawSine * sphere->dim.modelSphere.center.z);

            sphere->dim.worldSphere.center.y =
                (s16)statue->dyna.actor.home.pos.y + sphere->dim.modelSphere.center.y;
            sphere->dim.worldSphere.center.z =
                int16_t((statue->dyna.actor.home.pos.z - yawSine * sphere->dim.modelSphere.center.x) +
                        yawCosine * sphere->dim.modelSphere.center.z);
        }

        RegisterColliderBase(play, &statue->collider.base, COLL_AT);
        func_8002F974(&statue->dyna.actor, NA_SE_EV_FIRE_PILLAR - SFX_FLAG);
    }
};

}

#endif
