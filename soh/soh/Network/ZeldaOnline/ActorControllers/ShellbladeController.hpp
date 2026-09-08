#ifndef SHELLBLADECONTROLLERH
#define SHELLBLADECONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ny/z_en_ny.h"

void EnNy_Move(EnNy* ny, PlayState* play);
void EnNy_Die(EnNy* ny, PlayState* play);
void EnNy_TurnToStone(EnNy* ny, PlayState* play);
void EnNy_SetupDie(EnNy* ny, PlayState* play);
void func_80ABCE50(EnNy* ny, PlayState* play);
void func_80ABCE90(EnNy* ny, PlayState* play);
void func_80ABCEEC(EnNy* ny, PlayState* play);
void func_80ABD11C(EnNy* ny, PlayState* play);

void func_80ABD3B8(EnNy* ny, f32 a, f32 b);
void EnNy_UpdateDeath(Actor* thisx, PlayState* play);
void EnNy_DrawDeathEffect(Actor* thisx, PlayState* play);
}

namespace ZeldaOnline {

class ShellbladeController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnNy* Typed() const {
        return reinterpret_cast<EnNy*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using NyActionFunc = decltype(&EnNy_Move);
    static const NyActionFunc* ActionTable(size_t* count) {
        static const NyActionFunc sTable[] = {
            EnNy_Move,     EnNy_Die,      EnNy_TurnToStone, EnNy_SetupDie,
            func_80ABCE50, func_80ABCE90, func_80ABCEEC,    func_80ABD11C,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    
    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    }


    u8 CurrentActionIndex() const {
        size_t count;
        const NyActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_MOTION,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnNy* ny = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);


        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(ny->timer) << PackedInt2(ny->unk_1CA) << PackedInt2(ny->hitPlayer)
                                  << PackedUInt2(ny->unk_1CE) << PackedUInt1(ny->unk_1D0) << PackedInt1(ny->unk_1D1)
                                  << PackedInt4(ny->stoneTimer) << PackedFloat4(ny->unk_1E0)
                                  << PackedUInt1(ny->actor.colChkInfo.health),
                     out);
        PackProperty(PROP_MOTION,
                     ByteStream() << PackedFloat4(ny->unk_1E4) << PackedFloat4(ny->unk_1E8) << PackedFloat4(ny->unk_1EC)
                                  << PackedFloat4(ny->unk_1F4),
                     out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);

    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnNy* ny = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const NyActionFunc* table = ActionTable(&count);
                if (id < count)
                    ny->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                ny->timer = (s16)(data.Read<PackedInt2>().value());
                ny->unk_1CA = (s16)(data.Read<PackedInt2>().value());
                ny->hitPlayer = (s16)(data.Read<PackedInt2>().value());
                ny->unk_1CE = (u16)(data.Read<PackedUInt2>().value());
                ny->unk_1D0 = (u8)(data.Read<PackedUInt1>().value());
                ny->unk_1D1 = (s8)(data.Read<PackedInt1>().value());
                ny->stoneTimer = data.Read<PackedInt4>().value();
                ny->unk_1E0 = data.Read<PackedFloat4>().value();
                ny->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_MOTION:
                ny->unk_1E4 = data.Read<PackedFloat4>().value();
                ny->unk_1E8 = data.Read<PackedFloat4>().value();
                ny->unk_1EC = data.Read<PackedFloat4>().value();
                ny->unk_1F4 = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnNy* ny = Typed();

        if (ny->actionFunc == EnNy_SetupDie || ny->actionFunc == EnNy_Die)
        {
            EnNy_SetupDie(ny, gPlayState);
            GoLocal();
        }
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
        ReinstallUpdate();
    }

    void UpdatePuppet(PlayState* play) override {
        EnNy* ny = Typed();

        Actor_SetFocus(&ny->actor, 0.0f);
        Actor_SetScale(&ny->actor, 0.01f);

        f32 blend = ny->unk_1E0 - 0.25f;
        ny->collider.elements[0].dim.scale = 1.33f * blend + 1.0f;

        f32 hover = (24.0f * blend) + 12.0f;
        func_80ABD3B8(ny, hover + 10.0f, hover - 10.0f);
        ny->unk_1F0 = hover;

        if (ny->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        ny->collider.base.acFlags &= ~AC_HIT;
        ny->collider.base.atFlags &= ~AT_HIT;

        if (ny->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        u8 roles = COLL_AC | COLL_OC;
        if (ny->unk_1E0 > 0.25f)
            roles |= COLL_AT;

        RegisterColliderBase(play, &ny->collider.base, roles);
    }
};

} // namespace ZeldaOnline

#endif