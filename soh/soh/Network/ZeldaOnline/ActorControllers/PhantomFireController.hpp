#ifndef PHANTOMFIRECONTROLLERH
#define PHANTOMFIRECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Fhg_Fire/z_en_fhg_fire.h"

void EnFhgFire_LightningStrike(EnFhgFire* fire, PlayState* play);
void EnFhgFire_LightningTrail(EnFhgFire* fire, PlayState* play);
void EnFhgFire_LightningShock(EnFhgFire* fire, PlayState* play);
void EnFhgFire_LightningBurst(EnFhgFire* fire, PlayState* play);
void EnFhgFire_SpearLight(EnFhgFire* fire, PlayState* play);
void EnFhgFire_EnergyBall(EnFhgFire* fire, PlayState* play);
void EnFhgFire_PhantomWarp(EnFhgFire* fire, PlayState* play);
}

namespace ZeldaOnline {

class PhantomFireController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnFhgFire* Typed() const {
        return reinterpret_cast<EnFhgFire*>(m_actor);
    }


    static bool IsNetworkedVariant(s16 params) {
        return params != FHGFIRE_LIGHTNING_TRAIL && params != FHGFIRE_SPEAR_LIGHT;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using FireUpdateFunc = void (*)(EnFhgFire*, PlayState*);
    static const FireUpdateFunc* UpdateTable(size_t* count) {
        static const FireUpdateFunc sTable[] = {
            EnFhgFire_LightningStrike, EnFhgFire_LightningTrail, EnFhgFire_LightningShock,
            EnFhgFire_LightningBurst,  EnFhgFire_SpearLight,     EnFhgFire_EnergyBall,
            EnFhgFire_PhantomWarp,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentUpdateIndex() const {
        size_t count;
        const FireUpdateFunc* table = UpdateTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->updateFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsEnergyBall() const {
        return Typed()->actor.params == FHGFIRE_ENERGY_BALL;
    }

    enum {
        PROP_UPDATE = PROP_CUSTOM_START,
        PROP_WORK,
        PROP_FWORK,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnFhgFire* fire = Typed();

        PackProperty(PROP_UPDATE, PackedUInt1(CurrentUpdateIndex()), out);

        ByteStream work;
        for (s32 i = 0; i < FHGFIRE_SHORT_COUNT; i++)
            work << PackedInt2(fire->work[i]);
        PackProperty(PROP_WORK, work, out);

        ByteStream fwork;
        for (s32 i = 0; i < FHGFIRE_FLOAT_COUNT; i++)
            fwork << PackedFloat4(fire->fwork[i]);
        PackProperty(PROP_FWORK, fwork, out);

        u8 roles = 0;
        if (fire->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        if (fire->collider.base.atFlags & AT_ON)
            roles |= COLL_AT;
        PackProperty(PROP_COLL_ROLES, PackedUInt1(roles), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnFhgFire* fire = Typed();

        switch (index) {
            case PROP_UPDATE: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const FireUpdateFunc* table = UpdateTable(&count);
                if (id < count)
                    fire->updateFunc = table[id];
                break;
            }
            case PROP_WORK:
                for (s32 i = 0; i < FHGFIRE_SHORT_COUNT; i++)
                    fire->work[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FWORK:
                for (s32 i = 0; i < FHGFIRE_FLOAT_COUNT; i++)
                    fire->fwork[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnFhgFire* fire = Typed();


        if (IsEnergyBall() && (fire->collider.base.acFlags & AC_HIT)) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        fire->collider.base.acFlags &= ~AC_HIT;

        if (m_roles != 0) {
            Collider_UpdateCylinder(&fire->actor, &fire->collider);
            RegisterColliderBase(play, &fire->collider.base, m_roles);
        }
    }

  private:
    u8 m_roles = 0;
};

}

#endif