#ifndef TRUTHSPINNERCONTROLLERH
#define TRUTHSPINNERCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Haka_Gate/z_bg_haka_gate.h"

void BgHakaGate_DoNothing(BgHakaGate* gate, PlayState* play);
void BgHakaGate_StatueInactive(BgHakaGate* gate, PlayState* play);
void BgHakaGate_StatueIdle(BgHakaGate* gate, PlayState* play);
void BgHakaGate_StatueTurn(BgHakaGate* gate, PlayState* play);
void BgHakaGate_FloorClosed(BgHakaGate* gate, PlayState* play);
void BgHakaGate_FloorOpen(BgHakaGate* gate, PlayState* play);
void BgHakaGate_GateWait(BgHakaGate* gate, PlayState* play);
void BgHakaGate_GateOpen(BgHakaGate* gate, PlayState* play);
void BgHakaGate_SkullOfTruth(BgHakaGate* gate, PlayState* play);
void BgHakaGate_FalseSkull(BgHakaGate* gate, PlayState* play);

extern s16* gBgHakaGateSkullOfTruthRotY;
extern u8* gBgHakaGatePuzzleState;
extern s16* gBgHakaGateStatueRotY;
extern f32* gBgHakaGateStatueDistToPlayer;
}

namespace ZeldaOnline {

class TruthSpinnerController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHakaGate* Typed() const {
        return reinterpret_cast<BgHakaGate*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr s16 SKULL_FOUND = 100;

