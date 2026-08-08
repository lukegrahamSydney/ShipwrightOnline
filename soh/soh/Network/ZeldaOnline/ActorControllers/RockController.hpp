#ifndef ROCKCONTROLLERH
#define ROCKCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ishi/z_en_ishi.h"

void EnIshi_Wait(EnIshi* ishi, PlayState* play);
void EnIshi_LiftedUp(EnIshi* ishi, PlayState* play);
void EnIshi_Fly(EnIshi* ishi, PlayState* play);

void EnIshi_DropCollectible(EnIshi* ishi, PlayState* play);
void EnIshi_SpawnFragmentsSmall(EnIshi* ishi, PlayState* play);
void EnIshi_SpawnFragmentsLarge(EnIshi* ishi, PlayState* play);
void EnIshi_SpawnDustSmall(EnIshi* ishi, PlayState* play);
void EnIshi_SpawnDustLarge(EnIshi* ishi, PlayState* play);
}

namespace ZeldaOnline {

class RockController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnIshi* Typed() const {
        return reinterpret_cast<EnIshi*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_WAIT = 0;

    static constexpr u32 BREAK_DMG_FLAGS = 0x40000048;

    bool IsLargeRock() const {
        return (Typed()->actor.params & 1) == ROCK_LARGE;
    }

    using IshiActionFunc = void (*)(EnIshi*, PlayState*);
    static const IshiActionFunc* ActionTable(size_t* count) {
        static const IshiActionFunc sTable[] = {
            EnIshi_Wait,
            EnIshi_LiftedUp,
            EnIshi_Fly,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const IshiActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool HitWouldReact() const {
        EnIshi* ishi = Typed();
        if (!(ishi->collider.base.acFlags & AC_HIT) || IsLargeRock())
            return false;
        ColliderInfo* hit = ishi->collider.info.acHitInfo;
        return hit != nullptr && (hit->toucher.dmgFlags & BREAK_DMG_FLAGS);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_MASS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnIshi* ishi = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_MASS, PackedUInt1(ishi->actor.colChkInfo.mass), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnIshi* ishi = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const IshiActionFunc* table = ActionTable(&count);
                if (id < count)
                    ishi->actionFunc = table[id];
                break;
            }
            case PROP_MASS:
                ishi->actor.colChkInfo.mass = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        EnIshi* ishi = Typed();
        if (gPlayState == nullptr || ishi == nullptr)
            return;

        EnIshi_DropCollectible(ishi, gPlayState);
        if (IsLargeRock())
            EnIshi_SpawnFragmentsLarge(ishi, gPlayState);
        else
            EnIshi_SpawnFragmentsSmall(ishi, gPlayState);

        if (!(ishi->actor.bgCheckFlags & 0x20)) {
            if (IsLargeRock())
                EnIshi_SpawnDustLarge(ishi, gPlayState);
            else
                EnIshi_SpawnDustSmall(ishi, gPlayState);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnIshi* ishi = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }

        if (ishi->actor.parent == &GET_PLAYER(play)->actor && !IsRunningLocally()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }

        ishi->collider.base.acFlags &= ~AC_HIT;

        if (ishi->actor.parent != nullptr)
            ishi->actor.room = -1;

        if (m_currentActionIndex == ID_WAIT && ishi->actor.parent == nullptr && ishi->actor.xzDistToPlayer < 600.0f) {

            Collider_UpdateCylinder(&ishi->actor, &ishi->collider);

            u8 roles = COLL_AC;
            if (ishi->actor.xzDistToPlayer < 400.0f)
                roles |= COLL_OC;
            RegisterColliderBase(play, &ishi->collider.base, roles);

            if (ishi->actor.xzDistToPlayer < 90.0f) {
                if (IsLargeRock())
                    Actor_OfferGetItem(&ishi->actor, play, GI_NONE, 80.0f, 20.0f);
                else
                    Actor_OfferGetItem(&ishi->actor, play, GI_NONE, 50.0f, 10.0f);
            }
        }
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

}

#endif
