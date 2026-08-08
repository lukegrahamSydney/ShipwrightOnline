#ifndef BEAMOSCONTROLLERH
#define BEAMOSCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Vm/z_en_vm.h"

void EnVm_Wait(EnVm* vm, PlayState* play);
void EnVm_Attack(EnVm* vm, PlayState* play);
void EnVm_Stun(EnVm* vm, PlayState* play);
void EnVm_Die(EnVm* vm, PlayState* play);
void EnVm_SetupDie(EnVm* thisx);
void func_80033480(PlayState* play, Vec3f* pos, f32 arg2, s32 arg3, s16 arg4, s16 arg5, u8 arg6);
}

namespace ZeldaOnline {

class BeamosController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnVm* Typed() const {
        return reinterpret_cast<EnVm*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_WAIT = 0;
    static constexpr u8 ID_ATTACK = 1;
    static constexpr u8 ID_DIE = 3;

    static constexpr s16 BEAM_STATE_FIRING = 3;

    using VmActionFunc = void (*)(EnVm*, PlayState*);
    static const VmActionFunc* ActionTable(size_t* count) {
        static const VmActionFunc sTable[] = {
            EnVm_Wait,
            EnVm_Attack,
            EnVm_Stun,
            EnVm_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const VmActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool HitWouldReact(PlayState* play) const {
        EnVm* vm = Typed();
        if (vm->actor.colChkInfo.health == 0)
            return false;
        if (Actor_GetCollidedExplosive(play, &vm->colliderCylinder.base) != nullptr)
            return true;
        return (vm->colliderQuad2.base.acFlags & AC_HIT) && vm->unk_21C != 2;
    }

    void ClearHitFlags() {
        EnVm* vm = Typed();
        vm->colliderCylinder.base.acFlags &= ~AC_HIT;
        vm->colliderQuad2.base.acFlags &= ~AC_HIT;
        vm->colliderQuad1.base.atFlags &= ~AT_HIT;
    }

    u8 CurrentCylinderRoles() const {
        EnVm* vm = Typed();
        u8 roles = COLL_OC;
        if (vm->actor.colorFilterTimer == 0 && vm->actor.colChkInfo.health != 0)
            roles |= COLL_AC;
        return roles;
    }
    u8 CurrentBeamRoles() const {
        return (Typed()->unk_260 == BEAM_STATE_FIRING) ? COLL_AT : 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_PHASE,
        PROP_BEAM_AIM,
        PROP_BEAM_GEOM,
        PROP_BEAM_SPEED,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnVm* vm = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(vm->actor.colChkInfo.health), out);

        PackProperty(PROP_PHASE,
                     ByteStream() << PackedInt4(vm->timer) << PackedInt4(vm->unk_21C) << PackedInt2(vm->unk_25E)
                                  << PackedInt2(vm->unk_260),
                     out);

        PackProperty(PROP_BEAM_AIM,
                     ByteStream() << PackedInt2(vm->headRotY) << PackedInt2(vm->beamRot.x) << PackedInt2(vm->beamRot.y)
                                  << PackedInt2(vm->beamRot.z),
                     out);
        PackProperty(PROP_BEAM_GEOM,
                     ByteStream() << PackedFloat4(vm->beamPos1.x) << PackedFloat4(vm->beamPos1.y)
                                  << PackedFloat4(vm->beamPos1.z) << PackedFloat4(vm->beamPos3.x)
                                  << PackedFloat4(vm->beamPos3.y) << PackedFloat4(vm->beamPos3.z)
                                  << PackedFloat4(vm->beamScale.x) << PackedFloat4(vm->beamScale.y)
                                  << PackedFloat4(vm->beamScale.z),
                     out);
        PackProperty(PROP_BEAM_SPEED, PackedFloat4(vm->beamSpeed), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentCylinderRoles()) << PackedUInt1(CurrentBeamRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(vm->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(0, &vm->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnVm* vm = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const VmActionFunc* table = ActionTable(&count);
                if (id < count)
                    vm->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                vm->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PHASE:
                vm->timer = data.Read<PackedInt4>().value();
                vm->unk_21C = data.Read<PackedInt4>().value();
                vm->unk_25E = (s16)(data.Read<PackedInt2>().value());
                vm->unk_260 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BEAM_AIM:
                vm->headRotY = (s16)(data.Read<PackedInt2>().value());
                vm->beamRot.x = (s16)(data.Read<PackedInt2>().value());
                vm->beamRot.y = (s16)(data.Read<PackedInt2>().value());
                vm->beamRot.z = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BEAM_GEOM:
                vm->beamPos1.x = data.Read<PackedFloat4>().value();
                vm->beamPos1.y = data.Read<PackedFloat4>().value();
                vm->beamPos1.z = data.Read<PackedFloat4>().value();
                vm->beamPos3.x = data.Read<PackedFloat4>().value();
                vm->beamPos3.y = data.Read<PackedFloat4>().value();
                vm->beamPos3.z = data.Read<PackedFloat4>().value();
                vm->beamScale.x = data.Read<PackedFloat4>().value();
                vm->beamScale.y = data.Read<PackedFloat4>().value();
                vm->beamScale.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_BEAM_SPEED:
                vm->beamSpeed = data.Read<PackedFloat4>().value();
                break;

            case PROP_COLL_ROLES:
                m_cylinderRoles = (u8)(data.Read<PackedUInt1>().value());
                m_beamRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                vm->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM:
                data.Read<PackedUInt1>();
                ApplyAnimProperty(nullptr, &vm->skelAnime, LOCK_CUR_FRAME ? vm->skelAnime.curFrame : 0.0f, data);
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnVm_Die) {
            EnVm_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnVm* vm = Typed();

        UpdateAnimation(&vm->skelAnime, LOCK_CUR_FRAME);

        if (m_currentActionIndex == ID_ATTACK && vm->skelAnime.curFrame >= vm->skelAnime.endFrame)
            vm->skelAnime.curFrame = vm->skelAnime.startFrame;

        if (HitWouldReact(play)) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }
        ClearHitFlags();

        if (m_currentActionIndex == ID_WAIT && vm->actor.xzDistToPlayer < vm->beamSightRange && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        if (vm->unk_260 == 4) {
            EffectSsDeadDs_SpawnStationary(play, &vm->beamPos3, 20, -1, 255, 20);
            func_80033480(play, &vm->beamPos3, 6.0f, 1, 120, 20, 1);
        }

        vm->beamTexScroll += 0xC;

        vm->actor.focus.pos = vm->actor.world.pos;
        vm->actor.focus.pos.y += (6500.0f + vm->actor.shape.yOffset) * vm->actor.scale.y;

        Collider_UpdateCylinder(&vm->actor, &vm->colliderCylinder);
        RegisterColliderBase(play, &vm->colliderCylinder.base, m_cylinderRoles);
        RegisterColliderBase(play, &vm->colliderQuad2.base, COLL_AC);
        if (m_beamRoles != 0)
            RegisterColliderBase(play, &vm->colliderQuad1.base, m_beamRoles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_cylinderRoles = COLL_OC | COLL_AC;
    u8 m_beamRoles = 0;
};

}

#endif
