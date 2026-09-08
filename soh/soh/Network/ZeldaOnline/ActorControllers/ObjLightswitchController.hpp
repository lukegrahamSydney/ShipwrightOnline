#ifndef OBJLIGHTSWITCHCONTROLLERH
#define OBJLIGHTSWITCHCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Obj_Lightswitch/z_obj_lightswitch.h"

void ObjLightswitch_Off(ObjLightswitch*, PlayState* play);
void ObjLightswitch_TurnOn(ObjLightswitch*, PlayState* play);
void ObjLightswitch_On(ObjLightswitch*, PlayState* play);
void ObjLightswitch_TurnOff(ObjLightswitch*, PlayState* play);
void ObjLightswitch_DisappearDelay(ObjLightswitch*, PlayState* play);
void ObjLightswitch_Disappear(ObjLightswitch*, PlayState* play);
}

namespace ZeldaOnline {

class ObjLightswitchController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    ObjLightswitch* Typed() const {
        return reinterpret_cast<ObjLightswitch*>(m_actor);
    }

    bool RidesBlock() const {
        return (Typed()->actor.params & 1) == 1;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using LightswitchActionFunc = void (*)(ObjLightswitch*, PlayState*);
    static const LightswitchActionFunc* ActionTable(size_t* count) {
        static const LightswitchActionFunc sTable[] = {
            ObjLightswitch_Off,           ObjLightswitch_TurnOn,     ObjLightswitch_On,
            ObjLightswitch_TurnOff,       ObjLightswitch_DisappearDelay, ObjLightswitch_Disappear,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const LightswitchActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_VISUAL,
    };

    void BuildCustomProperties(ByteStream& out) override {
        ObjLightswitch* ls = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(ls->timer) << PackedInt2(ls->toggleDelay)
                                  << PackedUInt1(ls->prevFrameACflags),
                     out);
        PackProperty(PROP_VISUAL,
                     ByteStream() << PackedInt2(ls->faceTextureIndex) << PackedInt2(ls->color[0])
                                  << PackedInt2(ls->color[1]) << PackedInt2(ls->color[2]) << PackedInt2(ls->alpha)
                                  << PackedInt2(ls->flameRingRot) << PackedInt2(ls->flameRingRotSpeed),
                     out);

        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        ObjLightswitch* ls = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const LightswitchActionFunc* table = ActionTable(&count);
                if (id < count)
                    ls->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                ls->timer = (s16)(data.Read<PackedInt2>().value());
                ls->toggleDelay = (s16)(data.Read<PackedInt2>().value());
                ls->prevFrameACflags = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_VISUAL:
                ls->faceTextureIndex = (s16)(data.Read<PackedInt2>().value());
                ls->color[0] = (s16)(data.Read<PackedInt2>().value());
                ls->color[1] = (s16)(data.Read<PackedInt2>().value());
                ls->color[2] = (s16)(data.Read<PackedInt2>().value());
                ls->alpha = (s16)(data.Read<PackedInt2>().value());
                ls->flameRingRot = (s16)(data.Read<PackedInt2>().value());
                ls->flameRingRotSpeed = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        ObjLightswitch* ls = Typed();


        if (ls->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        if (ls->actor.child != nullptr && RidesBlock()) {
            ls->actor.world.pos.x = ls->actor.child->world.pos.x;
            ls->actor.world.pos.y = ls->actor.child->world.pos.y + 60.0f;
            ls->actor.world.pos.z = ls->actor.child->world.pos.z;
            Actor_SetFocus(&ls->actor, 0.0f);
        }

        ls->prevFrameACflags = ls->collider.base.acFlags;
        ls->collider.base.acFlags &= ~AC_HIT;

        RegisterColliderBase(play, &ls->collider.base, COLL_OC | COLL_AC);
    }
};

} // namespace ZeldaOnline

#endif
