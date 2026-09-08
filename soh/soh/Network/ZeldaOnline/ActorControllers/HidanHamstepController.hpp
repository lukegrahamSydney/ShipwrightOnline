#ifndef HIDANHAMSTEPCONTROLLERH
#define HIDANHAMSTEPCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Hamstep/z_bg_hidan_hamstep.h"
#include "objects/object_hidan_objects/object_hidan_objects.h"

void func_808887C4(BgHidanHamstep* step, PlayState* play);
void BgHidanHamstep_SetupAction(BgHidanHamstep* step, s32 action);
s32 BgHidanHamstep_SpawnChildren(BgHidanHamstep* thisx, PlayState* play2);
extern ColliderTrisInit* gBgHidanHamstepTrisInit;
}

namespace ZeldaOnline {

class HidanHamstepController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    static void RegisterHooks(s16 actorID, bool enabled) {
        COND_ID_HOOK(ShouldActorInit, ACTOR_BG_HIDAN_HAMSTEP, enabled, [&](void* actorRef, bool*) {
            Actor* actor = (Actor*)actorRef;

            AbstractActorController::InstallCustomInit(actor, HidanHamstepController::BgHidanHamstep_Init);
        });
    }

    BgHidanHamstep* Typed() const {
        return reinterpret_cast<BgHidanHamstep*>(m_actor);
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

    static void BgHidanHamstep_Init(Actor* thisx, PlayState* play) {
        static InitChainEntry sInitChain[] = {
            ICHAIN_VEC3F_DIV1000(scale, 100, ICHAIN_STOP),
        };
        static f32 sYPosOffsets[] = {
            -20.0f, -120.0f, -220.0f, -320.0f, -420.0f,
        };


        BgHidanHamstep* step = reinterpret_cast<BgHidanHamstep*>(thisx);
        CollisionHeader* colHeader = NULL;
        Vec3f v[3];

        DynaPolyActor_Init(&step->dyna, DPM_PLAYER);
        Actor_ProcessInitChain(&step->dyna.actor, sInitChain);

        if ((step->dyna.actor.params & 0xFF) == 0) {
            Collider_InitTris(play, &step->collider);
            Collider_SetTris(play, &step->collider, &step->dyna.actor, gBgHidanHamstepTrisInit, step->colliderItems);

            for (s32 i = 0; i < 2; i++) {
                for (s32 i2 = 0; i2 < 3; i2++) {
                    v[i2].x = gBgHidanHamstepTrisInit->elements[i].dim.vtx[i2].x + step->dyna.actor.home.pos.x;
                    v[i2].y = gBgHidanHamstepTrisInit->elements[i].dim.vtx[i2].y + step->dyna.actor.home.pos.y;
                    v[i2].z = gBgHidanHamstepTrisInit->elements[i].dim.vtx[i2].z + step->dyna.actor.home.pos.z;
                }
                Collider_SetTrisVertices(&step->collider, i, &v[0], &v[1], &v[2]);
            }
        }

        if ((step->dyna.actor.params & 0xFF) == 0) {
            CollisionHeader_GetVirtual((void*)gFireTempleStoneStep1Col, &colHeader);
        } else {
            CollisionHeader_GetVirtual((void*)gFireTempleStoneStep2Col, &colHeader);
        }

        step->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &step->dyna.actor, colHeader);

        if (Flags_GetSwitch(play, (step->dyna.actor.params >> 8) & 0xFF)) {
            if ((step->dyna.actor.params & 0xFF) == 0) {
                step->dyna.actor.world.pos.y = step->dyna.actor.home.pos.y + (-20.0f);
            } else {
                step->dyna.actor.world.pos.y =
                    sYPosOffsets[(step->dyna.actor.params & 0xFF) - 1] + step->dyna.actor.home.pos.y;
            }
            BgHidanHamstep_SetupAction(step, 4);
        } else if ((step->dyna.actor.params & 0xFF) == 0) {
            BgHidanHamstep_SetupAction(step, 0);
        } else {
            BgHidanHamstep_SetupAction(step, 2);
        }

        step->dyna.actor.gravity = -1.2f;
        step->dyna.actor.minVelocityY = -12.0f;

        if ((step->dyna.actor.params & 0xFF) == 0) {
            //The only change is we dont kill self on fail here
            BgHidanHamstep_SpawnChildren(step, play);
        }
    }


  protected:
    static constexpr s32 ACTION_WAIT_FOR_HAMMER = 0;

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHidanHamstep* step = Typed();

        PackProperty(PROP_ACTION, PackedInt4(step->action), out);
        PackProperty(PROP_STATE, PackedInt4(step->unk_244), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanHamstep* step = Typed();

        switch (index) {
            case PROP_ACTION: {
                s32 action = data.Read<PackedInt4>().value();
                if (action != step->action && action >= 0 && action <= 4)
                    BgHidanHamstep_SetupAction(step, action);
                break;
            }
            case PROP_STATE:
                step->unk_244 = data.Read<PackedInt4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanHamstep* step = Typed();

        if (step->action == ACTION_WAIT_FOR_HAMMER) {
            if (step->collider.base.acFlags & AC_HIT) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }

            RegisterColliderBase(play, &step->collider.base, COLL_AC);
        }
    }
};

}

#endif
