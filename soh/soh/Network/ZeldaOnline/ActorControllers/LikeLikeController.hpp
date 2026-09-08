#ifndef LIKELIKECONTROLLERH
#define LIKELIKECONTROLLERH

#include <string>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Rr/z_en_rr.h"
    #include "src/overlays/actors/ovl_En_Dh/z_en_dh.h"

void EnRr_Approach(EnRr* rr, PlayState* play);
void EnRr_Reach(EnRr* rr, PlayState* play);
void EnRr_GrabPlayer(EnRr* rr, PlayState* play);
void EnRr_Damage(EnRr* rr, PlayState* play);
void EnRr_Death(EnRr* rr, PlayState* play);
void EnRr_Retreat(EnRr* rr, PlayState* play);
void EnRr_Stunned(EnRr* rr, PlayState* play);

void EnRr_UpdateBodySegments(EnRr* rr, PlayState* play);

void EnRr_SetupDeath(EnRr* rr);

void func_8002F6D4(PlayState* play, Actor* actor, f32 speed, s16 yaw, f32 velocityY, u32 flags);
}

namespace ZeldaOnline {

class LikeLikeController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnRr* Typed() const {
        return reinterpret_cast<EnRr*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using RrActionFunc = void (*)(EnRr*, PlayState*);
    static const RrActionFunc* ActionTable(size_t* count) {
        static const RrActionFunc sTable[] = {
            EnRr_Approach,
            EnRr_Reach,
            EnRr_GrabPlayer,
            EnRr_Damage,
            EnRr_Death,
            EnRr_Retreat,
            EnRr_Stunned,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const RrActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool HasSwallowedSomeone() const {
        return Typed()->hasPlayer != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_GRAB,
        PROP_DROP_TYPE,
        PROP_TIMERS,
        PROP_SEG_MOTION,
        PROP_SEG_TARGETS,
        PROP_HEALTH,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnRr* rr = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_GRAB,
                     ByteStream() << PackedInt2(rr->hasPlayer) << PackedUInt1(rr->reachState)
                                  << PackedUInt1(rr->retreat) << PackedUInt1(rr->stopScroll) << PackedUInt1(rr->isDead),
                     out);
        PackProperty(PROP_DROP_TYPE, PackedUInt1(rr->dropType), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(rr->frameCount) << PackedInt2(rr->actionTimer)
                                  << PackedInt2(rr->scrollTimer) << PackedInt2(rr->grabTimer)
                                  << PackedInt2(rr->invincibilityTimer) << PackedInt2(rr->effectTimer)
                                  << PackedInt2(rr->ocTimer),
                     out);
        PackProperty(PROP_SEG_MOTION,
                     ByteStream() << PackedInt2(rr->segMovePhase) << PackedFloat4(rr->segPhaseVel)
                                  << PackedFloat4(rr->segPhaseVelTarget) << PackedFloat4(rr->segPulsePhaseDiff)
                                  << PackedFloat4(rr->segWobblePhaseDiffX) << PackedFloat4(rr->segWobbleXTarget)
                                  << PackedFloat4(rr->segWobblePhaseDiffZ) << PackedFloat4(rr->segWobbleZTarget)
                                  << PackedFloat4(rr->pulseSize) << PackedFloat4(rr->pulseSizeTarget)
                                  << PackedFloat4(rr->wobbleSize) << PackedFloat4(rr->wobbleSizeTarget)
                                  << PackedFloat4(rr->segMoveRate) << PackedFloat4(rr->shrinkRate)
                                  << PackedFloat4(rr->swallowOffset),
                     out);
        {
            ByteStream seg;
            for (int i = 0; i < 5; i++) {
                seg << PackedFloat4(rr->bodySegs[i].heightTarget) << PackedFloat4(rr->bodySegs[i].scaleTarget.x)
                    << PackedFloat4(rr->bodySegs[i].rotTarget.x) << PackedFloat4(rr->bodySegs[i].rotTarget.z);
            }
            PackProperty(PROP_SEG_TARGETS, seg, out);
        }
        PackProperty(PROP_HEALTH, PackedUInt1(rr->actor.colChkInfo.health), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnRr* rr = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const RrActionFunc* table = ActionTable(&count);
                if (id < count)
                    rr->actionFunc = table[id];
                break;
            }
            case PROP_GRAB:
                rr->hasPlayer = (s16)(data.Read<PackedInt2>().value());
                rr->reachState = (u8)(data.Read<PackedUInt1>().value());
                rr->retreat = (u8)(data.Read<PackedUInt1>().value());
                rr->stopScroll = (u8)(data.Read<PackedUInt1>().value());
                rr->isDead = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_DROP_TYPE:
                rr->dropType = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                rr->frameCount = (s16)(data.Read<PackedInt2>().value());
                rr->actionTimer = (s16)(data.Read<PackedInt2>().value());
                rr->scrollTimer = (s16)(data.Read<PackedInt2>().value());
                rr->grabTimer = (s16)(data.Read<PackedInt2>().value());
                rr->invincibilityTimer = (s16)(data.Read<PackedInt2>().value());
                rr->effectTimer = (s16)(data.Read<PackedInt2>().value());
                rr->ocTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SEG_MOTION:
                rr->segMovePhase = (s16)(data.Read<PackedInt2>().value());
                rr->segPhaseVel = data.Read<PackedFloat4>().value();
                rr->segPhaseVelTarget = data.Read<PackedFloat4>().value();
                rr->segPulsePhaseDiff = data.Read<PackedFloat4>().value();
                rr->segWobblePhaseDiffX = data.Read<PackedFloat4>().value();
                rr->segWobbleXTarget = data.Read<PackedFloat4>().value();
                rr->segWobblePhaseDiffZ = data.Read<PackedFloat4>().value();
                rr->segWobbleZTarget = data.Read<PackedFloat4>().value();
                rr->pulseSize = data.Read<PackedFloat4>().value();
                rr->pulseSizeTarget = data.Read<PackedFloat4>().value();
                rr->wobbleSize = data.Read<PackedFloat4>().value();
                rr->wobbleSizeTarget = data.Read<PackedFloat4>().value();
                rr->segMoveRate = data.Read<PackedFloat4>().value();
                rr->shrinkRate = data.Read<PackedFloat4>().value();
                rr->swallowOffset = data.Read<PackedFloat4>().value();
                break;
            case PROP_SEG_TARGETS:
                for (int i = 0; i < 5; i++) {
                    rr->bodySegs[i].heightTarget = data.Read<PackedFloat4>().value();
                    rr->bodySegs[i].scaleTarget.x = data.Read<PackedFloat4>().value();
                    rr->bodySegs[i].scaleTarget.z = rr->bodySegs[i].scaleTarget.x;
                    rr->bodySegs[i].rotTarget.x = data.Read<PackedFloat4>().value();
                    rr->bodySegs[i].rotTarget.z = data.Read<PackedFloat4>().value();
                }
                break;
            case PROP_HEALTH:
                rr->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;

            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnRr_Death) {
            EnRr_SetupDeath(Typed());
            GoLocal();
        }
    }

