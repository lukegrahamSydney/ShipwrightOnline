#ifndef STALCHILDCONTROLLERH
#define STALCHILDCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
#include "objects/object_skb/object_skb.h"

void func_80AFCE5C(EnSkb* skb, PlayState* play);
void func_80AFCFF0(EnSkb* skb, PlayState* play);
void EnSkb_Advance(EnSkb* skb, PlayState* play);
void EnSkb_SetupAttack(EnSkb* skb, PlayState* play);
void func_80AFD508(EnSkb* skb, PlayState* play);
void func_80AFD59C(EnSkb* skb, PlayState* play);
void func_80AFD6CC(EnSkb* skb, PlayState* play);
void func_80AFD880(EnSkb* skb, PlayState* play);

void func_80AFD7B4(EnSkb* skb, PlayState* play);
}

namespace ZeldaOnline {

class StalchildController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnSkb* Typed() const {
        return reinterpret_cast<EnSkb*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 STATE_DYING = 1;
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using SkbActionFunc = void (*)(EnSkb*, PlayState*);
    static const SkbActionFunc* ActionTable(size_t* count) {
        static const SkbActionFunc sTable[] = {
            func_80AFCE5C,
            func_80AFCFF0,
            EnSkb_Advance,
            EnSkb_SetupAttack,
            func_80AFD508,
            func_80AFD59C,
            func_80AFD6CC,
            func_80AFD880,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const SkbActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        EnSkb* skb = Typed();
        u8 roles = COLL_OC;
        if (skb->setColliderAT != 0)
            roles |= COLL_AT;
        if (skb->actionState >= 3 && (skb->actor.colorFilterTimer == 0 || (skb->actor.colorFilterParams & 0x4000) == 0))
            roles |= COLL_AC;
        return roles;
    }

    static constexpr u8 ANIM_UNCURL = 0;
    static constexpr u8 ANIM_WALK = 1;
    static constexpr u8 ANIM_ATTACK = 2;
    static constexpr u8 ANIM_DAMAGED = 3;
    static constexpr u8 ANIM_DYING = 4;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static AnimationHeader* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_UNCURL:
                return (AnimationHeader*)&gStalchildUncurlingAnim;
            case ANIM_WALK:
                return (AnimationHeader*)&gStalchildWalkingAnim;
            case ANIM_ATTACK:
                return (AnimationHeader*)&gStalchildAttackingAnim;
            case ANIM_DAMAGED:
                return (AnimationHeader*)&gStalchildDamagedAnim;
            case ANIM_DYING:
                return (AnimationHeader*)&gStalchildDyingAnim;
            default:
                return nullptr;
        }
    }
    u8 CurrentAnimIndex() const {
        const char* a = static_cast<const char*>(Typed()->skelAnime.animation);
        if (a == nullptr)
            return ANIM_UNKNOWN;
        if (strcmp(a, gStalchildUncurlingAnim) == 0)
            return ANIM_UNCURL;
        if (strcmp(a, gStalchildWalkingAnim) == 0)
            return ANIM_WALK;
        if (strcmp(a, gStalchildAttackingAnim) == 0)
            return ANIM_ATTACK;
        if (strcmp(a, gStalchildDamagedAnim) == 0)
            return ANIM_DAMAGED;
        if (strcmp(a, gStalchildDyingAnim) == 0)
            return ANIM_DYING;
        return ANIM_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ACTION_STATE,
        PROP_HEALTH,
        PROP_BREAK_FLAGS,
        PROP_HEADLESS_YAW,
        PROP_SET_COLLIDER_AT,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    bool AnyAcHit() const {
        return (Typed()->collider.base.acFlags & AC_HIT) != 0;
    }
    void ClearHitFlags() {
        EnSkb* skb = Typed();
        skb->collider.base.acFlags &= ~AC_HIT;
        skb->collider.base.atFlags &= ~AT_HIT;
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnSkb* skb = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ACTION_STATE, PackedUInt1(skb->actionState), out);
        PackProperty(PROP_HEALTH, PackedUInt1(skb->actor.colChkInfo.health), out);
        PackProperty(PROP_BREAK_FLAGS, PackedUInt1(skb->breakFlags), out);
        PackProperty(PROP_HEADLESS_YAW, PackedInt2(skb->headlessYawOffset), out);
        PackProperty(PROP_SET_COLLIDER_AT, PackedUInt1(skb->setColliderAT), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(skb->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &skb->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnSkb* skb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const SkbActionFunc* table = ActionTable(&count);
                if (id < count)
                    skb->actionFunc = table[id];
                break;
            }
            case PROP_ACTION_STATE:
                skb->actionState = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEALTH:
                skb->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_BREAK_FLAGS:
                skb->breakFlags = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEADLESS_YAW:
                skb->headlessYawOffset = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SET_COLLIDER_AT:
                skb->setColliderAT = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                skb->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                AnimationHeader* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &skb->skelAnime, LOCK_CUR_FRAME ? skb->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void OnBecomeLeader() override {
        size_t count;
        const SkbActionFunc* table = ActionTable(&count);
        if (m_currentActionIndex < count)
            Typed()->actionFunc = table[m_currentActionIndex];
    }

    void OnPropertiesApplied(u64 changed) override {
        EnSkb* skb = Typed();

        if (skb->actionState == STATE_DYING) {
            func_80AFD7B4(skb, gPlayState);
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnSkb* skb = Typed();

        if (AnyAcHit()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        ClearHitFlags();

        u8 st = skb->actionState;
        bool active = (st == 0 || st == 4);
        if (active && skb->actor.xzDistToPlayer < 500.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        UpdateAnimation(&skb->skelAnime, LOCK_CUR_FRAME);

        skb->actor.focus.pos = skb->actor.world.pos;
        skb->actor.focus.pos.y += (3000.0f * skb->actor.scale.y);

        RegisterColliderBase(play, &skb->collider.base, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_OC;
};

}

#endif
