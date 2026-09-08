#ifndef CUCCOCONTROLLERH
#define CUCCOCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Niw/z_en_niw.h"
#include "objects/object_niw/object_niw.h"

void EnNiw_ResetAction(EnNiw* niw, PlayState* play);
void func_80AB6324(EnNiw* niw, PlayState* play);
void func_80AB63A8(EnNiw* niw, PlayState* play);
void func_80AB6450(EnNiw* niw, PlayState* play);
void func_80AB6570(EnNiw* niw, PlayState* play);
void func_80AB6A38(EnNiw* niw, PlayState* play);
void func_80AB6BF8(EnNiw* niw, PlayState* play);
void func_80AB6D08(EnNiw* niw, PlayState* play);
void func_80AB6EB4(EnNiw* niw, PlayState* play);
void func_80AB6F04(EnNiw* niw, PlayState* play);
void func_80AB70A0(EnNiw* niw, PlayState* play);
void func_80AB70F8(EnNiw* niw, PlayState* play);
void func_80AB714C(EnNiw* niw, PlayState* play);
void func_80AB7204(EnNiw* niw, PlayState* play);
void func_80AB7290(EnNiw* niw, PlayState* play);
void func_80AB7328(EnNiw* niw, PlayState* play);
void func_80AB7420(EnNiw* niw, PlayState* play);
}

namespace ZeldaOnline {

class CuccoController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnNiw* Typed() const {
        return reinterpret_cast<EnNiw*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_RESET_ACTION = 0;

    using NiwActionFunc = void (*)(EnNiw*, PlayState*);
    static const NiwActionFunc* ActionTable(size_t* count) {
        static const NiwActionFunc sTable[] = {
            EnNiw_ResetAction,
            func_80AB6324,
            func_80AB63A8,
            func_80AB6450,
            func_80AB6570,
            func_80AB6A38,
            func_80AB6BF8,
            func_80AB6D08,
            func_80AB6EB4,
            func_80AB6F04,
            func_80AB70A0,
            func_80AB70F8,
            func_80AB714C,
            func_80AB7204,
            func_80AB7290,
            func_80AB7328,
            func_80AB7420,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const NiwActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsRampaging() const {
        EnNiw* niw = Typed();
        return niw->actionFunc == func_80AB70A0 || niw->actionFunc == func_80AB70F8 ||
               niw->actionFunc == func_80AB714C || niw->actionFunc == func_80AB7204;
    }

    bool HitWouldReact() const {
        EnNiw* niw = Typed();
        if (niw->unk_2A8 != 0 || niw->actor.params == 0xA || niw->actionFunc == func_80AB6450)
            return false;
        return (niw->collider.base.acFlags & AC_HIT) != 0;
    }

    u8 CurrentColliderRoles() const {
        EnNiw* niw = Typed();
        if (IsRampaging())
            return 0;
        u8 roles = 0;
        if (niw->actor.params != 0xA && niw->actor.params != 0xD && niw->actor.params != 0xE && niw->actor.params != 4)
            roles |= COLL_AC;
        if (niw->actionFunc != func_80AB6BF8 && niw->actionFunc != func_80AB6D08 && niw->actionFunc != func_80AB6324 &&
            niw->actionFunc != func_80AB63A8 && niw->actionFunc != func_80AB6450)
            roles |= COLL_OC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMERS_A,
        PROP_TIMERS_B,
        PROP_MOTION,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnNiw* niw = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(IsRampaging() ? ID_RESET_ACTION : CurrentActionIndex()), out);

        PackProperty(PROP_TIMERS_A,
                     ByteStream() << PackedInt2(niw->timer1) << PackedInt2(niw->timer2) << PackedInt2(niw->timer3)
                                  << PackedInt2(niw->timer4) << PackedInt2(niw->timer5),
                     out);
        PackProperty(PROP_TIMERS_B,
                     ByteStream() << PackedInt2(niw->timer6) << PackedInt2(niw->timer7) << PackedInt2(niw->timer8)
                                  << PackedInt2(niw->timer9),
                     out);
        {
            ByteStream motion;
            for (int i = 0; i < 10; i++)
                motion << PackedFloat4(niw->unk_26C[i]);
            PackProperty(PROP_MOTION, motion, out);
        }

        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(niw->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(0, &niw->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnNiw* niw = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const NiwActionFunc* table = ActionTable(&count);
                if (id < count)
                    niw->actionFunc = table[id];
                break;
            }
            case PROP_TIMERS_A:
                niw->timer1 = (s16)(data.Read<PackedInt2>().value());
                niw->timer2 = (s16)(data.Read<PackedInt2>().value());
                niw->timer3 = (s16)(data.Read<PackedInt2>().value());
                niw->timer4 = (s16)(data.Read<PackedInt2>().value());
                niw->timer5 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS_B:
                niw->timer6 = (s16)(data.Read<PackedInt2>().value());
                niw->timer7 = (s16)(data.Read<PackedInt2>().value());
                niw->timer8 = (s16)(data.Read<PackedInt2>().value());
                niw->timer9 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_MOTION:
                for (int i = 0; i < 10; i++)
                    niw->unk_26C[i] = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                niw->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM:
                data.Read<PackedUInt1>();
                ApplyAnimProperty(nullptr, &niw->skelAnime, LOCK_CUR_FRAME ? niw->skelAnime.curFrame : 0.0f, data);
                break;
            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void UpdatePuppet(PlayState* play) override {
        EnNiw* niw = Typed();

        UpdateAnimation(&niw->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        niw->collider.base.acFlags &= ~AC_HIT;

        niw->unk_2A8 = 0;
        niw->unk_296 = 0;

        niw->unk_2FC = 0.0f;
        niw->unk_300 = 0.0f;

        if (niw->actor.parent == &GET_PLAYER(play)->actor) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        if (!IsRampaging() && niw->actor.xzDistToPlayer < 200.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        if (Math3D_Vec3fDistSq(&niw->unk_2AC, &niw->actor.world.pos) > (500.0f * 500.0f)) {
            niw->unk_2AC = niw->actor.world.pos;
            niw->unk_2B8 = niw->actor.world.pos;
        }

        Actor_SetFocus(&niw->actor, niw->unk_304);

        {
            Vec3f rayOrigin = niw->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            niw->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &niw->actor.floorPoly, &floorBgId,
                                                                 &niw->actor, &rayOrigin);
            niw->actor.floorBgId = floorBgId;
        }

        if (niw->actor.parent == nullptr && !IsRampaging())
            Actor_OfferCarry(&niw->actor, play);

        if (m_roles != 0) {
            Collider_UpdateCylinder(&niw->actor, &niw->collider);
            RegisterColliderBase(play, &niw->collider.base, m_roles);
        }
    }

  private:
    u8 m_roles = COLL_AC | COLL_OC;
};

}

#endif
