#ifndef ARMOSCONTROLLERH
#define ARMOSCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Am/z_en_am.h"
#include "objects/object_am/object_am.h"

void EnAm_Statue(EnAm* am, PlayState* play);
void EnAm_Sleep(EnAm* am, PlayState* play);
void EnAm_Lunge(EnAm* am, PlayState* play);
void EnAm_RotateToHome(EnAm* am, PlayState* play);
void EnAm_MoveToHome(EnAm* am, PlayState* play);
void EnAm_RotateToInit(EnAm* am, PlayState* play);
void EnAm_Cooldown(EnAm* am, PlayState* play);
void EnAm_Ricochet(EnAm* am, PlayState* play);
void EnAm_Stunned(EnAm* am, PlayState* play);
void EnAm_RecoilFromDamage(EnAm* am, PlayState* play);

void EnAm_SpawnEffects(EnAm* am, PlayState* play);
}

namespace ZeldaOnline {

class ArmosController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnAm* Typed() const {
        return reinterpret_cast<EnAm*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using ArmosActionFunc = void (*)(EnAm*, PlayState*);
    static const ArmosActionFunc* ActionTable(size_t* count) {
        static const ArmosActionFunc sTable[] = {
            EnAm_Statue,
            EnAm_Sleep,
            EnAm_Lunge,
            EnAm_RotateToHome,
            EnAm_MoveToHome,
            EnAm_RotateToInit,
            EnAm_Cooldown,
            EnAm_Ricochet,
            EnAm_Stunned,
            EnAm_RecoilFromDamage,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ArmosActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_HOP = 0;
    static constexpr u8 ANIM_DAMAGED = 1;
    static constexpr u8 ANIM_RICOCHET = 2;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 3;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_HOP:
                return gArmosHopAnim;
            case ANIM_DAMAGED:
                return gArmosDamagedAnim;
            case ANIM_RICOCHET:
                return gArmosRicochetAnim;
            default:
                return nullptr;
        }
    }
    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    bool IsStatue() const {
        return Typed()->dyna.actor.params == ARMOS_STATUE;
    }

    u8 CurrentHurtRoles() const {
        u8 roles = COLL_OC;
        if (!IsStatue())
            roles |= COLL_AC;
        return roles;
    }
    u8 CurrentBlockRoles() const {
        EnAm* am = Typed();
        if (IsStatue())
            return COLL_OC;
        return (am->dyna.actor.colorFilterTimer == 0) ? COLL_AC : 0;
    }
    u8 CurrentHitRoles() const {
        EnAm* am = Typed();
        if (IsStatue() || am->behavior < 4 || am->unk_264 <= 0)
            return 0;
        if (am->hitCollider.base.atFlags & AT_BOUNCED)
            return 0;
        return COLL_AT;
    }

    bool HitWouldReact() const {
        EnAm* am = Typed();
        if (IsStatue())
            return false;
        return (am->hurtCollider.base.acFlags & AC_HIT) || (am->blockCollider.base.acFlags & AC_HIT);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_HEALTH,
        PROP_TEXTURE_BLEND,
        PROP_BEHAVIOR,
        PROP_PHASE,
        PROP_TIMERS,
        PROP_DAMAGE_EFFECT,
        PROP_SHAKE_ORIGIN,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnAm* am = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(am->dyna.actor.colChkInfo.health), out);
        PackProperty(PROP_TEXTURE_BLEND, PackedUInt1(am->textureBlend), out);
        PackProperty(PROP_BEHAVIOR, PackedInt4(am->behavior), out);
        PackProperty(PROP_PHASE, ByteStream() << PackedInt2(am->unk_258) << PackedInt2(am->unk_264), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(am->cooldownTimer) << PackedInt2(am->attackTimer)
                                  << PackedInt2(am->iceTimer) << PackedInt2(am->deathTimer)
                                  << PackedInt2(am->panicSpinRot),
                     out);
        PackProperty(PROP_DAMAGE_EFFECT, PackedUInt1(am->damageEffect), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_SHAKE_ORIGIN,
                     ByteStream() << PackedFloat4(am->shakeOrigin.x) << PackedFloat4(am->shakeOrigin.y)
                                  << PackedFloat4(am->shakeOrigin.z),
                     out);
        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentHurtRoles()) << PackedUInt1(CurrentBlockRoles())
                                  << PackedUInt1(CurrentHitRoles()),
                     out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(am->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &am->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnAm* am = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const ArmosActionFunc* table = ActionTable(&count);
                if (id < count)
                    am->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                am->dyna.actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TEXTURE_BLEND:
                am->textureBlend = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_BEHAVIOR:
                am->behavior = data.Read<PackedInt4>().value();
                break;
            case PROP_PHASE:
                am->unk_258 = (s16)(data.Read<PackedInt2>().value());
                am->unk_264 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                am->cooldownTimer = (s16)(data.Read<PackedInt2>().value());
                am->attackTimer = (s16)(data.Read<PackedInt2>().value());
                am->iceTimer = (s16)(data.Read<PackedInt2>().value());
                am->deathTimer = (s16)(data.Read<PackedInt2>().value());
                am->panicSpinRot = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_DAMAGE_EFFECT:
                am->damageEffect = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_SHAKE_ORIGIN:
                am->shakeOrigin.x = data.Read<PackedFloat4>().value();
                am->shakeOrigin.y = data.Read<PackedFloat4>().value();
                am->shakeOrigin.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_hurtRoles = (u8)(data.Read<PackedUInt1>().value());
                m_blockRoles = (u8)(data.Read<PackedUInt1>().value());
                m_hitRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                am->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &am->skelAnime, LOCK_CUR_FRAME ? am->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        if (gPlayState == nullptr || Typed() == nullptr)
            return;
        EnAm_SpawnEffects(Typed(), gPlayState);
    }

    void UpdatePuppet(PlayState* play) override {
        EnAm* am = Typed();

        UpdateAnimation(&am->skelAnime, LOCK_CUR_FRAME);

        if (fabsf(am->dyna.unk_150) > 0.001f) {
            ClaimLeadership(CLAIM_REASON_PROXIMITY);
            am->dyna.unk_150 = 0.0f;
        }

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        am->hurtCollider.base.acFlags &= ~AC_HIT;
        am->blockCollider.base.acFlags &= ~AC_HIT;
        am->hitCollider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);

        if (!IsStatue() && am->actionFunc == EnAm_Sleep && am->dyna.actor.xzDistToPlayer < 240.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        Actor_MoveXZGravity(&am->dyna.actor);
        Actor_UpdateBgCheckInfo(play, &am->dyna.actor, 20.0f, 28.0f, 80.0f, 0x1D);

        if (!IsStatue())
            Actor_SetFocus(&am->dyna.actor, am->dyna.actor.scale.x * 4500.0f);

        Collider_UpdateCylinder(&am->dyna.actor, &am->hurtCollider);
        Collider_UpdateCylinder(&am->dyna.actor, &am->blockCollider);

        RegisterColliderBase(play, &am->hurtCollider.base, m_hurtRoles);
        RegisterColliderBase(play, &am->blockCollider.base, m_blockRoles);
        if (m_hitRoles != 0)
            RegisterColliderBase(play, &am->hitCollider.base, m_hitRoles);
    }

  private:
    u8 m_hurtRoles = COLL_OC;
    u8 m_blockRoles = COLL_OC;
    u8 m_hitRoles = 0;
};

}

#endif
