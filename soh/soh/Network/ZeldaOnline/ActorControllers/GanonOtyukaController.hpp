#ifndef GANONOTYUKACONTROLLERH
#define GANONOTYUKACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Ganon_Otyuka/z_bg_ganon_otyuka.h"

void BgGanonOtyuka_WaitToFall(BgGanonOtyuka*, PlayState* play);
void BgGanonOtyuka_Fall(BgGanonOtyuka*, PlayState* play);
}

namespace ZeldaOnline {

class GanonOtyukaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgGanonOtyuka* Typed() const {
        return reinterpret_cast<BgGanonOtyuka*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return params != 0x23;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using OtyukaActionFunc = void (*)(BgGanonOtyuka*, PlayState*);
    static const OtyukaActionFunc* ActionTable(size_t* count) {
        static const OtyukaActionFunc sTable[] = {
            BgGanonOtyuka_WaitToFall,
            BgGanonOtyuka_Fall,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const OtyukaActionFunc* table = ActionTable(&count);
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
        BgGanonOtyuka* otk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(otk->dropTimer) << PackedUInt1(otk->isFalling)
                                  << PackedUInt1(otk->unwalledSides) << PackedUInt1(otk->visibleSides),
                     out);

        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgGanonOtyuka* otk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const OtyukaActionFunc* table = ActionTable(&count);
                if (id < count)
                    otk->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                otk->dropTimer = (s16)(data.Read<PackedInt2>().value());
                otk->isFalling = (u8)(data.Read<PackedUInt1>().value());
                otk->unwalledSides = (u8)(data.Read<PackedUInt1>().value());
                otk->visibleSides = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgGanonOtyuka* otk = Typed();

        otk->flashTimer++;

        if (otk->flashState == 1) {
            Math_ApproachF(&otk->flashPrimColorB, 170.0f, 1.0f, 8.5f);
            Math_ApproachF(&otk->flashEnvColorR, 120.0f, 1.0f, 13.5f);
            Math_ApproachF(&otk->flashYScale, 2.5f, 1.0f, 0.25f);
            if (otk->flashYScale == 2.5f)
                otk->flashState = 2;
        } else if (otk->flashState == 2) {
            Math_ApproachF(&otk->flashPrimColorG, 0.0f, 1.0f, 25.5f);
            Math_ApproachF(&otk->flashEnvColorR, 0.0f, 1.0f, 12.0f);
            Math_ApproachF(&otk->flashEnvColorG, 0.0f, 1.0f, 25.5f);
            Math_ApproachZeroF(&otk->flashYScale, 1.0f, 0.25f);
            if (otk->flashYScale == 0.0f)
                otk->flashState = 0;
        }
    }

    void OnPropertiesApplied(u64 changed) override {
        BgGanonOtyuka* otk = Typed();

        if (!(changed & (1ull << PROP_STATE)) || !otk->isFalling || m_startedFalling)
            return;

        m_startedFalling = true;
        otk->flashState = 1;
        otk->flashTimer = 0;
        otk->flashPrimColorR = 255.0f;
        otk->flashPrimColorG = 255.0f;
        otk->flashPrimColorB = 255.0f;
        otk->flashEnvColorR = 255.0f;
        otk->flashEnvColorG = 255.0f;
        otk->flashEnvColorB = 0.0f;
    }

  private:
    bool m_startedFalling = false;
};

} // namespace ZeldaOnline

#endif