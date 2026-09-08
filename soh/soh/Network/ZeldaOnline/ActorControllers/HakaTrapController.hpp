#ifndef HAKATRAPCONTROLLERH
#define HAKATRAPCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Haka_Trap/z_bg_haka_trap.h"

void BgHakaTrap_SpikedWall_CloseIn(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_SpikedWall_Burn(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_Guillotine_Fall(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_Guillotine_Lift(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_SpikedCrusher_Fall(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_SpikedCrusher_Lift(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_FanBlade_Idle(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_FireBarrier_Idle(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_FireBarrier_UpdateLayout(BgHakaTrap* trap, PlayState* play);

void BgHakaTrap_UpdateBodyColliderPos(BgHakaTrap* trap, PlayState* play);
void BgHakaTrap_FanBlade_UpdateFanRotation(BgHakaTrap* trap, PlayState* play, s16 arg2);

extern UNK_TYPE D_80880F30;
extern UNK_TYPE* gBgHakaTrapWallMask;
}

namespace ZeldaOnline {

class HakaTrapController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHakaTrap* Typed() const {
        return reinterpret_cast<BgHakaTrap*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 WALL_FIRED = 4;
    static constexpr u8 WALL_RESET_DELAY = 20;

    using HakaTrapActionFunc = decltype(&BgHakaTrap_Guillotine_Fall);
    static const HakaTrapActionFunc* ActionTable(size_t* count) {
        static const HakaTrapActionFunc sTable[] = {
            BgHakaTrap_SpikedWall_CloseIn, BgHakaTrap_SpikedWall_Burn,    BgHakaTrap_Guillotine_Fall,
            BgHakaTrap_Guillotine_Lift,    BgHakaTrap_SpikedCrusher_Fall, BgHakaTrap_SpikedCrusher_Lift,
            BgHakaTrap_FanBlade_Idle,      BgHakaTrap_FireBarrier_Idle,   BgHakaTrap_FireBarrier_UpdateLayout,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HakaTrapActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsWall() const {
        s16 p = Typed()->dyna.actor.params;
        return p == HAKA_TRAP_SPIKED_WALL || p == HAKA_TRAP_SPIKED_WALL_2;
    }

    bool WallAtHome() const {
        BgHakaTrap* trap = Typed();
        return fabsf(trap->dyna.actor.world.pos.x - trap->dyna.actor.home.pos.x) < 1.0f;
    }

    bool HitWouldReact() const {
        BgHakaTrap* trap = Typed();
        return IsWall() && trap->actionFunc == BgHakaTrap_SpikedWall_CloseIn &&
               (trap->colliderSpikes.base.acFlags & AC_HIT);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };


    void OnActorInit() override {
        Typed()->dyna.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    }

    void UpdateLeader(PlayState* play) override {
        Typed()->dyna.actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
        AbstractActorController::UpdateLeader(play);

        if (IsWall()) {
            UpdateWallMask();
            TickWallReset();
        }
    }

    void BuildCustomProperties(ByteStream& out) override {
        BgHakaTrap* trap = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(trap->timer) << PackedUInt1(trap->isSpikedCrusherStationary)
                                  << PackedInt2(trap->unk_16A),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHakaTrap* trap = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const HakaTrapActionFunc* table = ActionTable(&count);
                if (id < count)
                    trap->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                trap->timer = (u8)(data.Read<PackedUInt1>().value());
                trap->isSpikedCrusherStationary = (u8)(data.Read<PackedUInt1>().value());
                trap->unk_16A = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        D_80880F30 = 0;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHakaTrap* trap = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        if (IsWall())
            trap->colliderSpikes.base.acFlags &= ~AC_HIT;

        switch (trap->dyna.actor.params) {
            case HAKA_TRAP_GUILLOTINE_SLOW:
                UpdateGuillotine(play);
                break;
            case HAKA_TRAP_SPIKED_WALL:
            case HAKA_TRAP_SPIKED_WALL_2:
                UpdateWall(play);
                break;
            case HAKA_TRAP_PROPELLER:
                UpdatePropeller(play);
                break;
            default:
                break;
        }
    }

  private:

    void UpdateGuillotine(PlayState* play) {
        BgHakaTrap* trap = Typed();

        BgHakaTrap_UpdateBodyColliderPos(trap, play);
        trap->colliderCylinder.dim.pos.y = (s16)trap->dyna.actor.world.pos.y;

        u8 roles = COLL_AC | COLL_OC;
        if (trap->actionFunc == BgHakaTrap_Guillotine_Fall &&
            trap->dyna.actor.world.pos.y > (trap->dyna.actor.home.pos.y - 185.0f))
            roles |= COLL_AT;

        RegisterColliderBase(play, &trap->colliderCylinder.base, roles);
    }


    void UpdateWallMask() {
        BgHakaTrap* trap = Typed();

        if (!WallAtHome()) {

            *gBgHakaTrapWallMask = 0;
            return;
        }

        if (trap->dyna.actor.params == HAKA_TRAP_SPIKED_WALL)
            *gBgHakaTrapWallMask |= 1;
        else
            *gBgHakaTrapWallMask |= 2;
    }


    void TickWallReset() {
        BgHakaTrap* trap = Typed();

        if (trap->actionFunc != BgHakaTrap_SpikedWall_CloseIn) {
            return;
        }

        if (!WallAtHome()) {
            trap->timer = 0;
            return;
        }

        if (trap->timer == 0) {
            trap->timer = WALL_RESET_DELAY;
            return;
        }

        trap->timer--;
        if (trap->timer != 0) {
            return;
        }

        f32 startOffset = (trap->dyna.actor.params == HAKA_TRAP_SPIKED_WALL) ? 200.0f : -200.0f;
        trap->dyna.actor.world.pos.x = trap->dyna.actor.home.pos.x + startOffset;
        *gBgHakaTrapWallMask = 0;
    }

    void UpdateWall(PlayState* play) {
        BgHakaTrap* trap = Typed();

        if (trap->actionFunc == BgHakaTrap_SpikedWall_Burn) {
            D_80880F30 = 1;
            SpawnBurnEffects(play);
            return;
        }

        BgHakaTrap_UpdateBodyColliderPos(trap, play);
        trap->colliderCylinder.dim.pos.y = (s16)trap->dyna.actor.world.pos.y;
        RegisterColliderBase(play, &trap->colliderCylinder.base, COLL_AT);
        RegisterColliderBase(play, &trap->colliderSpikes.base, COLL_AC);

        UpdateWallMask();


        if (*gBgHakaTrapWallMask == 3 && trap->dyna.actor.xzDistToPlayer < 300.0f) {
            *gBgHakaTrapWallMask |= WALL_FIRED;
            GET_PLAYER(play)->actor.bgCheckFlags |= 0x100;
        }
    }

    void UpdatePropeller(PlayState* play) {
        BgHakaTrap* trap = Typed();

        BgHakaTrap_FanBlade_UpdateFanRotation(trap, play, trap->dyna.actor.world.rot.z);
    }

    void SpawnBurnEffects(PlayState* play) {
        static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };

        BgHakaTrap* trap = Typed();
        Vec3f vector;
        f32 xScale;
        s32 i;

        for (i = 0; i < 2; i++) {
            f32 rand = Rand_ZeroOne();

            xScale = (trap->dyna.actor.params == HAKA_TRAP_SPIKED_WALL) ? -30.0f : 30.0f;

            vector.x = xScale * rand + trap->dyna.actor.world.pos.x;
            vector.y = Rand_ZeroOne() * 10.0f + trap->dyna.actor.world.pos.y + 30.0f;
            vector.z = Rand_CenteredFloat(320.0f) + trap->dyna.actor.world.pos.z;

            EffectSsDeadDb_Spawn(play, &vector, &zeroVec, &zeroVec, 130, 20, 255, 255, 150, 170, 255, 0, 0, 1, 9,
                                 false);
        }
    }
};

} // namespace ZeldaOnline

#endif