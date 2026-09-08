#ifndef BOWLWALLCONTROLLERH
#define BOWLWALLCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Bowl_Wall/z_bg_bowl_wall.h"

void BgBowlWall_SpawnBullseyes(BgBowlWall* wall, PlayState* play);
void BgBowlWall_WaitForHit(BgBowlWall* wall, PlayState* play);
void BgBowlWall_FallDoEffects(BgBowlWall* wall, PlayState* play);
void BgBowlWall_FinishFall(BgBowlWall* wall, PlayState* play);
void BgBowlWall_Reset(BgBowlWall* wall, PlayState* play);
}

#include "src/overlays/actors/ovl_En_Wall_Tubo/z_en_wall_tubo.h"

namespace ZeldaOnline {


class BowlWallController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgBowlWall* Typed() const {
        return reinterpret_cast<BgBowlWall*>(m_actor);
    }

    virtual bool CanSpawnActorOverNetwork(s16 actorId, s16 params) {
        return true;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    enum {
        PROP_TARGET_ROT = PROP_CUSTOM_START,
        PROP_BULLSEYE,
        PROP_WALL_STATUS,
        PROP_LEADER_PLAYING,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgBowlWall* wall = Typed();

        PackProperty(PROP_TARGET_ROT, PackedInt2(wall->dyna.actor.shape.rot.z), out);
        PackProperty(PROP_BULLSEYE,
                     ByteStream() << PackedFloat4(wall->bullseyeCenter.x) << PackedFloat4(wall->bullseyeCenter.y)
                                  << PackedFloat4(wall->bullseyeCenter.z),
                     out);
        u8 status = 0;
        if (wall->chuGirl != nullptr)
            status = (u8)wall->chuGirl->wallStatus[wall->dyna.actor.params];
        PackProperty(PROP_WALL_STATUS, PackedUInt1(status), out);

        PackProperty(PROP_LEADER_PLAYING, PackedUInt1(Flags_GetSwitch(gPlayState, 0x38)), out);

    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgBowlWall* wall = Typed();

        switch (index) {
            case PROP_TARGET_ROT: {
                s16 rotZ = (s16)(data.Read<PackedInt2>().value());
                wall->dyna.actor.shape.rot.z = wall->dyna.actor.world.rot.z = rotZ;
                break;
            }
            case PROP_BULLSEYE:
                wall->bullseyeCenter.x = data.Read<PackedFloat4>().value();
                wall->bullseyeCenter.y = data.Read<PackedFloat4>().value();
                wall->bullseyeCenter.z = data.Read<PackedFloat4>().value();
                break;

            case PROP_WALL_STATUS: {
                u8 status = (u8)(data.Read<PackedUInt1>().value());
                if (wall->chuGirl != nullptr)
                    wall->chuGirl->wallStatus[wall->dyna.actor.params] = status;
                break;
            }

            case PROP_LEADER_PLAYING: {
                m_leaderPlaying = data.Read<PackedUInt1>().value() == 1;
                break;
            }

            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BgBowlWall* wall = Typed();

        if (changed & (1U << PROP_BULLSEYE))
        {


            if (wall->dyna.actor.child != nullptr && wall->dyna.actor.child->id == ACTOR_EN_WALL_TUBO &&
                wall->dyna.actor.child->update != nullptr) {
                Actor_Kill(wall->dyna.actor.child);
                wall->dyna.actor.child = nullptr;
            }

            Actor* bullseye = Actor_SpawnAsChild(&gPlayState->actorCtx, &wall->dyna.actor, gPlayState,
                                                    ACTOR_EN_WALL_TUBO, wall->bullseyeCenter.x, wall->bullseyeCenter.y,
                                                    wall->bullseyeCenter.z, 0, 0, 0, wall->dyna.actor.params);
            if (bullseye != nullptr) {
                ((EnWallTubo*)bullseye)->explosionCenter = wall->bullseyeCenter;
            }
            
        }
    }

    void UpdatePuppet(PlayState* play) override {
        BgBowlWall* wall = Typed();

        if (wall->chuGirl == nullptr) {
            for (Actor* a = play->actorCtx.actorLists[ACTORCAT_NPC].head; a != nullptr; a = a->next) {
                if (a->id == ACTOR_EN_BOM_BOWL_MAN) {
                    wall->chuGirl = (EnBomBowlMan*)a;
                    break;
                }
            }
        }

        if (Flags_GetSwitch(gPlayState, 0x38) && !m_leaderPlaying)
            ClaimLeadership(CLAIM_REASON_NOW);

    }
  private:
    Vec3f m_placedAt{};
    bool m_leaderPlaying = false;
};

} // namespace ZeldaOnline

#endif