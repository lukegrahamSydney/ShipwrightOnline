#ifndef IKCONTROLLERH
#define IKCONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ik/z_en_ik.h"
#include "objects/object_ik/object_ik.h"

void func_80A747C0(EnIk*, PlayState* play);
void func_80A7492C(EnIk*, PlayState* play);
void func_80A74BA4(EnIk*, PlayState* play);
void func_80A74EBC(EnIk*, PlayState* play);
void func_80A7510C(EnIk*, PlayState* play);
void func_80A75260(EnIk*, PlayState* play);
void func_80A7545C(EnIk*, PlayState* play);
void func_80A75530(EnIk*, PlayState* play);
void func_80A7567C(EnIk*, PlayState* play);
void func_80A758B0(EnIk*, PlayState* play);
void func_80A75A38(EnIk*, PlayState* play); //death

void func_80A7598C(EnIk*);  //Setup death
}

namespace ZeldaOnline {

class IkController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnIk* Typed() const {
        return reinterpret_cast<EnIk*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 14;

    using IkActionFunc = void (*)(EnIk*, PlayState*);
    static const IkActionFunc* ActionTable(size_t* count) {
        static const IkActionFunc sTable[] = {
            func_80A747C0, func_80A7492C, func_80A74BA4, func_80A74EBC, func_80A7510C, func_80A75260,
            func_80A7545C, func_80A75530, func_80A7567C, func_80A758B0, func_80A75A38,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    void OnActorInit() override {
        ReinstallUpdate();
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const IkActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gIronKnuckleWalkAnim,
            gIronKnuckleRunAnim,
            gIronKnuckleStandUpAnim,
            gIronKnuckleBlockAnim,
            gIronKnuckleVerticalAttackAnim,
            gIronKnuckleHorizontalAttackAnim,
            gIronKnuckleRecoverFromVerticalAttackAnim,
            gIronKnuckleRecoverFromHorizontalAttackAnim,
            gIronKnuckleFrontHitAnim,
            gIronKnuckleBackHitAnim,
            gIronKnuckleDeathAnim,
            gIronKnuckleAxeStuckAnim,
            gIronKnuckleNabooruSummonAxeAnim,
            gIronKnuckleNabooruDeathAnim,
        };
        if (i >= ANIM_COUNT)
            return nullptr;
        return sAnims[i];
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++)
            if (strcmp(AnimForIndex(i), cur) == 0)
                return i;
        return ANIM_UNKNOWN;
    }

    void CurrentColliderRoles(u8& body, u8& axe, u8& shield) const {
        EnIk* ik = Typed();

        body = COLL_OC;
        axe = 0;
        shield = 0;

        if ((ik->actor.colChkInfo.health > 0) && (ik->actor.colorFilterTimer == 0) && (ik->unk_2F8 >= 2))
            body |= COLL_AC;

        if (ik->unk_2FE > 0)
            axe |= COLL_AT;

        if (ik->unk_2F8 == 9)
            shield |= COLL_AC;
    }

    bool HitWouldReact() const {
        EnIk* ik = Typed();

        if ((ik->unk_2F8 == 3) || (ik->unk_2F8 == 2))
            return false;

        if (ik->shieldCollider.base.acFlags & AC_BOUNCED)
            return false;

        return (ik->bodyCollider.base.acFlags & AC_HIT) &&
               (ik->actor.colChkInfo.damageEffect != 0 || ik->actor.colChkInfo.damage != 0);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_MODE,
        PROP_STATE,
        PROP_HEALTH,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnIk* ik = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(
            PROP_MODE,
            ByteStream() << PackedInt4(ik->action) << PackedInt4(ik->drawMode) << PackedInt4(ik->isAxeSummoned), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(ik->unk_2F8) << PackedUInt1(ik->animationTimer)
                                  << PackedUInt1(ik->drawArmorFlag) << PackedUInt1(ik->armorStatusFlag)
                                  << PackedUInt1(ik->isBreakingProp) << PackedUInt1(ik->damageReaction)
                                  << PackedInt1(ik->unk_2FE) << PackedInt1(ik->unk_2FF) << PackedInt2(ik->unk_300),
                     out);
        PackProperty(PROP_HEALTH, PackedInt1(ik->actor.colChkInfo.health), out);

        {
            u8 body, axe, shield;
            CurrentColliderRoles(body, axe, shield);
            PackProperty(PROP_COLL_ROLES, ByteStream() << PackedUInt1(body) << PackedUInt1(axe) << PackedUInt1(shield),
                         out);
        }

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(ik->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &ik->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnIk* ik = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const IkActionFunc* table = ActionTable(&count);
                if (id < count)
                    ik->actionFunc = table[id];
                break;
            }
            case PROP_MODE:
                ik->action = (s32)(data.Read<PackedInt4>().value());
                ik->drawMode = (s32)(data.Read<PackedInt4>().value());
                ik->isAxeSummoned = (s32)(data.Read<PackedInt4>().value());
                break;
            case PROP_STATE:
                ik->unk_2F8 = (u8)(data.Read<PackedUInt1>().value());
                ik->animationTimer = (u8)(data.Read<PackedUInt1>().value());
                ik->drawArmorFlag = (u8)(data.Read<PackedUInt1>().value());
                ik->armorStatusFlag = (u8)(data.Read<PackedUInt1>().value());
                ik->isBreakingProp = (u8)(data.Read<PackedUInt1>().value());
                ik->damageReaction = (u8)(data.Read<PackedUInt1>().value());
                ik->unk_2FE = (s8)(data.Read<PackedInt1>().value());
                ik->unk_2FF = (s8)(data.Read<PackedInt1>().value());
                ik->unk_300 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                ik->actor.colChkInfo.health = (s8)(data.Read<PackedInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_axeRoles = (u8)(data.Read<PackedUInt1>().value());
                m_shieldRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                ik->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &ik->skelAnime, LOCK_CUR_FRAME ? ik->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnIk* ik = Typed();

        if (ik->armorStatusFlag != 0 && ik->bodyBreak.matrices == NULL) {
            BodyBreak_Alloc(&ik->bodyBreak, 3, gPlayState);
        }

        if (ik->actionFunc == func_80A75A38)
        {
            func_80A7598C(ik);
            GoLocal();
        }
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
        ReinstallUpdate();
    }

    void UpdatePuppet(PlayState* play) override {
        EnIk* ik = Typed();

        UpdateAnimation(&ik->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        ik->bodyCollider.base.acFlags &= ~AC_HIT;
        ik->shieldCollider.base.acFlags &= ~(AC_HIT | AC_BOUNCED);
        ik->axeCollider.base.atFlags &= ~AT_HIT;

        if (ik->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);


        ik->actor.focus.pos = ik->actor.world.pos;
        ik->actor.focus.pos.y += 45.0f;

        Collider_UpdateCylinder(&ik->actor, &ik->bodyCollider);

        if (m_bodyRoles != 0)
            RegisterColliderBase(play, &ik->bodyCollider.base, m_bodyRoles);
        if (m_axeRoles != 0)
            RegisterColliderBase(play, &ik->axeCollider.base, m_axeRoles);
        if (m_shieldRoles != 0)
            RegisterColliderBase(play, &ik->shieldCollider.base, m_shieldRoles);
    }

  private:
    u8 m_bodyRoles = 0;
    u8 m_axeRoles = 0;
    u8 m_shieldRoles = 0;
};

} // namespace ZeldaOnline

#endif