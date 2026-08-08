#ifndef FLOORMASTERCONTROLLERH
#define FLOORMASTERCONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Floormas/z_en_floormas.h"
#include "assets/objects/object_wallmaster/object_wallmaster.h"

void EnFloormas_Stand(EnFloormas* fm, PlayState* play);
void EnFloormas_BigDecideAction(EnFloormas* fm, PlayState* play);
void EnFloormas_BigWalk(EnFloormas* fm, PlayState* play);
void EnFloormas_BigStopWalk(EnFloormas* fm, PlayState* play);
void EnFloormas_Run(EnFloormas* fm, PlayState* play);
void EnFloormas_Turn(EnFloormas* fm, PlayState* play);
void EnFloormas_Hover(EnFloormas* fm, PlayState* play);
void EnFloormas_Charge(EnFloormas* fm, PlayState* play);
void EnFloormas_Land(EnFloormas* fm, PlayState* play);
void EnFloormas_Split(EnFloormas* fm, PlayState* play);
void EnFloormas_SmWalk(EnFloormas* fm, PlayState* play);
void EnFloormas_SmDecideAction(EnFloormas* fm, PlayState* play);
void EnFloormas_SmShrink(EnFloormas* fm, PlayState* play);
void EnFloormas_JumpAtLink(EnFloormas* fm, PlayState* play);
void EnFloormas_GrabLink(EnFloormas* fm, PlayState* play);
void EnFloormas_SmSlaveJumpAtMaster(EnFloormas* fm, PlayState* play);
void EnFloormas_Merge(EnFloormas* fm, PlayState* play);
void EnFloormas_SmWait(EnFloormas* fm, PlayState* play);
void EnFloormas_TakeDamage(EnFloormas* fm, PlayState* play);
void EnFloormas_Recover(EnFloormas* fm, PlayState* play);
void EnFloormas_Freeze(EnFloormas* fm, PlayState* play);

void EnFloormas_SetupLand(EnFloormas* fm);
void EnFloormas_SetupSmWait(EnFloormas* fm);
void EnFloormas_SetupSmShrink(EnFloormas* fm, PlayState* play);
void EnFloormas_SetupBigDecideAction(EnFloormas* fm);
void EnFloormas_DrawHighlighted(Actor* thisx, PlayState* play);
void EnFloormas_Draw(Actor* thisx, PlayState* play);

extern ColliderCylinderInit* gEnFloormasCylinderInit;
extern DamageTable* gFloorMasterDamageTable;
}

namespace ZeldaOnline {

static constexpr s32 FLOORMAS_SPAWN_INVISIBLE = 0x8000;
static constexpr s16 FLOORMAS_SPAWN_SMALL = 0x10;

class FloormasterController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnFloormas* Typed() const {
        return reinterpret_cast<EnFloormas*>(m_actor);
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    static void EnFloormas_Init(Actor* thisx, PlayState* play) {
        static InitChainEntry sInitChain[] = {
            ICHAIN_S8(naviEnemyId, 0x31, ICHAIN_CONTINUE),
            ICHAIN_F32(targetArrowOffset, 5500, ICHAIN_CONTINUE),
            ICHAIN_F32_DIV1000(gravity, -1000, ICHAIN_STOP),
        };
        static CollisionCheckInfoInit sColChkInfoInit = { 4, 30, 40, 150 };

        EnFloormas* fm = reinterpret_cast<EnFloormas*>(thisx);
        s32 invisible;

        Actor_ProcessInitChain(&fm->actor, sInitChain);
        ActorShape_Init(&fm->actor.shape, 0.0f, ActorShadow_DrawCircle, 50.0f);
        SkelAnime_InitFlex(play, &fm->skelAnime, (FlexSkeletonHeader*)&gWallmasterSkel,
                           (AnimationHeader*)&gWallmasterWaitAnim, fm->jointTable, fm->morphTable, 25);
        Collider_InitCylinder(play, &fm->collider);
        Collider_SetCylinder(play, &fm->collider, &fm->actor, gEnFloormasCylinderInit);
        CollisionCheck_SetInfo(&fm->actor.colChkInfo, gFloorMasterDamageTable, &sColChkInfoInit);
        fm->zOffset = -1600;

        invisible = fm->actor.params & FLOORMAS_SPAWN_INVISIBLE;
        fm->actor.params &= (s16)~FLOORMAS_SPAWN_INVISIBLE;
        if (invisible) {
            fm->actor.flags |= ACTOR_FLAG_REACT_TO_LENS;
            fm->actor.draw = EnFloormas_DrawHighlighted;
        }

        if (fm->actor.params == FLOORMAS_SPAWN_SMALL) {
            fm->actor.draw = NULL;
            fm->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
            fm->actionFunc = EnFloormas_SmWait;
        } else {
            s16 smallParams = (s16)(invisible + FLOORMAS_SPAWN_SMALL);

            fm->actor.parent = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FLOORMAS, fm->actor.world.pos.x,
                                           fm->actor.world.pos.y, fm->actor.world.pos.z, 0, 0, 0, smallParams);
            fm->actor.child = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_FLOORMAS, fm->actor.world.pos.x,
                                          fm->actor.world.pos.y, fm->actor.world.pos.z, 0, 0, 0, smallParams);

