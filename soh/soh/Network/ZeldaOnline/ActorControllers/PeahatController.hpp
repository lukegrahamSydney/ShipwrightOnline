#ifndef PEAHATCONTROLLERH
#define PEAHATCONTROLLERH

#include <cstring>
#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Peehat/z_en_peehat.h"
#include "objects/object_peehat/object_peehat.h"

void EnPeehat_Ground_StateGround(EnPeehat* ph, PlayState* play);
void EnPeehat_Ground_StateRise(EnPeehat* ph, PlayState* play);
void EnPeehat_Ground_StateSeekPlayer(EnPeehat* ph, PlayState* play);
void EnPeehat_Ground_StateHover(EnPeehat* ph, PlayState* play);
void EnPeehat_Ground_StateReturnHome(EnPeehat* ph, PlayState* play);
void EnPeehat_Ground_StateLanding(EnPeehat* ph, PlayState* play);
void EnPeehat_Flying_StateGrounded(EnPeehat* ph, PlayState* play);
void EnPeehat_Flying_StateRise(EnPeehat* ph, PlayState* play);
void EnPeehat_Flying_StateFly(EnPeehat* ph, PlayState* play);
void EnPeehat_Flying_StateLanding(EnPeehat* ph, PlayState* play);
void EnPeehat_Larva_StateSeekPlayer(EnPeehat* ph, PlayState* play);
void EnPeehat_StateAttackRecoil(EnPeehat* ph, PlayState* play);
void EnPeehat_StateBoomerangStunned(EnPeehat* ph, PlayState* play);
void EnPeehat_Adult_StateDie(EnPeehat* ph, PlayState* play);
void EnPeehat_StateExplode(EnPeehat* ph, PlayState* play);

void EnPeehat_SpawnDust(PlayState* play, EnPeehat* ph, Vec3f* pos, f32 arg3, s32 arg4, f32 arg5, f32 arg6);
}

namespace ZeldaOnline {
typedef enum {
     PEAHAT_STATE_DYING,
     PEAHAT_STATE_EXPLODE,
     PEAHAT_STATE_3 = 3,
     PEAHAT_STATE_4,
     PEAHAT_STATE_FLY,
     PEAHAT_STATE_ATTACK_RECOIL = 7,
     PEAHAT_STATE_8,
     PEAHAT_STATE_9,
     PEAHAT_STATE_LANDING,
     PEAHAT_STATE_RETURN_HOME = 12,
     PEAHAT_STATE_STUNNED,
     PEAHAT_STATE_SEEK_PLAYER,
     PEAHAT_STATE_15
} PeahatState;

class PeahatController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnPeehat* Typed() const {
        return reinterpret_cast<EnPeehat*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_DIE = 13;
    static constexpr u8 ID_EXPLODE = 14;

    bool IsAdult() const {
        return Typed()->actor.params <= PEAHAT_TYPE_FLYING;
    }
    bool IsGrounded() const {
        return Typed()->actor.params < PEAHAT_TYPE_FLYING;
    }
    bool IsFlying() const {
        return Typed()->actor.params == PEAHAT_TYPE_FLYING;
    }

