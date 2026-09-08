#ifndef FLYINGTILECONTROLLERH
#define FLYINGTILECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Yukabyun/z_en_yukabyun.h"
#include "objects/object_yukabyun/object_yukabyun.h"
void func_80B43A94(EnYukabyun* tile, PlayState* play);
void func_80B43AD4(EnYukabyun* tile, PlayState* play);
void func_80B43B6C(EnYukabyun* tile, PlayState* play);
void EnYukabyun_Break(EnYukabyun* tile, PlayState* play);
}

namespace ZeldaOnline {

class FlyingTileController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnYukabyun* Typed() const {
        return reinterpret_cast<EnYukabyun*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using TileActionFunc = void (*)(EnYukabyun*, PlayState*);
    static const TileActionFunc* ActionTable(size_t* count) {
        static const TileActionFunc sTable[] = {
            func_80B43A94, func_80B43AD4, func_80B43B6C, EnYukabyun_Break,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TileActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionfunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsAirborne() const {
        EnYukabyun* tile = Typed();
        return tile->actionfunc != func_80B43A94 && tile->actionfunc != EnYukabyun_Break;
    }

    bool HitWouldReact() const {
        EnYukabyun* tile = Typed();
        return IsAirborne() && ((tile->collider.base.acFlags & AC_HIT) || (tile->collider.base.atFlags & AT_HIT));
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnYukabyun* tile = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);


        PackProperty(PROP_STATE, ByteStream() << PackedInt2(tile->unk_150) << PackedUInt1(tile->unk_152), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnYukabyun* tile = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TileActionFunc* table = ActionTable(&count);
                if (id < count)
                    tile->actionfunc = table[id];
                break;
            }
            case PROP_STATE:
                tile->unk_150 = (s16)(data.Read<PackedInt2>().value());
                tile->unk_152 = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnServerDestroy() override {
        EnYukabyun* tile = Typed();


        if (gPlayState != nullptr && tile->actionfunc != EnYukabyun_Break) {
            EffectSsHahen_SpawnBurst(gPlayState, &tile->actor.world.pos, 8.0f, 0, 1300, 300, 15, OBJECT_YUKABYUN, 10,
                                     (Gfx*)gFloorTileEnemyFragmentDL);
            SoundSource_PlaySfxAtFixedWorldPos(gPlayState, &tile->actor.world.pos, 30, NA_SE_EN_OCTAROCK_ROCK);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnYukabyun* tile = Typed();

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        tile->collider.base.acFlags &= ~AC_HIT;
        tile->collider.base.atFlags &= ~AT_HIT;
        tile->collider.base.ocFlags1 &= ~OC1_HIT;

        Actor_SetFocus(&tile->actor, 4.0f);

        if (IsAirborne()) {
            Collider_UpdateCylinder(&tile->actor, &tile->collider);
            RegisterColliderBase(play, &tile->collider.base, COLL_AT | COLL_AC | COLL_OC);
        }
    }
};

}

#endif
