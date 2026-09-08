#ifndef STALFOSCONTROLLERH
#define STALFOSCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Test/z_en_test.h"
#include "objects/object_sk2/object_sk2.h"

void EnTest_WaitGround(EnTest* st, PlayState* play);
void EnTest_Rise(EnTest* st, PlayState* play);
void EnTest_WaitAbove(EnTest* st, PlayState* play);
void EnTest_Fall(EnTest* st, PlayState* play);
void EnTest_Idle(EnTest* st, PlayState* play);
void EnTest_Land(EnTest* st, PlayState* play);
void EnTest_WalkAndBlock(EnTest* st, PlayState* play);
void func_80860C24(EnTest* st, PlayState* play);
void func_80860F84(EnTest* st, PlayState* play);
void EnTest_SlashDown(EnTest* st, PlayState* play);
void EnTest_SlashDownEnd(EnTest* st, PlayState* play);
void EnTest_SlashUp(EnTest* st, PlayState* play);
void EnTest_JumpBack(EnTest* st, PlayState* play);
void EnTest_Jumpslash(EnTest* st, PlayState* play);
void EnTest_JumpUp(EnTest* st, PlayState* play);
void EnTest_StopAndBlock(EnTest* st, PlayState* play);
void EnTest_IdleFromBlock(EnTest* st, PlayState* play);
void func_808621D4(EnTest* st, PlayState* play);
void func_80862418(EnTest* st, PlayState* play);
void EnTest_Stunned(EnTest* st, PlayState* play);
void func_808628C8(EnTest* st, PlayState* play);
void func_80862E6C(EnTest* st, PlayState* play);
void func_80863044(EnTest* st, PlayState* play);
void func_808633E8(EnTest* st, PlayState* play);
void func_8086318C(EnTest* st, PlayState* play);
void EnTest_Recoil(EnTest* st, PlayState* play);
}

static u8 sJointCopyFlags[STALFOS_LIMB_MAX] = {
    false, false, false, false, true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
    true,  true,  true,  true,  true,  true,  true,  false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false,
};

