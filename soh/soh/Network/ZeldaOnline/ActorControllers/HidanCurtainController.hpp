#ifndef HIDANCURTAINCONTROLLERH
#define HIDANCURTAINCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Curtain/z_bg_hidan_curtain.h"

void BgHidanCurtain_WaitForSwitchOn(BgHidanCurtain* curtain, PlayState* play);
void BgHidanCurtain_WaitForCutscene(BgHidanCurtain* curtain, PlayState* play);
void BgHidanCurtain_WaitForClear(BgHidanCurtain* curtain, PlayState* play);
void BgHidanCurtain_TurnOn(BgHidanCurtain* curtain, PlayState* play);
void BgHidanCurtain_TurnOff(BgHidanCurtain* curtain, PlayState* play);
void BgHidanCurtain_WaitForTimer(BgHidanCurtain* curtain, PlayState* play);
}

namespace ZeldaOnline {

class HidanCurtainController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanCurtain* Typed() const {
        return reinterpret_cast<BgHidanCurtain*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using CurtainActionFunc = void (*)(BgHidanCurtain*, PlayState*);
    static const CurtainActionFunc* ActionTable(size_t* count) {
        static const CurtainActionFunc sTable[] = {
            BgHidanCurtain_WaitForSwitchOn, BgHidanCurtain_WaitForCutscene, BgHidanCurtain_WaitForClear,
            BgHidanCurtain_TurnOn,          BgHidanCurtain_TurnOff,         BgHidanCurtain_WaitForTimer,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const CurtainActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static s16 HeightForSize(u8 size, f32* riseDist) {
        static const s16 sHeights[] = { 144, 88 };
        static const f32 sRiseDists[] = { 144.0f, 88.0f };

        u8 i = (size < 2) ? size : 0;
        *riseDist = sRiseDists[i];
        return sHeights[i];
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHidanCurtain* curtain = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(curtain->timer) << PackedUInt1(curtain->alpha),
                     out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanCurtain* curtain = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const CurtainActionFunc* table = ActionTable(&count);
                if (id < count)
                    curtain->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                curtain->timer = (s16)(data.Read<PackedInt2>().value());
                curtain->alpha = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanCurtain* curtain = Typed();

        curtain->texScroll++;


        if (curtain->alpha > 50) {
            f32 riseDist;
            s16 height = HeightForSize(curtain->size, &riseDist);
            f32 riseProgress =
                (riseDist - (curtain->actor.home.pos.y - curtain->actor.world.pos.y)) / riseDist;

            curtain->collider.dim.height = (s16)(height * riseProgress);
            Collider_UpdateCylinder(&curtain->actor, &curtain->collider);
            RegisterColliderBase(play, &curtain->collider.base, COLL_AT | COLL_OC);

            if (gSaveContext.sceneSetupIndex <= 3)
                func_8002F974(&curtain->actor, NA_SE_EV_FIRE_PILLAR_S - SFX_FLAG);
        }

        if (curtain->collider.base.atFlags & AT_HIT) {
            curtain->collider.base.atFlags &= ~AT_HIT;
            func_8002F71C(play, &curtain->actor, 5.0f, curtain->actor.yawTowardsPlayer, 1.0f);
        }
    }
};

}

#endif
