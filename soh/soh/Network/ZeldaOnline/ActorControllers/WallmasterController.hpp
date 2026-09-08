#ifndef WALLMASTERCONTROLLERH
#define WALLMASTERCONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Wallmas/z_en_wallmas.h"
#include "assets/objects/object_wallmaster/object_wallmaster.h"

void EnWallmas_WaitToDrop(EnWallmas* wm, PlayState* play);
void EnWallmas_Drop(EnWallmas* wm, PlayState* play);
void EnWallmas_Land(EnWallmas* wm, PlayState* play);
void EnWallmas_Stand(EnWallmas* wm, PlayState* play);
void EnWallmas_Walk(EnWallmas* wm, PlayState* play);
void EnWallmas_JumpToCeiling(EnWallmas* wm, PlayState* play);
void EnWallmas_ReturnToCeiling(EnWallmas* wm, PlayState* play);
void EnWallmas_TakeDamage(EnWallmas* wm, PlayState* play);
void EnWallmas_Cooldown(EnWallmas* wm, PlayState* play);
void EnWallmas_Die(EnWallmas* wm, PlayState* play);
void EnWallmas_TakePlayer(EnWallmas* wm, PlayState* play);
void EnWallmas_WaitForProximity(EnWallmas* wm, PlayState* play);
void EnWallmas_WaitForSwitchFlag(EnWallmas* wm, PlayState* play);
void EnWallmas_Stun(EnWallmas* wm, PlayState* play);

void EnWallmas_SetupReturnToCeiling(EnWallmas* wm);
void EnWallmas_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class WallmasterController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnWallmas* Typed() const {
        return reinterpret_cast<EnWallmas*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using WmActionFunc = void (*)(EnWallmas*, PlayState*);
    static const WmActionFunc* ActionTable(size_t* count) {
        static const WmActionFunc sTable[] = {
            EnWallmas_WaitToDrop,
            EnWallmas_Drop,
            EnWallmas_Land,
            EnWallmas_Stand,
            EnWallmas_Walk,
            EnWallmas_JumpToCeiling,
            EnWallmas_ReturnToCeiling,
            EnWallmas_TakeDamage,
            EnWallmas_Cooldown,
            EnWallmas_Die,
            EnWallmas_TakePlayer,
            EnWallmas_WaitForProximity,
            EnWallmas_WaitForSwitchFlag,
            EnWallmas_Stun,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const WmActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_LUNGE = 0;
    static constexpr u8 ANIM_HOVER = 1;
    static constexpr u8 ANIM_WALK = 2;
    static constexpr u8 ANIM_JUMP = 3;
    static constexpr u8 ANIM_WAIT = 4;
    static constexpr u8 ANIM_DAMAGE = 5;
    static constexpr u8 ANIM_RECOVER = 6;
    static constexpr u8 ANIM_STAND_UP = 7;
    static constexpr u8 ANIM_STOP_WALK = 8;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 9;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_LUNGE:
                return gWallmasterLungeAnim;
            case ANIM_HOVER:
                return gWallmasterHoverAnim;
            case ANIM_WALK:
                return gWallmasterWalkAnim;
            case ANIM_JUMP:
                return gWallmasterJumpAnim;
            case ANIM_WAIT:
                return gWallmasterWaitAnim;
            case ANIM_DAMAGE:
                return gWallmasterDamageAnim;
            case ANIM_RECOVER:
                return gWallmasterRecoverFromDamageAnim;
            case ANIM_STAND_UP:
                return gWallmasterStandUpAnim;
            case ANIM_STOP_WALK:
                return gWallmasterStopWalkAnim;
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

    bool IsHoldingSomeone() const {
        return Typed()->actionFunc == EnWallmas_TakePlayer;
    }

    u8 CurrentColliderRoles() const {
        EnWallmas* wm = Typed();
        if (wm->actionFunc == EnWallmas_Die || wm->actionFunc == EnWallmas_Drop)
            return 0;

        u8 roles = COLL_OC;
        if (wm->actionFunc != EnWallmas_TakeDamage && (wm->actor.bgCheckFlags & 1) && wm->actor.freezeTimer == 0)
            roles |= COLL_AC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
        PROP_Y_TARGET,
        PROP_SWITCH_FLAG,
        PROP_HEALTH,
        PROP_FREEZE,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnWallmas* wm = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(wm->timer), out);
        PackProperty(PROP_Y_TARGET, PackedFloat4(wm->yTarget), out);
        PackProperty(PROP_SWITCH_FLAG, PackedInt2(wm->switchFlag), out);
        PackProperty(PROP_HEALTH, PackedUInt1(wm->actor.colChkInfo.health), out);
        PackProperty(PROP_FREEZE, PackedUInt1(wm->actor.freezeTimer), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(wm->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &wm->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnWallmas* wm = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const WmActionFunc* table = ActionTable(&count);
                if (id < count)
                    wm->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                wm->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_Y_TARGET:
                wm->yTarget = data.Read<PackedFloat4>().value();
                break;
            case PROP_SWITCH_FLAG:
                wm->switchFlag = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                wm->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FREEZE:
                wm->actor.freezeTimer = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                wm->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &wm->skelAnime, LOCK_CUR_FRAME ? wm->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnBecomeLeader() override {
        if (Typed()->actionFunc == EnWallmas_TakePlayer) {
            EnWallmas_SetupReturnToCeiling(Typed());
        }
    }

    void EnsureDrawInstalled() {
        EnWallmas* wm = Typed();

        if (wm->actor.draw == NULL && wm->actionFunc != EnWallmas_WaitForProximity &&
            wm->actionFunc != EnWallmas_WaitForSwitchFlag) {
            wm->actor.draw = EnWallmas_Draw;
            wm->actor.flags |= ACTOR_FLAG_DRAW_CULLING_DISABLED;
        }
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawInstalled();

        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnWallmas* wm = Typed();

        UpdateAnimation(&wm->skelAnime, LOCK_CUR_FRAME);

        EnsureDrawInstalled();

        if (!IsHoldingSomeone()) {
            if (wm->collider.base.acFlags & AC_HIT) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }

            if (wm->actionFunc != EnWallmas_Die && wm->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
                ClaimLeadership(CLAIM_REASON_COOLDOWN);
        }
        wm->collider.base.acFlags &= ~AC_HIT;

        Actor_SetFocus(&wm->actor, 25.0f);

        if (wm->actionFunc != EnWallmas_TakeDamage)
            wm->actor.shape.rot.y = wm->actor.world.rot.y;

        if (m_roles != 0) {
            Collider_UpdateCylinder(&wm->actor, &wm->collider);
            RegisterColliderBase(play, &wm->collider.base, m_roles);
        }
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
