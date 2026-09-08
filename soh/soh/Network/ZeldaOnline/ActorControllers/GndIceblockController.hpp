#ifndef GNDICEBLOCKCONTROLLERH
#define GNDICEBLOCKCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Gnd_Iceblock/z_bg_gnd_iceblock.h"

void BgGndIceblock_Idle(BgGndIceblock*, PlayState* play);
void BgGndIceblock_Slide(BgGndIceblock*, PlayState* play);
void BgGndIceblock_Fall(BgGndIceblock*, PlayState* play);
void BgGndIceblock_Hole(BgGndIceblock*, PlayState* play);
void BgGndIceblock_Reset(BgGndIceblock*, PlayState* play);

extern u8* gGndIceblockPositions;
}

namespace ZeldaOnline {

class GndIceblockController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgGndIceblock* Typed() const {
        return reinterpret_cast<BgGndIceblock*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_IDLE = 0;

    using IceblockActionFunc = void (*)(BgGndIceblock*, PlayState*);
    static const IceblockActionFunc* ActionTable(size_t* count) {
        static const IceblockActionFunc sTable[] = {
            BgGndIceblock_Idle, BgGndIceblock_Slide, BgGndIceblock_Fall,
            BgGndIceblock_Hole, BgGndIceblock_Reset,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const IceblockActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TARGET,
        PROP_SLOTS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgGndIceblock* ice = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TARGET,
                     ByteStream() << PackedFloat4(ice->targetPos.x) << PackedFloat4(ice->targetPos.y)
                                  << PackedFloat4(ice->targetPos.z) << PackedInt2(ice->dyna.unk_158),
                     out);
        PackProperty(PROP_SLOTS,
                     ByteStream() << PackedUInt1(gGndIceblockPositions[0]) << PackedUInt1(gGndIceblockPositions[1]),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgGndIceblock* ice = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const IceblockActionFunc* table = ActionTable(&count);
                if (id < count)
                    ice->actionFunc = table[id];
                break;
            }
            case PROP_TARGET:
                ice->targetPos.x = data.Read<PackedFloat4>().value();
                ice->targetPos.y = data.Read<PackedFloat4>().value();
                ice->targetPos.z = data.Read<PackedFloat4>().value();
                ice->dyna.unk_158 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SLOTS:
                gGndIceblockPositions[0] = (u8)(data.Read<PackedUInt1>().value());
                gGndIceblockPositions[1] = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgGndIceblock* ice = Typed();
        Player* player = GET_PLAYER(play);

        if (ice->dyna.unk_150 != 0.0f) {
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;

            if (ice->actionFunc == BgGndIceblock_Idle && ice->dyna.unk_150 > 0.0f) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }

            ice->dyna.unk_150 = 0.0f;
        }
    }
};

} // namespace ZeldaOnline

#endif
