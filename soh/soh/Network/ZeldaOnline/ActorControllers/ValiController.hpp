#ifndef VALICONTROLLERH
#define VALICONTROLLERH

#include <cstring>
#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Vali/z_en_vali.h"
#include "objects/object_vali/object_vali.h"
void EnVali_Lurk(EnVali* vali, PlayState* play);
void EnVali_DropAppear(EnVali* vali, PlayState* play);
void EnVali_FloatIdle(EnVali* vali, PlayState* play);
void EnVali_Attacked(EnVali* vali, PlayState* play);
void EnVali_Retaliate(EnVali* vali, PlayState* play);
void EnVali_MoveArmsDown(EnVali* vali, PlayState* play);
void EnVali_Burnt(EnVali* vali, PlayState* play);
void EnVali_DivideAndDie(EnVali* vali, PlayState* play);
void EnVali_Stunned(EnVali* vali, PlayState* play);
void EnVali_Frozen(EnVali* vali, PlayState* play);
void EnVali_ReturnToLurk(EnVali* vali, PlayState* play);

void EnVali_DischargeLightning(EnVali* vali, PlayState* play);

void EnVali_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class ValiController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnVali* Typed() const {
        return reinterpret_cast<EnVali*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using ValiActionFunc = void (*)(EnVali*, PlayState*);
    static const ValiActionFunc* ActionTable(size_t* count) {
        static const ValiActionFunc sTable[] = {
            EnVali_Lurk,
            EnVali_DropAppear,
            EnVali_FloatIdle,
            EnVali_Attacked,
            EnVali_Retaliate,
            EnVali_MoveArmsDown,
            EnVali_Burnt,
            EnVali_DivideAndDie,
            EnVali_Stunned,
            EnVali_Frozen,
            EnVali_ReturnToLurk,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ValiActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_LURKING = 0;
    static constexpr u8 ANIM_WAITING = 1;
    static constexpr u8 ANIM_RETALIATING = 2;
    static constexpr u8 ANIM_MOVING_ARMS_DOWN = 3;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 4;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_LURKING:
                return gBariLurkingAnim;
            case ANIM_WAITING:
                return gBariWaitingAnim;
            case ANIM_RETALIATING:
                return gBariRetaliatingAnim;
            case ANIM_MOVING_ARMS_DOWN:
                return gBariMovingArmsDownAnim;
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

    void CurrentColliderRoles(u8* armRoles, u8* bodyRoles) const {
        EnVali* vali = Typed();
        *armRoles = 0;
        *bodyRoles = 0;

        if (vali->actionFunc == EnVali_Lurk || vali->actionFunc == EnVali_DivideAndDie)
            return;

        if (vali->actionFunc == EnVali_FloatIdle) {
            *armRoles |= COLL_AT;
            *bodyRoles |= COLL_AT;
        }
        if (vali->bodyCollider.base.acFlags & AC_ON)
            *bodyRoles |= COLL_AC;
        *bodyRoles |= COLL_OC;
    }

    bool HitWouldReact() const {
        EnVali* vali = Typed();
        if (!(vali->bodyCollider.base.acFlags & AC_HIT))
            return false;
        return vali->actor.colChkInfo.damageEffect != 0 || vali->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_PARAMS,
        PROP_TIMERS,
        PROP_FLOAT_HEIGHT,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnVali* vali = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(vali->actor.colChkInfo.health), out);
        PackProperty(PROP_PARAMS, PackedInt2(vali->actor.params), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(vali->timer) << PackedUInt1(vali->lightningTimer)
                                  << PackedUInt1(vali->slingshotReactionTimer),
                     out);
        PackProperty(PROP_FLOAT_HEIGHT, PackedFloat4(vali->floatHomeHeight), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        u8 armRoles, bodyRoles;
        CurrentColliderRoles(&armRoles, &bodyRoles);
        PackProperty(PROP_COLL_ROLES, ByteStream() << PackedUInt1(armRoles) << PackedUInt1(bodyRoles), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(vali->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &vali->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnVali* vali = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const ValiActionFunc* table = ActionTable(&count);
                if (id < count)
                    vali->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                vali->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PARAMS:
                vali->actor.params = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                vali->timer = (s16)(data.Read<PackedInt2>().value());
                vali->lightningTimer = (u8)(data.Read<PackedUInt1>().value());
                vali->slingshotReactionTimer = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FLOAT_HEIGHT:
                vali->floatHomeHeight = data.Read<PackedFloat4>().value();
                break;

            case PROP_COLL_ROLES:
                m_armRoles = (u8)(data.Read<PackedUInt1>().value());
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                vali->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &vali->skelAnime, LOCK_CUR_FRAME ? vali->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void EnsureDrawState() {
        EnVali* vali = Typed();
        bool shouldDraw = (vali->actionFunc != EnVali_Lurk) && (vali->actionFunc != EnVali_DivideAndDie);
        vali->actor.draw = shouldDraw ? EnVali_Draw : nullptr;
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawState();
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnVali* vali = Typed();

        UpdateAnimation(&vali->skelAnime, LOCK_CUR_FRAME);

        EnsureDrawState();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        vali->bodyCollider.base.acFlags &= ~AC_HIT;
        vali->leftArmCollider.base.atFlags &= ~AT_HIT;
        vali->rightArmCollider.base.atFlags &= ~AT_HIT;
        vali->bodyCollider.base.atFlags &= ~AT_HIT;

        if (vali->actionFunc != EnVali_DivideAndDie && vali->actionFunc != EnVali_Frozen &&
            vali->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        if (vali->actionFunc != EnVali_DivideAndDie && vali->actionFunc != EnVali_Lurk) {
            Collider_UpdateCylinder(&vali->actor, &vali->bodyCollider);

            if (m_armRoles != 0) {
                RegisterColliderBase(play, &vali->leftArmCollider.base, m_armRoles);
                RegisterColliderBase(play, &vali->rightArmCollider.base, m_armRoles);
            }
            if (m_bodyRoles != 0)
                RegisterColliderBase(play, &vali->bodyCollider.base, m_bodyRoles);

            Actor_SetFocus(&vali->actor, 0.0f);
        }

        if (vali->actionFunc == EnVali_Attacked && vali->lightningTimer != 0)
            EnVali_DischargeLightning(vali, play);

        if (vali->actionFunc == EnVali_DivideAndDie) {
            static Vec3f sZeroVel = { 0.0f, 0.0f, 0.0f };
            static Vec3f sZeroAccel = { 0.0f, 0.0f, 0.0f };
            for (int i = 0; i < 2; i++) {
                Vec3f pos;
                Vec3f velocity = sZeroVel;
                pos.x = vali->actor.world.pos.x + Rand_CenteredFloat(20.0f);
                pos.y = vali->actor.world.pos.y + Rand_CenteredFloat(8.0f);
                pos.z = vali->actor.world.pos.z + Rand_CenteredFloat(20.0f);
                velocity.y = (Rand_ZeroOne() + 1.0f);
                s16 scale = Rand_S16Offset(40, 40);
                EffectSsDtBubble_SpawnColorProfile(play, &pos, &velocity, &sZeroAccel, scale, 25,
                                                   (Rand_ZeroOne() < 0.7f) ? 2 : 0, 1);
            }
        }
    }

  private:
    u8 m_armRoles = 0;
    u8 m_bodyRoles = COLL_OC | COLL_AC;
};

}

#endif