namespace ZeldaOnline {

class StalfosController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTest* Typed() const {
        return reinterpret_cast<EnTest*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    static constexpr s32 kDmgEffFireMagic = 6;
    static constexpr s32 kDmgEffSling = 13;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using TestActionFunc = void (*)(EnTest*, PlayState*);
    static const TestActionFunc* ActionTable(size_t* count) {
        static const TestActionFunc sTable[] = {
            EnTest_WaitGround,
            EnTest_Rise,
            EnTest_WaitAbove,
            EnTest_Fall,
            EnTest_Idle,
            EnTest_Land,
            EnTest_WalkAndBlock,
            func_80860C24,
            func_80860F84,
            EnTest_SlashDown,
            EnTest_SlashDownEnd,
            EnTest_SlashUp,
            EnTest_JumpBack,
            EnTest_Jumpslash,
            EnTest_JumpUp,
            EnTest_StopAndBlock,
            EnTest_IdleFromBlock,
            func_808621D4,
            func_80862418,
            EnTest_Stunned,
            func_808628C8,
            func_80862E6C,
            func_80863044,
            func_808633E8,
            func_8086318C,
            EnTest_Recoil,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TestActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_BLOCK_SHIELD = 0;
    static constexpr u8 ANIM_DOWN_SLASH = 1;
    static constexpr u8 ANIM_FALL_BACK = 2;
    static constexpr u8 ANIM_FALL_FORWARD = 3;
    static constexpr u8 ANIM_FAST_ADVANCE = 4;
    static constexpr u8 ANIM_FLINCH_BEHIND = 5;
    static constexpr u8 ANIM_FLINCH_FRONT = 6;
    static constexpr u8 ANIM_JUMP = 7;
    static constexpr u8 ANIM_JUMP_BACK = 8;
    static constexpr u8 ANIM_JUMPSLASH = 9;
    static constexpr u8 ANIM_LAND = 10;
    static constexpr u8 ANIM_MIDDLE_GUARD = 11;
    static constexpr u8 ANIM_RECOVER_DOWN_SLASH = 12;
    static constexpr u8 ANIM_SIDESTEP = 13;
    static constexpr u8 ANIM_SLOW_ADVANCE = 14;
    static constexpr u8 ANIM_UP_SLASH = 15;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 16;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_BLOCK_SHIELD:
                return gStalfosBlockWithShieldAnim;
            case ANIM_DOWN_SLASH:
                return gStalfosDownSlashAnim;
            case ANIM_FALL_BACK:
                return gStalfosFallOverBackwardsAnim;
            case ANIM_FALL_FORWARD:
                return gStalfosFallOverForwardsAnim;
            case ANIM_FAST_ADVANCE:
                return gStalfosFastAdvanceAnim;
            case ANIM_FLINCH_BEHIND:
                return gStalfosFlinchFromHitBehindAnim;
            case ANIM_FLINCH_FRONT:
                return gStalfosFlinchFromHitFrontAnim;
            case ANIM_JUMP:
                return gStalfosJumpAnim;
            case ANIM_JUMP_BACK:
                return gStalfosJumpBackwardsAnim;
            case ANIM_JUMPSLASH:
                return gStalfosJumpslashAnim;
            case ANIM_LAND:
                return gStalfosLandFromLeapAnim;
            case ANIM_MIDDLE_GUARD:
                return gStalfosMiddleGuardAnim;
            case ANIM_RECOVER_DOWN_SLASH:
                return gStalfosRecoverFromDownSlashAnim;
            case ANIM_SIDESTEP:
                return gStalfosSidestepAnim;
            case ANIM_SLOW_ADVANCE:
                return gStalfosSlowAdvanceAnim;
            case ANIM_UP_SLASH:
                return gStalfosUpSlashAnim;
            default:
                return nullptr;
        }
    }
    static u8 AnimIndexFor(void* animPtr) {
        if (animPtr == nullptr)
            return ANIM_UNKNOWN;
        const char* cur = (const char*)animPtr;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }
    u8 CurrentAnimIndex() const {
        return AnimIndexFor(Typed()->skelAnime.animation);
    }

    bool HitWouldReact() const {
        EnTest* st = Typed();
        if (st->shieldCollider.base.acFlags & AC_BOUNCED)
            return true;
        if (!(st->bodyCollider.base.acFlags & AC_HIT))
            return false;
        return st->actor.colChkInfo.damageEffect != kDmgEffSling &&
               st->actor.colChkInfo.damageEffect != kDmgEffFireMagic;
    }

