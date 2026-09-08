#ifndef BONGOBONGOCONTROLLERH
#define BONGOBONGOCONTROLLERH
#include <cstring>

#include "../AbstractBossController.hpp"
#include <soh/Enhancements/game-interactor/GameInteractor_Hooks.h>

extern "C" {
#include "src/overlays/actors/ovl_Boss_Sst/z_boss_sst.h"
#include "src/overlays/actors/ovl_Bg_Sst_Floor/z_bg_sst_floor.h"
#include "overlays/ovl_Boss_Sst/ovl_Boss_Sst.h"
#include "objects/object_sst/object_sst.h"
#include "assets/textures/boss_title_cards/object_sst.h"

void BossSst_HeadLurk(BossSst* sst, PlayState* play);
void BossSst_HeadIntro(BossSst* sst, PlayState* play);
void BossSst_HeadNeutral(BossSst* sst, PlayState* play);
void BossSst_HeadWait(BossSst* sst, PlayState* play);
void BossSst_HeadDamagedHand(BossSst* sst, PlayState* play);
void BossSst_HeadReadyCharge(BossSst* sst, PlayState* play);
void BossSst_HeadCharge(BossSst* sst, PlayState* play);
void BossSst_HeadEndCharge(BossSst* sst, PlayState* play);
void BossSst_HeadFrozenHand(BossSst* sst, PlayState* play);
void BossSst_HeadUnfreezeHand(BossSst* sst, PlayState* play);
void BossSst_HeadStunned(BossSst* sst, PlayState* play);
void BossSst_HeadVulnerable(BossSst* sst, PlayState* play);
void BossSst_HeadDamage(BossSst* sst, PlayState* play);
void BossSst_HeadRecover(BossSst* sst, PlayState* play);
void BossSst_HeadDeath(BossSst* sst, PlayState* play);
void BossSst_HeadThrash(BossSst* sst, PlayState* play);
void BossSst_HeadDarken(BossSst* sst, PlayState* play);
void BossSst_HeadFall(BossSst* sst, PlayState* play);
void BossSst_HeadMelt(BossSst* sst, PlayState* play);
void BossSst_HeadFinish(BossSst* sst, PlayState* play);
void BossSst_HeadSetupDeath(BossSst* thisx, PlayState* play);

void BossSst_HandWait(BossSst* sst, PlayState* play);
void BossSst_HandDownbeat(BossSst* sst, PlayState* play);
void BossSst_HandOffbeat(BossSst* sst, PlayState* play);
void BossSst_HandDownbeatEnd(BossSst* sst, PlayState* play);
void BossSst_HandOffbeatEnd(BossSst* sst, PlayState* play);
void BossSst_HandReadySlam(BossSst* sst, PlayState* play);
void BossSst_HandSlam(BossSst* sst, PlayState* play);
void BossSst_HandEndSlam(BossSst* sst, PlayState* play);
void BossSst_HandReadySweep(BossSst* sst, PlayState* play);
void BossSst_HandSweep(BossSst* sst, PlayState* play);
void BossSst_HandReadyPunch(BossSst* sst, PlayState* play);
void BossSst_HandPunch(BossSst* sst, PlayState* play);
void BossSst_HandReadyClap(BossSst* sst, PlayState* play);
void BossSst_HandClap(BossSst* sst, PlayState* play);
void BossSst_HandEndClap(BossSst* sst, PlayState* play);
void BossSst_HandReadyGrab(BossSst* sst, PlayState* play);
void BossSst_HandGrab(BossSst* sst, PlayState* play);
void BossSst_HandCrush(BossSst* sst, PlayState* play);
void BossSst_HandEndCrush(BossSst* sst, PlayState* play);
void BossSst_HandSwing(BossSst* sst, PlayState* play);
void BossSst_HandRetreat(BossSst* sst, PlayState* play);
void BossSst_HandReel(BossSst* sst, PlayState* play);
void BossSst_HandReadyShake(BossSst* sst, PlayState* play);
void BossSst_HandShake(BossSst* sst, PlayState* play);
void BossSst_HandReadyCharge(BossSst* sst, PlayState* play);
void BossSst_HandFrozen(BossSst* sst, PlayState* play);
void BossSst_HandReadyBreakIce(BossSst* sst, PlayState* play);
void BossSst_HandBreakIce(BossSst* sst, PlayState* play);
void BossSst_HandStunned(BossSst* sst, PlayState* play);
void BossSst_HandDamage(BossSst* sst, PlayState* play);
void BossSst_HandRecover(BossSst* sst, PlayState* play);
void BossSst_HandThrash(BossSst* sst, PlayState* play);
void BossSst_HandDarken(BossSst* sst, PlayState* play);
void BossSst_HandFall(BossSst* sst, PlayState* play);
void BossSst_HandMelt(BossSst* sst, PlayState* play);
void BossSst_HandFinish(BossSst* sst, PlayState* play);

void BossSst_HandSetupEndCrush(BossSst* thisx);
void BossSst_HandSetupRetreat(BossSst* thisx);


void BossSst_HeadSetupLurk(BossSst* sst);
void BossSst_HandSetupWait(BossSst* sst);

void BossSst_HandSetupThrash(BossSst* thisx);
void BossSst_UpdateHead(Actor* thisx, PlayState* play);
void BossSst_UpdateHand(Actor* thisx, PlayState* play);
void BossSst_DrawHead(Actor* thisx, PlayState* play);
void BossSst_DrawHand(Actor* thisx, PlayState* play);
void BossSst_UpdateEffect(Actor* thisx, PlayState* play);
void BossSst_HandReleasePlayer(BossSst* sst, PlayState* play, s32 dropPlayer);

extern BossSst** gBossSstHead;
extern BossSst** gBossSstHands;
extern BgSstFloor** gBossSstFloor;
extern s32* gBossSstHandState;
extern u32* gBossSstBodyStatic;

extern ColliderJntSphInit* gBossSstJntSphInitHead;
extern ColliderJntSphInit* gBossSstJntSphInitHand;
extern ColliderCylinderInit* gBossSstCylinderInitHead;
extern ColliderCylinderInit* gBossSstCylinderInitHand;
extern CollisionCheckInfoInit* gBossSstColChkInfoInit;
extern DamageTable* gBossSstDamageTable;

extern s16* gBossSstCutsceneCamera;
extern Vec3f* gBossSstCameraAt;
extern Vec3f* gBossSstCameraEye;

u16 func_800FA0B4(u8 seqPlayerIndex);
void func_80064534(PlayState* play, CutsceneContext* csCtx);
}
#define ROOM_CENTER_X -50.0f
#define ROOM_CENTER_Y 0.0f
#define ROOM_CENTER_Z 0.0f
namespace ZeldaOnline {

class BongoBongoController : public AbstractBossController {
    typedef enum {
        /* 0 */ BONGOFLOOR_REST,
        /* 1 */ BONGOFLOOR_HIT
    } BgSstFloorParams;

