#ifndef MIZUWATERCONTROLLERH
#define MIZUWATERCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Mizu_Water/z_bg_mizu_water.h"

void BgMizuWater_WaitForAction(BgMizuWater* water, PlayState* play);
void BgMizuWater_ChangeWaterLevel(BgMizuWater* water, PlayState* play);

void BgMizuWater_SetWaterBoxesHeight(WaterBox* waterBoxes, f32 height);
}

namespace ZeldaOnline {


class MizuWaterController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgMizuWater* Typed() const {
        return reinterpret_cast<BgMizuWater*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using WaterActionFunc = decltype(&BgMizuWater_WaitForAction);
    static const WaterActionFunc* ActionTable(size_t* count) {
        static const WaterActionFunc sTable[] = {
            BgMizuWater_WaitForAction,
            BgMizuWater_ChangeWaterLevel,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const WaterActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_LEVEL,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgMizuWater* water = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_LEVEL,
                     ByteStream() << PackedInt2(water->actor.params) << PackedFloat4(water->targetY)
                                  << PackedFloat4(water->actor.world.pos.y),
                     out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgMizuWater* water = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const WaterActionFunc* table = ActionTable(&count);
                if (id < count)
                    water->actionFunc = table[id];
                break;
            }
            case PROP_LEVEL: {
                water->actor.params = (s16)(data.Read<PackedInt2>().value());
                water->targetY = data.Read<PackedFloat4>().value();

                f32 y = data.Read<PackedFloat4>().value();
                if (water->actor.world.pos.y != y) {
                    water->actor.world.pos.y = y;


                    if (water->type == 0 && gPlayState != nullptr) {
                        BgMizuWater_SetWaterBoxesHeight(gPlayState->colCtx.colHeader->waterBoxes, y);
                    }
                }
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgMizuWater* water = Typed();

        WaterBox* waterBoxes = play->colCtx.colHeader->waterBoxes;

        switch (water->type) {
            case 2:
                waterBoxes[6].ySurface = (s16)water->actor.world.pos.y;
                break;
            case 3:
                waterBoxes[8].ySurface = (s16)water->actor.world.pos.y;
                break;
            case 4:
                waterBoxes[16].ySurface = (s16)water->actor.world.pos.y;
                break;
            default:
                break;
        }
    }
};

}

#endif
