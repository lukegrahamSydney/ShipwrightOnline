#ifndef POTCONTROLLERH
#define POTCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Tsubo/z_obj_tsubo.h"

void ObjTsubo_WaitForObject(ObjTsubo* pot, PlayState* play);
void ObjTsubo_Idle(ObjTsubo* pot, PlayState* play);
void ObjTsubo_LiftedUp(ObjTsubo* pot, PlayState* play);
void ObjTsubo_Thrown(ObjTsubo* pot, PlayState* play);

void ObjTsubo_AirBreak(ObjTsubo* pot, PlayState* play);
void ObjTsubo_WaterBreak(ObjTsubo* pot, PlayState* play);
void ObjTsubo_SpawnCollectible(ObjTsubo* pot, PlayState* play);
void ObjTsubo_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class PotController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    ObjTsubo* Typed() const {
        return reinterpret_cast<ObjTsubo*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_IDLE = 1;

    static constexpr u32 BREAK_DMG_FLAGS = 0x4FC1FFFC;

    using TsuboActionFunc = void (*)(ObjTsubo*, PlayState*);
    static const TsuboActionFunc* ActionTable(size_t* count) {
        static const TsuboActionFunc sTable[] = {
            ObjTsubo_WaitForObject,
            ObjTsubo_Idle,
            ObjTsubo_LiftedUp,
            ObjTsubo_Thrown,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TsuboActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool HitWouldReact() const {
        ObjTsubo* pot = Typed();
        if (!(pot->collider.base.acFlags & AC_HIT))
            return false;
        ColliderInfo* hit = pot->collider.info.acHitInfo;
        return hit != nullptr && (hit->toucher.dmgFlags & BREAK_DMG_FLAGS);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
    };

    void BuildCustomProperties(ByteStream& out) override {
        ObjTsubo* pot = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjTsubo* pot = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const TsuboActionFunc* table = ActionTable(&count);
                if (id < count)
                    pot->actionFunc = table[id];
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        ObjTsubo* pot = Typed();
        if (gPlayState == nullptr || pot == nullptr)
            return;

        if (pot->actor.bgCheckFlags & 0x20)
            ObjTsubo_WaterBreak(pot, gPlayState);
        else
            ObjTsubo_AirBreak(pot, gPlayState);

        ObjTsubo_SpawnCollectible(pot, gPlayState);
        SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &pot->actor.world.pos, 20, NA_SE_EV_POT_BROKEN);
    }

    void EnsureDrawInstalled(PlayState* play) {
        ObjTsubo* pot = Typed();
        if (pot->actor.draw == nullptr && pot->objTsuboBankIndex >= 0 &&
            Object_IsLoaded(&play->objectCtx, pot->objTsuboBankIndex)) {
            pot->actor.draw = ObjTsubo_Draw;
            pot->actor.objBankIndex = pot->objTsuboBankIndex;
        }
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawInstalled(play);
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        ObjTsubo* pot = Typed();

        EnsureDrawInstalled(play);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        if (pot->actor.parent == &GET_PLAYER(play)->actor) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        pot->collider.base.acFlags &= ~AC_HIT;
        pot->collider.base.atFlags &= ~AT_HIT;

        if (m_currentActionIndex == ID_IDLE && pot->actor.parent == nullptr) {
            if (pot->actor.xzDistToPlayer < 600.0f) {
                Collider_UpdateCylinder(&pot->actor, &pot->collider);

                u8 roles = COLL_AC;
                if (pot->actor.xzDistToPlayer < 150.0f)
                    roles |= COLL_OC;
                RegisterColliderBase(play, &pot->collider.base, roles);
            }

            if (pot->actor.xzDistToPlayer < 100.0f) {
                s16 yawDiff = pot->actor.yawTowardsPlayer - GET_PLAYER(play)->actor.world.rot.y;
                if (ABS(yawDiff) >= 0x5556)
                    Actor_OfferGetItem(&pot->actor, play, GI_NONE, 30.0f, 30.0f);
            }
        }
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

}

#endif