  public:
    using AbstractBossController::AbstractBossController;

    // The ending Actor_Kills the head and both hands, and those arrive as separate
    // destroys on every other client -- so a hand can run a frame after the head's
    // memory is gone, while EnsureDrawInstalled and BossActive both deref sHead.
    ~BongoBongoController() override {
        BossSst* sst = Typed();

        if (sst == nullptr)
            return;

        if (sst->actor.params == BONGO_HEAD) {
            if (*gBossSstHead == sst)
                *gBossSstHead = nullptr;
        } else if (sst->actor.params == BONGO_LEFT_HAND || sst->actor.params == BONGO_RIGHT_HAND) {
            if (gBossSstHands[sst->actor.params] == sst)
                gBossSstHands[sst->actor.params] = nullptr;

            BossSst* other = gBossSstHands[sst->actor.params == BONGO_LEFT_HAND ? BONGO_RIGHT_HAND : BONGO_LEFT_HAND];
            if (other != nullptr && other->actor.child == &sst->actor)
                other->actor.child = NULL;
        }
    }

    BossSst* Typed() const {
        return reinterpret_cast<BossSst*>(m_actor);
    }

    static void RegisterHooks(s16 actorID, bool enabled) {
        COND_ID_HOOK(ShouldActorInit, actorID, enabled, [&](void* actorRef, bool*) {
            Actor* actor = (Actor*)actorRef;

            AbstractActorController::InstallCustomInit(actor, BongoBongoController::ReplacementInit);
        });
    }

    static constexpr bool LOCK_CUR_FRAME = true;


