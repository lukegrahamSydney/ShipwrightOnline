#ifndef BILICONTROLLERH
#define BILICONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bili/z_en_bili.h"
#include "objects/object_bl/object_bl.h"

void EnBili_FloatIdle(EnBili* bi, PlayState* play);
void EnBili_SpawnedFlyApart(EnBili* bi, PlayState* play);
void EnBili_DischargeLightning(EnBili* bi, PlayState* play);
void EnBili_Climb(EnBili* bi, PlayState* play);
void EnBili_ApproachPlayer(EnBili* bi, PlayState* play);
void EnBili_SetNewHomeHeight(EnBili* bi, PlayState* play);
void EnBili_Recoil(EnBili* bi, PlayState* play);
void EnBili_Burnt(EnBili* bi, PlayState* play);
void EnBili_Die(EnBili* bi, PlayState* play);
void EnBili_Stunned(EnBili* bi, PlayState* play);
void EnBili_Frozen(EnBili* bi, PlayState* play);

void EnBili_SetupDie(EnBili* bi);

void EnBili_UpdateTentaclesIndex(EnBili* bi);
}

namespace ZeldaOnline {

class BiliController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBili* Typed() const {
        return reinterpret_cast<EnBili*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using BiliActionFunc = void (*)(EnBili*, PlayState*);
    static const BiliActionFunc* ActionTable(size_t* count) {
        static const BiliActionFunc sTable[] = {
            EnBili_FloatIdle,
            EnBili_SpawnedFlyApart,
            EnBili_DischargeLightning,
            EnBili_Climb,
            EnBili_ApproachPlayer,
            EnBili_SetNewHomeHeight,
            EnBili_Recoil,
            EnBili_Burnt,
            EnBili_Die,
            EnBili_Stunned,
            EnBili_Frozen,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BiliActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_DEFAULT = 0;
    static constexpr u8 ANIM_DISCHARGE = 1;
    static constexpr u8 ANIM_CLIMB = 2;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 3;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_DEFAULT:
                return gBiriDefaultAnim;
            case ANIM_DISCHARGE:
                return gBiriDischargeLightningAnim;
            case ANIM_CLIMB:
                return gBiriClimbAnim;
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
        EnBili* bi = Typed();
        if (!(bi->collider.base.acFlags & AC_HIT) || bi->actor.colChkInfo.health == 0)
            return false;
        return bi->actor.colChkInfo.damageEffect != 0 || bi->actor.colChkInfo.damage != 0;
    }

    u8 CurrentColliderRoles() const {
        EnBili* bi = Typed();
        u8 roles = COLL_OC;
        if (bi->collider.base.atFlags & AT_ON)
            roles |= COLL_AT;
        if (bi->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_BATTERY,
        PROP_HOME_HEIGHT,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBili* bi = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(bi->actor.colChkInfo.health), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_BATTERY, ByteStream() << PackedInt2(bi->timer) << PackedUInt1(bi->playFlySound ? 1u : 0u),
                     out);
        PackProperty(PROP_HOME_HEIGHT, PackedFloat4(bi->actor.home.pos.y), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(bi->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &bi->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBili* bi = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BiliActionFunc* table = ActionTable(&count);
                if (id < count)
                    bi->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                bi->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_BATTERY:
                bi->timer = (s16)(data.Read<PackedInt2>().value());
                bi->playFlySound = data.Read<PackedUInt1>().value() != 0;
                break;
            case PROP_HOME_HEIGHT:
                bi->actor.home.pos.y = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                bi->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &bi->skelAnime, LOCK_CUR_FRAME ? bi->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnBili_Die) {
            EnBili_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnBili* bi = Typed();

        UpdateAnimation(&bi->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        bi->collider.base.acFlags &= ~AC_HIT;
        bi->collider.base.atFlags &= ~AT_HIT;

        if (bi->actionFunc != EnBili_Die && bi->actor.colChkInfo.health > 0 && bi->actor.xzDistToPlayer < 300.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        if (bi->actionFunc == EnBili_Die)
            return;

        EnBili_UpdateTentaclesIndex(bi);

        Actor_SetFocus(&bi->actor, 0.0f);

        Vec3f rayOrigin = bi->actor.world.pos;
        rayOrigin.y += 5.0f;
        s32 floorBgId = BGCHECK_SCENE;
        bi->actor.floorHeight =
            BgCheck_EntityRaycastFloor5(play, &play->colCtx, &bi->actor.floorPoly, &floorBgId, &bi->actor, &rayOrigin);
        bi->actor.floorBgId = floorBgId;

        Collider_UpdateCylinder(&bi->actor, &bi->collider);
        RegisterColliderBase(play, &bi->collider.base, m_roles);
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
