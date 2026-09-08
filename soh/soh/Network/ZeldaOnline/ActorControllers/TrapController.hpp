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
    static constexpr s32 DAMAGE_COOLDOWN = 15;

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
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTrap* trap = Typed();

        PackProperty(PROP_GENERIC1, PackedInt2(trap->genericVar1), out);
        PackProperty(PROP_GENERIC2, PackedFloat4(trap->genericVar2), out);
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

    void DamageLocalPlayer(PlayState* play) {
        EnTrap* trap = Typed();
        s16 angleToKnockPlayer;

        if (trap->actor.colorFilterTimer != 0)
            return;

        DECR(trap->playerDmgTimer);

        if ((trap->actor.xzDistToPlayer <= DAMAGE_XZ_RANGE) && (trap->playerDmgTimer == 0) &&
            (trap->actor.yDistToPlayer <= DAMAGE_Y_RANGE)) {
            if (!(trap->actor.params & (SPIKETRAP_MODE_LINEAR | SPIKETRAP_MODE_CIRCULAR))) {
                if ((s16)(trap->genericVar1 - trap->actor.yawTowardsPlayer) >= 0)
                    angleToKnockPlayer = trap->genericVar1 - 0x4000;
                else
                    angleToKnockPlayer = trap->genericVar1 + 0x4000;
            } else {
                angleToKnockPlayer = trap->actor.yawTowardsPlayer;
            }

            play->damagePlayer(play, -4);
            func_8002F7A0(play, &trap->actor, 6.0f, angleToKnockPlayer, 6.0f);
            trap->playerDmgTimer = DAMAGE_COOLDOWN;
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnTrap* trap = Typed();

        if (trap->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        trap->collider.base.ocFlags1 &= ~OC1_HIT;

        DamageLocalPlayer(play);

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

} // namespace ZeldaOnline

#endif