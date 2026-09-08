#ifndef ICEBLOCKCONTROLLERH
#define ICEBLOCKCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Ice_Objects/z_bg_ice_objects.h"

void BgIceObjects_Idle(BgIceObjects* blk, PlayState* play);
void BgIceObjects_Slide(BgIceObjects* blk, PlayState* play);
void BgIceObjects_Reset(BgIceObjects* blk, PlayState* play);
void BgIceObjects_Stuck(BgIceObjects* blk, PlayState* play);
}

namespace ZeldaOnline {

class IceBlockController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgIceObjects* Typed() const {
        return reinterpret_cast<BgIceObjects*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using IceObjectsActionFunc = decltype(&BgIceObjects_Idle);
    static const IceObjectsActionFunc* ActionTable(size_t* count) {
        static const IceObjectsActionFunc sTable[] = {
            BgIceObjects_Idle,
            BgIceObjects_Slide,
            BgIceObjects_Reset,
            BgIceObjects_Stuck,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const IceObjectsActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TARGET,
        PROP_SLIDE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgIceObjects* blk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_TARGET,
                     ByteStream() << PackedFloat4(blk->targetPos.x) << PackedFloat4(blk->targetPos.y)
                                  << PackedFloat4(blk->targetPos.z),
                     out);

        PackProperty(PROP_SLIDE,
                     ByteStream() << PackedInt2(blk->dyna.unk_158) << PackedInt2(blk->dyna.actor.params)
                                  << PackedFloat4(blk->dyna.actor.speedXZ),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgIceObjects* blk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const IceObjectsActionFunc* table = ActionTable(&count);
                if (id < count)
                    blk->actionFunc = table[id];
                break;
            }
            case PROP_TARGET:
                blk->targetPos.x = data.Read<PackedFloat4>().value();
                blk->targetPos.y = data.Read<PackedFloat4>().value();
                blk->targetPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_SLIDE:
                blk->dyna.unk_158 = (s16)(data.Read<PackedInt2>().value());
                blk->dyna.actor.params = (s16)(data.Read<PackedInt2>().value());
                blk->dyna.actor.speedXZ = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgIceObjects* blk = Typed();


        if (blk->dyna.unk_150 != 0.0f) {
            if (blk->actionFunc == BgIceObjects_Idle) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }
            GET_PLAYER(play)->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;
            blk->dyna.unk_150 = 0.0f;
        }

        if (blk->actionFunc == BgIceObjects_Slide && blk->dyna.actor.speedXZ > 6.0f &&
            blk->dyna.actor.world.pos.y >= 0.0f) {
            SpawnSlideSpray(play);
        }
    }

  private:
    void SpawnSlideSpray(PlayState* play) {
        static Color_RGBA8 sWhite = { 250, 250, 250, 255 };
        static Color_RGBA8 sGray = { 180, 180, 180, 255 };
        static Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };

        BgIceObjects* blk = Typed();
        Actor* thisx = &blk->dyna.actor;
        Vec3f pos;
        Vec3f velocity;
        f32 spread;

        spread = Rand_CenteredFloat(120.0f);
        velocity.x = -(1.5f + Rand_ZeroOne()) * Math_SinS(blk->dyna.unk_158);
        velocity.y = Rand_ZeroOne() + 1.0f;
        velocity.z = -(1.5f + Rand_ZeroOne()) * Math_CosS(blk->dyna.unk_158);
        pos.x = thisx->world.pos.x - (60.0f * Math_SinS(blk->dyna.unk_158)) - (Math_CosS(blk->dyna.unk_158) * spread);
        pos.z = thisx->world.pos.z - (60.0f * Math_CosS(blk->dyna.unk_158)) + (Math_SinS(blk->dyna.unk_158) * spread);
        pos.y = thisx->world.pos.y;
        func_8002829C(play, &pos, &velocity, &sZeroVec, &sWhite, &sGray, 250, Rand_S16Offset(40, 15));

        spread = Rand_CenteredFloat(120.0f);
        pos.x = thisx->world.pos.x - (60.0f * Math_SinS(blk->dyna.unk_158)) + (Math_CosS(blk->dyna.unk_158) * spread);
        pos.z = thisx->world.pos.z - (60.0f * Math_CosS(blk->dyna.unk_158)) - (Math_SinS(blk->dyna.unk_158) * spread);
        func_8002829C(play, &pos, &velocity, &sZeroVec, &sWhite, &sGray, 250, Rand_S16Offset(40, 15));
    }
};

}

#endif
