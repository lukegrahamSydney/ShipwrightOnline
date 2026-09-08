#ifndef REDEADCONTROLLERH
#define REDEADCONTROLLERH

#include <cstring>
#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Rd/z_en_rd.h"
#include "objects/object_rd/object_rd.h"

void EnRd_Idle(EnRd* rd, PlayState* play);
void EnRd_RiseFromCoffin(EnRd* rd, PlayState* play);
void EnRd_WalkToHome(EnRd* rd, PlayState* play);
void EnRd_WalkToParent(EnRd* rd, PlayState* play);
void EnRd_WalkToPlayer(EnRd* rd, PlayState* play);
void EnRd_StandUp(EnRd* rd, PlayState* play);
void EnRd_Crouch(EnRd* rd, PlayState* play);
void EnRd_AttemptPlayerFreeze(EnRd* rd, PlayState* play);
void EnRd_Grab(EnRd* rd, PlayState* play);
void EnRd_Damaged(EnRd* rd, PlayState* play);
void EnRd_Dead(EnRd* rd, PlayState* play);
void EnRd_Stunned(EnRd* rd, PlayState* play);

void EnRd_SetupDead(EnRd* rd);
}

namespace ZeldaOnline {

class RedeadController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnRd* Typed() const {
        return reinterpret_cast<EnRd*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_DEAD = 10;
    static constexpr s16 ACTION_GRAB = 8;
    static constexpr s16 ACTION_DAMAGED = 9;
    static constexpr s16 ACTION_DEAD = 10;

    using RdActionFunc = void (*)(EnRd*, PlayState*);
    static const RdActionFunc* ActionTable(size_t* count) {
        static const RdActionFunc sTable[] = {
            EnRd_Idle,
            EnRd_RiseFromCoffin,
            EnRd_WalkToHome,
            EnRd_WalkToParent,
            EnRd_WalkToPlayer,
            EnRd_StandUp,
            EnRd_Crouch,
            EnRd_AttemptPlayerFreeze,
            EnRd_Grab,
            EnRd_Damaged,
            EnRd_Dead,
            EnRd_Stunned,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const RdActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_IDLE = 0;
    static constexpr u8 ANIM_SOBBING = 1;
    static constexpr u8 ANIM_WIPING_TEARS = 2;
    static constexpr u8 ANIM_WALK = 3;
    static constexpr u8 ANIM_GRAB_START = 4;
    static constexpr u8 ANIM_GRAB_ATTACK = 5;
    static constexpr u8 ANIM_GRAB_END = 6;
    static constexpr u8 ANIM_LOOK_BACK = 7;
    static constexpr u8 ANIM_STAND_UP = 8;
    static constexpr u8 ANIM_DAMAGE = 9;
    static constexpr u8 ANIM_DEATH = 10;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 11;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_IDLE:
                return gGibdoRedeadIdleAnim;
            case ANIM_SOBBING:
                return gGibdoRedeadSobbingAnim;
            case ANIM_WIPING_TEARS:
                return gGibdoRedeadWipingTearsAnim;
            case ANIM_WALK:
                return gGibdoRedeadWalkAnim;
            case ANIM_GRAB_START:
                return gGibdoRedeadGrabStartAnim;
            case ANIM_GRAB_ATTACK:
                return gGibdoRedeadGrabAttackAnim;
            case ANIM_GRAB_END:
                return gGibdoRedeadGrabEndAnim;
            case ANIM_LOOK_BACK:
                return gGibdoRedeadLookBackAnim;
            case ANIM_STAND_UP:
                return gGibdoRedeadStandUpAnim;
            case ANIM_DAMAGE:
                return gGibdoRedeadDamageAnim;
            case ANIM_DEATH:
                return gGibdoRedeadDeathAnim;
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
        EnRd* rd = Typed();
        if (rd->actor.colChkInfo.health == 0 || rd->action == ACTION_GRAB)
            return 0;

        u8 roles = COLL_OC;
        roles |= COLL_AC;
        return roles;
    }

    bool HitWouldReact() const {
        EnRd* rd = Typed();
        if (!(rd->collider.base.acFlags & AC_HIT))
            return false;
        u8 effect = rd->actor.colChkInfo.damageEffect;
        return effect != 0 && effect != 6;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ACTION_NUM,
        PROP_HEALTH,
        PROP_TIMERS,
        PROP_STUN,
        PROP_FADE,
        PROP_LIMB_ROT,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnRd* rd = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ACTION_NUM, PackedUInt1(rd->action), out);
        PackProperty(PROP_HEALTH, PackedUInt1(rd->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(rd->timer) << PackedUInt1(rd->playerStunWaitTimer)
                                  << PackedInt2(rd->sunsSongStunTimer),
                     out);
        PackProperty(PROP_STUN,
                     ByteStream() << PackedUInt1(rd->stunnedBySunsSong) << PackedUInt1(rd->damageReaction)
                                  << PackedUInt1(rd->unk_31D) << PackedUInt1(rd->fireTimer),
                     out);
        PackProperty(PROP_FADE, PackedUInt1(rd->alpha), out);
        PackProperty(PROP_LIMB_ROT, ByteStream() << PackedInt2(rd->headYRotation) << PackedInt2(rd->upperBodyYRotation),
                     out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(rd->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &rd->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);

        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnRd* rd = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const RdActionFunc* table = ActionTable(&count);
                if (id < count)
                    rd->actionFunc = table[id];
                break;
            }
            case PROP_ACTION_NUM:
                rd->action = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEALTH:
                rd->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                rd->timer = (s16)(data.Read<PackedInt2>().value());
                rd->playerStunWaitTimer = (u8)(data.Read<PackedUInt1>().value());
                rd->sunsSongStunTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_STUN:
                rd->stunnedBySunsSong = (u8)(data.Read<PackedUInt1>().value());
                rd->damageReaction = (u8)(data.Read<PackedUInt1>().value());
                rd->unk_31D = (u8)(data.Read<PackedUInt1>().value());
                rd->fireTimer = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FADE:
                rd->alpha = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_LIMB_ROT:
                rd->headYRotation = (s16)(data.Read<PackedInt2>().value());
                rd->upperBodyYRotation = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                rd->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &rd->skelAnime, LOCK_CUR_FRAME ? rd->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (m_currentActionIndex == ID_DEAD) {
            EnRd_SetupDead(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnRd* rd = Typed();

        UpdateAnimation(&rd->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        rd->collider.base.acFlags &= ~AC_HIT;

        if (rd->action != ACTION_DEAD && rd->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        rd->actor.focus.pos = rd->actor.world.pos;
        rd->actor.focus.pos.y += 50.0f;

        if (m_roles != 0) {
            Collider_UpdateCylinder(&rd->actor, &rd->collider);
            RegisterColliderBase(play, &rd->collider.base, m_roles);
        }

    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
