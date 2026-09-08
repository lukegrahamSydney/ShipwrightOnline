#ifndef MORIBIGSTCONTROLLERH
#define MORIBIGSTCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Mori_Bigst/z_bg_mori_bigst.h"

void BgMoriBigst_WaitForMoriTex(BgMoriBigst* bigst, PlayState* play);
void BgMoriBigst_StalfosFight(BgMoriBigst* bigst, PlayState* play);
void BgMoriBigst_Fall(BgMoriBigst* bigst, PlayState* play);
void BgMoriBigst_Landing(BgMoriBigst* bigst, PlayState* play);
void BgMoriBigst_StalfosPairFight(BgMoriBigst* bigst, PlayState* play);

void BgMoriBigst_Draw(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class MoriBigstController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgMoriBigst* Typed() const {
        return reinterpret_cast<BgMoriBigst*>(m_actor);
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override{
        return true;
    }
  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_NULL = 0xFE;

    using BigstActionFunc = void (*)(BgMoriBigst*, PlayState*);
    static const BigstActionFunc* ActionTable(size_t* count) {
        static const BigstActionFunc sTable[] = {
            BgMoriBigst_WaitForMoriTex, BgMoriBigst_StalfosFight,     BgMoriBigst_Fall,
            BgMoriBigst_Landing,        BgMoriBigst_StalfosPairFight,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        if (Typed()->actionFunc == nullptr) {
            return ID_NULL;
        }
        size_t count;
        const BigstActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WAIT_TIMER,
        PROP_STALFOS_COUNT,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgMoriBigst* bigst = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_WAIT_TIMER, PackedInt2(bigst->waitTimer), out);


        PackProperty(PROP_STALFOS_COUNT, PackedInt2(bigst->dyna.actor.home.rot.z), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgMoriBigst* bigst = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                if (id == ID_NULL) {
                    bigst->actionFunc = nullptr;
                    break;
                }
                size_t count;
                const BigstActionFunc* table = ActionTable(&count);
                if (id < count)
                    bigst->actionFunc = table[id];
                break;
            }
            case PROP_WAIT_TIMER:
                bigst->waitTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_STALFOS_COUNT:
                bigst->dyna.actor.home.rot.z = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdateLeader(PlayState* play) override {
        BgMoriBigst* bigst = Typed();
        s16 alive = 0;
        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].head; it != nullptr; it = it->next) {
            if (it->id == ACTOR_EN_TEST && it->parent == &bigst->dyna.actor && it->update != nullptr)
                alive++;
        }
        bigst->dyna.actor.home.rot.z = alive;

        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        BgMoriBigst* bigst = Typed();

        if (bigst->dyna.actor.draw == nullptr && bigst->moriTexObjIndex >= 0 &&
            Object_IsLoaded(&play->objectCtx, bigst->moriTexObjIndex)) {
            bigst->dyna.actor.draw = BgMoriBigst_Draw;
        }

        if (DynaPolyActor_IsPlayerAbove(&bigst->dyna)) {
            func_80074CE8(play, 6);
        }

        Actor_SetFocus(&bigst->dyna.actor, 50.0f);
    }

    bool m_stalfosSpawned = false;
};

} // namespace ZeldaOnline

#endif