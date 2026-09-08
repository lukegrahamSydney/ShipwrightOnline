#ifndef HAKAPLATFORMCONTROLLERH
#define HAKAPLATFORMCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Haka_MeganeBG/z_bg_haka_meganebg.h"

void BgHakaMeganeBG_HiddenMovingPlatform_Idle(BgHakaMeganeBG* plat, PlayState* play);
void BgHakaMeganeBG_HiddenMovingPlatform_Move(BgHakaMeganeBG* plat, PlayState* play);
void BgHakaMeganeBG_ElevatorPlatform_Drop(BgHakaMeganeBG* plat, PlayState* play);
void BgHakaMeganeBG_ElevatorPlatform_Raise(BgHakaMeganeBG* plat, PlayState* play);
void BgHakaMeganeBG_RotatingPlatform_Spin(BgHakaMeganeBG* plat, PlayState* play);
void BgHakaMeganeBG_Gate_WaitForSwitchFlag(BgHakaMeganeBG* plat, PlayState* play);
void BgHakaMeganeBG_Gate_Open(BgHakaMeganeBG* plat, PlayState* play);
void BgHakaMeganeBG_DoNothing(BgHakaMeganeBG* plat, PlayState* play);
}

namespace ZeldaOnline {

class HakaPlatformController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHakaMeganeBG* Typed() const {
        return reinterpret_cast<BgHakaMeganeBG*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using MeganeBGActionFunc = decltype(&BgHakaMeganeBG_DoNothing);
    static const MeganeBGActionFunc* ActionTable(size_t* count) {
        static const MeganeBGActionFunc sTable[] = {
            BgHakaMeganeBG_HiddenMovingPlatform_Idle, BgHakaMeganeBG_HiddenMovingPlatform_Move,
            BgHakaMeganeBG_ElevatorPlatform_Drop,     BgHakaMeganeBG_ElevatorPlatform_Raise,
            BgHakaMeganeBG_RotatingPlatform_Spin,     BgHakaMeganeBG_Gate_WaitForSwitchFlag,
            BgHakaMeganeBG_Gate_Open,                 BgHakaMeganeBG_DoNothing,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const MeganeBGActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };


    void OnActorInit() override {
        Typed()->dyna.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    }

    void UpdateLeader(PlayState* play) override {
        Typed()->dyna.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
        AbstractActorController::UpdateLeader(play);
    }

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(Typed()->timer), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHakaMeganeBG* plat = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const MeganeBGActionFunc* table = ActionTable(&count);
                if (id < count)
                    plat->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                plat->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BgHakaMeganeBG* plat = Typed();

        if ((changed & (1ull << PROP_ACTION)) && plat->actionFunc == BgHakaMeganeBG_Gate_Open)
            OnePointCutscene_Attention(gPlayState, &plat->dyna.actor);
    }

    void UpdatePuppet(PlayState* play) override {
        Typed()->dyna.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    }
};

}

#endif