    static const EnPeehatActionFunc* ActionTable(size_t* count) {
        static const EnPeehatActionFunc sTable[] = {
            EnPeehat_Ground_StateGround,
            EnPeehat_Ground_StateRise,
            EnPeehat_Ground_StateSeekPlayer,
            EnPeehat_Ground_StateHover,
            EnPeehat_Ground_StateReturnHome,
            EnPeehat_Ground_StateLanding,
            EnPeehat_Flying_StateGrounded,
            EnPeehat_Flying_StateRise,
            EnPeehat_Flying_StateFly,
            EnPeehat_Flying_StateLanding,
            EnPeehat_Larva_StateSeekPlayer,
            EnPeehat_StateAttackRecoil,
            EnPeehat_StateBoomerangStunned,
            EnPeehat_Adult_StateDie,
            EnPeehat_StateExplode,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const EnPeehatActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_RISING = 0;
    static constexpr u8 ANIM_FLYING = 1;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 2;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_RISING:
                return gPeehatRisingAnim;
            case ANIM_FLYING:
                return gPeehatFlyingAnim;
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

    bool IsBladeActiveState() const {
        s32 s = Typed()->state;
        return s == PEAHAT_STATE_15 || s == PEAHAT_STATE_SEEK_PLAYER || s == PEAHAT_STATE_FLY ||
               s == PEAHAT_STATE_RETURN_HOME || s == PEAHAT_STATE_EXPLODE;
    }

    bool HitWouldReact() const {
        EnPeehat* ph = Typed();
        if ((ph->colCylinder.base.acFlags & AC_BOUNCED) || (ph->colQuad.base.acFlags & AC_BOUNCED))
            return false;
        return (ph->colJntSph.base.acFlags & AC_HIT) != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_HEALTH,
        PROP_TIMERS,
        PROP_BLADE,
        PROP_WOBBLE,
        PROP_MISC,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnPeehat* ph = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, PackedInt4(ph->state), out);
        PackProperty(PROP_HEALTH, PackedUInt1(ph->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(ph->riseDelayTimer) << PackedInt2(ph->seekPlayerTimer)
                                  << PackedInt2(ph->animTimer) << PackedInt2(ph->unk_2F4) << PackedInt2(ph->unk_2FA),
                     out);
        PackProperty(PROP_BLADE, ByteStream() << PackedInt2(ph->bladeRot) << PackedInt2(ph->bladeRotVel), out);
        PackProperty(PROP_WOBBLE,
                     ByteStream() << PackedFloat4(ph->jiggleRot) << PackedFloat4(ph->jiggleRotInc)
                                  << PackedFloat4(ph->scaleShift),
                     out);
        PackProperty(PROP_MISC,
                     ByteStream() << PackedInt4(ph->unk_2D4) << PackedFloat4(ph->unk_2E0)
                                  << PackedInt4(ph->isStateDieFirstUpdate),
                     out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &ph->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);

    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnPeehat* ph = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const EnPeehatActionFunc* table = ActionTable(&count);
                if (id < count)
                    ph->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                ph->state = data.Read<PackedInt4>().value();
                break;
            case PROP_HEALTH:
                ph->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                ph->riseDelayTimer = (s16)(data.Read<PackedInt2>().value());
                ph->seekPlayerTimer = (s16)(data.Read<PackedInt2>().value());
                ph->animTimer = (s16)(data.Read<PackedInt2>().value());
                ph->unk_2F4 = (s16)(data.Read<PackedInt2>().value());
                ph->unk_2FA = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_BLADE:
                ph->bladeRot = (s16)(data.Read<PackedInt2>().value());
                ph->bladeRotVel = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_WOBBLE:
                ph->jiggleRot = data.Read<PackedFloat4>().value();
                ph->jiggleRotInc = data.Read<PackedFloat4>().value();
                ph->scaleShift = data.Read<PackedFloat4>().value();
                break;
            case PROP_MISC:
                ph->unk_2D4 = data.Read<PackedInt4>().value();
                ph->unk_2E0 = data.Read<PackedFloat4>().value();
                ph->isStateDieFirstUpdate = data.Read<PackedInt4>().value();
                break;

            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &ph->skelAnime, LOCK_CUR_FRAME ? ph->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnPeehat* ph = Typed();

        UpdateAnimation(&ph->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        ph->colCylinder.base.acFlags &= ~(AC_HIT | AC_BOUNCED);
        ph->colQuad.base.acFlags &= ~(AC_HIT | AC_BOUNCED);
        ph->colJntSph.base.acFlags &= ~AC_HIT;
        ph->colQuad.base.atFlags &= ~AT_HIT;

        if (m_currentActionIndex != ID_DIE && m_currentActionIndex != ID_EXPLODE &&
            ph->actor.xzDistToPlayer < ph->xzDistToRise && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        ph->jiggleRot += ph->jiggleRotInc;

        if (IsGrounded()) {
            ph->actor.focus.pos.x = ph->colJntSph.elements[0].dim.worldSphere.center.x;
            ph->actor.focus.pos.y = ph->colJntSph.elements[0].dim.worldSphere.center.y;
            ph->actor.focus.pos.z = ph->colJntSph.elements[0].dim.worldSphere.center.z;
            Math_SmoothStepToS(&ph->actor.shape.rot.x, (ph->state == PEAHAT_STATE_SEEK_PLAYER) ? 6000 : 0, 1, 300, 0);
        } else {
            ph->actor.focus.pos = ph->actor.world.pos;
        }

        Collider_UpdateCylinder(&ph->actor, &ph->colCylinder);

        if (ph->actor.colChkInfo.health > 0) {
            if (IsAdult()) {
                RegisterColliderBase(play, &ph->colCylinder.base, COLL_OC);
                RegisterColliderBase(play, &ph->colJntSph.base, COLL_OC);
                if ((ph->actor.colorFilterTimer == 0 || !(ph->actor.colorFilterParams & 0x4000)) &&
                    ph->state != PEAHAT_STATE_EXPLODE) {
                    RegisterColliderBase(play, &ph->colJntSph.base, COLL_AC);
                }
            }
        }

        if (IsBladeActiveState()) {
            if (!IsFlying()) {
                RegisterColliderBase(play, &ph->colQuad.base, COLL_AT | COLL_AC);
            }

            if (IsGrounded() && (ph->actor.flags & ACTOR_FLAG_INSIDE_CULLING_VOLUME)) {
                for (s32 i = 1; i >= 0; i--) {
                    Vec3f posResult;
                    CollisionPoly* poly = nullptr;
                    s32 bgId;
                    if (BgCheck_EntityLineTest1(&play->colCtx, &ph->actor.world.pos, &ph->bladeTip[i], &posResult,
                                                &poly, true, true, false, true, &bgId) == 1) {
                        func_80033480(play, &posResult, 0.0f, 1, 300, 150, 1);
                        EnPeehat_SpawnDust(play, ph, &posResult, 0.0f, 3, 1.05f, 1.5f);
                    }
                }
            } else if (!IsFlying()) {
                RegisterColliderBase(play, &ph->colCylinder.base, COLL_AC);
            }
        } else {
            RegisterColliderBase(play, &ph->colCylinder.base, COLL_AC);
        }
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
};

}

#endif
