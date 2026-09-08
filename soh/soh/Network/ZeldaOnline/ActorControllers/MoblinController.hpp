#ifndef MOBLINCONTROLLERH
#define MOBLINCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Mb/z_en_mb.h"
#include "objects/object_mb/object_mb.h"

void EnMb_SpearGuardLookAround(EnMb* mb, PlayState* play);
void EnMb_SpearGuardWalk(EnMb* mb, PlayState* play);
void EnMb_SpearGuardPrepareAndCharge(EnMb* mb, PlayState* play);
void EnMb_SpearPatrolPrepareAndCharge(EnMb* mb, PlayState* play);
void EnMb_SpearPatrolTurnTowardsWaypoint(EnMb* mb, PlayState* play);
void EnMb_SpearPatrolWalkTowardsWaypoint(EnMb* mb, PlayState* play);
void EnMb_SpearPatrolEndCharge(EnMb* mb, PlayState* play);
void EnMb_SpearPatrolImmediateCharge(EnMb* mb, PlayState* play);
void EnMb_SpearEndChargeQuick(EnMb* mb, PlayState* play);
void EnMb_SpearDamaged(EnMb* mb, PlayState* play);
void EnMb_SpearDead(EnMb* mb, PlayState* play);
void EnMb_ClubWaitPlayerNear(EnMb* mb, PlayState* play);
void EnMb_ClubAttack(EnMb* mb, PlayState* play);
void EnMb_ClubWaitAfterAttack(EnMb* mb, PlayState* play);
void EnMb_ClubDamaged(EnMb* mb, PlayState* play);
void EnMb_ClubDamagedWhileKneeling(EnMb* mb, PlayState* play);
void EnMb_ClubDead(EnMb* mb, PlayState* play);
void EnMb_Stunned(EnMb* mb, PlayState* play);

void EnMb_SetupSpearDead(EnMb* mb);
void EnMb_SetupClubDead(EnMb* mb);
void EnMb_ClubWaitAfterAttack(EnMb* mb, PlayState* play);
void EffectSsBlast_SpawnWhiteShockwave(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* accel);
void func_80033480(PlayState* play, Vec3f* posBase, f32 randRangeDiameter, s32 amountMinusOne, s16 scaleBase,
                   s16 scaleStep, u8 arg6);
    
Actor* func_800358DC(Actor* actor, Vec3f* spawnPos, Vec3s* spawnRot, f32* arg3, s32 timer, s16* unused, PlayState* play,
                     s16 params, s32 arg8);
void func_800AA000(f32 arg0, u8 arg1, u8 arg2, u8 arg3);

}

