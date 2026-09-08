#ifndef GNDFIREMEIROCONTROLLERH
#define GNDFIREMEIROCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Gnd_Firemeiro/z_bg_gnd_firemeiro.h"

void BgGndFiremeiro_Sink(BgGndFiremeiro*, PlayState* play);
void BgGndFiremeiro_Shake(BgGndFiremeiro*, PlayState* play);
void BgGndFiremeiro_Rise(BgGndFiremeiro*, PlayState* play);
}

namespace ZeldaOnline {

class GndFiremeiroController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgGndFiremeiro* Typed() const {
        return reinterpret_cast<BgGndFiremeiro*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return params == 0;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using FiremeiroActionFunc = void (*)(BgGndFiremeiro*, PlayState*);
    static const FiremeiroActionFunc* ActionTable(size_t* count) {
        static const FiremeiroActionFunc sTable[] = {
            BgGndFiremeiro_Sink,
            BgGndFiremeiro_Shake,
            BgGndFiremeiro_Rise,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const FiremeiroActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgGndFiremeiro* fm = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedUInt2(fm->timer), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgGndFiremeiro* fm = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const FiremeiroActionFunc* table = ActionTable(&count);
                if (id < count)
                    fm->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                fm->timer = (u16)(data.Read<PackedUInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgGndFiremeiro* fm = Typed();
        Player* player = GET_PLAYER(play);

        if (fm->actionFunc != BgGndFiremeiro_Rise)
            return;

        if (player->currentBoots != PLAYER_BOOTS_HOVER && DynaPolyActor_IsPlayerOnTop(&fm->dyna)) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
            UpdateLeader(play);
        }
    }
};

} // namespace ZeldaOnline

#endif
