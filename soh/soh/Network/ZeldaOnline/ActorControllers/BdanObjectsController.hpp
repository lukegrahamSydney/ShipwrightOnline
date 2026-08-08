#ifndef BDANOBJECTSCONTROLLERH
#define BDANOBJECTSCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Bdan_Objects/z_bg_bdan_objects.h"

void BgBdanObjects_OctoPlatform_WaitForRutoToStartCutscene(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_OctoPlatform_RaiseToUpperPosition(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_OctoPlatform_WaitForRutoToAdvanceCutscene(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_OctoPlatform_PauseBeforeDescending(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_OctoPlatform_WaitForBigOctoToStartBattle(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_OctoPlatform_BattleInProgress(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_OctoPlatform_DescendWithBigOcto(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_SinkToFloorHeight(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_WaitForPlayerInRange(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_RaiseToUpperPosition(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_DoNothing(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_ElevatorOscillate(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_WaitForSwitch(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_ChangeWaterBoxLevel(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_WaitForTimerExpired(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_WaitForPlayerOnTop(BgBdanObjects* bd, PlayState* play);
void BgBdanObjects_FallToLowerPos(BgBdanObjects* bd, PlayState* play);
}

namespace ZeldaOnline {

class BdanObjectsController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgBdanObjects* Typed() const {
        return reinterpret_cast<BgBdanObjects*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using BdanActionFunc = void (*)(BgBdanObjects*, PlayState*);
    static const BdanActionFunc* ActionTable(size_t* count) {
        static const BdanActionFunc sTable[] = {
            BgBdanObjects_OctoPlatform_WaitForRutoToStartCutscene,
            BgBdanObjects_OctoPlatform_RaiseToUpperPosition,
            BgBdanObjects_OctoPlatform_WaitForRutoToAdvanceCutscene,
            BgBdanObjects_OctoPlatform_PauseBeforeDescending,
            BgBdanObjects_OctoPlatform_WaitForBigOctoToStartBattle,
            BgBdanObjects_OctoPlatform_BattleInProgress,
            BgBdanObjects_OctoPlatform_DescendWithBigOcto,
            BgBdanObjects_SinkToFloorHeight,
            BgBdanObjects_WaitForPlayerInRange,
            BgBdanObjects_RaiseToUpperPosition,
            BgBdanObjects_DoNothing,
            BgBdanObjects_ElevatorOscillate,
            BgBdanObjects_WaitForSwitch,
            BgBdanObjects_ChangeWaterBoxLevel,
            BgBdanObjects_WaitForTimerExpired,
            BgBdanObjects_WaitForPlayerOnTop,
            BgBdanObjects_FallToLowerPos,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BdanActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        return (Typed()->actionFunc == BgBdanObjects_OctoPlatform_BattleInProgress) ? COLL_AT : 0;
    }

    bool CanRelinquishLeadership() const override {
        return false;
    }

    bool ShouldLockActor() const override {
        return false;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_POSITION,
        PROP_ROTATION,
        PROP_BATTERY,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgBdanObjects* bd = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(
            PROP_POSITION,
            ByteStream() << PackedFloat4(bd->dyna.actor.world.pos.y) << PackedFloat4(bd->dyna.actor.home.pos.y), out);
        PackProperty(PROP_ROTATION,
                     ByteStream() << PackedInt2(bd->dyna.actor.shape.rot.y) << PackedInt2(bd->dyna.actor.world.rot.y)
                                  << PackedInt2(bd->dyna.actor.home.rot.y),
                     out);
        PackProperty(
            PROP_BATTERY,
                     ByteStream() << PackedUInt1(bd->switchFlag)
                                  << PackedInt2(bd->timer) ,
                     out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);

    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgBdanObjects* bd = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BdanActionFunc* table = ActionTable(&count);
                if (id < count)
                    bd->actionFunc = table[id];
                break;
            }
            case PROP_POSITION:
                bd->dyna.actor.world.pos.y = data.Read<PackedFloat4>().value();
                bd->dyna.actor.home.pos.y = data.Read<PackedFloat4>().value();
                break;
            case PROP_ROTATION:
                bd->dyna.actor.shape.rot.y = (s16)(data.Read<PackedInt2>().value());
                bd->dyna.actor.world.rot.y = (s16)(data.Read<PackedInt2>().value());
                bd->dyna.actor.home.rot.y = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BATTERY:
                bd->switchFlag = data.Read<PackedUInt1>().value();
                bd->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;

            default:
                return false;
        }
        return true;
    }

    bool m_inPlace = false;
    void CatchUpStragglers(PlayState* play) {
        BgBdanObjects* bd = Typed();

        if (m_inPlace || bd->dyna.actor.params != 0 || bd->actionFunc != BgBdanObjects_RaiseToUpperPosition)
            return;

        if (DynaPolyActor_IsPlayerOnTop(&bd->dyna)) {
            m_inPlace = true;
            return;
        }

        Player* localPlayer = GET_PLAYER(play);

        Collider_UpdateCylinder(&bd->dyna.actor, &bd->collider);
        f32 centreX = bd->collider.dim.pos.x;
        f32 centreZ = bd->collider.dim.pos.z;

        f32 maxRadius = bd->collider.dim.radius * 0.5f;

        u32 h = (u32)(uintptr_t)localPlayer * 2654435761u;
        f32 angle = (h >> 16) * (2.0f * (f32)M_PI / 65536.0f);
        f32 radius = maxRadius * 0.4f + (h & 0x1F) * (maxRadius * 0.02f);

        localPlayer->actor.world.pos.x = centreX + cosf(angle) * radius;
        localPlayer->actor.world.pos.z = centreZ + sinf(angle) * radius;
        localPlayer->actor.world.pos.y = bd->dyna.actor.world.pos.y + 150;
        localPlayer->actor.velocity.y = 0.0f;
    }

    void UpdatePuppet(PlayState* play) override {
        BgBdanObjects* bd = Typed();

        if (bd->dyna.actor.params == 1)
            printf("PLATFORM PUPPET\n");
        if (bd->dyna.actor.params == 2)
            play->colCtx.colHeader->waterBoxes[7].ySurface = int16_t(bd->dyna.actor.world.pos.y);

        else if (bd->dyna.actor.params != 0) {
            if (bd->actionFunc != BgBdanObjects_DoNothing && bd->dyna.actor.xzDistToPlayer < 400.0f &&
                IsLocalPlayerClosest())
                ClaimLeadership(CLAIM_REASON_PROXIMITY);
        }

        Actor_SetFocus(&bd->dyna.actor, 50.0f);

        if (bd->actionFunc == BgBdanObjects_OctoPlatform_BattleInProgress) {
            Collider_UpdateCylinder(&bd->dyna.actor, &bd->collider);
        }
        if (m_roles != 0)
            RegisterColliderBase(play, &bd->collider.base, m_roles);

        CatchUpStragglers(play);
    }

  private:
    u8 m_roles = 0;
};

}

#endif
