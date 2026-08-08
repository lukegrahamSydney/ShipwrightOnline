#ifndef KEESECONTROLLERH
#define KEESECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Firefly/z_en_firefly.h"

void EnFirefly_FlyIdle(EnFirefly* keese, PlayState* play);
void EnFirefly_Fall(EnFirefly* keese, PlayState* play);
void EnFirefly_Die(EnFirefly* keese, PlayState* play);
void EnFirefly_DiveAttack(EnFirefly* keese, PlayState* play);
void EnFirefly_Rebound(EnFirefly* keese, PlayState* play);
void EnFirefly_FlyAway(EnFirefly* keese, PlayState* play);
void EnFirefly_Stunned(EnFirefly* keese, PlayState* play);
void EnFirefly_FrozenFall(EnFirefly* keese, PlayState* play);
void EnFirefly_Perch(EnFirefly* keese, PlayState* play);
void EnFirefly_DisturbDiveAttack(EnFirefly* keese, PlayState* play);

void EnFirefly_SetupDie(EnFirefly* keese);
}

namespace ZeldaOnline {

class KeeseController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnFirefly* Typed() const {
        return reinterpret_cast<EnFirefly*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using KeeseActionFunc = void (*)(EnFirefly*, PlayState*);
    static const KeeseActionFunc* ActionTable(size_t* count) {
        static const KeeseActionFunc sTable[] = {
            EnFirefly_FlyIdle,
            EnFirefly_Fall,
            EnFirefly_Die,
            EnFirefly_DiveAttack,
            EnFirefly_Rebound,
            EnFirefly_FlyAway,
            EnFirefly_Stunned,
            EnFirefly_FrozenFall,
            EnFirefly_Perch,
            EnFirefly_DisturbDiveAttack,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const KeeseActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        EnFirefly* keese = Typed();
        u8 roles = COLL_OC;
        if (keese->actionFunc == EnFirefly_DiveAttack || keese->actionFunc == EnFirefly_DisturbDiveAttack)
            roles |= COLL_AT;
        if (keese->actor.colChkInfo.health != 0)
            roles |= COLL_AC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_TIMER,
        PROP_TARGET_PITCH,
        PROP_MAX_ALTITUDE,
        PROP_AURA,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnFirefly* keese = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(keese->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMER, PackedInt2(keese->timer), out);
        PackProperty(PROP_TARGET_PITCH, PackedInt2(keese->targetPitch), out);
        PackProperty(PROP_MAX_ALTITUDE, PackedFloat4(keese->maxAltitude), out);
        PackProperty(PROP_AURA, ByteStream() << PackedUInt1(keese->auraType) << PackedUInt1(keese->onFire), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(keese->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(0, &keese->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnFirefly* keese = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const KeeseActionFunc* table = ActionTable(&count);
                if (id < count)
                    keese->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                keese->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMER:
                keese->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TARGET_PITCH:
                keese->targetPitch = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_MAX_ALTITUDE:
                keese->maxAltitude = data.Read<PackedFloat4>().value();
                break;
            case PROP_AURA:
                keese->auraType = (u8)(data.Read<PackedUInt1>().value());
                keese->onFire = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                keese->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM:
                data.Read<PackedUInt1>();
                ApplyAnimProperty(nullptr, &keese->skelAnime, LOCK_CUR_FRAME ? keese->skelAnime.curFrame : 0.0f, data);
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnFirefly_Die) {
            EnFirefly_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnFirefly* keese = Typed();

        UpdateAnimation(&keese->skelAnime, LOCK_CUR_FRAME);

        if (keese->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        keese->collider.base.acFlags &= ~AC_HIT;
        keese->collider.base.atFlags &= ~AT_HIT;

        if ((keese->actionFunc == EnFirefly_Perch || keese->actionFunc == EnFirefly_FlyIdle) &&
            keese->actor.xzDistToPlayer < 160.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        keese->collider.elements[0].dim.worldSphere.center.x = (int16_t)keese->actor.world.pos.x;
        keese->collider.elements[0].dim.worldSphere.center.y = (int16_t)keese->actor.world.pos.y + 10;
        keese->collider.elements[0].dim.worldSphere.center.z = (int16_t)keese->actor.world.pos.z;

        RegisterColliderBase(play, &keese->collider.base, m_roles);

        keese->actor.focus.pos.x = (10.0f * Math_SinS(keese->actor.shape.rot.x) * Math_SinS(keese->actor.shape.rot.y)) +
                                   keese->actor.world.pos.x;
        keese->actor.focus.pos.y = (10.0f * Math_CosS(keese->actor.shape.rot.x)) + keese->actor.world.pos.y;
        keese->actor.focus.pos.z = (10.0f * Math_SinS(keese->actor.shape.rot.x) * Math_CosS(keese->actor.shape.rot.y)) +
                                   keese->actor.world.pos.z;
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
