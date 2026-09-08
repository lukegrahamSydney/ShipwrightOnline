#ifndef HIDANSIMACONTROLLERH
#define HIDANSIMACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Sima/z_bg_hidan_sima.h"

void func_8088E518(BgHidanSima* sima, PlayState* play);
void func_8088E5D0(BgHidanSima* sima, PlayState* play);
void func_8088E6D0(BgHidanSima* sima, PlayState* play);
void func_8088E760(BgHidanSima* sima, PlayState* play);
void func_8088E7A8(BgHidanSima* sima, PlayState* play);

void func_8088E90C(BgHidanSima* sima);
}

namespace ZeldaOnline {

class HidanSimaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanSima* Typed() const {
        return reinterpret_cast<BgHidanSima*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using SimaActionFunc = void (*)(BgHidanSima*, PlayState*);
    static const SimaActionFunc* ActionTable(size_t* count) {
        static const SimaActionFunc sTable[] = {
            func_8088E518, func_8088E5D0, func_8088E6D0, func_8088E760, func_8088E7A8,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const SimaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHidanSima* sima = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);


        PackProperty(PROP_TIMER, PackedInt2(sima->timer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanSima* sima = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const SimaActionFunc* table = ActionTable(&count);
                if (id < count)
                    sima->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                sima->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanSima* sima = Typed();

        if (sima->actionFunc == func_8088E7A8) {
            func_8088E90C(sima);
            RegisterColliderBase(play, &sima->collider.base, COLL_AT);
        }
    }
};

}

#endif
