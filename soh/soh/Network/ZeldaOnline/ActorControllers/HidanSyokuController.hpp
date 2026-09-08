#ifndef HIDANSYOKUCONTROLLERH
#define HIDANSYOKUCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Syoku/z_bg_hidan_syoku.h"

void func_8088F4B8(BgHidanSyoku* platform, PlayState* play);
void func_8088F514(BgHidanSyoku* platform, PlayState* play);
void func_8088F5A0(BgHidanSyoku* platform, PlayState* play);
void func_8088F62C(BgHidanSyoku* platform, PlayState* play);
}

namespace ZeldaOnline {

class HidanSyokuController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanSyoku* Typed() const {
        return reinterpret_cast<BgHidanSyoku*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using SyokuActionFunc = void (*)(BgHidanSyoku*, PlayState*);
    static const SyokuActionFunc* ActionTable(size_t* count) {
        static const SyokuActionFunc sTable[] = {
            func_8088F4B8,
            func_8088F514,
            func_8088F5A0,
            func_8088F62C,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const SyokuActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsMoving() const {
        BgHidanSyoku* platform = Typed();
        return platform->actionFunc == func_8088F514 || platform->actionFunc == func_8088F5A0;
    }


    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(Typed()->timer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanSyoku* platform = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const SyokuActionFunc* table = ActionTable(&count);
                if (id < count)
                    platform->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                platform->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanSyoku* platform = Typed();


        if (DynaPolyActor_IsPlayerOnTop(&platform->dyna)) {
            if (platform->unk_168 == 0)
                platform->unk_168 = 3;
            Camera_ChangeSetting(play->cameraPtrs[MAIN_CAM], CAM_SET_FIRE_PLATFORM);
        } else {
            if (platform->unk_168 != 0)
                Camera_ChangeSetting(play->cameraPtrs[MAIN_CAM], CAM_SET_DUNGEON0);
            platform->unk_168 = 0;
        }


        if (!IsMoving() && DynaPolyActor_IsPlayerOnTop(&platform->dyna))
            ClaimLeadership(CLAIM_REASON_NOW);
    }
};

}

#endif
