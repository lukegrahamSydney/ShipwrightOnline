#ifndef STINGERCONTROLLERH
#define STINGERCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Weiyer/z_en_weiyer.h"
#include "objects/object_ei/object_ei.h"

void EnWeiyer_InitInsideWaterBox(EnWeiyer* wy, PlayState* play);
void EnWeiyer_FreeSwim(EnWeiyer* wy, PlayState* play);
void EnWeiyer_TurnAround(EnWeiyer* wy, PlayState* play);
void EnWeiyer_StuckOnFloor(EnWeiyer* wy, PlayState* play);
void EnWeiyer_Attack(EnWeiyer* wy, PlayState* play);
void EnWeiyer_Inactive(EnWeiyer* wy, PlayState* play);
void EnWeiyer_Hurt(EnWeiyer* wy, PlayState* play);
void EnWeiyer_Stunned(EnWeiyer* wy, PlayState* play);
void EnWeiyer_Die(EnWeiyer* wy, PlayState* play);
void EnWeiyer_Dead(EnWeiyer* wy, PlayState* play);
void EnWeiyer_OutOfWater(EnWeiyer* wy, PlayState* play);

void EnWeiyer_SetupDie(EnWeiyer* wy);
}

namespace ZeldaOnline {

class StingerController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnWeiyer* Typed() const {
        return reinterpret_cast<EnWeiyer*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using WeiyerActionFunc = decltype(&EnWeiyer_FreeSwim);
    static const WeiyerActionFunc* ActionTable(size_t* count) {
        static const WeiyerActionFunc sTable[] = {
            EnWeiyer_InitInsideWaterBox, EnWeiyer_FreeSwim, EnWeiyer_TurnAround, EnWeiyer_StuckOnFloor,
            EnWeiyer_Attack,             EnWeiyer_Inactive, EnWeiyer_Hurt,       EnWeiyer_Stunned,
            EnWeiyer_Die,                EnWeiyer_Dead,     EnWeiyer_OutOfWater,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const WeiyerActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 3;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0: return gStingerIdleAnim;
            case 1: return gStingerPopOutAnim;
            case 2: return gStingerHitAnim;
            default: return nullptr;
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

    u8 CurrentColliderRoles() const {
        EnWeiyer* wy = Typed();
        u8 roles = COLL_OC;
        if (wy->collider.base.atFlags & AT_ON)
            roles |= COLL_AT;
        if (wy->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        return roles;
    }

    bool HitWouldReact() const {
        EnWeiyer* wy = Typed();
        if (!(wy->collider.base.acFlags & AC_HIT))
            return false;
        return wy->actor.colChkInfo.damageEffect != 0 || wy->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_TIMER,
        PROP_TARGET_YAW,
        PROP_SWIM_HEIGHT,
        PROP_DMG_FLAGS,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnWeiyer* wy = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(wy->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMER, PackedInt2(wy->timer), out);
        PackProperty(PROP_TARGET_YAW, PackedInt2(wy->targetYaw), out);


        PackProperty(PROP_SWIM_HEIGHT,
                     ByteStream() << PackedFloat4(wy->swimHeight) << PackedFloat4(wy->targetSwimHeight),
                     out);

        PackProperty(PROP_DMG_FLAGS, PackedUInt4(wy->collider.info.bumper.dmgFlags), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(wy->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &wy->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnWeiyer* wy = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const WeiyerActionFunc* table = ActionTable(&count);
                if (id < count)
                    wy->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                wy->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMER:
                wy->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TARGET_YAW:
                wy->targetYaw = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SWIM_HEIGHT:
                wy->swimHeight = data.Read<PackedFloat4>().value();
                wy->targetSwimHeight = data.Read<PackedFloat4>().value();
                break;
            case PROP_DMG_FLAGS:
                wy->collider.info.bumper.dmgFlags = data.Read<PackedUInt4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                wy->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &wy->skelAnime, LOCK_CUR_FRAME ? wy->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnWeiyer_Die) {
            EnWeiyer_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnWeiyer* wy = Typed();

        UpdateAnimation(&wy->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        wy->collider.base.acFlags &= ~AC_HIT;
        wy->collider.base.atFlags &= ~AT_HIT;

        if (wy->actionFunc != EnWeiyer_Die && wy->actionFunc != EnWeiyer_Dead &&
            wy->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        wy->actor.home.pos.y = wy->actor.yDistToWater + wy->actor.world.pos.y - 5.0f;

        Actor_SetFocus(&wy->actor, 0.0f);

        if (m_roles != 0) {
            Collider_UpdateCylinder(&wy->actor, &wy->collider);
            RegisterColliderBase(play, &wy->collider.base, m_roles);
        }
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