    static void ReplacementInit(Actor* thisx, PlayState* play) {
        BossSst* sst = reinterpret_cast<BossSst*>(thisx);

        thisx->naviEnemyId = 0x29;
        thisx->targetMode = 5;
        Actor_SetScale(thisx, 0.02f);

        Collider_InitCylinder(play, &sst->colliderCyl);
        Collider_InitJntSph(play, &sst->colliderJntSph);
        CollisionCheck_SetInfo(&thisx->colChkInfo, gBossSstDamageTable, gBossSstColChkInfoInit);
        Flags_SetSwitch(play, 0x14);

        if (thisx->params == BONGO_HEAD) {
            *gBossSstFloor = (BgSstFloor*)(Actor_Spawn(&play->actorCtx, play, ACTOR_BG_SST_FLOOR, ROOM_CENTER_X,
                                                       ROOM_CENTER_Y, ROOM_CENTER_Z, 0, 0, 0, BONGOFLOOR_REST));
            SkelAnime_InitFlex(play, &sst->skelAnime, (FlexSkeletonHeader*)&gBongoHeadSkel,
                               (AnimationHeader*)&gBongoHeadEyeOpenIdleAnim, sst->jointTable, sst->morphTable, 45);
            ActorShape_Init(&thisx->shape, 70000.0f, ActorShadow_DrawCircle, 95.0f);
            Collider_SetJntSph(play, &sst->colliderJntSph, thisx, gBossSstJntSphInitHead, sst->colliderItems);
            Collider_SetCylinder(play, &sst->colliderCyl, thisx, gBossSstCylinderInitHead);
            *gBossSstHead = sst;
            thisx->world.pos.x = ROOM_CENTER_X + 50.0f;
            thisx->world.pos.y = ROOM_CENTER_Y + 0.0f;
            thisx->world.pos.z = ROOM_CENTER_Z - 650.0f;
            thisx->home.pos = thisx->world.pos;
            thisx->shape.rot.y = 0;

            if (Flags_GetClear(play, play->roomCtx.curRoom.num)) {
                if (GameInteractor_Should(VB_SPAWN_BLUE_WARP, true, sst)) {
                    Actor_Spawn(&play->actorCtx, play, ACTOR_DOOR_WARP1, ROOM_CENTER_X, ROOM_CENTER_Y,
                                ROOM_CENTER_Z + 400.0f, 0, 0, 0, WARP_DUNGEON_ADULT);
                }
                if (GameInteractor_Should(VB_SPAWN_HEART_CONTAINER, true)) {
                    Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_B_HEART, ROOM_CENTER_X, ROOM_CENTER_Y,
                                ROOM_CENTER_Z - 200.0f, 0, 0, 0, 0);
                }
                Actor_Kill(thisx);
                return;
            }

            BossSst* left = (BossSst*)(Actor_Spawn(&play->actorCtx, play, ACTOR_BOSS_SST, thisx->world.pos.x + 200.0f,
                                                   thisx->world.pos.y, thisx->world.pos.z + 400.0f, 0,
                                                   thisx->shape.rot.y, 0, BONGO_LEFT_HAND));
            BossSst* right = (BossSst*)(Actor_Spawn(
                &play->actorCtx, play, ACTOR_BOSS_SST, thisx->world.pos.x + (-200.0f), thisx->world.pos.y,
                thisx->world.pos.z + 400.0f, 0, thisx->shape.rot.y, 0, BONGO_RIGHT_HAND));

            if (left != nullptr)
                gBossSstHands[BONGO_LEFT_HAND] = left;
            if (right != nullptr)
                gBossSstHands[BONGO_RIGHT_HAND] = right;

            thisx->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
            thisx->update = BossSst_UpdateHead;
            sst->radius = -650.0f;
            thisx->targetArrowOffset = 4000.0f;


            thisx->draw = NULL;
            sst->actionVar = 0;
            sst->actionFunc = BossSst_HeadLurk;

            Actor_ChangeCategory(play, &play->actorCtx, thisx, ACTORCAT_BOSS);
        } else {
            Collider_SetJntSph(play, &sst->colliderJntSph, thisx, gBossSstJntSphInitHand, sst->colliderItems);
            Collider_SetCylinder(play, &sst->colliderCyl, thisx, gBossSstCylinderInitHand);

            if (thisx->params == BONGO_LEFT_HAND) {
                SkelAnime_InitFlex(play, &sst->skelAnime, (FlexSkeletonHeader*)&gBongoLeftHandSkel,
                                   (AnimationHeader*)&gBongoLeftHandIdleAnim, sst->jointTable, sst->morphTable, 27);
                sst->actionVar = -1;
                sst->colliderJntSph.elements[0].dim.modelSphere.center.z *= -1;
            } else {
                SkelAnime_InitFlex(play, &sst->skelAnime, (FlexSkeletonHeader*)&gBongoRightHandSkel,
                                   (AnimationHeader*)&gBongoRightHandIdleAnim, sst->jointTable, sst->morphTable, 27);
                sst->actionVar = 1;
            }

            ActorShape_Init(&thisx->shape, 0.0f, ActorShadow_DrawCircle, 95.0f);
            sst->handZPosMod = -3500;
            thisx->targetArrowOffset = 5000.0f;
            thisx->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
            thisx->draw = NULL;
            BossSst_HandSetupWait(sst);
        }
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_COUNT_HEAD = 10;
    static constexpr u8 ANIM_COUNT_HAND = 8;

