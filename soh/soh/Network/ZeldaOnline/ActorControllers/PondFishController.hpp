#ifndef PONDFISHCONTROLLERH
#define PONDFISHCONTROLLERH

#include "../AbstractActorController.hpp"
#include "../ZeldaOnlineClient.hpp"
#include "PlayerPuppetController.hpp"
#include "src/overlays/actors/ovl_Fishing/z_fishing.h"
#include "soh/Enhancements/nametag.h"

extern "C" {


extern Vec3f sLurePos;
extern Fishing* sFishingHookedFish;
bool getShouldSpawnLoaches();
extern FishingFishInit sFishInits[];
extern f32 sFishOnHandLength;
}

namespace ZeldaOnline {

class PondFishController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    Fishing* Typed() const {
        return reinterpret_cast<Fishing*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return params != EN_FISH_AQUARIUM;
    }

    bool IsFish() const {
        return m_actor->params != EN_FISH_OWNER;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId == ACTOR_FISHING;
    }

  protected:
    enum {
        PROP_STATE = PROP_CUSTOM_START,
        PROP_ROTATION,
        PROP_LIMBS,
        PROP_SWIM,
        PROP_TARGET_POS,
        PROP_MOUTH_POS,
        PROP_TIMERS,
    };

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    bool IsHeld() const {
        return Typed()->fishState == 6;
    }

    f32 WeightLbs() const {
        Fishing* fish = Typed();
        f32 lbs = (fish->fishLength * fish->fishLength * 0.0036f) + 0.5f;
        return fish->isLoach ? (lbs * 2.0f) : lbs;
    }


    void BuildCustomProperties(ByteStream& out) override {
        Fishing* fish = Typed();

        if (!IsFish()) {
            return;
        }

        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(fish->fishState) << PackedInt2(fish->fishStateNext)
                                  << PackedInt2(fish->stateAndTimer) << PackedUInt1(fish->lilyTimer)
                                  << PackedUInt1(fish->bubbleTime) << PackedUInt1(fish->keepState)
                                  << PackedFloat4(fish->fishLength) << PackedUInt1(fish->isLoach),
                     out);

        PackProperty(PROP_ROTATION,
                     ByteStream() << PackedInt2(fish->unk_160) << PackedInt2(fish->unk_162) << PackedInt2(fish->unk_164)
                                  << PackedInt2(fish->rotationTarget.x) << PackedInt2(fish->rotationTarget.y)
                                  << PackedInt2(fish->rotationTarget.z) << PackedFloat4(fish->rotationStep),
                     out);


        PackProperty(PROP_LIMBS,
                     ByteStream() << PackedInt2(fish->fishLimb23RotYDelta) << PackedInt2(fish->fishLimbDRotZDelta)
                                  << PackedInt2(fish->fishLimbEFRotYDelta) << PackedInt2(fish->fishLimb89RotYDelta)
                                  << PackedInt2(fish->fishLimb4RotYDelta) << PackedInt2(fish->loachRotYDelta[0])
                                  << PackedInt2(fish->loachRotYDelta[1]) << PackedInt2(fish->loachRotYDelta[2]),
                     out);

        PackProperty(PROP_SWIM,
                     ByteStream() << PackedFloat4(fish->fishLimbRotPhase) << PackedFloat4(fish->fishLimbRotPhaseStep)
                                  << PackedFloat4(fish->fishLimbRotPhaseMag) << PackedFloat4(fish->speedTarget),
                     out);

        PackProperty(PROP_TARGET_POS,
                     ByteStream() << PackedFloat4(fish->fishTargetPos.x) << PackedFloat4(fish->fishTargetPos.y)
                                  << PackedFloat4(fish->fishTargetPos.z),
                     out);

        PackProperty(PROP_MOUTH_POS,
                     ByteStream() << PackedFloat4(fish->fishMouthPos.x) << PackedFloat4(fish->fishMouthPos.y)
                                  << PackedFloat4(fish->fishMouthPos.z),
                     out);

        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(fish->timerArray[0]) << PackedInt2(fish->timerArray[1])
                                  << PackedInt2(fish->timerArray[2]) << PackedInt2(fish->timerArray[3])
                                  << PackedInt2(fish->bumpTimer) << PackedInt2(fish->unk_1A2)
                                  << PackedInt2(fish->unk_1A4),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        Fishing* fish = Typed();

        if (!IsFish()) {
            return false;
        }

