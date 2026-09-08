#ifndef SCYTHETRAPCONTROLLERH
#define SCYTHETRAPCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Haka_Sgami/z_bg_haka_sgami.h"

void BgHakaSgami_SetupSpin(BgHakaSgami* sg, PlayState* play);
void BgHakaSgami_Spin(BgHakaSgami* sg, PlayState* play);
void BgHakaSgami_Draw(Actor* thisx, PlayState* play);

extern ColliderTrisInit* gBgHakaSgamiTrisInit;
}

namespace ZeldaOnline {

class ScytheTrapController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHakaSgami* Typed() const {
        return reinterpret_cast<BgHakaSgami*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using SgamiActionFunc = decltype(&BgHakaSgami_Spin);
    static const SgamiActionFunc* ActionTable(size_t* count) {
        static const SgamiActionFunc sTable[] = {
            BgHakaSgami_SetupSpin,
            BgHakaSgami_Spin,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const SgamiActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };


    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    }

    void UpdateLeader(PlayState* play) override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
        AbstractActorController::UpdateLeader(play);
    }

    void BuildCustomProperties(ByteStream& out) override {
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(Typed()->timer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHakaSgami* sg = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const SgamiActionFunc* table = ActionTable(&count);
                if (id < count)
                    sg->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                sg->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHakaSgami* sg = Typed();

        sg->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;

        if (sg->actor.draw == NULL) {
            if (!Object_IsLoaded(&play->objectCtx, sg->requiredObjBankIndex))
                return;
            sg->actor.objBankIndex = sg->requiredObjBankIndex;
            sg->actor.draw = BgHakaSgami_Draw;
        }


        Player* player = GET_PLAYER(play);
        if (player->stateFlags1 & (PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_ITEM_CS |
                                   PLAYER_STATE1_IN_CUTSCENE))
            return;

        SpinLocal(play);
    }

  private:
    void SpinLocal(PlayState* play) {
        static Vec3f sBlureVertices2[] = {
            { -20.0f, 50.0f, 130.0f },
            { -50.0f, 33.0f, 20.0f },
        };
        static Vec3f sBlureVertices1[] = {
            { 380.0f, 50.0f, 50.0f },
            { 310.0f, 33.0f, 0.0f },
        };

        BgHakaSgami* sg = Typed();
        s32 i;
        s32 j;
        Vec3f scytheVertices[3];
        f32 actorRotYSin = Math_SinS(sg->actor.shape.rot.y);
        f32 actorRotYCos = Math_CosS(sg->actor.shape.rot.y);
        s32 iterateCount = (sg->actor.params != 0) ? 4 : 2;
        ColliderTrisElementInit* elementInit;

        for (i = iterateCount - 2; i < iterateCount; i++) {
            elementInit = &gBgHakaSgamiTrisInit->elements[i];

            for (j = 0; j < 3; j++) {
                scytheVertices[j].x = sg->actor.world.pos.x + elementInit->dim.vtx[j].z * actorRotYSin +
                                      elementInit->dim.vtx[j].x * actorRotYCos;
                scytheVertices[j].y = sg->actor.world.pos.y + elementInit->dim.vtx[j].y;
                scytheVertices[j].z = sg->actor.world.pos.z + elementInit->dim.vtx[j].z * actorRotYCos -
                                      elementInit->dim.vtx[j].x * actorRotYSin;
            }

            Collider_SetTrisVertices(&sg->colliderScythe, i, &scytheVertices[0], &scytheVertices[1],
                                     &scytheVertices[2]);

            for (j = 0; j < 3; j++) {
                scytheVertices[j].x = (2 * sg->actor.world.pos.x) - scytheVertices[j].x;
                scytheVertices[j].z = (2 * sg->actor.world.pos.z) - scytheVertices[j].z;
            }

            Collider_SetTrisVertices(&sg->colliderScythe, (i + 2) % 4, &scytheVertices[0], &scytheVertices[1],
                                     &scytheVertices[2]);
        }

        if ((sg->unk_151 == 0) || play->actorCtx.lensActive) {
            scytheVertices[0].x = sg->actor.world.pos.x + sBlureVertices1[sg->actor.params].z * actorRotYSin +
                                  sBlureVertices1[sg->actor.params].x * actorRotYCos;
            scytheVertices[0].y = sg->actor.world.pos.y + sBlureVertices1[sg->actor.params].y;
            scytheVertices[0].z = sg->actor.world.pos.z + sBlureVertices1[sg->actor.params].z * actorRotYCos -
                                  sBlureVertices1[sg->actor.params].x * actorRotYSin;
            scytheVertices[1].x = sg->actor.world.pos.x + sBlureVertices2[sg->actor.params].z * actorRotYSin +
                                  sBlureVertices2[sg->actor.params].x * actorRotYCos;
            scytheVertices[1].y = sg->actor.world.pos.y + sBlureVertices2[sg->actor.params].y;
            scytheVertices[1].z = sg->actor.world.pos.z + sBlureVertices2[sg->actor.params].z * actorRotYCos -
                                  sBlureVertices2[sg->actor.params].x * actorRotYSin;
            EffectBlure_AddVertex((EffectBlure*)Effect_GetByIndex(sg->blureEffectIndex[0]), &scytheVertices[0], &scytheVertices[1]);

            for (j = 0; j < 2; j++) {
                scytheVertices[j].x = (2 * sg->actor.world.pos.x) - scytheVertices[j].x;
                scytheVertices[j].z = (2 * sg->actor.world.pos.z) - scytheVertices[j].z;
            }

            EffectBlure_AddVertex((EffectBlure*)Effect_GetByIndex(sg->blureEffectIndex[1]), &scytheVertices[0],
                                  &scytheVertices[1]);
        }

        CollisionCheck_SetAT(play, &play->colChkCtx, &sg->colliderScythe.base);
        CollisionCheck_SetOC(play, &play->colChkCtx, &sg->colliderScytheCenter.base);
    }
};

}

#endif
