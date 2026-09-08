#ifndef JYACOBRACONTROLLERH
#define JYACOBRACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Jya_Cobra/z_bg_jya_cobra.h"

void func_80896918(BgJyaCobra*, PlayState* play);
void func_80896950(BgJyaCobra*, PlayState* play);
void func_80896ABC(BgJyaCobra*, PlayState* play);
void func_80895A70(BgJyaCobra*);
void func_80895C74(BgJyaCobra*, PlayState* play);
void BgJyaCobra_UpdateShadowFromSide(BgJyaCobra*);
}

namespace ZeldaOnline {

class JyaCobraController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgJyaCobra* Typed() const {
        return reinterpret_cast<BgJyaCobra*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return (params & 3) == 0;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using CobraActionFunc = void (*)(BgJyaCobra*, PlayState*);
    static const CobraActionFunc* ActionTable(size_t* count) {
        static const CobraActionFunc sTable[] = {
            func_80896950,
            func_80896ABC,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const CobraActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgJyaCobra* cobra = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(cobra->unk_168) << PackedInt2(cobra->unk_16A)
                                  << PackedInt2(cobra->unk_16C) << PackedInt2(cobra->unk_16E)
                                  << PackedInt2(cobra->unk_170) << PackedUInt1(cobra->unk_172),
                     out);

        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgJyaCobra* cobra = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const CobraActionFunc* table = ActionTable(&count);
                if (id < count)
                    cobra->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                cobra->unk_168 = (s16)(data.Read<PackedInt2>().value());
                cobra->unk_16A = (s16)(data.Read<PackedInt2>().value());
                cobra->unk_16C = (s16)(data.Read<PackedInt2>().value());
                cobra->unk_16E = (s16)(data.Read<PackedInt2>().value());
                cobra->unk_170 = (s16)(data.Read<PackedInt2>().value());
                cobra->unk_172 = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgJyaCobra* cobra = Typed();
        Player* player = GET_PLAYER(play);

        if (cobra->actionFunc == func_80896950 && cobra->dyna.unk_150 > 0.001f) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
            UpdateLeader(play);
            return;
        }

        if (fabsf(cobra->dyna.unk_150) > 0.001f) {
            cobra->dyna.unk_150 = 0.0f;
            player->stateFlags2 &= ~PLAYER_STATE2_MOVING_DYNAPOLY;
        }

        func_80895C74(cobra, play);
        func_80895A70(cobra);
        BgJyaCobra_UpdateShadowFromSide(cobra);

        if (cobra->actionFunc == func_80896ABC)
            func_8002F974(&cobra->dyna.actor, NA_SE_EV_ROCK_SLIDE - SFX_FLAG);
    }
};

} // namespace ZeldaOnline

#endif
