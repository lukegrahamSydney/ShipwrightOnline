#ifndef GOROIWACONTROLLERH
#define GOROIWACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Goroiwa/z_en_goroiwa.h"

void EnGoroiwa_Roll(EnGoroiwa* g, PlayState* play);
void EnGoroiwa_MoveAndFallToGround(EnGoroiwa* g, PlayState* play);
void EnGoroiwa_Wait(EnGoroiwa* g, PlayState* play);
void EnGoroiwa_MoveUp(EnGoroiwa* g, PlayState* play);
void EnGoroiwa_MoveDown(EnGoroiwa* g, PlayState* play);

void EnGoroiwa_UpdateCollider(EnGoroiwa* g);
}

namespace ZeldaOnline {

class GoroiwaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnGoroiwa* Typed() const {
        return reinterpret_cast<EnGoroiwa*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    static constexpr u8 SF_ENABLE_AT = (1 << 0);
    static constexpr u8 SF_ENABLE_OC = (1 << 1);
    static constexpr u8 SF_PLAYER_IN_THE_WAY = (1 << 2);
    static constexpr u8 SF_RETAIN_ROT_SPEED = (1 << 3);
    static constexpr u8 SF_IN_WATER = (1 << 4);

    static constexpr f32 REGISTER_RADIUS = 300.0f;

    using GoroiwaActionFunc = void (*)(EnGoroiwa*, PlayState*);
    static const GoroiwaActionFunc* ActionTable(size_t* count) {
        static const GoroiwaActionFunc sTable[] = {
            EnGoroiwa_Roll,
            EnGoroiwa_MoveAndFallToGround,
            EnGoroiwa_Wait,
            EnGoroiwa_MoveUp,
            EnGoroiwa_MoveDown,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const GoroiwaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        EnGoroiwa* g = Typed();
        if (g->collisionDisabledTimer > 0)
            return 0;
        u8 roles = 0;
        if (g->stateFlags & SF_ENABLE_AT)
            roles |= COLL_AT;
        if (g->stateFlags & SF_ENABLE_OC)
            roles |= COLL_OC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ROLL_STATE,
        PROP_WAYPOINTS,
        PROP_TIMERS,
        PROP_STATE_FLAGS,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnGoroiwa* g = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_ROLL_STATE,
                     ByteStream() << PackedFloat4(g->rollRotSpeed) << PackedFloat4(g->prevRollAngleDiff)
                                  << PackedFloat4(g->prevUnitRollAxis.x) << PackedFloat4(g->prevUnitRollAxis.y)
                                  << PackedFloat4(g->prevUnitRollAxis.z),
                     out);

        PackProperty(PROP_WAYPOINTS,
                     ByteStream() << PackedInt2(g->currentWaypoint) << PackedInt2(g->nextWaypoint)
                                  << PackedInt2(g->pathDirection),
                     out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(g->waitTimer) << PackedInt2(g->bounceCount)
                                  << PackedInt2(g->collisionDisabledTimer),
                     out);
        PackProperty(PROP_STATE_FLAGS, PackedUInt1(g->stateFlags), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnGoroiwa* g = Typed();
        printf("B\n");
        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const GoroiwaActionFunc* table = ActionTable(&count);
                if (id < count)
                    g->actionFunc = table[id];
                break;
            }
            case PROP_ROLL_STATE:
                g->rollRotSpeed = data.Read<PackedFloat4>().value();
                g->prevRollAngleDiff = data.Read<PackedFloat4>().value();
                g->prevUnitRollAxis.x = data.Read<PackedFloat4>().value();
                g->prevUnitRollAxis.y = data.Read<PackedFloat4>().value();
                g->prevUnitRollAxis.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_WAYPOINTS:
                g->currentWaypoint = (s16)(data.Read<PackedInt2>().value());
                g->nextWaypoint = (s16)(data.Read<PackedInt2>().value());
                g->pathDirection = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                g->waitTimer = (s16)(data.Read<PackedInt2>().value());
                g->bounceCount = (s16)(data.Read<PackedInt2>().value());
                g->collisionDisabledTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_STATE_FLAGS:
                g->stateFlags = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void UpdatePuppet(PlayState* play) override {
        EnGoroiwa* g = Typed();

        if (g->actor.xzDistToPlayer < REGISTER_RADIUS && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        {
            Vec3f rayOrigin = g->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            g->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &g->actor.floorPoly, &floorBgId,
                                                               &g->actor, &rayOrigin);
            g->actor.floorBgId = floorBgId;
        }

        EnGoroiwa_UpdateCollider(g);

        if (g->actor.xzDistToPlayer < REGISTER_RADIUS)
            RegisterColliderBase(play, &g->collider.base, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AT | COLL_OC;
};

}

#endif
