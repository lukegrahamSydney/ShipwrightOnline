#ifndef BLKOBJCONTROLLERH
#define BLKOBJCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Blkobj/z_en_blkobj.h"

void EnBlkobj_Wait(EnBlkobj* obj, PlayState* play);
void EnBlkobj_SpawnDarkLink(EnBlkobj* obj, PlayState* play);
void EnBlkobj_DoNothing(EnBlkobj* obj, PlayState* play);
}

namespace ZeldaOnline {


class BlkobjController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBlkobj* Typed() const {
        return reinterpret_cast<EnBlkobj*>(m_actor);
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId == ACTOR_EN_TORCH2;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using BlkActionFunc = decltype(&EnBlkobj_Wait);
    static const BlkActionFunc* ActionTable(size_t* count) {
        static const BlkActionFunc sTable[] = {
            EnBlkobj_Wait,
            EnBlkobj_SpawnDarkLink,
            EnBlkobj_DarkLinkFight,
            EnBlkobj_DoNothing,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BlkActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBlkobj* obj = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, ByteStream() << PackedInt2(obj->alpha) << PackedInt2(obj->timer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBlkobj* obj = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BlkActionFunc* table = ActionTable(&count);
                if (id < count)
                    obj->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                obj->alpha = (s16)(data.Read<PackedInt2>().value());
                obj->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnBlkobj* obj = Typed();


        if (obj->actionFunc == EnBlkobj_Wait || obj->actionFunc == EnBlkobj_SpawnDarkLink) {
            GET_PLAYER(play)->stateFlags2 |= PLAYER_STATE2_REFLECTION;
        }


        if (obj->actionFunc == EnBlkobj_Wait && obj->dyna.actor.xzDistToPlayer < 120.0f) {
            ClaimLeadership(CLAIM_REASON_NOW);
        }
    }
};

}

#endif
