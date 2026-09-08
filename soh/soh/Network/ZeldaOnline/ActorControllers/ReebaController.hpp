#ifndef REEBACONTROLLERH
#define REEBACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Reeba/z_en_reeba.h"

void EnReeba_SetupSurface(EnReeba*, PlayState* play);
void EnReeba_Surface(EnReeba*, PlayState* play);
void EnReeba_Move(EnReeba*, PlayState* play);
void EnReeba_SetupSink(EnReeba*, PlayState* play);
void EnReeba_Sink(EnReeba*, PlayState* play);
void EnReeba_SetupMoveBig(EnReeba*, PlayState* play);
void EnReeba_MoveBig(EnReeba*, PlayState* play);
void EnReeba_Recoiled(EnReeba*, PlayState* play);
void EnReeba_SetupStunned(EnReeba*, PlayState* play);
void EnReeba_Stunned(EnReeba*, PlayState* play);
void EnReeba_StunRecover(EnReeba*, PlayState* play);
void EnReeba_StunDie(EnReeba*, PlayState* play);
void EnReeba_SetupDamaged(EnReeba*, PlayState* play);
void EnReeba_Damaged(EnReeba*, PlayState* play);
void EnReeba_SetupDie(EnReeba*, PlayState* play);
void EnReeba_Die(EnReeba*, PlayState* play);
}

namespace ZeldaOnline {

class ReebaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnReeba* Typed() const {
        return reinterpret_cast<EnReeba*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using ReebaActionFunc = void (*)(EnReeba*, PlayState*);
    static const ReebaActionFunc* ActionTable(size_t* count) {
        static const ReebaActionFunc sTable[] = {
            EnReeba_SetupSurface, EnReeba_Surface,      EnReeba_Move,        EnReeba_SetupSink,
            EnReeba_Sink,         EnReeba_SetupMoveBig, EnReeba_MoveBig,     EnReeba_Recoiled,
            EnReeba_SetupStunned, EnReeba_Stunned,      EnReeba_StunRecover, EnReeba_StunDie,
            EnReeba_SetupDamaged, EnReeba_Damaged,      EnReeba_SetupDie,    EnReeba_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ReebaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionfunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsDying() const {
        EnReeba* rb = Typed();
        return rb->actionfunc == EnReeba_SetupDie || rb->actionfunc == EnReeba_Die ||
               rb->actionfunc == EnReeba_StunDie;
    }

    void CurrentColliderRoles(u8& roles) const {
        EnReeba* rb = Typed();

        roles = 0;

        if ((rb->actor.shape.yOffset < -700.0f) || (rb->actor.colChkInfo.health <= 0) ||
            (rb->actionfunc == EnReeba_Sink))
            return;

        roles |= COLL_OC;

        if (rb->actor.shape.yOffset < 0.0f)
            return;

        roles |= COLL_AC;

        if (rb->actionfunc == EnReeba_Move || rb->actionfunc == EnReeba_MoveBig)
            roles |= COLL_AT;
    }

    void OnActorInit() override {
        EnReeba* rb = Typed();

        if (rb->actor.parent != nullptr)
            return;

        for (Actor* it = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].head; it != nullptr; it = it->next) {
            if (it->id == ACTOR_EN_ENCOUNT1) {
                rb->actor.parent = it;
                return;
            }
        }
    }


    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_TIMERS,
        PROP_HEALTH,
        PROP_ANIM_CUR_FRAME,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnReeba* rb = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(rb->stunType) << PackedInt2(rb->aimType)
                                  << PackedInt2(rb->unkDamageField) << PackedFloat4(rb->yOffsetTarget)
                                  << PackedFloat4(rb->yOffsetStep) << PackedFloat4(rb->scale),
                     out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(rb->bigLeeverTimer) << PackedInt2(rb->moveTimer)
                                  << PackedInt2(rb->sfxTimer) << PackedInt2(rb->damagedTimer)
                                  << PackedInt2(rb->waitTimer),
                     out);
        PackProperty(PROP_HEALTH, PackedInt1(rb->actor.colChkInfo.health), out);
        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(rb->skelanime.curFrame), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnReeba* rb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const ReebaActionFunc* table = ActionTable(&count);
                if (id < count)
                    rb->actionfunc = table[id];
                break;
            }
            case PROP_STATE:
                rb->stunType = (s16)(data.Read<PackedInt2>().value());
                rb->aimType = (s16)(data.Read<PackedInt2>().value());
                rb->unkDamageField = (s16)(data.Read<PackedInt2>().value());
                rb->yOffsetTarget = data.Read<PackedFloat4>().value();
                rb->yOffsetStep = data.Read<PackedFloat4>().value();
                rb->scale = data.Read<PackedFloat4>().value();
                break;
            case PROP_TIMERS:
                rb->bigLeeverTimer = (s16)(data.Read<PackedInt2>().value());
                rb->moveTimer = (s16)(data.Read<PackedInt2>().value());
                rb->sfxTimer = (s16)(data.Read<PackedInt2>().value());
                rb->damagedTimer = (s16)(data.Read<PackedInt2>().value());
                rb->waitTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                rb->actor.colChkInfo.health = (s8)(data.Read<PackedInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                rb->skelanime.curFrame = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnReeba* rb = Typed();
        u8 roles;

        UpdateAnimation(&rb->skelanime, LOCK_CUR_FRAME);

        if (IsDying())
            return;

        rb->actor.focus.pos = rb->actor.world.pos;
        rb->actor.focus.pos.y += rb->isBig ? 30.0f : 15.0f;

        if (rb->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        rb->collider.base.acFlags &= ~AC_HIT;
        rb->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);

        if (rb->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Collider_UpdateCylinder(&rb->actor, &rb->collider);

        CurrentColliderRoles(roles);
        if (roles != 0)
            RegisterColliderBase(play, &rb->collider.base, roles);
    }
};

} // namespace ZeldaOnline

#endif
