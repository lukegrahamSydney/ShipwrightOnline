#ifndef WOLFOSCONTROLLERH
#define WOLFOSCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "objects/object_wf/object_wf.h"

void EnWf_WaitToAppear(EnWf* wf, PlayState* play);
void EnWf_Wait(EnWf* wf, PlayState* play);
void EnWf_RunAtPlayer(EnWf* wf, PlayState* play);
void EnWf_SearchForPlayer(EnWf* wf, PlayState* play);
void EnWf_RunAroundPlayer(EnWf* wf, PlayState* play);
void EnWf_Slash(EnWf* wf, PlayState* play);
void EnWf_RecoilFromBlockedSlash(EnWf* wf, PlayState* play);
void EnWf_BackflipAway(EnWf* wf, PlayState* play);
void EnWf_Stunned(EnWf* wf, PlayState* play);
void EnWf_Damaged(EnWf* wf, PlayState* play);
void EnWf_SomersaultAndAttack(EnWf* wf, PlayState* play);
void EnWf_Blocking(EnWf* wf, PlayState* play);
void EnWf_Sidestep(EnWf* wf, PlayState* play);
void EnWf_Die(EnWf* wf, PlayState* play);

void EnWf_SetupDie(EnWf* wf);
}

static constexpr s32 kEnWfDmgEffIceMagic = 6;

