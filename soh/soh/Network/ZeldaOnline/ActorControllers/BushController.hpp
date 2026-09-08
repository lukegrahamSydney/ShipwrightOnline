#ifndef BUSHCONTROLLERH
#define BUSHCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Kusa/z_en_kusa.h"

void EnKusa_WaitObject(EnKusa* kusa, PlayState* play);
void EnKusa_Main(EnKusa* kusa, PlayState* play);
void EnKusa_LiftedUp(EnKusa* kusa, PlayState* play);
void EnKusa_Fall(EnKusa* kusa, PlayState* play);
void EnKusa_CutWaitRegrow(EnKusa* kusa, PlayState* play);
void EnKusa_DoNothing(EnKusa* kusa, PlayState* play);
void EnKusa_UprootedWaitRegrow(EnKusa* kusa, PlayState* play);
void EnKusa_Regrow(EnKusa* kusa, PlayState* play);

void EnKusa_SpawnFragments(EnKusa* kusa, PlayState* play);
void EnKusa_DropCollectible(EnKusa* kusa, PlayState* play);
void EnKusa_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class BushController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnKusa* Typed() const {
        return reinterpret_cast<EnKusa*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using KusaActionFunc = void (*)(EnKusa*, PlayState*);
    static const KusaActionFunc* ActionTable(size_t* count) {
        static const KusaActionFunc sTable[] = {
            EnKusa_WaitObject,
            EnKusa_Main,
            EnKusa_LiftedUp,
            EnKusa_Fall,
            EnKusa_CutWaitRegrow,
            EnKusa_DoNothing,
            EnKusa_UprootedWaitRegrow,
            EnKusa_Regrow,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const KusaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsCut() const {
        EnKusa* kusa = Typed();
        return kusa->actionFunc == EnKusa_CutWaitRegrow || kusa->actionFunc == EnKusa_DoNothing ||
               kusa->actionFunc == EnKusa_UprootedWaitRegrow;
    }

    bool HitWouldReact(PlayState* play) const {
        EnKusa* kusa = Typed();
        return kusa->actionFunc == EnKusa_Main && (kusa->collider.base.acFlags & AC_HIT) && play->csCtx.state == 0;
    }

    void EnsureDrawInstalled(PlayState* play) {
        EnKusa* kusa = Typed();
        if (play == nullptr || kusa->actor.draw != nullptr)
            return;
        if (kusa->objBankIndex >= 0 && Object_IsLoaded(&play->objectCtx, kusa->objBankIndex)) {
            kusa->actor.draw = EnKusa_Draw;
            kusa->actor.objBankIndex = kusa->objBankIndex;
        }
    }

    void ShowCutEffects() {
        if (gPlayState == nullptr)
            return;
        EnKusa_SpawnFragments(Typed(), gPlayState);
        EnKusa_DropCollectible(Typed(), gPlayState);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };


    void BuildCustomProperties(ByteStream& out) override {
        EnKusa* kusa = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(kusa->timer), out);

        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnKusa* kusa = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const KusaActionFunc* table = ActionTable(&count);
                if (id < count)
                    kusa->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                kusa->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        bool cutNow = IsCut();
        if (cutNow && !m_wasCut)
            ShowCutEffects();
        m_wasCut = cutNow;
    }

    void OnServerDestroy() override {
        if (!m_wasCut)
            ShowCutEffects();
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawInstalled(play);
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnKusa* kusa = Typed();

        EnsureDrawInstalled(play);

        if (HitWouldReact(play)) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        if (kusa->actor.parent == &GET_PLAYER(play)->actor) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        kusa->collider.base.acFlags &= ~AC_HIT;

        kusa->actor.shape.yOffset = (kusa->actor.flags & ACTOR_FLAG_GRASS_DESTROYED) ? -6.25f : 0.0f;

        if (kusa->actionFunc == EnKusa_Main && kusa->actor.parent == nullptr) {
            if (!(kusa->collider.base.ocFlags1 & OC1_TYPE_PLAYER) && kusa->actor.xzDistToPlayer > 12.0f)
                kusa->collider.base.ocFlags1 |= OC1_TYPE_PLAYER;

            if (kusa->actor.xzDistToPlayer < 600.0f) {
                Collider_UpdateCylinder(&kusa->actor, &kusa->collider);

                u8 roles = COLL_AC;
                if (kusa->actor.xzDistToPlayer < 400.0f)
                    roles |= COLL_OC;
                RegisterColliderBase(play, &kusa->collider.base, roles);

                if (kusa->actor.xzDistToPlayer < 100.0f)
                    Actor_OfferCarry(&kusa->actor, play);
            }
        }
    }

  private:
    bool m_wasCut = false;
};

}

#endif
