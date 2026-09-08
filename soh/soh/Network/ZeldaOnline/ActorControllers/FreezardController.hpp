#ifndef FREEZARDCONTROLLERH
#define FREEZARDCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Fz/z_en_fz.h"

void EnFz_Wait(EnFz* fz, PlayState* play);
void EnFz_Appear(EnFz* fz, PlayState* play);
void EnFz_AimForMove(EnFz* fz, PlayState* play);
void EnFz_MoveTowardsPlayer(EnFz* fz, PlayState* play);
void EnFz_AimForFreeze(EnFz* fz, PlayState* play);
void EnFz_BlowSmoke(EnFz* fz, PlayState* play);
void EnFz_Disappear(EnFz* fz, PlayState* play);
void EnFz_Despawn(EnFz* fz, PlayState* play);
void EnFz_Melt(EnFz* fz, PlayState* play);
void EnFz_BlowSmokeStationary(EnFz* fz, PlayState* play);

void EnFz_UpdateTargetPos(EnFz* fz, PlayState* play);
void EnFz_UpdateIceSmoke(EnFz* fz, PlayState* play);
void EnFz_SpawnIceSmokeFreeze(EnFz* fz, Vec3f* pos, Vec3f* velocity, Vec3f* accel, f32 xyScale, f32 xyScaleTarget,
                              s16 primAlpha, u8 isTimerMod8);
void EnFz_SpawnIceSmokeGrowingState(EnFz* fz);
void EnFz_SpawnIceSmokeActiveState(EnFz* fz);
void EnFz_Damaged(EnFz* fz, PlayState* play, Vec3f* vec, s32 numEffects, f32 unkFloat);
}

