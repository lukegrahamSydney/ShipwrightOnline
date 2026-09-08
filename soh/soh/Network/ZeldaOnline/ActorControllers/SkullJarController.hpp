#ifndef SKULLJARCONTROLLERH
#define SKULLJARCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Haka_Tubo/z_bg_haka_tubo.h"
#include "objects/gameplay_keep/gameplay_keep.h"

void BgHakaTubo_Idle(BgHakaTubo* pot, PlayState* play);
void BgHakaTubo_DropCollectible(BgHakaTubo* pot, PlayState* play);

extern s32* gBgHakaTuboPotsDestroyed;
}

namespace ZeldaOnline {

class SkullJarController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHakaTubo* Typed() const {
        return reinterpret_cast<BgHakaTubo*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr s16 SPINNING_POT_ROOM = 12;

    using HakaTuboActionFunc = decltype(&BgHakaTubo_Idle);
    static const HakaTuboActionFunc* ActionTable(size_t* count) {
        static const HakaTuboActionFunc sTable[] = {
            BgHakaTubo_Idle,
            BgHakaTubo_DropCollectible,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const HakaTuboActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_DROP_TIMER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_DROP_TIMER, PackedInt2(Typed()->dropTimer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHakaTubo* pot = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const HakaTuboActionFunc* table = ActionTable(&count);
                if (id < count)
                    pot->actionFunc = table[id];
                break;
            }
            case PROP_DROP_TIMER:
                pot->dropTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        BgHakaTubo* pot = Typed();

        if (!(changed & (1ull << PROP_ACTION)) || pot->actionFunc != BgHakaTubo_DropCollectible)
            return;

        SpawnBreakEffects();
        pot->dyna.actor.draw = NULL;
        Actor_SetScale(&pot->dyna.actor, 0.0f);

        GoLocal();
    }

    void UpdatePuppet(PlayState* play) override {
        BgHakaTubo* pot = Typed();

        pot->fireScroll++;


        if (pot->flamesCollider.base.atFlags & AT_HIT) {
            pot->flamesCollider.base.atFlags &= ~AT_HIT;
            func_8002F71C(play, &pot->dyna.actor, 5.0f, pot->dyna.actor.yawTowardsPlayer, 5.0f);
        }

        if (pot->potCollider.base.acFlags & AC_HIT) {
            Actor* ac = pot->potCollider.base.ac;
            if (ac != NULL && Actor_WorldDistXZToPoint(&pot->dyna.actor, &ac->world.pos) < 50.0f &&
                (ac->world.pos.y - pot->dyna.actor.world.pos.y) < 50.0f) {
                ClaimLeadership(CLAIM_REASON_NOW);
                UpdateLeader(play);
                return;
            }
            pot->potCollider.base.acFlags &= ~AC_HIT;
            return;
        }

        Collider_UpdateCylinder(&pot->dyna.actor, &pot->flamesCollider);
        Collider_UpdateCylinder(&pot->dyna.actor, &pot->potCollider);
        RegisterColliderBase(play, &pot->potCollider.base, COLL_AC);
        RegisterColliderBase(play, &pot->flamesCollider.base, COLL_AT | COLL_OC);
    }

  private:
    void SpawnBreakEffects() {
        static Vec3f sZeroVector = { 0.0f, 0.0f, 0.0f };

        BgHakaTubo* pot = Typed();
        Vec3f pos;

        pos.x = pot->dyna.actor.world.pos.x;
        pos.z = pot->dyna.actor.world.pos.z;
        pos.y = pot->dyna.actor.world.pos.y + 80.0f;

        EffectSsBomb2_SpawnLayered(gPlayState, &pos, &sZeroVector, &sZeroVector, 100, 45);
        EffectSsHahen_SpawnBurst(gPlayState, &pos, 20.0f, 0, 350, 100, 50, OBJECT_HAKA_OBJECTS, 40, (Gfx*)gEffFragments2DL);
    }
};

} // namespace ZeldaOnline

#endif