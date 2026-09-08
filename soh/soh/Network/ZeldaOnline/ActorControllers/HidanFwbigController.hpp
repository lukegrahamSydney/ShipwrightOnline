#ifndef HIDANFWBIGCONTROLLERH
#define HIDANFWBIGCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Fwbig/z_bg_hidan_fwbig.h"

void BgHidanFwbig_WaitForSwitch(BgHidanFwbig* wall, PlayState* play);
void BgHidanFwbig_WaitForCs(BgHidanFwbig* wall, PlayState* play);
void BgHidanFwbig_Rise(BgHidanFwbig* wall, PlayState* play);
void BgHidanFwbig_Lower(BgHidanFwbig* wall, PlayState* play);
void BgHidanFwbig_WaitForTimer(BgHidanFwbig* wall, PlayState* play);
void BgHidanFwbig_WaitForPlayer(BgHidanFwbig* wall, PlayState* play);
void BgHidanFwbig_Move(BgHidanFwbig* wall, PlayState* play);

void BgHidanFwbig_MoveCollider(BgHidanFwbig* wall, PlayState* play);
}

namespace ZeldaOnline {

class HidanFwbigController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanFwbig* Typed() const {
        return reinterpret_cast<BgHidanFwbig*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using FwbigActionFunc = void (*)(BgHidanFwbig*, PlayState*);
    static const FwbigActionFunc* ActionTable(size_t* count) {
        static const FwbigActionFunc sTable[] = {
            BgHidanFwbig_WaitForSwitch, BgHidanFwbig_WaitForCs,     BgHidanFwbig_Rise,
            BgHidanFwbig_Lower,         BgHidanFwbig_WaitForTimer,  BgHidanFwbig_WaitForPlayer,
            BgHidanFwbig_Move,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const FwbigActionFunc* table = ActionTable(&count);
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
        BgHidanFwbig* wall = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt1(wall->direction) << PackedUInt1(wall->moveState)
                                  << PackedInt2(wall->timer),
                     out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanFwbig* wall = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const FwbigActionFunc* table = ActionTable(&count);
                if (id < count)
                    wall->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                wall->direction = (s8)(data.Read<PackedInt1>().value());
                wall->moveState = (u8)(data.Read<PackedUInt1>().value());
                wall->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanFwbig* wall = Typed();

        if (wall->collider.base.atFlags & AT_HIT) {
            wall->collider.base.atFlags &= ~AT_HIT;
            func_8002F71C(play, &wall->actor, 5.0f, wall->actor.world.rot.y, 1.0f);
        }

        if ((wall->actor.home.pos.y - 200.0f) < wall->actor.world.pos.y) {
            if (gSaveContext.sceneSetupIndex < 4)
                func_8002F974(&wall->actor, NA_SE_EV_BURNING - SFX_FLAG);
            else if ((s16)wall->actor.world.pos.x == -513)
                func_8002F974(&wall->actor, NA_SE_EV_FLAME_OF_FIRE - SFX_FLAG);

            BgHidanFwbig_MoveCollider(wall, play);
            RegisterColliderBase(play, &wall->collider.base, COLL_AT | COLL_OC);
        }

        if (wall->actionFunc == BgHidanFwbig_WaitForTimer) {
            func_8002F994(&wall->actor, wall->timer);
        }
    }
};

}

#endif