namespace ZeldaOnline {

class WolfosController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnWf* Typed() const {
        return reinterpret_cast<EnWf*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using WfActionFunc = void (*)(EnWf*, PlayState*);
    static const WfActionFunc* ActionTable(size_t* count) {
        static const WfActionFunc sTable[] = {
            EnWf_WaitToAppear,
            EnWf_Wait,
            EnWf_RunAtPlayer,
            EnWf_SearchForPlayer,
            EnWf_RunAroundPlayer,
            EnWf_Slash,
            EnWf_RecoilFromBlockedSlash,
            EnWf_BackflipAway,
            EnWf_Stunned,
            EnWf_Damaged,
            EnWf_SomersaultAndAttack,
            EnWf_Blocking,
            EnWf_Sidestep,
            EnWf_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const WfActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_REARING = 0;
    static constexpr u8 ANIM_WAITING = 1;
    static constexpr u8 ANIM_RUNNING = 2;
    static constexpr u8 ANIM_SIDESTEPPING = 3;
    static constexpr u8 ANIM_SLASHING = 4;
    static constexpr u8 ANIM_BACKFLIPPING = 5;
    static constexpr u8 ANIM_DAMAGED = 6;
    static constexpr u8 ANIM_BLOCKING = 7;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 8;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_REARING:
                return gWolfosRearingUpFallingOverAnim;
            case ANIM_WAITING:
                return gWolfosWaitingAnim;
            case ANIM_RUNNING:
                return gWolfosRunningAnim;
            case ANIM_SIDESTEPPING:
                return gWolfosSidesteppingAnim;
            case ANIM_SLASHING:
                return gWolfosSlashingAnim;
            case ANIM_BACKFLIPPING:
                return gWolfosBackflippingAnim;
            case ANIM_DAMAGED:
                return gWolfosDamagedAnim;
            case ANIM_BLOCKING:
                return gWolfosBlockingAnim;
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

    bool HitWouldReact() const {
        EnWf* wf = Typed();
        if (wf->colliderSpheres.base.acFlags & AC_BOUNCED)
            return true;
        if (!(wf->colliderCylinderBody.base.acFlags & AC_HIT) && !(wf->colliderCylinderTail.base.acFlags & AC_HIT))
            return false;
        if (wf->action < WOLFOS_ACTION_WAIT)
            return false;
        return wf->actor.colChkInfo.damageEffect != kEnWfDmgEffIceMagic;
    }

    u8 CurrentSphereRoles() const {
        EnWf* wf = Typed();
        u8 roles = COLL_OC;
        if (wf->action == WOLFOS_ACTION_BLOCKING)
            roles |= COLL_AC;
        if (wf->slashStatus > 0 && !(wf->colliderSpheres.base.atFlags & AT_BOUNCED))
            roles |= COLL_AT;
        return roles;
    }
    u8 CurrentBodyTailRoles() const {
        EnWf* wf = Typed();
        if (wf->action < WOLFOS_ACTION_WAIT)
            return 0;
        bool invulnerable = wf->actor.colorFilterTimer != 0 && (wf->actor.colorFilterParams & 0x4000);
        return invulnerable ? 0 : COLL_AC;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_HEALTH,
        PROP_BATTERY,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnWf* wf = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, PackedUInt1((u8)(wf->action)), out);
        PackProperty(PROP_HEALTH, PackedUInt1(wf->actor.colChkInfo.health), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_BATTERY,
                     ByteStream() << PackedInt2(wf->fireTimer) << PackedInt2(wf->slashStatus)
                                  << PackedUInt1((u8)(wf->switchFlag)) << PackedInt2(wf->runAngle)
                                  << PackedInt4(wf->actionTimer),
                     out);
        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentSphereRoles()) << PackedUInt1(CurrentBodyTailRoles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(wf->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &wf->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnWf* wf = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const WfActionFunc* table = ActionTable(&count);
                if (id < count)
                    wf->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                wf->action = (s32)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEALTH:
                wf->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_BATTERY:
                wf->fireTimer = (s16)(data.Read<PackedInt2>().value());
                wf->slashStatus = (s16)(data.Read<PackedInt2>().value());
                wf->switchFlag = (s16)(data.Read<PackedUInt1>().value());
                wf->runAngle = (s16)(data.Read<PackedInt2>().value());
                wf->actionTimer = data.Read<PackedInt4>().value();
                break;
            case PROP_COLL_ROLES:
                m_sphereRoles = (u8)(data.Read<PackedUInt1>().value());
                m_bodyTailRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                wf->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &wf->skelAnime, LOCK_CUR_FRAME ? wf->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    bool OtherLiveWolfNearby(PlayState* play) const {
        EnWf* wf = Typed();
        Actor* other = play->actorCtx.actorLists[ACTORCAT_ENEMY].head;
        for (; other != nullptr; other = other->next) {
            if (other == &wf->actor || other->id != ACTOR_EN_WF)
                continue;
            EnWf* sibling = reinterpret_cast<EnWf*>(other);
            if (sibling->actor.colChkInfo.health == 0)
                continue;
            if (Math_Vec3f_DistXYZ(&wf->actor.world.pos, &other->world.pos) < 500.0f)
                return true;
        }
        return false;
    }

    void UpdateLeader(PlayState* play) override {
        if (!OtherLiveWolfNearby(play))
            Typed()->actor.isTargeted = true;
        AbstractActorController::UpdateLeader(play);
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnWf_Die) {
            EnWf_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnWf* wf = Typed();

        UpdateAnimation(&wf->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        wf->colliderSpheres.base.acFlags &= ~(AC_HIT | AC_BOUNCED);
        wf->colliderCylinderBody.base.acFlags &= ~AC_HIT;
        wf->colliderCylinderTail.base.acFlags &= ~AC_HIT;
        wf->colliderSpheres.base.atFlags &= ~(AT_HIT | AT_BOUNCED);

        if (wf->actionFunc != EnWf_Die && wf->actor.colChkInfo.health > 0 && wf->actor.xzDistToPlayer < 300.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        if (wf->eyeIndex == 0) {
            if ((Rand_ZeroOne() < 0.2f) && ((play->gameplayFrames % 4) == 0) && (wf->actor.colorFilterTimer == 0))
                wf->eyeIndex++;
        } else {
            wf->eyeIndex = (wf->eyeIndex + 1) & 3;
        }

        wf->actor.focus.pos = wf->actor.world.pos;
        wf->actor.focus.pos.y += 25.0f;

        Collider_UpdateCylinder(&wf->actor, &wf->colliderCylinderBody);

        RegisterColliderBase(play, &wf->colliderSpheres.base, m_sphereRoles);
        if (m_bodyTailRoles != 0) {
            RegisterColliderBase(play, &wf->colliderCylinderBody.base, m_bodyTailRoles);
            RegisterColliderBase(play, &wf->colliderCylinderTail.base, m_bodyTailRoles);
        }
    }

  private:
    u8 m_sphereRoles = COLL_OC;
    u8 m_bodyTailRoles = COLL_AC;
};

}

#endif
