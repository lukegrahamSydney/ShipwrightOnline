#ifndef PUSHBLOCKCONTROLLERH
#define PUSHBLOCKCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Oshihiki/z_obj_oshihiki.h"

void ObjOshihiki_OnScene(ObjOshihiki* block, PlayState* play);
void ObjOshihiki_OnActor(ObjOshihiki* block, PlayState* play);
void ObjOshihiki_Push(ObjOshihiki* block, PlayState* play);
void ObjOshihiki_Fall(ObjOshihiki* block, PlayState* play);
s32 ObjOshihiki_NoSwitchPress(ObjOshihiki*, DynaPolyActor* dyna, PlayState* play);
s32 ObjOshihiki_CheckFloor(ObjOshihiki*, PlayState* play);
}

namespace ZeldaOnline {

class PushBlockController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    ObjOshihiki* Typed() const {
        return reinterpret_cast<ObjOshihiki*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u16 STATE_FLAGS_SYNCED_MASK = 0x00FF;
    static constexpr u16 PUSHBLOCK_MOVE_UNDER_BIT = (1 << 8);

    using BlockAction = void (*)(ObjOshihiki*, PlayState*);
    static const BlockAction* ActionTable(size_t* count) {
        static const BlockAction sTable[] = {
            ObjOshihiki_OnScene,
            ObjOshihiki_OnActor,
            ObjOshihiki_Push,
            ObjOshihiki_Fall,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BlockAction* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++) {
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        }
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
        PROP_HOME_POS,
        PROP_STATE_FLAGS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        ObjOshihiki* block = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(block->timer), out);
        Vec3f h = block->dyna.actor.home.pos;
        PackProperty(PROP_HOME_POS, ByteStream() << PackedFloat4(h.x) << PackedFloat4(h.y) << PackedFloat4(h.z), out);
        PackProperty(PROP_STATE_FLAGS, PackedUInt1(block->stateFlags & STATE_FLAGS_SYNCED_MASK), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjOshihiki* block = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const BlockAction* table = ActionTable(&count);
                if (id < count)
                    block->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                block->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HOME_POS:
                block->dyna.actor.home.pos.x = data.Read<PackedFloat4>().value();
                block->dyna.actor.home.pos.y = data.Read<PackedFloat4>().value();
                block->dyna.actor.home.pos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_STATE_FLAGS: {
                u16 synced = (u16)(data.Read<PackedUInt1>().value());
                block->stateFlags = (block->stateFlags & PUSHBLOCK_MOVE_UNDER_BIT) | (synced & STATE_FLAGS_SYNCED_MASK);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        ObjOshihiki* block = Typed();

        if (fabsf(block->dyna.unk_150) > 0.001f) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
            block->dyna.unk_150 = 0.0f;
        }

        if (ObjOshihiki_CheckFloor(block, play)) {
            s32 bgId = block->floorBgIds[block->highestFloor];
            if (bgId != BGCHECK_SCENE) {
                DynaPolyActor* floor = DynaPoly_GetActor(&play->colCtx, bgId);
                if (floor != nullptr) {
                    DynaPolyActor_SetActorOnTop(floor);
                    DynaPolyActor_SetSwitchPressed(floor);
                }
            }
        }

        block->stateFlags |= PUSHBLOCK_MOVE_UNDER_BIT;
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

}

#endif
