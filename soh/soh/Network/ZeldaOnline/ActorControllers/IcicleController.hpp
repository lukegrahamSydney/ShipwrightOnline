#ifndef ICICLECONTROLLERH
#define ICICLECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Ice_Turara/z_bg_ice_turara.h"

void BgIceTurara_Stalagmite(BgIceTurara* ic, PlayState* play);
void BgIceTurara_Wait(BgIceTurara* ic, PlayState* play);
void BgIceTurara_Shiver(BgIceTurara* ic, PlayState* play);
void BgIceTurara_Fall(BgIceTurara* ic, PlayState* play);
void BgIceTurara_Regrow(BgIceTurara* ic, PlayState* play);

void BgIceTurara_Break(BgIceTurara* ic, PlayState* play, f32 arg2);
}

namespace ZeldaOnline {

class IcicleController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgIceTurara* Typed() const {
        return reinterpret_cast<BgIceTurara*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using TuraraActionFunc = decltype(&BgIceTurara_Wait);
    static const TuraraActionFunc* ActionTable(size_t* count) {
        static const TuraraActionFunc sTable[] = {
            BgIceTurara_Stalagmite, BgIceTurara_Wait, BgIceTurara_Shiver, BgIceTurara_Fall, BgIceTurara_Regrow,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TuraraActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        BgIceTurara* ic = Typed();
        if (ic->actionFunc == BgIceTurara_Stalagmite)
            return COLL_AC;
        if (ic->actionFunc == BgIceTurara_Fall)
            return COLL_AT;
        return 0;
    }

    bool HitWouldReact() const {
        BgIceTurara* ic = Typed();
        if (ic->actionFunc == BgIceTurara_Stalagmite)
            return (ic->collider.base.acFlags & AC_HIT) != 0;
        if (ic->actionFunc == BgIceTurara_Fall)
            return (ic->collider.base.atFlags & AT_HIT) != 0;
        return false;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_SHIVER_TIMER,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgIceTurara* ic = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_SHIVER_TIMER, PackedInt2(ic->shiverTimer), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgIceTurara* ic = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TuraraActionFunc* table = ActionTable(&count);
                if (id < count)
                    ic->actionFunc = table[id];
                break;
            }
            case PROP_SHIVER_TIMER:
                ic->shiverTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BgIceTurara* ic = Typed();

        if ((changed & (1ull << PROP_ACTION)) && ic->actionFunc == BgIceTurara_Regrow)
            BgIceTurara_Break(ic, gPlayState, 40.0f);
    }

    void OnServerDestroy() override {
        BgIceTurara* ic = Typed();

        if (ic->actionFunc == BgIceTurara_Stalagmite)
            BgIceTurara_Break(ic, gPlayState, 50.0f);
        else if (ic->actionFunc == BgIceTurara_Fall)
            BgIceTurara_Break(ic, gPlayState, 40.0f);
    }

    void UpdatePuppet(PlayState* play) override {
        BgIceTurara* ic = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        ic->collider.base.acFlags &= ~AC_HIT;
        ic->collider.base.atFlags &= ~AT_HIT;

        if (ic->actionFunc == BgIceTurara_Wait && ic->dyna.actor.xzDistToPlayer < 100.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);


        if (ic->actionFunc == BgIceTurara_Fall) {
            func_8003EBF8(play, &play->colCtx.dyna, ic->dyna.bgId);
        } else {
            func_8003EC50(play, &play->colCtx.dyna, ic->dyna.bgId);
        }

        if (m_roles != 0) {
            Collider_UpdateCylinder(&ic->dyna.actor, &ic->collider);
            RegisterColliderBase(play, &ic->collider.base, m_roles);
        }
    }

  private:
    u8 m_roles = 0;
};

} // namespace ZeldaOnline

#endif