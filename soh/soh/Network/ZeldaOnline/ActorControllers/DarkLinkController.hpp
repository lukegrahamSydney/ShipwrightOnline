#ifndef DARKLINKCONTROLLERH
#define DARKLINKCONTROLLERH

#include "../AbstractActorController.hpp"
extern "C" {
#include "z64player.h"
#include "src/overlays/actors/ovl_En_Torch2/z_en_torch2.h"
#include <macros.h>

extern f32* gEnTorch2StickTilt;
extern s16* gEnTorch2StickAngle;
extern f32* gEnTorch2SwordJumpHeight;
extern s32* gEnTorch2HoldShieldTimer;
extern u8* gEnTorch2ZTargetFlag;
extern u8* gEnTorch2DeathFlag;
extern u8* gEnTorch2SwordJumpState;
extern Vec3f* gEnTorch2SpawnPoint;
extern u8* gEnTorch2JumpslashTimer;
extern u8* gEnTorch2JumpslashFlag;
extern u8* gEnTorch2ActionState;
extern u8* gEnTorch2SwordJumpTimer;
extern u8* gEnTorch2CounterState;
extern u8* gEnTorch2DodgeRollState;
extern u8* gEnTorch2StaggerCount;
extern u8* gEnTorch2StaggerTimer;
extern s8* gEnTorch2LastSwordAnim;
extern u8* gEnTorch2Alpha;
}

namespace ZeldaOnline {

class DarkLinkController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    Player* Typed() const {
        return reinterpret_cast<Player*>(m_actor);
    }

  protected:



    static constexpr bool SEND_JOINT_TABLE = true;

    enum {
        PROP_AI = PROP_CUSTOM_START,
        PROP_SPAWN_POINT,
        PROP_GROUND,
        PROP_TIMERS,
        PROP_HEALTH,
        PROP_STATE_FLAGS,
        PROP_ANIM,
        PROP_JOINTS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        Player* dl = Typed();


        PackProperty(PROP_AI,
                     ByteStream() << PackedUInt1((*gEnTorch2ActionState)) << PackedUInt1((*gEnTorch2DodgeRollState))
                                  << PackedUInt1((*gEnTorch2CounterState)) << PackedUInt1((*gEnTorch2JumpslashFlag))
                                  << PackedUInt1((*gEnTorch2SwordJumpState)) << PackedUInt1((*gEnTorch2ZTargetFlag))
                                  << PackedUInt1((*gEnTorch2DeathFlag)) << PackedInt1((*gEnTorch2LastSwordAnim))
                                  << PackedFloat4((*gEnTorch2StickTilt)) << PackedInt2((*gEnTorch2StickAngle))
                                  << PackedFloat4((*gEnTorch2SwordJumpHeight)),
                     out);

        PackProperty(PROP_SPAWN_POINT,
                     ByteStream() << PackedFloat4((*gEnTorch2SpawnPoint).x) << PackedFloat4((*gEnTorch2SpawnPoint).y)
                                  << PackedFloat4((*gEnTorch2SpawnPoint).z),
                     out);

        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt4((*gEnTorch2HoldShieldTimer)) << PackedUInt1((*gEnTorch2JumpslashTimer))
                                  << PackedUInt1((*gEnTorch2SwordJumpTimer)) << PackedUInt1((*gEnTorch2StaggerCount))
                                  << PackedUInt1((*gEnTorch2StaggerTimer)) << PackedUInt1((*gEnTorch2Alpha)),
                     out);
        PackProperty(PROP_HEALTH, PackedUInt1(dl->actor.colChkInfo.health), out);


        PackProperty(PROP_STATE_FLAGS,
                     ByteStream() << PackedUInt4(dl->stateFlags1) << PackedUInt4(dl->stateFlags2)
                                  << PackedUInt4(dl->stateFlags3) << PackedInt1(dl->meleeWeaponState)
                                  << PackedUInt1(dl->meleeWeaponAnimation),
                     out);

        PackProperty(PROP_ANIM,
                     ByteStream() << PackedFloat4(dl->skelAnime.curFrame) << PackedFloat4(dl->skelAnime.playSpeed)
                                  << PackedFloat4(dl->skelAnime.startFrame) << PackedFloat4(dl->skelAnime.endFrame)
                                  << PackedUInt1(dl->skelAnime.mode),
                     out);



