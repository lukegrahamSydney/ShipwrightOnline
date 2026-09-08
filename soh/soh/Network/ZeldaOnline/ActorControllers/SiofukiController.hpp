#ifndef SIOFUKICONTROLLERH
#define SIOFUKICONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Siofuki/z_en_siofuki.h"

void func_80AFC218(EnSiofuki* spout, PlayState* play);
void func_80AFC34C(EnSiofuki* spout, PlayState* play);
void func_80AFC3C8(EnSiofuki* spout, PlayState* play);
void func_80AFC478(EnSiofuki* spout, PlayState* play);
void func_80AFC544(EnSiofuki* spout, PlayState* play);

void func_80AFBE8C(EnSiofuki* spout, PlayState* play);
}

namespace ZeldaOnline {

class SiofukiController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnSiofuki* Typed() const {
        return reinterpret_cast<EnSiofuki*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using SiofukiActionFunc = decltype(&func_80AFC218);
    static const SiofukiActionFunc* ActionTable(size_t* count) {
        static const SiofukiActionFunc sTable[] = {
            func_80AFC218, func_80AFC34C, func_80AFC3C8, func_80AFC478, func_80AFC544,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const SiofukiActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsActive() const {
        return Typed()->actionFunc == func_80AFC218;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEIGHT,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnSiofuki* spout = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_HEIGHT,
                     ByteStream() << PackedFloat4(spout->currentHeight) << PackedFloat4(spout->targetHeight)
                                  << PackedFloat4(spout->oscillation) << PackedFloat4(spout->unk_170)
                                  << PackedFloat4(spout->unk_174),
                     out);


        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt4(spout->timer) << PackedInt2(spout->activeTime)
                                  << PackedUInt1(spout->sfxFlags),
                     out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnSiofuki* spout = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const SiofukiActionFunc* table = ActionTable(&count);
                if (id < count)
                    spout->actionFunc = table[id];
                break;
            }
            case PROP_HEIGHT:
                spout->currentHeight = data.Read<PackedFloat4>().value();
                spout->targetHeight = data.Read<PackedFloat4>().value();
                spout->oscillation = data.Read<PackedFloat4>().value();
                spout->unk_170 = data.Read<PackedFloat4>().value();
                spout->unk_174 = data.Read<PackedFloat4>().value();
                break;
            case PROP_STATE:
                spout->timer = data.Read<PackedInt4>().value();
                spout->activeTime = (s16)(data.Read<PackedInt2>().value());
                spout->sfxFlags = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnSiofuki* spout = Typed();

        if (IsActive()) {

            func_80AFBE8C(spout, play);

            if (spout->timer >= 0) {
                func_8002F994(&spout->dyna.actor, spout->timer);
            }
        }
    }
};

}

#endif
