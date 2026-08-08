#ifndef SKULLTULACONTROLLERH
#define SKULLTULACONTROLLERH

/*
* Most controllers are using the same pattern as this so i'm going to only comment this one
* 
* The way these work, you have one leader client who runs the real vanilla update for this actor
* The rest of the clients are calling UpdatePuppet() which is only showing the animation and detecting
* hits, seeing if they're the closest player, z-targetting, shadow position, special effects. If a hit is detected 
on the puppet side, they will claim leadership. Same with detecting if the local player is closest, leadership can be switched to that player

we are syncing as much properties as possible so any client can become leader at any time and continue the simulation without fault

After you have made a controller, you need to add its registration into ActorControllerFactory. If you don't this actor will not be
synced. There would be a separate copy on each client.
*/

#include "../AbstractActorController.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "src/overlays/actors/ovl_En_St/z_en_st.h"

extern "C" {
void EnSt_WaitOnCeiling(EnSt* skulltula, PlayState* play);
void EnSt_WaitOnGround(EnSt* skulltula, PlayState* play);
void EnSt_LandOnGround(EnSt* skulltula, PlayState* play);
void EnSt_MoveToGround(EnSt* skulltula, PlayState* play);
void EnSt_ReturnToCeiling(EnSt* skulltula, PlayState* play);
void EnSt_StartOnCeilingOrGround(EnSt* skulltula, PlayState* play);
void EnSt_BounceAround(EnSt* skulltula, PlayState* play);
void EnSt_FinishBouncing(EnSt* skulltula, PlayState* play);
void EnSt_Die(EnSt* skulltula, PlayState* play);
void EnSt_SetupAction(EnSt* skulltula, EnStActionFunc actionFunc);

void EnSt_SetBodyCylinderAC(EnSt* skulltula, PlayState* play);
void EnSt_SetLegsCylinderAC(EnSt* skulltula, PlayState* play);
s32 EnSt_SetCylinderOC(EnSt* skulltula, PlayState* play);

extern "C" AnimationInfo* gEnStAnimationInfo;
}

namespace ZeldaOnline {

class SkulltulaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    //Returns the overlay struct* for this actor
    EnSt* Typed() const {
        return reinterpret_cast<EnSt*>(m_actor);
    }

    //If this is true, the leader will also send their exact animation frame
    //And a puppet will lock it to this frame instead of continuing to the next frame
    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ACTION_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 6;

