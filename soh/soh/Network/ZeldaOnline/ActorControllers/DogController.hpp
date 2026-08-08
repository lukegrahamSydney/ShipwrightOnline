#ifndef DOGCONTROLLERH
#define DOGCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Dog/z_en_dog.h"

void EnDog_Wait(EnDog* dog, PlayState* play);
void EnDog_ChooseMovement(EnDog* dog, PlayState* play);
void EnDog_FollowPath(EnDog* dog, PlayState* play);
void EnDog_FollowPlayer(EnDog* dog, PlayState* play);
void EnDog_FaceLink(EnDog* dog, PlayState* play);
void EnDog_RunAway(EnDog* dog, PlayState* play);

s32 EnDog_PlayAnimAndSFX(EnDog* dog);
}

namespace ZeldaOnline {

class DogController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDog* Typed() const {
        return reinterpret_cast<EnDog*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    static constexpr s16 BEHAVIOR_WALK = 0;
    static constexpr s16 BEHAVIOR_RUN = 1;
    static constexpr s16 BEHAVIOR_BARK = 2;
    static constexpr s16 BEHAVIOR_SIT = 3;
    static constexpr s16 BEHAVIOR_SIT_2 = 4;
    static constexpr s16 BEHAVIOR_BOW = 5;
    static constexpr s16 BEHAVIOR_BOW_2 = 6;
    static constexpr s16 BEHAVIOR_FORCE_REISSUE = -1;

    using DogActionFunc = void (*)(EnDog*, PlayState*);
    static const DogActionFunc* ActionTable(size_t* count) {
        static const DogActionFunc sTable[] = {
            EnDog_Wait,
            EnDog_ChooseMovement,
            EnDog_FollowPath,
            EnDog_FollowPlayer,
            EnDog_FaceLink,
            EnDog_RunAway,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DogActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        return COLL_OC;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_BEHAVIOR,
        PROP_NEXT_BEHAVIOR,
        PROP_BEHAVIOR_TIMER,
        PROP_WAYPOINT,
        PROP_REVERSE,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnDog* dog = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_BEHAVIOR, PackedInt2(dog->behavior), out);
        PackProperty(PROP_NEXT_BEHAVIOR, PackedInt2(dog->nextBehavior), out);
        PackProperty(PROP_BEHAVIOR_TIMER, PackedInt2(dog->behaviorTimer), out);
        PackProperty(PROP_WAYPOINT, PackedInt2(dog->waypoint), out);
        PackProperty(PROP_REVERSE, PackedUInt1(dog->reverse), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDog* dog = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const DogActionFunc* table = ActionTable(&count);
                if (id < count)
                    dog->actionFunc = table[id];
                break;
            }
            case PROP_BEHAVIOR:
                dog->behavior = (s16)(data.Read<PackedInt2>().value());
                if (!m_animIssued) {
                    dog->behavior = BEHAVIOR_FORCE_REISSUE;
                    m_animIssued = true;
                }
                break;
            case PROP_NEXT_BEHAVIOR:
                dog->nextBehavior = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BEHAVIOR_TIMER:
                dog->behaviorTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_WAYPOINT:
                dog->waypoint = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_REVERSE:
                dog->reverse = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void UpdatePuppet(PlayState* play) override {
        EnDog* dog = Typed();

        EnDog_PlayAnimAndSFX(dog);
        UpdateAnimation(&dog->skelAnime, false);

        if (dog->actor.xzDistToPlayer < 150.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        {
            Vec3f rayOrigin = dog->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            dog->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &dog->actor.floorPoly, &floorBgId,
                                                                 &dog->actor, &rayOrigin);
            dog->actor.floorBgId = floorBgId;
        }

        RegisterCylinder(play, &dog->collider, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_OC;
    bool m_animIssued = false;
};

}

#endif
