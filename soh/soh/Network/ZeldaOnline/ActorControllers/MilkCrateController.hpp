#ifndef MILKCRATECONTROLLERH
#define MILKCRATECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Spot15_Rrbox/z_bg_spot15_rrbox.h"

void func_808B4084(BgSpot15Rrbox* crate, PlayState* play);
void func_808B40AC(BgSpot15Rrbox* crate, PlayState* play);
void func_808B4194(BgSpot15Rrbox* crate, PlayState* play);
void func_808B43D0(BgSpot15Rrbox* crate, PlayState* play);
void func_808B44CC(BgSpot15Rrbox* crate, PlayState* play);
}

namespace ZeldaOnline {

class MilkCrateController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgSpot15Rrbox* Typed() const {
        return reinterpret_cast<BgSpot15Rrbox*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using CrateAction = void (*)(BgSpot15Rrbox*, PlayState*);
    static const CrateAction* ActionTable(size_t* count) {
        static const CrateAction sTable[] = {
            func_808B40AC,
            func_808B4194,
            func_808B43D0,
            func_808B44CC,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const CrateAction* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++) {
            if (table[i] == Typed()->actionFunc)
                return static_cast<u8>(i);
        }
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
        PROP_HOME_POS,
        PROP_PUSH,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgSpot15Rrbox* crate = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(crate->unk_168), out);

        Vec3f h = crate->dyna.actor.home.pos;
        PackProperty(PROP_HOME_POS, ByteStream() << PackedFloat4(h.x) << PackedFloat4(h.y) << PackedFloat4(h.z), out);

        PackProperty(PROP_PUSH,
                     ByteStream() << PackedFloat4(crate->unk_174) << PackedFloat4(crate->unk_178)
                                  << PackedFloat4(crate->unk_17C),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgSpot15Rrbox* crate = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = static_cast<u8>(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const CrateAction* table = ActionTable(&count);
                if (id < count)
                    crate->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                crate->unk_168 = static_cast<s16>(data.Read<PackedInt2>().value());
                break;
            case PROP_HOME_POS:
                crate->dyna.actor.home.pos.x = data.Read<PackedFloat4>().value();
                crate->dyna.actor.home.pos.y = data.Read<PackedFloat4>().value();
                crate->dyna.actor.home.pos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_PUSH:
                crate->unk_174 = data.Read<PackedFloat4>().value();
                crate->unk_178 = data.Read<PackedFloat4>().value();
                crate->unk_17C = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgSpot15Rrbox* crate = Typed();

        if (fabsf(crate->dyna.unk_150) > 0.001f) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
            crate->dyna.unk_150 = 0.0f;
        }

        crate->dyna.actor.world.rot.y = crate->dyna.unk_158;
        crate->unk_16C = Math_SinS(crate->dyna.actor.world.rot.y);
        crate->unk_170 = Math_CosS(crate->dyna.actor.world.rot.y);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

} // namespace ZeldaOnline

#endif