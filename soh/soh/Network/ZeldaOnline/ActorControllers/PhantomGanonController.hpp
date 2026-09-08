#ifndef PHANTOMGANONCONTROLLERH
#define PHANTOMGANONCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Ganondrof/z_boss_ganondrof.h"
#include "src/overlays/actors/ovl_En_Fhg_Fire/z_en_fhg_fire.h"
#include "objects/object_gnd/object_gnd.h"

void BossGanondrof_Intro(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Paintings(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Neutral(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Throw(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Block(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Return(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Charge(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Stunned(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_Death(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_SetupDeath(BossGanondrof* gnd, PlayState* play);
void BossGanondrof_SetColliderPos(Vec3f* pos, ColliderCylinder* collider);
void BossGanondrof_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class PhantomGanonController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BossGanondrof* Typed() const {
        return reinterpret_cast<BossGanondrof*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    static bool IsNetworkedVariant(s16 params) {
        return true;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        if (actorId == ACTOR_EN_FHG)
            return params < GND_FAKE_BOSS;

        return actorId == ACTOR_EN_FHG_FIRE;
    }


    void OnTrigger(const std::string& name, ByteStream& data) override {
        if (name == "gndhit") {
            m_pendingHit = true;
            m_pendingDmgFlags = data.Read<PackedUInt4>().value();
        }
    }

    bool IsOnHorse() const {
        return Typed()->flyMode == GND_FLY_PAINTING;
    }

   

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using GndActionFunc = void (*)(BossGanondrof*, PlayState*);
    static const GndActionFunc* ActionTable(size_t* count) {
        static const GndActionFunc sTable[] = {
            BossGanondrof_Intro,  BossGanondrof_Paintings, BossGanondrof_Neutral,
            BossGanondrof_Throw,  BossGanondrof_Block,     BossGanondrof_Return,
            BossGanondrof_Charge, BossGanondrof_Stunned,   BossGanondrof_Death,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const GndActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 23;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gPhantomGanonNeutralAnim;
            case 1:
                return gPhantomGanonThrowAnim;
            case 2:
                return gPhantomGanonThrowEndAnim;
            case 3:
                return gPhantomGanonBlockAnim;
            case 4:
                return gPhantomGanonReturn1Anim;
            case 5:
                return gPhantomGanonReturn2Anim;
            case 6:
                return gPhantomGanonChargeStartAnim;
            case 7:
                return gPhantomGanonChargeWindupAnim;
            case 8:
                return gPhantomGanonChargeAnim;
            case 9:
                return gPhantomGanonStunnedAnim;
            case 10:
                return gPhantomGanonAirDamageAnim;
            case 11:
                return gPhantomGanonGroundDamageAnim;
            case 12:
                return gPhantomGanonDeathBlowAnim;
            case 13:
                return gPhantomGanonLastPoseAnim;
            case 14:
                return gPhantomGanonLimpAnim;
            case 15:
                return gPhantomGanonMaskOnAnim;
            case 16:
                return gPhantomGanonScreamAnim;
            case 17:
                return gPhantomGanonHorseRearingAnim;
            case 18:
                return gPhantomGanonRideAnim;
            case 19:
                return gPhantomGanonRidePoseAnim;
            case 20:
                return gPhantomGanonRideSpearRaiseAnim;
            case 21:
                return gPhantomGanonRideSpearResetAnim;
            case 22:
                return gPhantomGanonRideSpearStrikeAnim;
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

    bool IsDying() const {
        return Typed()->actionFunc == BossGanondrof_Death;
    }

    bool HitWouldReact() const {
        BossGanondrof* gnd = Typed();
        if (gnd->actor.params != GND_REAL_BOSS)
            return false;
        return (gnd->colliderBody.base.acFlags & AC_HIT) != 0;
    }

    u8 CurrentColliderRoles() const {
        BossGanondrof* gnd = Typed();
        u8 roles = 0;
        if (gnd->colliderBody.base.acFlags & AC_ON)
            roles |= COLL_AC;
        if (gnd->colliderBody.base.ocFlags1 & OC1_ON)
            roles |= COLL_OC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WORK,
        PROP_TIMERS,
        PROP_FWORK,
        PROP_BOSS_STATE,
        PROP_SPEAR,
        PROP_TARGET,
        PROP_LIMBS,
        PROP_COLL_ROLES,

        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void InitActorHealth() override {
        //Typed()->actor.colChkInfo.health *= 2;
    }

    void OnActorInit() override {

        if (Flags_GetClear(gPlayState, gPlayState->roomCtx.curRoom.num))
            Actor_Kill(m_actor);
    }

    void BuildCustomProperties(ByteStream& out) override {
        BossGanondrof* gnd = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        ByteStream work;
        for (s32 i = 0; i < GND_SHORT_COUNT; i++)
            work << PackedInt2(gnd->work[i]);
        PackProperty(PROP_WORK, work, out);

        ByteStream timers;
        for (s32 i = 0; i < 5; i++)
            timers << PackedInt2(gnd->timers[i]);
        PackProperty(PROP_TIMERS, timers, out);

        ByteStream fwork;
        for (s32 i = 0; i < GND_FLOAT_COUNT; i++)
            fwork << PackedFloat4(gnd->fwork[i]);
        PackProperty(PROP_FWORK, fwork, out);

        PackProperty(PROP_BOSS_STATE,
                     ByteStream() 
                         << PackedUInt1(gnd->returnCount) << PackedUInt1(gnd->shockTimer) << PackedUInt1(gnd->flyMode)
                         << PackedUInt1(gnd->returnSuccess) << PackedInt2(gnd->deathState)
                         << PackedUInt1(gnd->actor.colChkInfo.health),
                     out);
        PackProperty(PROP_SPEAR,
                     ByteStream() << PackedFloat4(gnd->spearTip.x) << PackedFloat4(gnd->spearTip.y)
                                  << PackedFloat4(gnd->spearTip.z),
                     out);
        PackProperty(PROP_TARGET,
                     ByteStream() << PackedFloat4(gnd->targetPos.x) << PackedFloat4(gnd->targetPos.y)
                                  << PackedFloat4(gnd->targetPos.z),
                     out);


        PackProperty(PROP_LIMBS,
                     ByteStream() << PackedFloat4(gnd->legRotY) << PackedFloat4(gnd->legRotZ)
                                  << PackedFloat4(gnd->legSplitY) << PackedFloat4(gnd->armRotY)
                                  << PackedFloat4(gnd->armRotZ),
                     out);

        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(gnd->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &gnd->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossGanondrof* gnd = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const GndActionFunc* table = ActionTable(&count);
                if (id < count)
                    gnd->actionFunc = table[id];
                break;
            }
            case PROP_WORK:
                for (s32 i = 0; i < GND_SHORT_COUNT; i++)
                    gnd->work[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 5; i++)
                    gnd->timers[i] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FWORK:
                for (s32 i = 0; i < GND_FLOAT_COUNT; i++)
                    gnd->fwork[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_BOSS_STATE:
                gnd->returnCount = (u8)(data.Read<PackedUInt1>().value());
                gnd->shockTimer = (u8)(data.Read<PackedUInt1>().value());
                gnd->flyMode = (u8)(data.Read<PackedUInt1>().value());
                gnd->returnSuccess = (u8)(data.Read<PackedUInt1>().value());
                gnd->deathState = (s16)(data.Read<PackedInt2>().value());
                gnd->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_SPEAR:
                gnd->spearTip.x = data.Read<PackedFloat4>().value();
                gnd->spearTip.y = data.Read<PackedFloat4>().value();
                gnd->spearTip.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_TARGET:
                gnd->targetPos.x = data.Read<PackedFloat4>().value();
                gnd->targetPos.y = data.Read<PackedFloat4>().value();
                gnd->targetPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_LIMBS:
                gnd->legRotY = data.Read<PackedFloat4>().value();
                gnd->legRotZ = data.Read<PackedFloat4>().value();
                gnd->legSplitY = data.Read<PackedFloat4>().value();
                gnd->armRotY = data.Read<PackedFloat4>().value();
                gnd->armRotZ = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_ANIM_CUR_FRAME:
                gnd->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &gnd->skelAnime, LOCK_CUR_FRAME ? gnd->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BossGanondrof* gnd = Typed();

        if (gnd->actionFunc != BossGanondrof_Death || gPlayState == nullptr) {
            return;
        }

        BossGanondrof_SetupDeath(gnd, gPlayState);
        GoLocal();
    }

    void RestoreHorseLink(PlayState* play) {
        BossGanondrof* gnd = Typed();
        if (gnd->actor.params != GND_REAL_BOSS)
            return;

        Actor* child = gnd->actor.child;
        if (child != nullptr && child->id == ACTOR_EN_FHG && child->params < GND_FAKE_BOSS)
            return;

        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_BG].head; it != nullptr; it = it->next) {
            if (it->id == ACTOR_EN_FHG && it->params < GND_FAKE_BOSS) {
                gnd->actor.child = it;
                return;
            }
        }
    }


    void UpdateLeader(PlayState* play) override {
        auto gnd = Typed();

        if (m_pendingHit) {
            m_pendingHit = false;
            m_fakeHitInfo.toucher.dmgFlags = m_pendingDmgFlags;
            gnd->colliderBody.info.acHitInfo = &m_fakeHitInfo;
            gnd->colliderBody.base.acFlags |= AC_HIT;
        }

        // Find the horse and make the horse leader also. The horse follows our leadership. Don't let two different
        // players control ganon and horse
        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_BOSS].head; it != nullptr; it = it->next) {
            if (it->id != ACTOR_EN_FHG || it->zoController == nullptr || it->params >= GND_FAKE_BOSS)
                continue;

            auto* ctl = reinterpret_cast<AbstractActorController*>(it->zoController);
            if (!ctl->IsLeader())
                ctl->ClaimLeadership(CLAIM_REASON_NOW);
            break;
        }

        AbstractActorController::UpdateLeader(play);
        if (gnd->actionFunc == BossGanondrof_Death && !Flags_GetClear(play, play->roomCtx.curRoom.num))
            Flags_SetClear(play, play->roomCtx.curRoom.num);
    }

    void UpdatePuppet(PlayState* play) override {
        BossGanondrof* gnd = Typed();
        RestoreHorseLink(play);

        if (gnd->actor.child == nullptr || gnd->actor.child->id != ACTOR_EN_FHG) {
            gnd->actor.draw = nullptr;
            return;
        }
        gnd->actor.draw = BossGanondrof_Draw;

        UpdateAnimation(&gnd->skelAnime, LOCK_CUR_FRAME);

        if (gnd->actor.params == GND_REAL_BOSS && HitWouldReact()) {
            if (IsOnHorse()) {
                SendTriggerToLeader("gndhit", ByteStream() << PackedUInt4(gnd->colliderBody.info.acHitInfo->toucher.dmgFlags));
            } else if (ClaimLeadership(CLAIM_REASON_COOLDOWN))
            {
                UpdateLeader(play);
                return;
            }
        }
        gnd->colliderBody.base.acFlags &= ~AC_HIT;
        gnd->colliderSpear.base.acFlags &= ~AC_HIT;

        Actor_SetFocus(&gnd->actor, 0.0f);

        /*
        if (gnd->work[GND_VARIANCE_TIMER] != m_lastVarianceTimer) {
            m_lastVarianceTimer = gnd->work[GND_VARIANCE_TIMER];

            if (!(m_lastVarianceTimer & 7)) {
                Actor_SpawnAsChild(&play->actorCtx, &gnd->actor, play, ACTOR_EN_FHG_FIRE, gnd->spearTip.x,
                                   gnd->spearTip.y, gnd->spearTip.z, 8, FHGFIRE_LIGHT_BLUE, 0, FHGFIRE_SPEAR_LIGHT);
            }
        }
        */
        if (m_roles != 0) {
            Collider_UpdateCylinder(&gnd->actor, &gnd->colliderBody);
            RegisterColliderBase(play, &gnd->colliderBody.base, m_roles);
        }


        BossGanondrof_SetColliderPos(&gnd->spearTip, &gnd->colliderSpear);
        RegisterColliderBase(play, &gnd->colliderSpear.base, COLL_AC);
    }

  private:
    bool m_pendingHit = false;
    u8 m_roles = COLL_AC | COLL_OC;
    s16 m_lastVarianceTimer = -1;
    ColliderInfo m_fakeHitInfo{};
    uint32_t m_pendingDmgFlags = 0;

};

} // namespace ZeldaOnline

#endif