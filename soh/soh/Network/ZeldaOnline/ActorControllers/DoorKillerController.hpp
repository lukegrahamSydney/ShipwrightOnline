#ifndef DOORKILLERCONTROLLERH
#define DOORKILLERCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Door_Killer/z_door_killer.h"

void DoorKiller_Wait(DoorKiller*, PlayState* play);
void DoorKiller_WaitBeforeWobble(DoorKiller*, PlayState* play);
void DoorKiller_Wobble(DoorKiller*, PlayState* play);
void DoorKiller_FallOver(DoorKiller*, PlayState* play);
void DoorKiller_RiseBackUp(DoorKiller*, PlayState* play);
void DoorKiller_Die(DoorKiller*, PlayState* play);
void DoorKiller_FallAsRubble(DoorKiller*, PlayState* play);
void DoorKiller_SetProperties(DoorKiller*, PlayState* play);
}

namespace ZeldaOnline {

class DoorKillerController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    DoorKiller* Typed() const {
        return reinterpret_cast<DoorKiller*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return (params & 0xFF) == DOOR_KILLER_DOOR;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using KillerActionFunc = void (*)(DoorKiller*, PlayState*);
    static const KillerActionFunc* ActionTable(size_t* count) {
        static const KillerActionFunc sTable[] = {
            DoorKiller_Wait,        DoorKiller_WaitBeforeWobble, DoorKiller_Wobble, DoorKiller_FallOver,
            DoorKiller_RiseBackUp,  DoorKiller_Die,              DoorKiller_SetProperties,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const KillerActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_JOINTS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        DoorKiller* dk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, ByteStream() << PackedInt2(dk->timer) << PackedUInt1(dk->hasHitPlayerOrGround), out);

        ByteStream joints;
        for (s32 i = 0; i < 9; i++)
            joints << PackedInt2(dk->jointTable[i].x) << PackedInt2(dk->jointTable[i].y)
                   << PackedInt2(dk->jointTable[i].z);
        PackProperty(PROP_JOINTS, joints, out);

        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        DoorKiller* dk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const KillerActionFunc* table = ActionTable(&count);
                if (id < count)
                    dk->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                dk->timer = (s16)(data.Read<PackedInt2>().value());
                dk->hasHitPlayerOrGround = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_JOINTS:
                for (s32 i = 0; i < 9; i++) {
                    dk->jointTable[i].x = (s16)(data.Read<PackedInt2>().value());
                    dk->jointTable[i].y = (s16)(data.Read<PackedInt2>().value());
                    dk->jointTable[i].z = (s16)(data.Read<PackedInt2>().value());
                }
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        DoorKiller* dk = Typed();
        Player* player = GET_PLAYER(play);
        Vec3f rel;
        s16 facing;

        if (dk->actionFunc != DoorKiller_Wait)
            return;


        if ((dk->colliderCylinder.base.acFlags & AC_HIT) || dk->playerIsOpening ||
            Actor_GetCollidedExplosive(play, &dk->colliderJntSph.base) != NULL) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        Actor_WorldToActorCoords(&dk->actor, &rel, &player->actor.world.pos);

        if (!Player_InCsMode(play) && (fabsf(rel.y) < 20.0f) && (fabsf(rel.x) < 20.0f) && (rel.z < 50.0f) &&
            (rel.z > 0.0f)) {
            facing = player->actor.shape.rot.y - dk->actor.shape.rot.y;
            if (rel.z > 0.0f) {
                facing = 0x8000 - facing;
            }
            if (ABS(facing) < 0x3000) {
                player->doorType = PLAYER_DOORTYPE_FAKE;
                player->doorDirection = (rel.z >= 0.0f) ? 1 : -1;
                player->doorActor = &dk->actor;
            }
        }

        Collider_UpdateCylinder(&dk->actor, &dk->colliderCylinder);
        RegisterColliderBase(play, &dk->colliderCylinder.base, COLL_AC);
        RegisterColliderBase(play, &dk->colliderJntSph.base, COLL_AC);
    }
};

} // namespace ZeldaOnline

#endif
