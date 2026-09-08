#ifndef FLYINGPOTCONTROLLERH
#define FLYINGPOTCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Tubo_Trap/z_en_tubo_trap.h"

void EnTuboTrap_WaitForProximity(EnTuboTrap* pot, PlayState* play);
void EnTuboTrap_Levitate(EnTuboTrap* pot, PlayState* play);
void EnTuboTrap_Fly(EnTuboTrap* pot, PlayState* play);

void EnTuboTrap_SpawnEffectsOnLand(EnTuboTrap* pot, PlayState* play);
void EnTuboTrap_SpawnEffectsInWater(EnTuboTrap* pot, PlayState* play);
}

namespace ZeldaOnline {

class FlyingPotController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTuboTrap* Typed() const {
        return reinterpret_cast<EnTuboTrap*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using TuboTrapActionFunc = decltype(&EnTuboTrap_Fly);
    static const TuboTrapActionFunc* ActionTable(size_t* count) {
        static const TuboTrapActionFunc sTable[] = {
            EnTuboTrap_WaitForProximity,
            EnTuboTrap_Levitate,
            EnTuboTrap_Fly,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TuboTrapActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }


    bool HitWouldReact() const {
        EnTuboTrap* pot = Typed();

        if (pot->actionFunc != EnTuboTrap_Fly)
            return false;
        if (pot->collider.base.acFlags & AC_HIT)
            return true;
        if (pot->collider.base.atFlags & AT_BOUNCED)
            return true;
        if ((pot->collider.base.atFlags & AT_HIT) && pot->collider.base.at == &GET_PLAYER(gPlayState)->actor)
            return true;
        return false;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WAKE,
        PROP_WATER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTuboTrap* pot = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_WAKE,
                     ByteStream() << PackedFloat4(pot->targetY) << PackedFloat4(pot->originPos.x)
                                  << PackedFloat4(pot->originPos.y) << PackedFloat4(pot->originPos.z),
                     out);

        PackProperty(PROP_WATER, PackedFloat4(pot->actor.yDistToWater), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTuboTrap* pot = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TuboTrapActionFunc* table = ActionTable(&count);
                if (id < count)
                    pot->actionFunc = table[id];
                break;
            }
            case PROP_WAKE:
                pot->targetY = data.Read<PackedFloat4>().value();
                pot->originPos.x = data.Read<PackedFloat4>().value();
                pot->originPos.y = data.Read<PackedFloat4>().value();
                pot->originPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_WATER:
                pot->actor.yDistToWater = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        EnTuboTrap* pot = Typed();

        if (pot->actor.yDistToWater > 15.0f)
            EnTuboTrap_SpawnEffectsInWater(pot, gPlayState);
        else
            EnTuboTrap_SpawnEffectsOnLand(pot, gPlayState);
    }

    void UpdatePuppet(PlayState* play) override {
        EnTuboTrap* pot = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        pot->collider.base.acFlags &= ~AC_HIT;
        pot->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);


        if (pot->actionFunc != EnTuboTrap_WaitForProximity && pot->actor.category != ACTORCAT_ENEMY)
            Actor_ChangeCategory(play, &play->actorCtx, &pot->actor, ACTORCAT_ENEMY);

        if (pot->actionFunc == EnTuboTrap_WaitForProximity) {
            Player* player = GET_PLAYER(play);
            if (pot->actor.xzDistToPlayer < 200.0f && pot->actor.world.pos.y <= player->actor.world.pos.y &&
                IsLocalPlayerClosest())
                ClaimLeadership(CLAIM_REASON_COOLDOWN);
        }

        Actor_SetFocus(&pot->actor, 0.0f);

        Collider_UpdateCylinder(&pot->actor, &pot->collider);
        RegisterColliderBase(play, &pot->collider.base, COLL_AT | COLL_AC);
    }
};

}

#endif
