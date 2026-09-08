#ifndef JYAZURERUKABECONTROLLERH
#define JYAZURERUKABECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Jya_Zurerukabe/z_bg_jya_zurerukabe.h"

void func_8089B4C8(BgJyaZurerukabe*, PlayState* play);
void func_8089B7C4(BgJyaZurerukabe*, PlayState* play);
void func_8089B870(BgJyaZurerukabe*, PlayState* play);

extern f32* gBgJyaZurerukabeSpeeds;
extern f32* gBgJyaZurerukabeStepRates;
}

namespace ZeldaOnline {

class JyaZurerukabeController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgJyaZurerukabe* Typed() const {
        return reinterpret_cast<BgJyaZurerukabe*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using ZurerukabeActionFunc = void (*)(BgJyaZurerukabe*, PlayState*);
    static const ZurerukabeActionFunc* ActionTable(size_t* count) {
        static const ZurerukabeActionFunc sTable[] = {
            func_8089B7C4,
            func_8089B870,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ZurerukabeActionFunc* table = ActionTable(&count);
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
        BgJyaZurerukabe* wall = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(wall->unk_16A) << PackedInt2(wall->unk_16C)
                                  << PackedInt2(wall->unk_16E),
                     out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgJyaZurerukabe* wall = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const ZurerukabeActionFunc* table = ActionTable(&count);
                if (id < count)
                    wall->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                wall->unk_16A = (s16)(data.Read<PackedInt2>().value());
                wall->unk_16C = (s16)(data.Read<PackedInt2>().value());
                wall->unk_16E = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgJyaZurerukabe* wall = Typed();
        s32 slot = wall->unk_168;

        if (slot < 0 || slot > 3)
            return;


        if (wall->actionFunc == func_8089B870) {
            gBgJyaZurerukabeSpeeds[slot] = gBgJyaZurerukabeStepRates[slot] * wall->unk_16E;
            func_8002F974(&wall->dyna.actor, NA_SE_EV_ELEVATOR_MOVE - SFX_FLAG);
        } else {
            gBgJyaZurerukabeSpeeds[slot] = 0.0f;
        }


        if (slot == 0)
            func_8089B4C8(wall, play);
    }
};

} // namespace ZeldaOnline

#endif