    u8 CurrentBodyRoles() const {
        EnTest* st = Typed();
        if (st->actor.colChkInfo.health == 0 && st->actor.colorFilterTimer == 0)
            return 0;
        u8 roles = COLL_OC;
        bool invulnerable = st->actor.colorFilterTimer != 0 && (st->actor.colorFilterParams & 0x4000);
        if (st->unk_7C8 >= 0xA && !invulnerable)
            roles |= COLL_AC;
        return roles;
    }
    u8 CurrentShieldRoles() const {
        EnTest* st = Typed();
        if (st->actor.colChkInfo.health == 0 && st->actor.colorFilterTimer == 0)
            return 0;
        return (st->unk_7DE != 0) ? COLL_AC : 0;
    }
    u8 CurrentSwordRoles() const {
        EnTest* st = Typed();
        if (st->meleeWeaponState < 1)
            return 0;
        return (st->swordCollider.base.atFlags & AT_BOUNCED) ? 0 : COLL_AT;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_HEALTH,
        PROP_BATTERY,
        PROP_HEAD_ROT,
        PROP_BLOCK_BLEND,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_BROKEN,
        PROP_BREAK_COUNT
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTest* st = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, PackedUInt1(st->unk_7C8), out);
        PackProperty(PROP_HEALTH, PackedUInt1(st->actor.colChkInfo.health), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_BATTERY,
                     ByteStream() << PackedUInt1((u8)(st->meleeWeaponState)) << PackedInt2(st->iceTimer)
                                  << PackedInt4(st->timer) << PackedUInt1(st->lastDamageEffect),
                     out);
        PackProperty(PROP_HEAD_ROT,
                     ByteStream() << PackedInt2(st->headRot.x) << PackedInt2(st->headRot.y) << PackedInt2(st->headRot.z)
                                  << PackedInt2(st->headRotOffset.x) << PackedInt2(st->headRotOffset.y)
                                  << PackedInt2(st->headRotOffset.z),
                     out);
        PackProperty(PROP_BLOCK_BLEND,
                     ByteStream() << PackedUInt1(st->unk_7DE) << PackedFloat4(st->upperSkelanime.curFrame)
                                  << PackedFloat4(st->upperSkelanime.morphWeight),
                     out);
        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentBodyRoles()) << PackedUInt1(CurrentShieldRoles())
                                  << PackedUInt1(CurrentSwordRoles()),
                     out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(st->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &st->skelAnime), out);
        PackProperty(PROP_BROKEN, PackedUInt1(st->actor.child != nullptr ? 1u : 0u), out);
        PackProperty(PROP_BREAK_COUNT, PackedInt2(st->actor.home.rot.x), out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTest* st = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TestActionFunc* table = ActionTable(&count);
                if (id < count)
                    st->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                st->unk_7C8 = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEALTH:
                st->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_BATTERY:
                st->meleeWeaponState = (s8)(data.Read<PackedUInt1>().value());
                st->iceTimer = (s16)(data.Read<PackedInt2>().value());
                st->timer = data.Read<PackedInt4>().value();
                st->lastDamageEffect = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEAD_ROT:
                st->headRot.x = (s16)(data.Read<PackedInt2>().value());
                st->headRot.y = (s16)(data.Read<PackedInt2>().value());
                st->headRot.z = (s16)(data.Read<PackedInt2>().value());
                st->headRotOffset.x = (s16)(data.Read<PackedInt2>().value());
                st->headRotOffset.y = (s16)(data.Read<PackedInt2>().value());
                st->headRotOffset.z = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BLOCK_BLEND:
                st->unk_7DE = (u8)(data.Read<PackedUInt1>().value());
                st->upperSkelanime.curFrame = data.Read<PackedFloat4>().value();
                st->upperSkelanime.morphWeight = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_shieldRoles = (u8)(data.Read<PackedUInt1>().value());
                m_swordRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                st->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &st->skelAnime, LOCK_CUR_FRAME ? st->skelAnime.curFrame : 0.0f, data);
                break;
            }

            case PROP_BROKEN: {
                u8 broken = (u8)(data.Read<PackedUInt1>().value());
                st->actor.child = broken ? &st->actor : nullptr;

                break;
            }

           case PROP_BREAK_COUNT:
                st->actor.home.rot.x = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    bool OtherLiveStalfosNearby(PlayState* play) const {
        EnTest* st = Typed();
        Actor* other = play->actorCtx.actorLists[ACTORCAT_ENEMY].head;
        for (; other != nullptr; other = other->next) {
            if (other == &st->actor || other->id != ACTOR_EN_TEST)
                continue;
            EnTest* sibling = reinterpret_cast<EnTest*>(other);
            if (sibling->actor.colChkInfo.health == 0)
                continue;
            if (Math_Vec3f_DistXYZ(&st->actor.world.pos, &other->world.pos) < 500.0f)
                return true;
        }
        return false;
    }

    void UpdateLeader(PlayState* play) override {
        if (!OtherLiveStalfosNearby(play))
            Typed()->actor.isTargeted = true;
        AbstractActorController::UpdateLeader(play);
    }

    void OnPropertiesApplied(u64 changed) override {
        EnTest* st = Typed();

        if (gPlayState == nullptr) {
            return;
        }

        if (!(changed & (1u << PROP_ACTION))) {
            return;
        }

        //Down but can respawn
        if (st->actionFunc == func_80862E6C && st->actor.child == nullptr) {
            BodyBreak_Alloc(&st->bodyBreak, 60, gPlayState);
        }


        if (st->actionFunc == func_808633E8) {
            BodyBreak_Alloc(&st->bodyBreak, 60, gPlayState);
            GoLocal();
        }
    }
    void UpdatePuppet(PlayState* play) override {
        EnTest* st = Typed();

        UpdateAnimation(&st->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        //In the down but can respawn position. Simulate the broken body parts 
        if (st->actionFunc == func_80862E6C && st->actor.child == nullptr) {
            if (BodyBreak_SpawnParts(&st->actor, &st->bodyBreak, play, st->actor.params + 8)) {
            }
        }

        st->bodyCollider.base.acFlags &= ~AC_HIT;
        st->shieldCollider.base.acFlags &= ~AC_BOUNCED;
        st->swordCollider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);

        if (st->actor.colChkInfo.health > 0 && st->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        if (st->unk_7DE == 1) {
            Animation_Change(&st->upperSkelanime, (AnimationHeader*) & gStalfosBlockWithShieldAnim, 2.0f, 0.0f,
                             Animation_GetLastFrame((void*) & gStalfosBlockWithShieldAnim), 2, 2.0f);
            AnimationContext_SetCopyTrue(play, st->skelAnime.limbCount, st->skelAnime.jointTable,
                                         st->upperSkelanime.jointTable, sJointCopyFlags);
        } else if (st->unk_7DE == 2) {
            SkelAnime_Update(&st->upperSkelanime);
            SkelAnime_CopyFrameTableTrue(&st->skelAnime, st->skelAnime.jointTable, st->upperSkelanime.jointTable,
                                         sJointCopyFlags);
        } else if (st->unk_7DE == 3 || st->unk_7DE == 4) {
            f32 oldWeight = st->upperSkelanime.morphWeight;
            if (oldWeight > 0.0f) {
                SkelAnime_InterpFrameTable(st->skelAnime.limbCount, st->upperSkelanime.jointTable,
                                           st->upperSkelanime.jointTable, st->skelAnime.jointTable,
                                           1.0f - (st->upperSkelanime.morphWeight / oldWeight));
            }
            SkelAnime_CopyFrameTableTrue(&st->skelAnime, st->skelAnime.jointTable, st->upperSkelanime.jointTable,
                                         sJointCopyFlags);
        }

        if (st->actor.colorFilterTimer != 0 || st->actor.colChkInfo.health == 0)
            Math_SmoothStepToS(&st->headRot.y, 0, 1, 0x3E8, 0);

        st->actor.focus.pos = st->actor.world.pos;
        st->actor.focus.pos.y += 45.0f;

        if (st->actor.params == STALFOS_TYPE_INVISIBLE) {
            if (play->actorCtx.lensActive) {
                st->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_REACT_TO_LENS;
                st->actor.shape.shadowDraw = ActorShadow_DrawFeet;
            } else {
                st->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_REACT_TO_LENS);
                st->actor.shape.shadowDraw = nullptr;
            }
        }

        Collider_UpdateCylinder(&st->actor, &st->bodyCollider);

        if (m_bodyRoles != 0)
            RegisterColliderBase(play, &st->bodyCollider.base, m_bodyRoles);
        if (m_shieldRoles != 0)
            RegisterColliderBase(play, &st->shieldCollider.base, m_shieldRoles);
        if (m_swordRoles != 0)
            RegisterColliderBase(play, &st->swordCollider.base, m_swordRoles);
    }

  private:
    u8 m_bodyRoles = COLL_OC | COLL_AC;
    u8 m_shieldRoles = 0;
    u8 m_swordRoles = 0;
};

}

#endif