        switch (index) {
            case PROP_STATE:
                fish->fishState = (s16)(data.Read<PackedInt2>().value());
                fish->fishStateNext = (s16)(data.Read<PackedInt2>().value());
                fish->stateAndTimer = (s16)(data.Read<PackedInt2>().value());
                fish->lilyTimer = (u8)(data.Read<PackedUInt1>().value());
                fish->bubbleTime = (u8)(data.Read<PackedUInt1>().value());
                fish->keepState = (u8)(data.Read<PackedUInt1>().value());
                fish->fishLength = data.Read<PackedFloat4>().value();
                fish->isLoach = (u8)data.Read<PackedUInt1>().value();
                break;
            case PROP_ROTATION:
                fish->unk_160 = (s16)(data.Read<PackedInt2>().value());
                fish->unk_162 = (s16)(data.Read<PackedInt2>().value());
                fish->unk_164 = (s16)(data.Read<PackedInt2>().value());
                fish->rotationTarget.x = (s16)(data.Read<PackedInt2>().value());
                fish->rotationTarget.y = (s16)(data.Read<PackedInt2>().value());
                fish->rotationTarget.z = (s16)(data.Read<PackedInt2>().value());
                fish->rotationStep = data.Read<PackedFloat4>().value();
                break;
            case PROP_LIMBS:
                fish->fishLimb23RotYDelta = (s16)(data.Read<PackedInt2>().value());
                fish->fishLimbDRotZDelta = (s16)(data.Read<PackedInt2>().value());
                fish->fishLimbEFRotYDelta = (s16)(data.Read<PackedInt2>().value());
                fish->fishLimb89RotYDelta = (s16)(data.Read<PackedInt2>().value());
                fish->fishLimb4RotYDelta = (s16)(data.Read<PackedInt2>().value());
                fish->loachRotYDelta[0] = (s16)(data.Read<PackedInt2>().value());
                fish->loachRotYDelta[1] = (s16)(data.Read<PackedInt2>().value());
                fish->loachRotYDelta[2] = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SWIM:
                fish->fishLimbRotPhase = data.Read<PackedFloat4>().value();
                fish->fishLimbRotPhaseStep = data.Read<PackedFloat4>().value();
                fish->fishLimbRotPhaseMag = data.Read<PackedFloat4>().value();
                fish->speedTarget = data.Read<PackedFloat4>().value();
                break;
            case PROP_TARGET_POS:
                fish->fishTargetPos.x = data.Read<PackedFloat4>().value();
                fish->fishTargetPos.y = data.Read<PackedFloat4>().value();
                fish->fishTargetPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_MOUTH_POS:
                fish->fishMouthPos.x = data.Read<PackedFloat4>().value();
                fish->fishMouthPos.y = data.Read<PackedFloat4>().value();
                fish->fishMouthPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_TIMERS:
                fish->timerArray[0] = (s16)(data.Read<PackedInt2>().value());
                fish->timerArray[1] = (s16)(data.Read<PackedInt2>().value());
                fish->timerArray[2] = (s16)(data.Read<PackedInt2>().value());
                fish->timerArray[3] = (s16)(data.Read<PackedInt2>().value());
                fish->bumpTimer = (s16)(data.Read<PackedInt2>().value());
                fish->unk_1A2 = (s16)(data.Read<PackedInt2>().value());
                fish->unk_1A4 = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    int GetFishCount() const {
        int count = 0;
        for (auto actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor; actor = actor->next)
        {
            if (actor->update && actor->id == ACTOR_FISHING && actor->params >= 100)
                ++count;
        }
        return count;
    }

    void ShowFishWeight() {
        int isHeld = (int)IsHeld();
        if (m_wasHeld != isHeld)
        {
            m_wasHeld = isHeld;

            if (isHeld) {
                char text[100];
                snprintf(text, sizeof(text), "%.1f Lbs", WeightLbs());
                NameTag_RegisterForActorWithOptions(m_actor, text, {});
            } else {
                //Alpha to 1 first...weird glitch with removing nametags
                Color_RGBA8 colour = Color_RGBA8{ 255, 255, 255, 1 };
                NameTag_ChangeActorTextColour(m_actor, &colour);
                NameTag_RemoveAllForActor(m_actor);
            
            }
        }

    }
    void UpdateLeader(PlayState* play) override {
        //!Fish means the fish controller...the man
        if (!IsFish()) {
            //No need to check this every frame...
            if ((rand() % 20) == 0) {
                int fishCount = GetFishCount();

                if (fishCount < 5) {
                    auto newfishCount = 10 + rand() % 10;

                    for (int i = fishCount; i < newfishCount; i++) {
                        int fishInit = rand() % 17;
                        Actor_Spawn(&play->actorCtx, play, ACTOR_FISHING, sFishInits[fishInit].pos.x,
                                    sFishInits[fishInit].pos.y, sFishInits[fishInit].pos.z, 0,
                                    s16(Rand_ZeroFloat(0x10000)), 0, 100 + (rand() % 17));
                    }
                }
            }

        } else
            ShowFishWeight();
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        Fishing* fish = Typed();

        if (!IsFish()) {
            // Dont call our new UpdateLeader() otherwise puppet "fish man" would also be spawning fish
            AbstractActorController::UpdateLeader(play);
            return;
        }

        ShowFishWeight();
        Player* localPlayer = GET_PLAYER(play);
        if (localPlayer->heldItemAction != PLAYER_IA_FISHING_POLE) {
            return;
        }


        if (localPlayer->actor.world.pos.z > 1150.0f) {
            fish->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
        } else {
            fish->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
        }

        fish->actor.focus.pos = fish->actor.world.pos;


        if (fish->fishState < 0 || sFishingHookedFish == fish) {
            return;
            
        }

        f32 ourDist = Math_Vec3f_DistXYZ(&fish->actor.world.pos, &sLurePos);

        for (auto& entry : ZeldaOnlineClient::Instance->NetworkedActors()) {
            auto* other = dynamic_cast<PlayerPuppetController*>(entry.second);
            if (other == nullptr || !other->HasFishingLure()) {
                continue;
            }

            if (Math_Vec3f_DistXYZ(&fish->actor.world.pos, &other->FishingLurePos()) <= ourDist) {
                return;
            }
        }

        ClaimLeadership(CLAIM_REASON_NOW);
    }

  private:
    int m_wasHeld = -1;

};

} 

#endif