#ifndef GANONORGANCONTROLLERH
#define GANONORGANCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ganon_Organ/z_en_ganon_organ.h"
#include "src/overlays/actors/ovl_Boss_Ganon/z_boss_ganon.h"

extern BossGanon* sBossGanonGanondorf;
}

namespace ZeldaOnline {

class GanonOrganController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnGanonOrgan* Typed() const {
        return reinterpret_cast<EnGanonOrgan*>(m_actor);
    }

  protected:
    void OnActorInit() override {
        EnGanonOrgan* organ = Typed();

        organ->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;

        if (organ->actor.parent == nullptr && sBossGanonGanondorf != nullptr)
            organ->actor.parent = &sBossGanonGanondorf->actor;
    }

    void UpdatePuppet(PlayState* play) override {
        EnGanonOrgan* organ = Typed();

        if (organ->actor.parent == nullptr && sBossGanonGanondorf != nullptr)
            organ->actor.parent = &sBossGanonGanondorf->actor;

        if (organ->actor.params != 1 || organ->actor.parent == nullptr)
            return;

        BossGanon* dorf = (BossGanon*)organ->actor.parent;

        if (dorf->organAlpha == 0)
            Actor_Kill(&organ->actor);
    }
};

} // namespace ZeldaOnline

#endif
