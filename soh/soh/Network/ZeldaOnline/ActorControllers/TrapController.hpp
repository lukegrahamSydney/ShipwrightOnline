#ifndef TRAPCONTROLLERH
#define TRAPCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Trap/z_en_trap.h"
}

namespace ZeldaOnline {

class TrapController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTrap* Typed() const {
        return reinterpret_cast<EnTrap*>(m_actor);
    }

  protected:
    static constexpr f32 DAMAGE_XZ_RANGE = 40.0f;
    static constexpr f32 DAMAGE_Y_RANGE = 20.0f;
    static constexpr f32 CLAIM_XZ_RANGE = 70.0f;
    static constexpr f32 CLAIM_Y_RANGE = 40.0f;

    u8 CurrentColliderRoles() const {
        EnTrap* trap = Typed();
        u8 roles = COLL_OC;
        if (trap->actor.colorFilterTimer == 0)
            roles |= COLL_AC;
        return roles;
    }

    enum {
        PROP_GENERIC1 = PROP_CUSTOM_START,
        PROP_GENERIC2,
        PROP_DMG_TIMER,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTrap* trap = Typed();

        PackProperty(PROP_GENERIC1, PackedInt2(trap->genericVar1), out);
        PackProperty(PROP_GENERIC2, PackedFloat4(trap->genericVar2), out);
        PackProperty(PROP_DMG_TIMER, PackedInt4(trap->playerDmgTimer), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTrap* trap = Typed();

        switch (index) {
            case PROP_GENERIC1:
                trap->genericVar1 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_GENERIC2:
                trap->genericVar2 = data.Read<PackedFloat4>().value();
                break;
            case PROP_DMG_TIMER:
                trap->playerDmgTimer = data.Read<PackedInt4>().value();
                break;

            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void UpdatePuppet(PlayState* play) override {
        EnTrap* trap = Typed();

        if (trap->actor.xzDistToPlayer <= CLAIM_XZ_RANGE && trap->actor.yDistToPlayer <= CLAIM_Y_RANGE) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }

        if (trap->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }

        trap->collider.base.ocFlags1 &= ~OC1_HIT;

        if (trap->actor.xzDistToPlayer < 200.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        if (trap->actor.colorFilterTimer != 0 && m_prevColorFilterTimer == 0) {
            Vec3f icePos = trap->actor.world.pos;
            icePos.y += 10.0f;
            icePos.z += 10.0f;
            EffectSsEnIce_SpawnFlyingVec3f(play, &trap->actor, &icePos, 150, 150, 150, 250, 235, 245, 255, 1.8f);
            icePos.x += 10.0f;
            icePos.z -= 20.0f;
            EffectSsEnIce_SpawnFlyingVec3f(play, &trap->actor, &icePos, 150, 150, 150, 250, 235, 245, 255, 1.8f);
            icePos.x -= 20.0f;
            EffectSsEnIce_SpawnFlyingVec3f(play, &trap->actor, &icePos, 150, 150, 150, 250, 235, 245, 255, 1.8f);
        }
        m_prevColorFilterTimer = trap->actor.colorFilterTimer;

        Collider_UpdateCylinder(&trap->actor, &trap->collider);
        RegisterColliderBase(play, &trap->collider.base, m_roles);
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
    u8 m_prevColorFilterTimer = 0;
};

}

#endif