namespace ZeldaOnline {

class FreezardController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnFz* Typed() const {
        return reinterpret_cast<EnFz*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using FzActionFunc = decltype(&EnFz_Wait);
    static const FzActionFunc* ActionTable(size_t* count) {
        static const FzActionFunc sTable[] = {
            EnFz_Wait,      EnFz_Appear,  EnFz_AimForMove, EnFz_MoveTowardsPlayer,   EnFz_AimForFreeze, EnFz_BlowSmoke,
            EnFz_Disappear, EnFz_Despawn, EnFz_Melt,       EnFz_BlowSmokeStationary,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const FzActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }


    u8 CurrentColliderRoles() const {
        EnFz* fz = Typed();
        if (fz->isDespawning || !fz->isFreezing)
            return 0;
        u8 roles = COLL_OC;
        if (fz->actor.colorFilterTimer == 0)
            roles |= COLL_AC;
        return roles;
    }

    bool HitWouldReact() const {
        EnFz* fz = Typed();
        if (!fz->isFreezing)
            return false;
        if (fz->collider2.base.acFlags & AC_BOUNCED)
            return true;
        if (!(fz->collider1.base.acFlags & AC_HIT))
            return false;
        return fz->actor.colChkInfo.damageEffect == 2 || fz->actor.colChkInfo.damageEffect == 0xF;
    }

    u8 StateBits() const {
        EnFz* fz = Typed();
        u8 bits = 0;
        if (fz->isActive)
            bits |= 1 << 0;
        if (fz->isDespawning)
            bits |= 1 << 1;
        if (fz->isFreezing)
            bits |= 1 << 2;
        if (fz->isMoving)
            bits |= 1 << 3;
        return bits;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_STATE,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnFz* fz = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(fz->actor.colChkInfo.health), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(fz->counter) << PackedInt2(fz->timer) << PackedUInt1(fz->state)
                                  << PackedUInt1(StateBits()) << PackedUInt1(fz->envAlpha) << PackedFloat4(fz->speedXZ),
                     out);

        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnFz* fz = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const FzActionFunc* table = ActionTable(&count);
                if (id < count)
                    fz->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                fz->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_STATE: {
                fz->counter = (s16)(data.Read<PackedInt2>().value());
                fz->timer = (s16)(data.Read<PackedInt2>().value());
                fz->state = (u8)(data.Read<PackedUInt1>().value());
                u8 bits = (u8)(data.Read<PackedUInt1>().value());
                fz->envAlpha = (u8)(data.Read<PackedUInt1>().value());
                fz->speedXZ = data.Read<PackedFloat4>().value();
                fz->isActive = (bits & (1 << 0)) != 0;
                fz->isDespawning = (bits & (1 << 1)) != 0;
                fz->isFreezing = (bits & (1 << 2)) != 0;
                fz->isMoving = (bits & (1 << 3)) != 0;
                break;
            }
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnFz* fz = Typed();
        Vec3f pos = fz->actor.world.pos;

        if ((changed & (1ull << PROP_HEALTH)) && fz->actor.colChkInfo.health != 0 && fz->actor.colorFilterTimer != 0)
            EnFz_Damaged(fz, gPlayState, &pos, 10, 0.0f);


        if ((changed & (1ull << PROP_ACTION)) && fz->actionFunc == EnFz_Despawn) {
            if (fz->state != 3)
                EnFz_Damaged(fz, gPlayState, &pos, 30, 10.0f);
            Actor_ChangeCategory(gPlayState, &gPlayState->actorCtx, &fz->actor, ACTORCAT_PROP);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnFz* fz = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        fz->collider1.base.acFlags &= ~AC_HIT;
        fz->collider2.base.acFlags &= ~(AC_HIT | AC_BOUNCED);
        fz->collider1.base.atFlags &= ~AT_HIT;
        fz->collider3.base.atFlags &= ~AT_HIT;

        if (!fz->isDespawning && fz->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Actor_SetFocus(&fz->actor, 50.0f);

        if (fz->state == 1)
            EnFz_SpawnIceSmokeGrowingState(fz);
        else if (fz->state >= 2)
            EnFz_SpawnIceSmokeActiveState(fz);

        if (fz->actionFunc == EnFz_BlowSmoke) {
            if (fz->timer >= 11) {
                s16 primAlpha = 150;
                if ((fz->timer - 10) < 16)
                    primAlpha = (fz->timer * 10) - 100;
                BlowBreath(primAlpha, (fz->timer % 8) == 0);
            }
            EnFz_UpdateTargetPos(fz, play);
        } else if (fz->actionFunc == EnFz_BlowSmokeStationary) {
            if (fz->counter & 0xC0) {
                EnFz_UpdateTargetPos(fz, play);
            } else {
                s16 primAlpha = 150;
                if ((fz->counter & 0x3F) >= 48)
                    primAlpha = 630 - ((fz->counter & 0x3F) * 10);
                BlowBreath(primAlpha, (fz->counter % 8) == 0);
            }
        }

        EnFz_UpdateIceSmoke(fz, play);

        if (m_roles != 0) {
            Collider_UpdateCylinder(&fz->actor, &fz->collider1);
            Collider_UpdateCylinder(&fz->actor, &fz->collider2);
            RegisterColliderBase(play, &fz->collider1.base, m_roles);
            if (m_roles & COLL_AC)
                RegisterColliderBase(play, &fz->collider2.base, COLL_AC);
        }
    }

  private:

    void BlowBreath(s16 primAlpha, u8 isTimerMod8) {
        EnFz* fz = Typed();
        Vec3f vec1;
        Vec3f pos;
        Vec3f velocity;
        Vec3f accel;

        accel.x = accel.z = 0.0f;
        accel.y = 0.6f;

        pos.x = fz->actor.world.pos.x;
        pos.y = fz->actor.world.pos.y + 20.0f;
        pos.z = fz->actor.world.pos.z;

        Matrix_RotateY((fz->actor.shape.rot.y / (f32)0x8000) * (f32)M_PI, MTXMODE_NEW);

        vec1.x = 0.0f;
        vec1.y = -2.0f;
        vec1.z = 20.0f;

        Matrix_MultVec3f(&vec1, &velocity);

        EnFz_SpawnIceSmokeFreeze(fz, &pos, &velocity, &accel, 2.0f, 25.0f, primAlpha, isTimerMod8);

        pos.x += (velocity.x * 0.5f);
        pos.y += (velocity.y * 0.5f);
        pos.z += (velocity.z * 0.5f);

        EnFz_SpawnIceSmokeFreeze(fz, &pos, &velocity, &accel, 2.0f, 25.0f, primAlpha, false);
    }

    u8 m_roles = 0;
};

} // namespace ZeldaOnline

#endif