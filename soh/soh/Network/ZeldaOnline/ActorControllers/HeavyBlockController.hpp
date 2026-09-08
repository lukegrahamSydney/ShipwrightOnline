#ifndef HEAVYBLOCKCONTROLLERH
#define HEAVYBLOCKCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Heavy_Block/z_bg_heavy_block.h"
#include "objects/object_heavy_object/object_heavy_object.h"

void BgHeavyBlock_MovePiece(BgHeavyBlock*, PlayState* play);
void BgHeavyBlock_Wait(BgHeavyBlock*, PlayState* play);
void BgHeavyBlock_LiftedUp(BgHeavyBlock*, PlayState* play);
void BgHeavyBlock_Fly(BgHeavyBlock*, PlayState* play);
void BgHeavyBlock_Land(BgHeavyBlock*, PlayState* play);
void BgHeavyBlock_DoNothing(BgHeavyBlock*, PlayState* play);
}

namespace ZeldaOnline {

class HeavyBlockController : public AbstractActorController {

    static void DrawHeld(Actor* thisx, PlayState* play) {
        static Vec3f sZero = { 0.0f, 0.0f, 0.0f };
        static Vec3f sTop = { 0.0f, 400.0f, 0.0f };
        BgHeavyBlock* blk = (BgHeavyBlock*)thisx;
        Player* holder = (thisx->parent != nullptr) ? (Player*)thisx->parent : GET_PLAYER(play);

        GraphicsContext* __gfxCtx = gPlayState->state.gfxCtx;
        Gfx* dispRefs[4];
        (void)__gfxCtx;
        Graph_OpenDisps(dispRefs, gPlayState->state.gfxCtx, __FILE__, __LINE__);
        {
            if (blk->actionFunc == BgHeavyBlock_LiftedUp) {
                Matrix_SetTranslateRotateYXZ(holder->leftHandPos.x, holder->leftHandPos.y, holder->leftHandPos.z,
                                             &thisx->shape.rot);
                Matrix_Translate(-blk->unk_164.x, -blk->unk_164.y, -blk->unk_164.z, MTXMODE_APPLY);
            } else if (thisx->gravity == 0.0f && blk->actionFunc == BgHeavyBlock_Land) {
                Matrix_SetTranslateRotateYXZ(thisx->home.pos.x, thisx->home.pos.y, thisx->home.pos.z,
                                             &thisx->shape.rot);
                Matrix_Translate(-sTop.x, -sTop.y, -sTop.z, MTXMODE_APPLY);
            }

            Matrix_MultVec3f(&sZero, &thisx->world.pos);
            Matrix_MultVec3f(&sTop, &thisx->home.pos);
            Gfx_SetupDL_25Opa(play->state.gfxCtx);

            gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gHeavyBlockEntirePillarDL);
        }
        Graph_CloseDisps(dispRefs, gPlayState->state.gfxCtx, __FILE__, __LINE__);
    }

  public:
    using AbstractActorController::AbstractActorController;

    BgHeavyBlock* Typed() const {
        return reinterpret_cast<BgHeavyBlock*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        s16 type = params & 0xFF;
        return type != HEAVYBLOCK_BIG_PIECE && type != HEAVYBLOCK_SMALL_PIECE;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId != ACTOR_BG_HEAVY_BLOCK;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_WAIT = 1;

    using BlockActionFunc = void (*)(BgHeavyBlock*, PlayState*);
    static const BlockActionFunc* ActionTable(size_t* count) {
        static const BlockActionFunc sTable[] = {
            BgHeavyBlock_MovePiece, BgHeavyBlock_Wait, BgHeavyBlock_LiftedUp,
            BgHeavyBlock_Fly,       BgHeavyBlock_Land, BgHeavyBlock_DoNothing,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BlockActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };
    
    void OnActorInit() override {
        Typed()->dyna.actor.draw = DrawHeld;
    }

    void BuildCustomProperties(ByteStream& out) override {
        BgHeavyBlock* blk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, ByteStream() << PackedInt2(blk->timer) << PackedUInt2(blk->pieceFlags), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHeavyBlock* blk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const BlockActionFunc* table = ActionTable(&count);
                if (id < count)
                    blk->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                blk->timer = (s16)(data.Read<PackedInt2>().value());
                blk->pieceFlags = (u16)(data.Read<PackedUInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHeavyBlock* blk = Typed();

        if (blk->dyna.actor.parent == &GET_PLAYER(play)->actor) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        if (m_currentActionIndex == ID_WAIT && blk->dyna.actor.parent == nullptr &&
            blk->dyna.actor.xzDistToPlayer < 90.0f) {
            Actor_OfferCarry(&blk->dyna.actor, play);
        }
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

} // namespace ZeldaOnline

#endif
