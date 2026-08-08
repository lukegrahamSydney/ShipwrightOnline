#ifndef MARKETNPCCONTROLLERH
#define MARKETNPCCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Hy/z_en_hy.h"

void EnHy_Walk(EnHy* npc, PlayState* play);
void EnHy_SetupPace(EnHy* npc, PlayState* play);
void EnHy_Pace(EnHy* npc, PlayState* play);
void EnHy_WatchDog(EnHy* npc, PlayState* play);
void EnHy_DoNothing(EnHy* npc, PlayState* play);
void EnHy_WaitDogFoundRewardGiven(EnHy* npc, PlayState* play);
void EnHy_FinishGivingDogFoundReward(EnHy* npc, PlayState* play);

void EnHy_InitImpl(EnHy* npc, PlayState* play);

void EnHy_UpdateEyes(EnHy* npc);
void EnHy_UpdateNPC(EnHy* npc, PlayState* play);
void EnHy_UpdateCollider(EnHy* npc, PlayState* play);
}

namespace ZeldaOnline {

class MarketNpcController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnHy* Typed() const {
        return reinterpret_cast<EnHy*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using HyActionFunc = void (*)(EnHy*, PlayState*);
    static const HyActionFunc* ActionTable(size_t* count) {
        static const HyActionFunc sTable[] = {
            EnHy_Walk,
            EnHy_SetupPace,
            EnHy_Pace,
            EnHy_WatchDog,
            EnHy_Fidget,
            EnHy_DoNothing,
            EnHy_WaitDogFoundRewardGiven,
            EnHy_FinishGivingDogFoundReward,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HyActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WAYPOINT,
        PROP_PATH_REVERSE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnHy* npc = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_WAYPOINT, PackedUInt1((u8)(npc->waypoint)), out);
        PackProperty(PROP_PATH_REVERSE, PackedUInt1(npc->pathReverse ? 1u : 0u), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnHy* npc = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                if (npc->actionFunc == EnHy_InitImpl)
                    break;
                size_t count;
                const HyActionFunc* table = ActionTable(&count);
                if (id < count)
                    npc->actionFunc = table[id];
                break;
            }
            case PROP_WAYPOINT:
                npc->waypoint = (s8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PATH_REVERSE:
                npc->pathReverse = data.Read<PackedUInt1>().value() != 0;
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnHy* npc = Typed();

        if (npc->actionFunc == EnHy_InitImpl) {
            m_originalUpdate(m_actor, play);
            return;
        }

        if (npc->actor.xzDistToPlayer < 150.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        gSegments[6] = VIRTUAL_TO_PHYSICAL(play->objectCtx.status[npc->objBankIndexOsAnime].segment);
        SkelAnime_Update(&npc->skelAnime);
        EnHy_UpdateEyes(npc);

        EnHy_UpdateNPC(npc, play);
        EnHy_UpdateCollider(npc, play);
    }
};

}

#endif
