#ifndef BROBCONTROLLERH
#define BROBCONTROLLERH

#include <cstring>
#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Brob/z_en_brob.h"
#include "objects/object_brob/object_brob.h"

void EnBrob_Idle(struct EnBrob* brob, PlayState* play);
void EnBrob_MoveUp(struct EnBrob* brob, PlayState* play);
void EnBrob_Wobble(struct EnBrob* brob, PlayState* play);
void EnBrob_Stunned(struct EnBrob* brob, PlayState* play);
void EnBrob_MoveDown(struct EnBrob* brob, PlayState* play);
void EnBrob_Shock(struct EnBrob* brob, PlayState* play);

void func_8003EBF8(PlayState* play, DynaCollisionContext* dyna, s32 bgId);
void func_8003EC50(PlayState* play, DynaCollisionContext* dyna, s32 bgId);
}

namespace ZeldaOnline {

class BrobController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBrob* Typed() const {
        return reinterpret_cast<EnBrob*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    static constexpr u32 DMGFLAG_STUN = 0x10;
    static constexpr u32 DMGFLAG_SHOCK = 0x100;

    using BrobActionFunc = void (*)(struct EnBrob*, PlayState*);
    static const BrobActionFunc* ActionTable(size_t* count) {
        static const BrobActionFunc sTable[] = {
            EnBrob_Idle,
            EnBrob_MoveUp,
            EnBrob_Wobble,
            EnBrob_Stunned,
            EnBrob_MoveDown,
            EnBrob_Shock,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BrobActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_RISE_FALL = 0;
    static constexpr u8 ANIM_WOBBLE = 1;
    static constexpr u8 ANIM_STUN = 2;
    static constexpr u8 ANIM_SHOCK = 3;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 4;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_RISE_FALL:
                return object_brob_Anim_001750;
            case ANIM_WOBBLE:
                return object_brob_Anim_001958;
            case ANIM_STUN:
                return object_brob_Anim_000290;
            case ANIM_SHOCK:
                return object_brob_Anim_001678;
            default:
                return nullptr;
        }
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        EnBrob* brob = Typed();
        if (brob->actionFunc == EnBrob_Idle || brob->actionFunc == EnBrob_MoveDown)
            return 0;

        u8 roles = COLL_OC;
        if (brob->actionFunc != EnBrob_Stunned) {
            roles |= COLL_AT;
            if (brob->actionFunc != EnBrob_MoveUp)
                roles |= COLL_AC;
        }
        return roles;
    }

    bool ShouldBeSolid() const {
        return Typed()->actionFunc == EnBrob_Idle;
    }

    bool HitWouldReact() const {
        EnBrob* brob = Typed();
        for (int i = 0; i < 2; i++) {
            ColliderCylinder* c = &brob->colliders[i];
            if (c->base.atFlags & AT_HIT)
                return true;
            if ((c->base.acFlags & AC_HIT) && c->info.acHitInfo != nullptr) {
                u32 flags = c->info.acHitInfo->toucher.dmgFlags;
                if (flags & (DMGFLAG_STUN | DMGFLAG_SHOCK))
                    return true;
            }
        }
        return false;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_MODEL_OFFSET,
        PROP_TIMER,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBrob* brob = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_MODEL_OFFSET, PackedInt2(brob->modelOffsetY), out);
        PackProperty(PROP_TIMER, PackedInt2(brob->timer), out);
        PackProperty(PROP_COLOR_FILTER,
                     ByteStream() << PackedUInt1(brob->dyna.actor.colorFilterTimer)
                                  << PackedUInt2(brob->dyna.actor.colorFilterParams),
                     out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(brob->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &brob->skelAnime), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBrob* brob = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BrobActionFunc* table = ActionTable(&count);
                if (id < count)
                    brob->actionFunc = table[id];
                break;
            }
            case PROP_MODEL_OFFSET:
                brob->modelOffsetY = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER:
                brob->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLOR_FILTER:
                brob->dyna.actor.colorFilterTimer = (u8)(data.Read<PackedUInt1>().value());
                brob->dyna.actor.colorFilterParams = (u16)(data.Read<PackedUInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                brob->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &brob->skelAnime, LOCK_CUR_FRAME ? brob->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnBrob* brob = Typed();

        UpdateAnimation(&brob->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        for (int i = 0; i < 2; i++) {
            brob->colliders[i].base.acFlags &= ~AC_HIT;
            brob->colliders[i].base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        }

        if (brob->actionFunc == EnBrob_Idle &&
            (DynaPolyActor_IsPlayerOnTop(&brob->dyna) || brob->dyna.actor.xzDistToPlayer < 300.0f) &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        bool solid = ShouldBeSolid();
        if (!m_solidityKnown || solid != m_isSolid) {
            if (solid)
                func_8003EC50(play, &play->colCtx.dyna, brob->dyna.bgId);
            else
                func_8003EBF8(play, &play->colCtx.dyna, brob->dyna.bgId);
            m_isSolid = solid;
            m_solidityKnown = true;
        }

        if (m_roles != 0) {
            RegisterColliderBase(play, &brob->colliders[0].base, m_roles);
            RegisterColliderBase(play, &brob->colliders[1].base, m_roles);
        }
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;

    bool m_isSolid = true;
    bool m_solidityKnown = false;
};

}

#endif