    //Returns an array with all the actors "actionFunc"
    static const EnStActionFunc* ActionTable(size_t* count) {
        static const EnStActionFunc sTable[] = {
            (EnStActionFunc)EnSt_WaitOnCeiling,
            (EnStActionFunc)EnSt_WaitOnGround,
            (EnStActionFunc)EnSt_LandOnGround,
            (EnStActionFunc)EnSt_MoveToGround,
            (EnStActionFunc)EnSt_ReturnToCeiling,
            (EnStActionFunc)EnSt_StartOnCeilingOrGround,
            (EnStActionFunc)EnSt_BounceAround,
            (EnStActionFunc)EnSt_FinishBouncing,
            (EnStActionFunc)EnSt_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    //Convert the actors "actionFunc" to an index to be sent
    u8 CurrentActionIndex() const {
        size_t count;
        const EnStActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ACTION_UNKNOWN;
    }

    //Anim index to -> anim
    static void* AnimForIndex(u8 i) {
        if (i >= ANIM_COUNT)
            return nullptr;
        return (void*)gEnStAnimationInfo[i].animation;
    }

    //current actors animation to an index to be sent
    u8 CurrentAnimIndex() const {
        for (int i = 0; i < ANIM_COUNT; i++)
            if (Typed()->skelAnime.animation == gEnStAnimationInfo[i].animation)
                return (u8)(i);
        return ANIM_UNKNOWN;
    }

    //Returns true if the actor was hit
    bool AnyAcHit() const {
        EnSt* st = Typed();
        for (int i = 0; i < 6; i++)
            if (st->colCylinder[i].base.acFlags & AC_HIT)
                return true;
        return (st->colSph.base.acFlags & AC_HIT) != 0;
    }

    //List of custom properties
    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_TIMERS,
        PROP_ANIM_FRAMES,
        PROP_INITIAL_YAW,
        PROP_TEETH,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    //Here we build the FULL properties list. The format is [index][len][data]
    //Another function will take these properties, compare it to the previous
    //time we sent their properties and only send the modified ones
    void BuildCustomProperties(ByteStream& out) override {
        EnSt* st = Typed();

        // BuildStandardExtendedProperty has various "Actor" properties already
        //written out for you
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(st->actor.colChkInfo.health), out);

        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(st->swayTimer) << PackedInt2(st->stunTimer)
                                  << PackedInt2(st->invulnerableTimer) << PackedInt2(st->takeDamageSpinTimer)
                                  << PackedInt2(st->gaveDamageSpinTimer) << PackedInt2(st->setTargetYawTimer)
                                  << PackedInt2(st->rotAwayTimer) << PackedInt2(st->rotTowardsTimer)
                                  << PackedInt2(st->deathTimer) << PackedInt2(st->finishDeathTimer)
                                  << PackedInt2(st->groundBounces) << PackedInt2(st->deathYawTarget)
                                  << PackedInt2(st->absPrevSwayAngle) << PackedUInt1(st->playSwayFlag),
                     out);

        PackProperty(PROP_ANIM_FRAMES, PackedInt2(st->animFrames), out);

        PackProperty(PROP_INITIAL_YAW, PackedInt2(st->initalYaw), out);

        PackProperty(PROP_TEETH,
                     ByteStream() << PackedUInt1(st->teethR) << PackedUInt1(st->teethG) << PackedUInt1(st->teethB),
                     out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(st->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &st->skelAnime), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

    }

    //Another function will call this to apply the property data that was sent
    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnSt* st = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != ACTION_UNKNOWN) {
                    size_t count;
                    const EnStActionFunc* table = ActionTable(&count);
                    if (ai < count)
                        EnSt_SetupAction(st, table[ai]);
                }
                return true;
            }
            case PROP_ANIM_CUR_FRAME:
                st->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                return true;
            case PROP_ANIM: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                void* anim = (ai != ANIM_UNKNOWN) ? AnimForIndex(ai) : nullptr;
                ApplyAnimProperty(anim, &st->skelAnime, LOCK_CUR_FRAME ? st->skelAnime.curFrame : 0.0f, data);
                return true;
            }
            case PROP_HEALTH:
                st->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_TIMERS:
                st->swayTimer = (s16)(data.Read<PackedInt2>().value());
                st->stunTimer = (s16)(data.Read<PackedInt2>().value());
                st->invulnerableTimer = (s16)(data.Read<PackedInt2>().value());
                st->takeDamageSpinTimer = (s16)(data.Read<PackedInt2>().value());
                st->gaveDamageSpinTimer = (s16)(data.Read<PackedInt2>().value());
                st->setTargetYawTimer = (s16)(data.Read<PackedInt2>().value());
                st->rotAwayTimer = (s16)(data.Read<PackedInt2>().value());
                st->rotTowardsTimer = (s16)(data.Read<PackedInt2>().value());
                st->deathTimer = (s16)(data.Read<PackedInt2>().value());
                st->finishDeathTimer = (s16)(data.Read<PackedInt2>().value());
                st->groundBounces = (s16)(data.Read<PackedInt2>().value());
                st->deathYawTarget = (s16)(data.Read<PackedInt2>().value());
                st->absPrevSwayAngle = (s16)(data.Read<PackedInt2>().value());
                st->playSwayFlag = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_ANIM_FRAMES:
                st->animFrames = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_INITIAL_YAW:
                st->initalYaw = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_TEETH:
                st->teethR = (u8)(data.Read<PackedUInt1>().value());
                st->teethG = (u8)(data.Read<PackedUInt1>().value());
                st->teethB = (u8)(data.Read<PackedUInt1>().value());
                return true;
            default:
                return false;
        }
    }

    //After we have applied all the new properties the leader sent
    //changed is is a bit flag of the props that were changed in this
    //packet example if(changed & (1 << PROP_TEETH)) will be true
    //if the PROP_TEETH was changed in this packet
    void OnPropertiesApplied(u64 changed) override {
        if (CurrentActionIndex() >= 6) {
            auto spider = Typed();

            spider->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
            spider->groundBounces = 3;
            spider->deathTimer = 20;
            spider->actor.gravity = -1.0f;
            Audio_PlayActorSound2(&spider->actor, NA_SE_EN_STALWALL_DEAD);
            GameInteractor_ExecuteOnEnemyDefeat(&spider->actor);

            EnSt_SetupAction(spider, EnSt_BounceAround);

            GoLocal();
        }
    }

    //Code that is run when we are the leader
    //This just calls the base function. You wont need to include this
    //if you havnt changed the leader logic
    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
    }

    //The code that is run when we are in puppet mode
    void UpdatePuppet(PlayState* play) override {
        EnSt* st = Typed();

        UpdateAnimation(&st->skelAnime, LOCK_CUR_FRAME);

        if (AnyAcHit()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        if (st->actionFunc == (EnStActionFunc)EnSt_WaitOnCeiling && st->actor.xzDistToPlayer < 150.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        //Setting the various hit box and collision boxes for this frame
        //OC is for collisions. Without setting it, you will walk right through the Actor
        //AC is the attack collisions.
        if (st->actor.colChkInfo.health != 0 || st->actionFunc == (EnStActionFunc)EnSt_FinishBouncing) {
            if (st->gaveDamageSpinTimer == 0)
                EnSt_SetCylinderOC(st, play);
            if (st->invulnerableTimer == 0 && st->takeDamageSpinTimer == 0) {
                EnSt_SetBodyCylinderAC(st, play);
                EnSt_SetLegsCylinderAC(st, play);
            }
        }
        Actor_SetFocus(&st->actor, 0.0f);
    }
};

}

#endif
