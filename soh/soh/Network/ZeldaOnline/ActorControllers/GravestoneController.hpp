#ifndef GRAVESTONECONTROLLERH
#define GRAVESTONECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Haka/z_bg_haka.h"

void BgHaka_IdleClosed(BgHaka* haka, PlayState* play);
void BgHaka_Pull(BgHaka* haka, PlayState* play);
void BgHaka_IdleOpened(BgHaka* haka, PlayState* play);
void BgHaka_IdleLockedClosed(BgHaka* haka, PlayState* play);
void BgHaka_CheckPlayerOnDirtPatch(BgHaka* haka, Player* player);
}

namespace ZeldaOnline {

class GravestoneController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHaka* Typed() const {
        return reinterpret_cast<BgHaka*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using HakaActionFunc = void (*)(BgHaka*, PlayState*);
    static const HakaActionFunc* ActionTable(size_t* count) {
        static const HakaActionFunc sTable[] = {
            BgHaka_IdleClosed,
            BgHaka_Pull,
            BgHaka_IdleOpened,
            BgHaka_IdleLockedClosed,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HakaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_PULL,
        PROP_STATE,
        PROP_PARAMS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHaka* haka = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_PULL,
                     ByteStream() << PackedFloat4(haka->dyna.actor.minVelocityY)
                                  << PackedFloat4(haka->dyna.actor.speedXZ) << PackedInt2(haka->dyna.actor.world.rot.y),
                     out);
        PackProperty(PROP_STATE, PackedUInt1(haka->state), out);
        PackProperty(PROP_PARAMS, PackedInt2(haka->dyna.actor.params), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHaka* haka = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const HakaActionFunc* table = ActionTable(&count);
                if (id < count)
                    haka->actionFunc = table[id];
                break;
            }
            case PROP_PULL:
                haka->dyna.actor.minVelocityY = data.Read<PackedFloat4>().value();
                haka->dyna.actor.speedXZ = data.Read<PackedFloat4>().value();
                haka->dyna.actor.world.rot.y = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_STATE:
                haka->state = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PARAMS:
                haka->dyna.actor.params = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHaka* haka = Typed();

        if (haka->dyna.unk_150 != 0.0f) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
            haka->dyna.unk_150 = 0.0f;
        }

        BgHaka_CheckPlayerOnDirtPatch(haka, GET_PLAYER(play));
    }
};

} // namespace ZeldaOnline

#endif