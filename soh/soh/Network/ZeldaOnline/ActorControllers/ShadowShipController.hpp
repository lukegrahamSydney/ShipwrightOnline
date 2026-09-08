#ifndef SHADOWSHIPCONTROLLERH
#define SHADOWSHIPCONTROLLERH

#include "../AbstractActorController.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Haka_Ship/z_bg_haka_ship.h"
#include "objects/object_haka_objects/object_haka_objects.h"

void BgHakaShip_ChildUpdatePosition(BgHakaShip* ship, PlayState* play);
void BgHakaShip_WaitForSong(BgHakaShip* ship, PlayState* play);
void BgHakaShip_CutsceneStationary(BgHakaShip* ship, PlayState* play);
void BgHakaShip_Move(BgHakaShip* ship, PlayState* play);
void BgHakaShip_SetupCrash(BgHakaShip* ship, PlayState* play);
void BgHakaShip_CrashShake(BgHakaShip* ship, PlayState* play);
void BgHakaShip_CrashFall(BgHakaShip* ship, PlayState* play);
}

namespace ZeldaOnline {

class ShadowShipController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHakaShip* Typed() const {
        return reinterpret_cast<BgHakaShip*>(m_actor);
    }

    
    static void RegisterHooks(s16 actorID, bool enabled) {
        COND_ID_HOOK(ShouldActorInit, actorID, enabled, [&](void* actorRef, bool*) {
            Actor* actor = (Actor*)actorRef;

            AbstractActorController::InstallCustomInit(actor, ShadowShipController::ReplacementInit);
        });
    }

    static void ReplacementInit(Actor* thisx, PlayState* play) {
        BgHakaShip* ship = reinterpret_cast<BgHakaShip*>(thisx);
        CollisionHeader* colHeader = NULL;

        Actor_SetScale(thisx, 0.1f);
        DynaPolyActor_Init(&ship->dyna, 1);
        ship->switchFlag = (thisx->params >> 8) & 0xFF;
        ship->dyna.actor.params &= 0xFF;

        if (ship->dyna.actor.params == 0) {
            CollisionHeader_GetVirtual((void*) & object_haka_objects_Col_00E408, &colHeader);
            ship->counter = 8;
            ship->actionFunc = BgHakaShip_WaitForSong;
        } else {
            CollisionHeader_GetVirtual((void*)&object_haka_objects_Col_00ED7C, &colHeader);
            ship->actionFunc = BgHakaShip_ChildUpdatePosition;
        }

        ship->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &ship->dyna.actor, colHeader);
        ship->dyna.actor.world.rot.y = ship->dyna.actor.shape.rot.y - 0x4000;
        ship->yOffset = 0;

        if (ship->dyna.actor.params == 0) {
            Actor_SpawnAsChild(&play->actorCtx, &ship->dyna.actor, play, ACTOR_BG_HAKA_SHIP,
                               ship->dyna.actor.world.pos.x + -10.0f, ship->dyna.actor.world.pos.y + 82.0f,
                               ship->dyna.actor.world.pos.z, 0, 0, 0, 1);
        }
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using HakaShipActionFunc = decltype(&BgHakaShip_Move);
    static const HakaShipActionFunc* ActionTable(size_t* count) {
        static const HakaShipActionFunc sTable[] = {
            BgHakaShip_ChildUpdatePosition, BgHakaShip_WaitForSong, BgHakaShip_CutsceneStationary,
            BgHakaShip_Move,                BgHakaShip_SetupCrash,  BgHakaShip_CrashShake,
            BgHakaShip_CrashFall,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HakaShipActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsHull() const {
        return Typed()->dyna.actor.params == 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHakaShip* ship = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, ByteStream() << PackedUInt1(ship->counter) << PackedInt2(ship->yOffset), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHakaShip* ship = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const HakaShipActionFunc* table = ActionTable(&count);
                if (id < count)
                    ship->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                ship->counter = (u8)(data.Read<PackedUInt1>().value());
                ship->yOffset = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BgHakaShip* ship = Typed();

        if ((changed & (1ull << PROP_ACTION)) && ship->actionFunc == BgHakaShip_CutsceneStationary)
            OnePointCutscene_Init(gPlayState, 3390, 999, &ship->dyna.actor, MAIN_CAM);
    }

    void UpdatePuppet(PlayState* play) override {
        BgHakaShip* ship = Typed();

        if (!IsHull()) {
            BgHakaShip_ChildUpdatePosition(ship, play);
            return;
        }


        if (ship->actionFunc == BgHakaShip_CrashFall &&
            (ship->dyna.actor.home.pos.y - ship->dyna.actor.world.pos.y) > 500.0f &&
            DynaPolyActor_IsPlayerOnTop(&ship->dyna))
            Play_TriggerVoidOut(play);
    }
};

}

#endif