namespace ZeldaOnline {
typedef enum {
    /* -1 */ ENMB_TYPE_SPEAR_GUARD = -1,
    /*  0 */ ENMB_TYPE_CLUB,
    /*  1 */ ENMB_TYPE_SPEAR_PATROL
} EnMbType;

class MoblinController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnMb* Typed() const {
        return reinterpret_cast<EnMb*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;
    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }
  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using MbActionFunc = void (*)(EnMb*, PlayState*);
    static const MbActionFunc* ActionTable(size_t* count) {
        static const MbActionFunc sTable[] = {
            EnMb_SpearGuardLookAround,
            EnMb_SpearGuardWalk,
            EnMb_SpearGuardPrepareAndCharge,
            EnMb_SpearPatrolPrepareAndCharge,
            EnMb_SpearPatrolTurnTowardsWaypoint,
            EnMb_SpearPatrolWalkTowardsWaypoint,
            EnMb_SpearPatrolEndCharge,
            EnMb_SpearPatrolImmediateCharge,
            EnMb_SpearEndChargeQuick,
            EnMb_SpearDamaged,
            EnMb_SpearDead,
            EnMb_ClubWaitPlayerNear,
            EnMb_ClubAttack,
            EnMb_ClubWaitAfterAttack,
            EnMb_ClubDamaged,
            EnMb_ClubDamagedWhileKneeling,
            EnMb_ClubDead,
            EnMb_Stunned,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const MbActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsCharging() const {
        EnMb* mb = Typed();
        return mb->actionFunc == EnMb_SpearGuardPrepareAndCharge ||
               mb->actionFunc == EnMb_SpearPatrolPrepareAndCharge ||
               mb->actionFunc == EnMb_SpearPatrolImmediateCharge || mb->actionFunc == EnMb_SpearEndChargeQuick ||
               mb->actionFunc == EnMb_SpearPatrolEndCharge;
    }



    bool IsDying() const {
        EnMb* mb = Typed();
        return mb->actionFunc == EnMb_SpearDead || mb->actionFunc == EnMb_ClubDead;
    }

    static constexpr u8 ANIM_COUNT = 17;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gEnMbSpearStandStillAnim;
            case 1:
                return gEnMbSpearLookLeftAndRightAnim;
            case 2:
                return gEnMbSpearWalkAnim;
            case 3:
                return gEnMbSpearPrepareChargeAnim;
            case 4:
                return gEnMbSpearChargeAnim;
            case 5:
                return gEnMbSpearSlowDownAnim;
            case 6:
                return gEnMbSpearDamagedFromFrontAnim;
            case 7:
                return gEnMbSpearDamagedFromBehindAnim;
            case 8:
                return gEnMbSpearFallFaceDownAnim;
            case 9:
                return gEnMbSpearFallOnItsBackAnim;
            case 10:
                return gEnMbClubStandStillClubDownAnim;
            case 11:
                return gEnMbClubLiftClubAnim;
            case 12:
                return gEnMbClubStrikeDownAnim;
            case 13:
                return gEnMbClubDamagedKneelAnim;
            case 14:
                return gEnMbClubBeatenKneelingAnim;
            case 15:
                return gEnMbClubStandUpAnim;
            case 16:
                return gEnMbClubFallOnItsBackAnim;
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


    u8 CurrentColliderRoles() const {
        EnMb* mb = Typed();
        u8 roles = COLL_AC | COLL_OC;
        if (mb->attack > 0) {
            roles |= COLL_AT;
        }
        return roles;
    }

    bool HitWouldReact() const {
        EnMb* mb = Typed();
        if (!(mb->hitbox.base.acFlags & AC_HIT))
            return false;
        return mb->actor.colChkInfo.damageEffect != 0 || mb->actor.colChkInfo.damage != 0;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_HEALTH,
        PROP_TIMERS,
        PROP_ATTACK,
        PROP_PATH,
        PROP_EFF_POS,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnMb* mb = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, PackedInt4(mb->state), out);
        PackProperty(PROP_HEALTH, PackedUInt1(mb->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(mb->timer1) << PackedInt2(mb->timer2) << PackedInt2(mb->timer3)
                                  << PackedInt2(mb->iceEffectTimer),
                     out);
        PackProperty(PROP_ATTACK,
                     ByteStream() << PackedInt2(mb->attack) << PackedInt2(mb->unk_332) << PackedInt2(mb->yawToWaypoint)
                                  << PackedUInt1(mb->damageEffect),
                     out);
        PackProperty(PROP_PATH,
                     ByteStream() << PackedInt1(mb->waypoint) << PackedInt1(mb->direction)
                                  << PackedFloat4(mb->waypointPos.x) << PackedFloat4(mb->waypointPos.y)
                                  << PackedFloat4(mb->waypointPos.z),
                     out);
        PackProperty(PROP_EFF_POS,
                     ByteStream() << PackedFloat4(mb->effSpawnPos.x) << PackedFloat4(mb->effSpawnPos.y)
                                  << PackedFloat4(mb->effSpawnPos.z),
                     out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(mb->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &mb->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnMb* mb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const MbActionFunc* table = ActionTable(&count);
                if (id < count)
                    mb->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                mb->state = (EnMbState)(data.Read<PackedInt4>().value());
                break;
            case PROP_HEALTH:
                mb->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                mb->timer1 = (s16)(data.Read<PackedInt2>().value());
                mb->timer2 = (s16)(data.Read<PackedInt2>().value());
                mb->timer3 = (s16)(data.Read<PackedInt2>().value());
                mb->iceEffectTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ATTACK:
                mb->attack = (s16)(data.Read<PackedInt2>().value());
                mb->unk_332 = (s16)(data.Read<PackedInt2>().value());
                mb->yawToWaypoint = (s16)(data.Read<PackedInt2>().value());
                mb->damageEffect = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PATH:
                mb->waypoint = (s8)(data.Read<PackedInt1>().value());
                mb->direction = (s8)(data.Read<PackedInt1>().value());
                mb->waypointPos.x = data.Read<PackedFloat4>().value();
                mb->waypointPos.y = data.Read<PackedFloat4>().value();
                mb->waypointPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_EFF_POS:
                mb->effSpawnPos.x = data.Read<PackedFloat4>().value();
                mb->effSpawnPos.y = data.Read<PackedFloat4>().value();
                mb->effSpawnPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                mb->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &mb->skelAnime, LOCK_CUR_FRAME ? mb->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnMb* mb = Typed();

        if (mb->actionFunc == EnMb_SpearDead)
        {
            EnMb_SetupSpearDead(mb);
            GoLocal();
        }
        else if (mb->actionFunc == EnMb_ClubDead)
        {
            EnMb_SetupClubDead(mb);
            GoLocal();
        }
    }

    void OnTrigger(const std::string& name, ByteStream& data) override {
        if (name != "clubsmash" || gPlayState == nullptr) {
            return;
        }

        EnMb* mb = Typed();
        Vec3f effSpawnPos;
        effSpawnPos.x = data.Read<PackedFloat4>().value();
        effSpawnPos.y = data.Read<PackedFloat4>().value();
        effSpawnPos.z = data.Read<PackedFloat4>().value();

        Vec3s spawnRot = mb->actor.world.rot;
        spawnRot.y = (s16)(data.Read<PackedInt2>().value());

        Vec3f effWhiteShockwaveDynamics = { 0.0f, 0.0f, 0.0f };
        f32 flamesParams[] = { 18.0f, 18.0f, 0.0f };
        s16 flamesUnused[] = { 20, 40, 0 };

        Audio_PlayActorSound2(&mb->actor, NA_SE_EN_MONBLIN_HAM_LAND);
        func_800AA000(mb->actor.xzDistToPlayer, 0xFF, 0x14, 0x96);
        EffectSsBlast_SpawnWhiteShockwave(gPlayState, &effSpawnPos, &effWhiteShockwaveDynamics,
                                          &effWhiteShockwaveDynamics);
        func_80033480(gPlayState, &effSpawnPos, 2.0f, 3, 0x12C, 0xB4, 1);

        if (!CVarGetInteger(CVAR_ENHANCEMENT("RandomizedEnemies"), 0)) {
            Camera_AddQuake(&gPlayState->mainCamera, 2, 0x19, 5);
        }

        func_800358DC(&mb->actor, &effSpawnPos, &spawnRot, flamesParams, 20, flamesUnused, gPlayState, -1, 0);
    }

    void UpdateLeader(PlayState* play) override {
        EnMb* mb = Typed();
        bool wasAttacking = mb->actionFunc == EnMb_ClubAttack;

        AbstractActorController::UpdateLeader(play);

        if (wasAttacking && mb->actionFunc == EnMb_ClubWaitAfterAttack) {
            Vec3f pos = mb->effSpawnPos;
            pos.y = mb->actor.floorHeight;
            SendTriggerToPuppets("clubsmash", ByteStream() << PackedFloat4(pos.x) << PackedFloat4(pos.y)
                                                           << PackedFloat4(pos.z) << PackedInt2(mb->actor.world.rot.y));
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnMb* mb = Typed();

        UpdateAnimation(&mb->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        mb->hitbox.base.acFlags &= ~AC_HIT;
        mb->attackCollider.base.atFlags &= ~AT_HIT;
        mb->frontShielding.base.acFlags &= ~AC_HIT;

        Player* localPlayer = GET_PLAYER(play);
        bool grabbedByUs =
            (localPlayer->stateFlags2 & PLAYER_STATE2_GRABBED_BY_ENEMY) && localPlayer->actor.parent == &mb->actor;

        if (grabbedByUs) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }


        if (!IsCharging() && !IsDying() && mb->actor.colChkInfo.health > 0 && mb->actor.xzDistToPlayer < 400.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Actor_SetFocus(&mb->actor, 40.0f);


        if (!IsCharging() && (localPlayer->stateFlags2 & PLAYER_STATE2_GRABBED_BY_ENEMY) &&
            localPlayer->actor.parent == &mb->actor) {
            localPlayer->stateFlags2 &= ~PLAYER_STATE2_GRABBED_BY_ENEMY;
            localPlayer->actor.parent = nullptr;
            localPlayer->av2.actionVar2 = 200;
        }

        Collider_UpdateCylinder(&mb->actor, &mb->hitbox);
        RegisterColliderBase(play, &mb->hitbox.base, m_roles & ~COLL_AT);

        if ((m_roles & COLL_AT) && mb->actor.isDrawn)
            RegisterColliderBase(play, &mb->attackCollider.base, COLL_AT);

        if (mb->actor.params != ENMB_TYPE_CLUB && mb->actor.isDrawn)
            RegisterColliderBase(play, &mb->frontShielding.base, COLL_AC);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AC | COLL_OC;
};

} // namespace ZeldaOnline

#endif