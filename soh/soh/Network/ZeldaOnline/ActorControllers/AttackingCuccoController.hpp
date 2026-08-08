#ifndef ATTACKINGCUCCOCONTROLLERH
#define ATTACKINGCUCCOCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Attack_Niw/z_en_attack_niw.h"

void func_809B5670(EnAttackNiw* aniw, PlayState* play);
void func_809B59B0(EnAttackNiw* aniw, PlayState* play);
void func_809B5C18(EnAttackNiw* aniw, PlayState* play);
}

namespace ZeldaOnline {

class AttackingCuccoController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnAttackNiw* Typed() const {
        return reinterpret_cast<EnAttackNiw*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using AttackNiwActionFunc = void (*)(EnAttackNiw*, PlayState*);
    static const AttackNiwActionFunc* ActionTable(size_t* count) {
        static const AttackNiwActionFunc sTable[] = {
            func_809B5670,
            func_809B59B0,
            func_809B5C18,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const AttackNiwActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnAttackNiw* aniw = Typed();
        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(aniw->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(0, &aniw->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnAttackNiw* aniw = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const AttackNiwActionFunc* table = ActionTable(&count);
                if (id < count)
                    aniw->actionFunc = table[id];
                break;
            }
            case PROP_ANIM_CUR_FRAME:
                aniw->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM:
                data.Read<PackedUInt1>();
                ApplyAnimProperty(nullptr, &aniw->skelAnime, LOCK_CUR_FRAME ? aniw->skelAnime.curFrame : 0.0f, data);
                break;
            default: return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void UpdatePuppet(PlayState* play) override {
        UpdateAnimation(&Typed()->skelAnime, LOCK_CUR_FRAME);
    }
};

}

#endif