    void OnTrigger(const std::string& name, ByteStream& data) override {
        if (name != "hit")
            return;

        EnRr* rr = Typed();
        if (data.BytesLeft() >= 2) {
            rr->actor.colChkInfo.damageEffect = (u8)(data.Read<PackedUInt1>().value());
            rr->actor.colChkInfo.damage = (u8)(data.Read<PackedUInt1>().value());
        }
        rr->collider1.base.acFlags |= AC_HIT;
    }

    void OnBecomeLeader() override {
        EnRr* rr = Typed();
        if (!rr->hasPlayer) {
            return;
        }

        rr->hasPlayer = false;
        rr->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
        rr->ocTimer = 110;
        rr->segMoveRate = 0.0f;
        rr->segPhaseVelTarget = 2500.0f;
        rr->wobbleSizeTarget = 2048.0f;
        rr->actionFunc = EnRr_Approach;
    }

    void UpdatePuppet(PlayState* play) override {
        EnRr* rr = Typed();

        if (HasSwallowedSomeone()) {
            if (rr->collider1.base.acFlags & AC_HIT) {
                ByteStream payload;
                payload << PackedUInt1(rr->actor.colChkInfo.damageEffect) << PackedUInt1(rr->actor.colChkInfo.damage);
                SendTriggerToLeader("hit", payload);
            }
            rr->collider1.base.acFlags &= ~AC_HIT;
            rr->collider2.base.acFlags &= ~AC_HIT;
        } else {
            if ((rr->collider1.base.acFlags & AC_HIT) || (rr->collider2.base.acFlags & AC_HIT)) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }

            if (rr->actionFunc != EnRr_Death && rr->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
                ClaimLeadership(CLAIM_REASON_COOLDOWN);

            rr->collider1.base.acFlags &= ~AC_HIT;
            rr->collider2.base.acFlags &= ~AC_HIT;
        }

        {
            Player* localPlayer = GET_PLAYER(play);
            if (!rr->hasPlayer && localPlayer->actor.parent == &rr->actor) {
                localPlayer->actor.parent = NULL;
                func_8002F6D4(play, &rr->actor, 4.0f, rr->actor.shape.rot.y, 12.0f, 8);
            }
        }

        Actor_SetFocus(&rr->actor, 30.0f);

        EnRr_UpdateBodySegments(rr, play);

        if (!rr->stopScroll) {
            for (int i = 0; i < 5; i++) {
                Math_SmoothStepToS(&rr->bodySegs[i].rot.x, (s16)(rr->bodySegs[i].rotTarget.x), 5,
                                   (s16)(rr->segMoveRate * 1000.0f), 0);
                Math_SmoothStepToS(&rr->bodySegs[i].rot.z, (s16)(rr->bodySegs[i].rotTarget.z), 5,
                                   (s16)(rr->segMoveRate * 1000.0f), 0);
                Math_ApproachF(&rr->bodySegs[i].scale.x, rr->bodySegs[i].scaleTarget.x, 1.0f, rr->segMoveRate * 0.2f);
                rr->bodySegs[i].scale.z = rr->bodySegs[i].scale.x;
                Math_ApproachF(&rr->bodySegs[i].height, rr->bodySegs[i].heightTarget, 1.0f, rr->segMoveRate * 300.0f);
            }
            Math_ApproachF(&rr->segMoveRate, 1.0f, 1.0f, 0.2f);
        }

        Collider_UpdateCylinder(&rr->actor, &rr->collider1);
        rr->collider2.dim.pos.x = (s16)(rr->mouthPos.x);
        rr->collider2.dim.pos.y = (s16)(rr->mouthPos.y);
        rr->collider2.dim.pos.z = (s16)(rr->mouthPos.z);

        if (!rr->isDead && rr->invincibilityTimer == 0) {
            RegisterColliderBase(play, &rr->collider1.base, COLL_AC);
            RegisterColliderBase(play, &rr->collider2.base, COLL_AC | COLL_OC);
            if (rr->ocTimer == 0)
                RegisterColliderBase(play, &rr->collider1.base, COLL_OC);
        } else {
            rr->collider1.base.ocFlags1 &= ~OC1_HIT;
            rr->collider2.base.ocFlags1 &= ~OC1_HIT;
        }
    }
};

}

#endif
