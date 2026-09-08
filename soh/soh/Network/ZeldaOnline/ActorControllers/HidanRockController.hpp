#ifndef HIDANROCKCONTROLLERH
#define HIDANROCKCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Hidan_Rock/z_bg_hidan_rock.h"

void func_8088B268(BgHidanRock* rock, PlayState* play);
void func_8088B5F4(BgHidanRock* rock, PlayState* play);
void func_8088B634(BgHidanRock* rock, PlayState* play);
void func_8088B69C(BgHidanRock* rock, PlayState* play);
void func_8088B79C(BgHidanRock* rock, PlayState* play);
void func_8088B90C(BgHidanRock* rock, PlayState* play);
void func_8088B954(BgHidanRock* rock, PlayState* play);
void func_8088B990(BgHidanRock* rock, PlayState* play);
}

namespace ZeldaOnline {

class HidanRockController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgHidanRock* Typed() const {
        return reinterpret_cast<BgHidanRock*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using RockActionFunc = void (*)(BgHidanRock*, PlayState*);
    static const RockActionFunc* ActionTable(size_t* count) {
        static const RockActionFunc sTable[] = {
            func_8088B268, func_8088B5F4, func_8088B634, func_8088B69C,
            func_8088B79C, func_8088B90C, func_8088B954, func_8088B990,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const RockActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_FLAME,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgHidanRock* rock = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, ByteStream() << PackedInt2(rock->timer) << PackedUInt1(rock->unk_169), out);


        PackProperty(PROP_FLAME,
                     ByteStream() << PackedFloat4(rock->unk_16C) << PackedFloat4(rock->unk_170.x)
                                  << PackedFloat4(rock->unk_170.y) << PackedFloat4(rock->unk_170.z),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgHidanRock* rock = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const RockActionFunc* table = ActionTable(&count);
                if (id < count)
                    rock->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                rock->timer = (s16)(data.Read<PackedInt2>().value());
                rock->unk_169 = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FLAME:
                rock->unk_16C = data.Read<PackedFloat4>().value();
                rock->unk_170.x = data.Read<PackedFloat4>().value();
                rock->unk_170.y = data.Read<PackedFloat4>().value();
                rock->unk_170.z = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgHidanRock* rock = Typed();

        if (rock->actionFunc == func_8088B634 && DynaPolyActor_IsPlayerOnTop(&rock->dyna)) {
            ClaimLeadership(CLAIM_REASON_NOW);
        }

        if (rock->unk_16C > 0.0f) {
            rock->collider.dim.height = (s16)(sCylinderHeight * rock->unk_16C);
            Collider_UpdateCylinder(&rock->dyna.actor, &rock->collider);
            RegisterColliderBase(play, &rock->collider.base, COLL_AT);
        }

        if (rock->dyna.unk_150 != 0.0f) {
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
            rock->dyna.unk_150 = 0.0f;
        }
    }

  private:
    static constexpr s16 sCylinderHeight = 77;
};

}

#endif
