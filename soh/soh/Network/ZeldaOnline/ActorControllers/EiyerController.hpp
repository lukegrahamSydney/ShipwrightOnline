#ifndef EIYERCONTROLLERH
#define EIYERCONTROLLERH

#include <cstring>
#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Eiyer/z_en_eiyer.h"
#include "objects/object_ei/object_ei.h"

void EnEiyer_AppearFromGround(EnEiyer* ei, PlayState* play);
void EnEiyer_WanderUnderground(EnEiyer* ei, PlayState* play);
void EnEiyer_CircleUnderground(EnEiyer* ei, PlayState* play);
void EnEiyer_Inactive(EnEiyer* ei, PlayState* play);
void EnEiyer_Ambush(EnEiyer* ei, PlayState* play);
void EnEiyer_Glide(EnEiyer* ei, PlayState* play);
void EnEiyer_StartAttack(EnEiyer* ei, PlayState* play);
void EnEiyer_DiveAttack(EnEiyer* ei, PlayState* play);
void EnEiyer_Land(EnEiyer* ei, PlayState* play);
void EnEiyer_Hurt(EnEiyer* ei, PlayState* play);
void EnEiyer_Die(EnEiyer* ei, PlayState* play);
void EnEiyer_Dead(EnEiyer* ei, PlayState* play);
void EnEiyer_Stunned(EnEiyer* ei, PlayState* play);

void EnEiyer_SetupDie(EnEiyer* ei);
}

namespace ZeldaOnline {

class EiyerController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnEiyer* Typed() const {
        return reinterpret_cast<EnEiyer*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr s16 EIYER_MAIN = 0;
    static constexpr s16 EIYER_LONER = 0xA;
    static constexpr u32 DMGFLAGS_UNDERGROUND = 0x19;

    using EiyerActionFunc = void (*)(EnEiyer*, PlayState*);
    static const EiyerActionFunc* ActionTable(size_t* count) {
        static const EiyerActionFunc sTable[] = {
            EnEiyer_AppearFromGround,
            EnEiyer_WanderUnderground,
            EnEiyer_CircleUnderground,
            EnEiyer_Inactive,
            EnEiyer_Ambush,
            EnEiyer_Glide,
            EnEiyer_StartAttack,
            EnEiyer_DiveAttack,
            EnEiyer_Land,
            EnEiyer_Hurt,
            EnEiyer_Die,
            EnEiyer_Dead,
            EnEiyer_Stunned,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const EiyerActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_IDLE = 0;
    static constexpr u8 ANIM_BACKFLIP = 1;
    static constexpr u8 ANIM_HIT = 2;
    static constexpr u8 ANIM_DIVE = 3;
    static constexpr u8 ANIM_POP_OUT = 4;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 5;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_IDLE:
                return gStingerIdleAnim;
            case ANIM_BACKFLIP:
                return gStingerBackflipAnim;
            case ANIM_HIT:
                return gStingerHitAnim;
            case ANIM_DIVE:
                return gStingerDiveAnim;
            case ANIM_POP_OUT:
                return gStingerPopOutAnim;
            default:
                return nullptr;
        }
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelanime.animation;
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
        EnEiyer* ei = Typed();
        u8 roles = 0;
        if (ei->collider.base.atFlags & AT_ON)
            roles |= COLL_AT;
        if (ei->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        if (ei->actionFunc != EnEiyer_Ambush)
            roles |= COLL_OC;
        return roles;
    }

    bool HitWouldReact() const {
        EnEiyer* ei = Typed();
        if (!(ei->collider.base.acFlags & AC_HIT))
            return false;
        return ei->actor.colChkInfo.damageEffect != 0 || ei->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_TIMER,
        PROP_TARGET_YAW,
        PROP_BASE_POS,
        PROP_DMG_FLAGS,
        PROP_COLL_HEIGHT,
        PROP_SHADOW,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnEiyer* ei = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(ei->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMER, PackedInt2(ei->timer), out);
        PackProperty(PROP_TARGET_YAW, PackedInt2(ei->targetYaw), out);
        PackProperty(PROP_BASE_POS,
                     ByteStream() << PackedFloat4(ei->basePos.x) << PackedFloat4(ei->basePos.y)
                                  << PackedFloat4(ei->basePos.z),
                     out);
        PackProperty(PROP_DMG_FLAGS, PackedUInt4(ei->collider.info.bumper.dmgFlags), out);
        PackProperty(PROP_COLL_HEIGHT, PackedInt2(ei->collider.dim.height), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(
            PROP_SHADOW,
            ByteStream() << PackedUInt1(ei->actor.shape.shadowAlpha) << PackedFloat4(ei->actor.shape.shadowScale), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(ei->skelanime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &ei->skelanime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnEiyer* ei = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const EiyerActionFunc* table = ActionTable(&count);
                if (id < count)
                    ei->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                ei->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMER:
                ei->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TARGET_YAW:
                ei->targetYaw = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BASE_POS:
                ei->basePos.x = data.Read<PackedFloat4>().value();
                ei->basePos.y = data.Read<PackedFloat4>().value();
                ei->basePos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_DMG_FLAGS:
                ei->collider.info.bumper.dmgFlags = data.Read<PackedUInt4>().value();
                break;
            case PROP_COLL_HEIGHT:
                ei->collider.dim.height = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SHADOW:
                ei->actor.shape.shadowAlpha = (u8)(data.Read<PackedUInt1>().value());
                ei->actor.shape.shadowScale = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                ei->skelanime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &ei->skelanime, LOCK_CUR_FRAME ? ei->skelanime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnEiyer_Die) {
            EnEiyer_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnEiyer* ei = Typed();

        UpdateAnimation(&ei->skelanime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        ei->collider.base.acFlags &= ~AC_HIT;
        ei->collider.base.atFlags &= ~AT_HIT;

        if (ei->actionFunc != EnEiyer_Die && ei->actionFunc != EnEiyer_Dead && ei->actor.xzDistToPlayer < 300.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        {
            Vec3f rayOrigin = ei->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            ei->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &ei->actor.floorPoly, &floorBgId,
                                                                &ei->actor, &rayOrigin);
            ei->actor.floorBgId = floorBgId;
        }

        if (m_roles != 0) {
            Collider_UpdateCylinder(&ei->actor, &ei->collider);
            RegisterColliderBase(play, &ei->collider.base, m_roles);
        }

        if (ei->actor.flags & ACTOR_FLAG_ATTENTION_ENABLED) {
            ei->actor.focus.pos.x = ei->actor.world.pos.x + Math_SinS(ei->actor.shape.rot.y) * 12.5f;
            ei->actor.focus.pos.z = ei->actor.world.pos.z + Math_CosS(ei->actor.shape.rot.y) * 12.5f;
            ei->actor.focus.pos.y = ei->actor.world.pos.y;
        }
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