        if (SEND_JOINT_TABLE) {
            ByteStream joints;
            for (s32 i = 0; i < PLAYER_LIMB_MAX; i++) {
                joints << PackedInt2(dl->skelAnime.jointTable[i].x) << PackedInt2(dl->skelAnime.jointTable[i].y)
                       << PackedInt2(dl->skelAnime.jointTable[i].z);
            }
            PackProperty(PROP_JOINTS, joints, out);
        } else {
            PackNullProperty(PROP_JOINTS, out);
        }

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        Player* dl = Typed();

        switch (index) {
            case PROP_AI:
                (*gEnTorch2ActionState) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2DodgeRollState) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2CounterState) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2JumpslashFlag) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2SwordJumpState) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2ZTargetFlag) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2DeathFlag) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2LastSwordAnim) = (s8)(data.Read<PackedInt1>().value());
                (*gEnTorch2StickTilt) = data.Read<PackedFloat4>().value();
                (*gEnTorch2StickAngle) = (s16)(data.Read<PackedInt2>().value());
                (*gEnTorch2SwordJumpHeight) = data.Read<PackedFloat4>().value();
                break;
            case PROP_SPAWN_POINT:
                (*gEnTorch2SpawnPoint).x = data.Read<PackedFloat4>().value();
                (*gEnTorch2SpawnPoint).y = data.Read<PackedFloat4>().value();
                (*gEnTorch2SpawnPoint).z = data.Read<PackedFloat4>().value();
                break;

            case PROP_TIMERS:
                (*gEnTorch2HoldShieldTimer) = data.Read<PackedInt4>().value();
                (*gEnTorch2JumpslashTimer) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2SwordJumpTimer) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2StaggerCount) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2StaggerTimer) = (u8)(data.Read<PackedUInt1>().value());
                (*gEnTorch2Alpha) = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEALTH: {
                u8 health = (u8)(data.Read<PackedUInt1>().value());
                if (health < dl->actor.colChkInfo.health || !IsRunningLocally())
                    dl->actor.colChkInfo.health = health;
                break;
            }
            case PROP_STATE_FLAGS:
                dl->stateFlags1 = data.Read<PackedUInt4>().value();
                dl->stateFlags2 = data.Read<PackedUInt4>().value();
                dl->stateFlags3 = data.Read<PackedUInt4>().value();
                dl->meleeWeaponState = (s8)(data.Read<PackedInt1>().value());
                dl->meleeWeaponAnimation = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM:
                dl->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                dl->skelAnime.playSpeed = data.Read<PackedFloat4>().value();
                dl->skelAnime.startFrame = data.Read<PackedFloat4>().value();
                dl->skelAnime.endFrame = data.Read<PackedFloat4>().value();
                dl->skelAnime.mode = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_JOINTS:
                if (propLen == 0)
                    break;
                for (s32 i = 0; i < PLAYER_LIMB_MAX; i++) {
                    dl->skelAnime.jointTable[i].x = (s16)(data.Read<PackedInt2>().value());
                    dl->skelAnime.jointTable[i].y = (s16)(data.Read<PackedInt2>().value());
                    dl->skelAnime.jointTable[i].z = (s16)(data.Read<PackedInt2>().value());
                }
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        Player* dl = Typed();


        if (dl->cylinder.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        dl->cylinder.base.acFlags &= ~AC_HIT;

        {
            Vec3f from = dl->actor.world.pos;
            from.y += 50.0f;

            CollisionPoly* poly = nullptr;
            s32 bgId = BGCHECK_SCENE;

            f32 floorY = BgCheck_EntityRaycastFloor4(&play->colCtx, &poly, &bgId, &dl->actor, &from);
            if (poly != nullptr) {
                dl->actor.floorPoly = poly;
                dl->actor.floorBgId = bgId;
                dl->actor.floorHeight = floorY;
            }
        }

        Collider_UpdateCylinder(&dl->actor, &dl->cylinder);
        RegisterColliderBase(play, &dl->cylinder.base, COLL_AC | COLL_OC);

        if (dl->meleeWeaponState > 0) {
            RegisterColliderBase(play, &dl->meleeWeaponQuads[0].base, COLL_AT);
            RegisterColliderBase(play, &dl->meleeWeaponQuads[1].base, COLL_AT);
        }
    }

    int m_mirrorPlayerID = 0;
};

} // namespace ZeldaOnline

#endif