    static constexpr s32 STATE_WAIT = 0;
    static constexpr s32 STATE_BEAT = 1;
    static constexpr s32 STATE_FROZEN = 9;
    static constexpr s32 STATE_DEATH = 11;

    bool IsHead() const {
        return Typed()->actor.params == BONGO_HEAD;
    }

    const char* GetTitleCard() const override {
        return gBongoTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return gBossSstCutsceneCamera;
    }

    Vec3f_* GetCameraAt() override {
        return gBossSstCameraAt;
    }

    Vec3f_* GetCameraEye() override {
        return gBossSstCameraEye;
    }

    using SstActionFunc = decltype(&BossSst_HeadLurk);
    static const SstActionFunc* ActionTable(size_t* count) {
        static const SstActionFunc sTable[] = {
            BossSst_HeadLurk,        BossSst_HeadIntro,        BossSst_HeadNeutral,       BossSst_HeadWait,
            BossSst_HeadDamagedHand, BossSst_HeadReadyCharge,  BossSst_HeadCharge,        BossSst_HeadEndCharge,
            BossSst_HeadFrozenHand,  BossSst_HeadUnfreezeHand, BossSst_HeadStunned,       BossSst_HeadVulnerable,
            BossSst_HeadDamage,      BossSst_HeadRecover,      BossSst_HeadDeath,         BossSst_HeadThrash,
            BossSst_HeadDarken,      BossSst_HeadFall,         BossSst_HeadMelt,          BossSst_HeadFinish,

            BossSst_HandWait,        BossSst_HandDownbeat,     BossSst_HandOffbeat,       BossSst_HandDownbeatEnd,
            BossSst_HandOffbeatEnd,  BossSst_HandReadySlam,    BossSst_HandSlam,          BossSst_HandEndSlam,
            BossSst_HandReadySweep,  BossSst_HandSweep,        BossSst_HandReadyPunch,    BossSst_HandPunch,
            BossSst_HandReadyClap,   BossSst_HandClap,         BossSst_HandEndClap,       BossSst_HandReadyGrab,
            BossSst_HandGrab,        BossSst_HandCrush,        BossSst_HandEndCrush,      BossSst_HandSwing,
            BossSst_HandRetreat,     BossSst_HandReel,         BossSst_HandReadyShake,    BossSst_HandShake,
            BossSst_HandReadyCharge, BossSst_HandFrozen,       BossSst_HandReadyBreakIce, BossSst_HandBreakIce,
            BossSst_HandStunned,     BossSst_HandDamage,       BossSst_HandRecover,       BossSst_HandThrash,
            BossSst_HandDarken,      BossSst_HandFall,         BossSst_HandMelt,          BossSst_HandFinish,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const SstActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }


    const char* AnimForIndex(u8 idx) const {
        if (IsHead()) {
            switch (idx) {
                case 0:
                    return gBongoHeadEyeOpenIdleAnim;
                case 1:
                    return gBongoHeadEyeCloseIdleAnim;
                case 2:
                    return gBongoHeadEyeOpenAnim;
                case 3:
                    return gBongoHeadEyeCloseAnim;
                case 4:
                    return gBongoHeadChargeAnim;
                case 5:
                    return gBongoHeadDamageAnim;
                case 6:
                    return gBongoHeadDamagedHandAnim;
                case 7:
                    return gBongoHeadKnockoutAnim;
                case 8:
                    return gBongoHeadRecoverAnim;
                case 9:
                    return gBongoHeadStunnedAnim;
                default:
                    return nullptr;
            }
        }

        bool left = Typed()->actor.params == BONGO_LEFT_HAND;
        switch (idx) {
            case 0:
                return left ? gBongoLeftHandIdleAnim : gBongoRightHandIdleAnim;
            case 1:
                return left ? gBongoLeftHandClenchAnim : gBongoRightHandClenchAnim;
            case 2:
                return left ? gBongoLeftHandDamagePoseAnim : gBongoRightHandDamagePoseAnim;
            case 3:
                return left ? gBongoLeftHandFistPoseAnim : gBongoRightHandFistPoseAnim;
            case 4:
                return left ? gBongoLeftHandFlatPoseAnim : gBongoRightHandFlatPoseAnim;
            case 5:
                return left ? gBongoLeftHandHangPoseAnim : gBongoRightHandHangPoseAnim;
            case 6:
                return left ? gBongoLeftHandOpenPoseAnim : gBongoRightHandOpenPoseAnim;
            case 7:
                return left ? gBongoLeftHandPushoffPoseAnim : gBongoRightHandPushoffPoseAnim;
            default:
                return nullptr;
        }
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        u8 n = IsHead() ? ANIM_COUNT_HEAD : ANIM_COUNT_HAND;
        for (u8 i = 0; i < n; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    bool BossActive() const {
        BossSst* head = *gBossSstHead;
        return head != nullptr && head->actionFunc != BossSst_HeadLurk && head->actionFunc != BossSst_HeadIntro;
    }

    static u8 RolesToColliderBits(u8 roles) {
        return ((roles & COLL_AC) ? 1 : 0) | ((roles & COLL_AT) ? 2 : 0) | ((roles & COLL_OC) ? 4 : 0);
    }

    void CurrentColliderRoles(u8& sphRoles, u8& cylRoles) const {
        BossSst* sst = Typed();
        sphRoles = 0;
        cylRoles = 0;

        if (sst->colliderJntSph.base.atFlags & AT_ON)
            sphRoles |= COLL_AT;
        if (sst->colliderJntSph.base.ocFlags1 & OC1_ON)
            sphRoles |= COLL_OC;

        if (IsHead()) {
            if (BossActive()) {
                sphRoles |= COLL_AC;
                if (sst->colliderCyl.base.acFlags & AC_ON)
                    cylRoles |= COLL_AC;
            }
        } else {
            if (BossActive() && (sst->colliderJntSph.base.acFlags & AC_ON))
                sphRoles |= COLL_AC;
            if (sst->colliderCyl.base.atFlags & AT_ON)
                cylRoles |= COLL_AT;
        }
    }

    bool HitWouldReact() const {
        BossSst* sst = Typed();

        if (IsHead()) {
            if (!(sst->colliderCyl.base.acFlags & AC_HIT))
                return false;
        } else {
            if (!(sst->colliderJntSph.base.acFlags & AC_HIT))
                return false;
            if (sst->colliderJntSph.base.colType == COLTYPE_HARD)
                return false;
        }

        return sst->actor.colChkInfo.damageEffect != 0 || sst->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_STATE,
        PROP_GEOM,
        PROP_HEALTH,
        PROP_COLL_ROLES,
        PROP_SHARED,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_COLL_ELEMS,
    };

    void OnActorInit() override {
        BossSst* sst = Typed();

        ReinstallUpdate();

        if (IsHead()) {
            *gBossSstHead = sst;
            return;
        }

        gBossSstHands[sst->actor.params] = sst;

        BossSst* other = gBossSstHands[sst->actor.params == BONGO_LEFT_HAND ? BONGO_RIGHT_HAND : BONGO_LEFT_HAND];
        if (other != nullptr) {
            sst->actor.child = &other->actor;
            other->actor.child = &sst->actor;
        }

        if (Flags_GetClear(gPlayState, gPlayState->roomCtx.curRoom.num))
            Actor_Kill(m_actor);
    }

    
    void OnBecomeLeader() override {
        BossSst* sst = Typed();

        if (gPlayState == nullptr || sst->actor.params == BONGO_HEAD)
            return;

        if (sst->actionFunc == BossSst_HandCrush) {
            BossSst_HandReleasePlayer(sst, gPlayState, true);
            BossSst_HandSetupEndCrush(sst);
        } else if (sst->actionFunc == BossSst_HandGrab || sst->actionFunc == BossSst_HandSwing) {
            BossSst_HandReleasePlayer(sst, gPlayState, false);
            sst->actor.speedXZ = 0.0f;
            sst->colliderJntSph.base.atFlags &= ~(AT_ON | AT_HIT);
            BossSst_HandSetupRetreat(sst);
        }
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawInstalled();

        if (!m_hasAllLinks)
        {
            if (gBossSstHands[BONGO_LEFT_HAND] && gBossSstHands[BONGO_RIGHT_HAND] && *gBossSstHead)
                m_hasAllLinks = true;
            else
                return;
        }
        if (IsHead()) {
            for (s32 i = 0; i < 2; i++) {
                BossSst* hand = gBossSstHands[i];
                if (hand == nullptr || hand->actor.zoController == nullptr)
                    continue;
                BongoBongoController* ctrl = reinterpret_cast<BongoBongoController*>(hand->actor.zoController);
                if (!ctrl->IsLeader())
                    ctrl->ClaimLeadership(CLAIM_REASON_NOW);
            }
        }

        AbstractActorController::UpdateLeader(play);

        if (*gBossSstFloor != nullptr && (*gBossSstFloor)->dyna.actor.params == BONGOFLOOR_HIT) {
            SendTriggerToPuppets("bounce", ByteStream());
        }

        if (IsHead())
        {
            auto self = Typed();
            if (self->actionFunc == BossSst_HeadDeath && !Flags_GetClear(play, play->roomCtx.curRoom.num))
                Flags_SetClear(play, play->roomCtx.curRoom.num);
        }
    }

    void OnTrigger(const std::string& name, ByteStream& data) override {
        BossSst* sst = Typed();

        if (name == "bounce") {
            if (*gBossSstFloor != nullptr) {
                (*gBossSstFloor)->dyna.actor.params = BONGOFLOOR_HIT;
            }
            return;
        }

        if (name == "hit" && !IsHead()) {
            sst->actor.colChkInfo.damageEffect = (u8)(data.Read<PackedUInt1>().value());
            sst->actor.colChkInfo.damage = (u8)(data.Read<PackedUInt1>().value());
            sst->colliderJntSph.base.acFlags |= AC_HIT;
        }
    }

    void BuildCustomProperties(ByteStream& out) override {
        BossSst* sst = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(
            PROP_STATE,
            ByteStream() << PackedInt1(sst->actionVar) << PackedInt1(sst->ready) << PackedUInt1(sst->effectMode)
                         << PackedInt2(sst->timer) << PackedInt2(sst->handAngSpeed) << PackedInt2(sst->handMaxSpeed)
                         << PackedInt2(sst->handZPosMod) << PackedInt2(sst->handYRotMod) << PackedInt2(sst->amplitude)
                         << PackedInt2(sst->targetYaw) << PackedInt2(sst->targetRoll),
            out);

        PackProperty(PROP_GEOM,
                     ByteStream() << PackedFloat4(sst->radius) << PackedFloat4(sst->center.x)
                                  << PackedFloat4(sst->center.y) << PackedFloat4(sst->center.z),
                     out);

        PackProperty(PROP_HEALTH, PackedUInt1(sst->actor.colChkInfo.health), out);
        {
            u8 sphRoles, cylRoles;
            CurrentColliderRoles(sphRoles, cylRoles);
            PackProperty(PROP_COLL_ROLES, ByteStream() << PackedUInt1(sphRoles) << PackedUInt1(cylRoles), out);
        }

        {
            u32 bump = 0;
            u32 touch = 0;
            for (s32 i = 0; i < sst->colliderJntSph.count && i < 32; i++) {
                if (sst->colliderJntSph.elements[i].info.bumperFlags & BUMP_ON)
                    bump |= (1u << i);
                if (sst->colliderJntSph.elements[i].info.toucherFlags & TOUCH_ON)
                    touch |= (1u << i);
            }
            PackProperty(PROP_COLL_ELEMS, ByteStream() << PackedUInt4(bump) << PackedUInt4(touch), out);
        }


        if (IsHead())
            PackProperty(PROP_SHARED, PackedUInt1((u8)(*gBossSstBodyStatic)), out);
        else
            PackProperty(PROP_SHARED, PackedInt4(gBossSstHandState[sst->actor.params]), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(sst->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &sst->skelAnime), out);

        if (IsHead()) {
            BuildBossProperty(PROP_BOSS_CAMERA, out);
            BuildBossProperty(PROP_BOSS_BGM, out);
            BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
            BuildBossProperty(PROP_BOSS_LIGHTING, out);

        }

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossSst* sst = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const SstActionFunc* table = ActionTable(&count);
                if (id < count)
                    sst->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                sst->actionVar = (s8)(data.Read<PackedInt1>().value());
                sst->ready = (s8)(data.Read<PackedInt1>().value());
                sst->effectMode = (u8)(data.Read<PackedUInt1>().value());
                sst->timer = (s16)(data.Read<PackedInt2>().value());
                sst->handAngSpeed = (s16)(data.Read<PackedInt2>().value());
                sst->handMaxSpeed = (s16)(data.Read<PackedInt2>().value());
                sst->handZPosMod = (s16)(data.Read<PackedInt2>().value());
                sst->handYRotMod = (s16)(data.Read<PackedInt2>().value());
                sst->amplitude = (s16)(data.Read<PackedInt2>().value());
                sst->targetYaw = (s16)(data.Read<PackedInt2>().value());
                sst->targetRoll = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_GEOM:
                sst->radius = data.Read<PackedFloat4>().value();
                sst->center.x = data.Read<PackedFloat4>().value();
                sst->center.y = data.Read<PackedFloat4>().value();
                sst->center.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_HEALTH:
                sst->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_sphRoles = (u8)(data.Read<PackedUInt1>().value());
                m_cylRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_SHARED:
                if (IsHead())
                    *gBossSstBodyStatic = (u8)(data.Read<PackedUInt1>().value());
                else
                    gBossSstHandState[sst->actor.params] = data.Read<PackedInt4>().value();
                break;
            
            case PROP_COLL_ELEMS: {
                u32 bump = data.Read<PackedUInt4>().value();
                u32 touch = data.Read<PackedUInt4>().value();

                for (s32 i = 0; i < sst->colliderJntSph.count && i < 32; i++) {
                    if (bump & (1u << i))
                        sst->colliderJntSph.elements[i].info.bumperFlags |= BUMP_ON;
                    else
                        sst->colliderJntSph.elements[i].info.bumperFlags &= ~BUMP_ON;

                    if (touch & (1u << i))
                        sst->colliderJntSph.elements[i].info.toucherFlags |= TOUCH_ON;
                    else
                        sst->colliderJntSph.elements[i].info.toucherFlags &= ~TOUCH_ON;
                }
                break;
            }
           

            case PROP_ANIM_CUR_FRAME:
                sst->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &sst->skelAnime, LOCK_CUR_FRAME ? sst->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }


    void OnPropertiesApplied(u64 changed) override {
        auto self = Typed();

        if (IsHead()) {
            if (self->actionFunc == BossSst_HeadDeath) {
                EndCutsceneCamera();
                BossSst_HeadSetupDeath(self, gPlayState);
                GoLocal();
            }
        } else {
            if (self->actionFunc == BossSst_HandThrash) {
                BossSst_HandSetupThrash(self);
                GoLocal();
            }
        }
    }
    void UpdatePuppet(PlayState* play) override {
        BossSst* sst = Typed();

        EnsureDrawInstalled();

        if (!IsHead())
            ReleaseLocalPlayer(play);

        UpdateAnimation(&sst->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            if (IsHead()) {
                if (ClaimLeadership(CLAIM_REASON_COOLDOWN)) {
                    UpdateLeader(play);
                    return;
                }
            }

            ByteStream hit;
            hit << PackedUInt1(sst->actor.colChkInfo.damageEffect) << PackedUInt1(sst->actor.colChkInfo.damage);
            SendTriggerToLeader("hit", hit);
        }
        sst->colliderJntSph.base.acFlags &= ~AC_HIT;
        sst->colliderJntSph.base.atFlags &= ~AT_HIT;
        sst->colliderCyl.base.acFlags &= ~AC_HIT;
        sst->colliderCyl.base.atFlags &= ~AT_HIT;

        Actor_SetFocus(&sst->actor, 0.0f);

        if (IsHead())
            UpdateHeadVisibility(play);
        else
            PushHandTrail();

        ApplyColliderFlags(sst->colliderJntSph.base, RolesToColliderBits(m_sphRoles));
        ApplyColliderFlags(sst->colliderCyl.base, RolesToColliderBits(m_cylRoles));

        if (m_sphRoles != 0)
            RegisterColliderBase(play, &sst->colliderJntSph.base, m_sphRoles);
        if (m_cylRoles != 0)
            RegisterColliderBase(play, &sst->colliderCyl.base, m_cylRoles);

        BossSst_UpdateEffect(&sst->actor, play);
    }

  private:

    void EnsureDrawInstalled() {
        BossSst* sst = Typed();
        BossSst* head = *gBossSstHead;

        if (head == nullptr || head->actionFunc == BossSst_HeadLurk)
            return;

        if (head->actionFunc == BossSst_HeadIntro && head->timer > 460)
            return;

        if (sst->actor.draw == NULL)
            sst->actor.draw = IsHead() ? BossSst_DrawHead : BossSst_DrawHand;
    }


    void UpdateHeadVisibility(PlayState* play) {
        BossSst* sst = Typed();

        if (sst->actionVar) {
            if (!play->actorCtx.lensActive || (sst->actor.colorFilterTimer != 0))
                sst->actor.flags &= ~ACTOR_FLAG_REACT_TO_LENS;
            else
                sst->actor.flags |= ACTOR_FLAG_REACT_TO_LENS;
        }

        if ((!sst->actionVar || CHECK_FLAG_ALL(sst->actor.flags, ACTOR_FLAG_REACT_TO_LENS)) &&
            ((sst->actionFunc == BossSst_HeadReadyCharge) || (sst->actionFunc == BossSst_HeadCharge) ||
             (sst->actionFunc == BossSst_HeadFrozenHand) || (sst->actionFunc == BossSst_HeadStunned) ||
             (sst->actionFunc == BossSst_HeadVulnerable) || (sst->actionFunc == BossSst_HeadDamage)))
            sst->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
        else
            sst->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    }


    void ReleaseLocalPlayer(PlayState* play) {
        BossSst* sst = Typed();
        Player* player = GET_PLAYER(play);

        if (player->stateFlags2 & PLAYER_STATE2_GRABBED_BY_ENEMY) {
            if (player->actor.parent != &sst->actor)
                return;

            player->actor.shape.rot.x = 0;
            player->actor.shape.rot.z = 0;

            if (sst->actor.child != NULL) {
                BossSst_HandReleasePlayer(sst, play, true);
                return;
            }

            player->stateFlags2 &= ~PLAYER_STATE2_GRABBED_BY_ENEMY;
            player->csAction = 0;
            player->actor.parent = NULL;
            player->av2.actionVar2 = 100;
            sst->colliderJntSph.base.ocFlags1 |= OC1_ON;
            func_8002F71C(play, &sst->actor, 0.0f, sst->actor.shape.rot.y, 0.0f);
        }
    }

    void PushHandTrail() {
        BossSst* sst = Typed();
        BossSstHandTrail* trail;
        s32 state = gBossSstHandState[sst->actor.params];

        if ((state != STATE_DEATH) && (state != STATE_WAIT) && (state != STATE_BEAT) && (state != STATE_FROZEN)) {
            sst->trailCount++;
            sst->trailCount = CLAMP_MAX(sst->trailCount, 7);
        } else {
            sst->trailCount--;
            sst->trailCount = CLAMP_MIN(sst->trailCount, 0);
        }

        trail = &sst->handTrails[sst->trailIndex];
        Math_Vec3f_Copy(&trail->world.pos, &sst->actor.world.pos);
        trail->world.rot = sst->actor.shape.rot;
        trail->zPosMod = sst->handZPosMod;
        trail->yRotMod = sst->handYRotMod;

        sst->trailIndex = (sst->trailIndex + 1) % 7;
    }

    u8 m_sphRoles = 0;
    u8 m_cylRoles = 0;
    bool m_hasAllLinks = false;
};

} // namespace ZeldaOnline

#endif