    using HakaGateActionFunc = decltype(&BgHakaGate_DoNothing);
    static const HakaGateActionFunc* ActionTable(size_t* count) {
        static const HakaGateActionFunc sTable[] = {
            BgHakaGate_DoNothing,  BgHakaGate_StatueInactive, BgHakaGate_StatueIdle, BgHakaGate_StatueTurn,
            BgHakaGate_FloorClosed, BgHakaGate_FloorOpen,     BgHakaGate_GateWait,   BgHakaGate_GateOpen,
            BgHakaGate_SkullOfTruth, BgHakaGate_FalseSkull,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HakaGateActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsSkull() const {
        return Typed()->dyna.actor.params == BGHAKAGATE_SKULL;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_VARS,
        PROP_TRUTH,
        PROP_STATUE_ROT,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHakaGate* gate = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);


        PackProperty(PROP_VARS,
                     ByteStream() << PackedInt2(gate->actionVar1) << PackedInt2(gate->actionVar2)
                                  << PackedInt2(gate->actionVar3) << PackedInt2(gate->actionVar4)
                                  << PackedInt2(IsSkull() ? 0 : gate->actionVar5),
                     out);
         

        if (IsSkull() && gate->actionVar4)
            PackProperty(PROP_TRUTH, PackedInt2(*gBgHakaGateSkullOfTruthRotY), out);

        if (gate->dyna.actor.params == BGHAKAGATE_STATUE)
            PackProperty(PROP_STATUE_ROT, PackedInt2(*gBgHakaGateStatueRotY), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHakaGate* gate = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const HakaGateActionFunc* table = ActionTable(&count);
                if (id < count)
                    gate->actionFunc = table[id];
                break;
            }
            case PROP_VARS: {
                gate->actionVar1 = (s16)(data.Read<PackedInt2>().value());
                gate->actionVar2 = (s16)(data.Read<PackedInt2>().value());
                gate->actionVar3 = (s16)(data.Read<PackedInt2>().value());
                gate->actionVar4 = (s16)(data.Read<PackedInt2>().value());
                s16 var5 = (s16)(data.Read<PackedInt2>().value());
                if (!IsSkull())
                    gate->actionVar5 = var5;
                break;
            }
            case PROP_TRUTH:
                *gBgHakaGateSkullOfTruthRotY = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_STATUE_ROT:
                *gBgHakaGateStatueRotY = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BgHakaGate* gate = Typed();

        if ((changed & (1ull << PROP_ACTION)) && gate->actionFunc == BgHakaGate_GateOpen)
            OnePointCutscene_Attention(gPlayState, &gate->dyna.actor);
    }

    void UpdatePuppet(PlayState* play) override {
        BgHakaGate* gate = Typed();

        switch (gate->dyna.actor.params) {
            case BGHAKAGATE_STATUE:
                UpdateStatue(play);
                break;
            case BGHAKAGATE_FLOOR:
                UpdateFloor(play);
                break;
            case BGHAKAGATE_SKULL:
                UpdateSkull(play);
                break;
            default:
                break;
        }
    }

  private:
    void UpdateStatue(PlayState* play) {
        BgHakaGate* gate = Typed();
        Player* player = GET_PLAYER(play);

        if (gate->actionFunc == BgHakaGate_StatueTurn) {
            CarryLocalRider(play);
            return;
        }

        m_rideDist = 0.0f;
        *gBgHakaGateStatueDistToPlayer = 0.0f;

        if (gate->dyna.unk_150 != 0.0f) {
            if (gate->actionFunc == BgHakaGate_StatueIdle && gate->actionVar1 == 0) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;
            gate->dyna.unk_150 = 0.0f;
        }

        if (Flags_GetSwitch(play, gate->switchFlag))
            *gBgHakaGatePuzzleState = SKULL_FOUND;
    }


    void CarryLocalRider(PlayState* play) {
        BgHakaGate* gate = Typed();
        Player* player = GET_PLAYER(play);

        if (m_rideDist <= 0.0f) {
            if (player->actor.floorBgId != gate->dyna.bgId)
                return;
            m_rideDist = gate->dyna.actor.xzDistToPlayer;
            m_rideInitAngle = gate->dyna.actor.shape.rot.y - gate->dyna.actor.yawTowardsPlayer;
            if (m_rideDist <= 0.0f)
                return;
        }

        if (player->actor.floorBgId != gate->dyna.bgId) {
            m_rideDist = 0.0f;
            return;
        }

        player->actor.world.pos.x =
            gate->dyna.actor.home.pos.x + (Math_SinS(gate->dyna.actor.shape.rot.y - m_rideInitAngle) * m_rideDist);
        player->actor.world.pos.z =
            gate->dyna.actor.home.pos.z + (Math_CosS(gate->dyna.actor.shape.rot.y - m_rideInitAngle) * m_rideDist);

        *gBgHakaGateStatueDistToPlayer = m_rideDist;
    }

    void UpdateFloor(PlayState* play) {
        BgHakaGate* gate = Typed();

        if (gate->actionFunc == BgHakaGate_FloorOpen)
            func_8003EBF8(play, &play->colCtx.dyna, gate->dyna.bgId);
        else
            func_8003EC50(play, &play->colCtx.dyna, gate->dyna.bgId);

        if (gate->actionFunc != BgHakaGate_FloorClosed)
            return;

        if ((*gBgHakaGateStatueDistToPlayer > 1.0f) && (*gBgHakaGateStatueRotY != 0)) {
            Player* player = GET_PLAYER(play);
            f32 cos = Math_CosS(*gBgHakaGateStatueRotY);
            f32 sin = Math_SinS(*gBgHakaGateStatueRotY);
            f32 dx = player->actor.world.pos.x - gate->dyna.actor.world.pos.x;
            f32 dz = player->actor.world.pos.z - gate->dyna.actor.world.pos.z;
            f32 radialDist = dx * cos - dz * sin;
            f32 angDist = dx * sin + dz * cos;

            if ((radialDist > 110.0f) || (fabsf(angDist) > 40.0f)) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
            }
        }
    }

    void UpdateSkull(PlayState* play) {
        BgHakaGate* gate = Typed();

        gate->actionVar5++;


        if (gate->actionFunc == BgHakaGate_FalseSkull) {
            if (play->actorCtx.lensActive)
                gate->dyna.actor.flags |= ACTOR_FLAG_REACT_TO_LENS;
            else
                gate->dyna.actor.flags &= ~ACTOR_FLAG_REACT_TO_LENS;
        }
    }

    f32 m_rideDist = 0.0f;
    s16 m_rideInitAngle = 0;
};

}

#endif