            if (fm->actor.parent != nullptr && fm->actor.child != nullptr) {
                fm->actor.parent->child = &fm->actor;
                fm->actor.parent->parent = fm->actor.child;
                fm->actor.child->parent = &fm->actor;
                fm->actor.child->child = fm->actor.parent;
            } else {
                fm->actor.parent = nullptr;
                fm->actor.child = nullptr;
            }

            EnFloormas_SetupBigDecideAction(fm);
        }
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    bool IsSmall() const {
        return (Typed()->actor.params & FLOORMAS_SPAWN_SMALL) != 0;
    }

    void LinkRing(PlayState* play) {
        EnFloormas* fm = Typed();

        if (fm->actor.params & FLOORMAS_SPAWN_SMALL) {
            return;
        }
        if (fm->actor.parent != nullptr && fm->actor.child != nullptr) {
            return;
        }

        Actor* found[2] = { nullptr, nullptr };
        int n = 0;
        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].head; it != nullptr && n < 2; it = it->next) {
            if (it->id != ACTOR_EN_FLOORMAS || it == &fm->actor)
                continue;
            if (!(it->params & FLOORMAS_SPAWN_SMALL))
                continue;
            if (it->parent != nullptr)
                continue;
            found[n++] = it;
        }

        if (n < 2) {
            return;
        }

        fm->actor.parent = found[0];
        fm->actor.child = found[1];
        fm->actor.parent->child = &fm->actor;
        fm->actor.parent->parent = fm->actor.child;
        fm->actor.child->parent = &fm->actor;
        fm->actor.child->child = fm->actor.parent;
    }

    using FmActionFunc = void (*)(EnFloormas*, PlayState*);
    static const FmActionFunc* ActionTable(size_t* count) {
        static const FmActionFunc sTable[] = {
            EnFloormas_Stand,
            EnFloormas_BigDecideAction,
            EnFloormas_BigWalk,
            EnFloormas_BigStopWalk,
            EnFloormas_Run,
            EnFloormas_Turn,
            EnFloormas_Hover,
            EnFloormas_Charge,
            EnFloormas_Land,
            EnFloormas_Split,
            EnFloormas_SmWalk,
            EnFloormas_SmDecideAction,
            EnFloormas_SmShrink,
            EnFloormas_JumpAtLink,
            EnFloormas_GrabLink,
            EnFloormas_SmSlaveJumpAtMaster,
            EnFloormas_Merge,
            EnFloormas_SmWait,
            EnFloormas_TakeDamage,
            EnFloormas_Recover,
            EnFloormas_Freeze,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const FmActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_WAIT = 0;
    static constexpr u8 ANIM_WALK = 1;
    static constexpr u8 ANIM_JUMP = 2;
    static constexpr u8 ANIM_HOVER = 3;
    static constexpr u8 ANIM_DAMAGE = 4;
    static constexpr u8 ANIM_RECOVER = 5;
    static constexpr u8 ANIM_STAND_UP = 6;
    static constexpr u8 ANIM_STOP_WALK = 7;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 8;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_WAIT:
                return gWallmasterWaitAnim;
            case ANIM_WALK:
                return gWallmasterWalkAnim;
            case ANIM_JUMP:
                return gWallmasterJumpAnim;
            case ANIM_HOVER:
                return gWallmasterHoverAnim;
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
        return Typed()->actionFunc == EnFloormas_GrabLink;
    }

    u8 CurrentColliderRoles() const {
        EnFloormas* fm = Typed();
        if (fm->actionFunc == EnFloormas_SmWait)
            return 0;

        u8 roles = 0;
        if (fm->actionFunc == EnFloormas_Charge)
            roles |= COLL_AT;

        if (fm->actionFunc != EnFloormas_GrabLink) {
            if (fm->actionFunc != EnFloormas_Split && fm->actionFunc != EnFloormas_TakeDamage &&
                fm->actor.freezeTimer == 0)
                roles |= COLL_AC;

            if (fm->actionFunc != EnFloormas_SmSlaveJumpAtMaster || fm->skelAnime.curFrame < 20.0f)
                roles |= COLL_OC;
        }
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMERS,
        PROP_Z_OFFSET,
        PROP_HEALTH,
        PROP_FREEZE,
        PROP_COL_TYPE,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnFloormas* fm = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(fm->actionTimer) << PackedInt2(fm->actionTarget)
                                  << PackedInt2(fm->smActionTimer),
                     out);
        PackProperty(PROP_Z_OFFSET, PackedInt2(fm->zOffset), out);
        PackProperty(PROP_HEALTH, PackedUInt1(fm->actor.colChkInfo.health), out);
        PackProperty(PROP_FREEZE, PackedUInt1(fm->actor.freezeTimer), out);
        PackProperty(PROP_COL_TYPE, PackedUInt1(fm->collider.base.colType), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(fm->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &fm->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnFloormas* fm = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const FmActionFunc* table = ActionTable(&count);
                if (id < count)
                    fm->actionFunc = table[id];
                break;
            }
            case PROP_TIMERS:
                fm->actionTimer = (s16)(data.Read<PackedInt2>().value());
                fm->actionTarget = (s16)(data.Read<PackedInt2>().value());
                fm->smActionTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_Z_OFFSET:
                fm->zOffset = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                fm->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FREEZE:
                fm->actor.freezeTimer = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COL_TYPE:
                fm->collider.base.colType = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                fm->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &fm->skelAnime, LOCK_CUR_FRAME ? fm->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnFloormas_SmShrink && !IsRunningLocally()) {
            EnFloormas_SetupSmShrink(Typed(), gPlayState);
            GoLocal();
        }
    }

    void OnBecomeLeader() override {
        EnFloormas* fm = Typed();
        if (fm->actionFunc != EnFloormas_GrabLink) {
            return;
        }

        fm->actor.shape.rot.x = 0;
        fm->actor.velocity.y = 6.0f;
        fm->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
        fm->actor.speedXZ = -3.0f;
        EnFloormas_SetupLand(fm);
    }

    void EnsureSmallPresentation() {
        EnFloormas* fm = Typed();

        if (fm->actionFunc == EnFloormas_SmWait)
            return;

        if (IsSmall() && fm->actor.draw == NULL) {
            fm->actor.draw =
                (fm->actor.flags & ACTOR_FLAG_REACT_TO_LENS) ? EnFloormas_DrawHighlighted : EnFloormas_Draw;
            fm->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
            fm->collider.dim.radius = (s16)(gEnFloormasCylinderInit->dim.radius * 0.6f);
            fm->collider.dim.height = (s16)(gEnFloormasCylinderInit->dim.height * 0.6f);
        }
    }

    void UpdateLeader(PlayState* play) override {
        LinkRing(play);

        EnsureSmallPresentation();

        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnFloormas* fm = Typed();

        LinkRing(play);

        if (fm->actionFunc == EnFloormas_SmWait) {
            return;
        }

        EnsureSmallPresentation();

        UpdateAnimation(&fm->skelAnime, LOCK_CUR_FRAME);

        if (!IsHoldingSomeone()) {
            if (fm->collider.base.acFlags & AC_HIT) {
                ClaimLeadership(CLAIM_REASON_HIT);
                UpdateLeader(play);
                return;
            }

            if (fm->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
                ClaimLeadership(CLAIM_REASON_PROXIMITY);
        }
        fm->collider.base.acFlags &= ~AC_HIT;
        fm->collider.base.atFlags &= ~AT_HIT;

        if (fm->actionFunc != EnFloormas_TakeDamage)
            fm->actor.world.rot.y = fm->actor.shape.rot.y;

        Actor_SetFocus(&fm->actor, fm->actor.scale.x * 2500.0f);

        if (m_roles != 0) {
            Collider_UpdateCylinder(&fm->actor, &fm->collider);
            RegisterColliderBase(play, &fm->collider.base, m_roles);
        }
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
