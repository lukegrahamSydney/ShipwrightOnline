#ifndef NUTSBALLCONTROLLERH
#define NUTSBALLCONTROLLERH

#include "../AbstractActorController.hpp"
#include "src/overlays/actors/ovl_En_Nutsball/z_en_nutsball.h"

extern "C" {
void EnNutsball_Draw(Actor* thisx, PlayState* play);
void func_80ABBB34(EnNutsball*, PlayState*);
void func_80ABBBA8(EnNutsball*, PlayState*);
}

namespace ZeldaOnline {

class NutsballController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnNutsball* Typed() const {
        return reinterpret_cast<EnNutsball*>(m_actor);
    }

  protected:
    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_COLL,
        PROP_TIMER,
    };

    static const EnNutsballActionFunc* ActionTable(size_t* count) {
        static const EnNutsballActionFunc sTable[] = {
            func_80ABBB34,
            func_80ABBBA8,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t n;
        const auto t = ActionTable(&n);
        for (size_t i = 0; i < n; i++)
            if (t[i] == (void*)Typed()->actionFunc)
                return (u8)(i);
        return 0xFF;
    }

    u8 CurrentColliderRoles() const {
        return COLL_AT | COLL_AC | COLL_OC;
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnNutsball* nut = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        u8 atConfig = nut->collider.base.atFlags & ~(AT_HIT | AT_BOUNCED);
        PackProperty(PROP_COLL,
                     ByteStream() << PackedUInt1(atConfig) << PackedUInt4(nut->collider.info.toucher.dmgFlags), out);

        PackProperty(PROP_TIMER, PackedInt2(nut->timer), out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnNutsball* nut = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != 0xFF) {
                    size_t n;
                    const auto t = ActionTable(&n);
                    if (ai < n)
                        nut->actionFunc = t[ai];
                }
                return true;
            }
            case PROP_COLL: {
                u8 atConfig = (u8)(data.Read<PackedUInt1>().value());
                nut->collider.base.atFlags = atConfig;
                nut->collider.info.toucher.dmgFlags = data.Read<PackedUInt4>().value();
                return true;
            }
            case PROP_TIMER:
                nut->timer = (s16)(data.Read<PackedInt2>().value());
                return true;
            default:
                return false;
        }
    }

    void EnsureDrawInstalled(PlayState* play) {
        EnNutsball* nut = Typed();
        if (nut->actor.draw == nullptr && Object_IsLoaded(&play->objectCtx, nut->objBankIndex)) {
            nut->actor.objBankIndex = nut->objBankIndex;
            nut->actor.draw = EnNutsball_Draw;
        }
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawInstalled(play);
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnNutsball* nut = Typed();

        EnsureDrawInstalled(play);

        if (nut->collider.base.atFlags & AT_BOUNCED) {
            nut->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        nut->collider.base.atFlags &= ~AT_HIT;
        nut->collider.base.acFlags &= ~(AC_HIT | AC_BOUNCED);
        nut->collider.base.ocFlags1 &= ~OC1_HIT;

        nut->actor.home.rot.z += 0x2AA8;

        RegisterCylinder(play, &nut->collider, CurrentColliderRoles());
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }
};

} // namespace ZeldaOnline

#endif