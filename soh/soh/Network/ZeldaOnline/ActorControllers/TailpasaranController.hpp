#ifndef TAILPASARANCONTROLLERH
#define TAILPASARANCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Tp/z_en_tp.h"

void EnTp_Head_Wait(EnTp* tp, PlayState* play);
void EnTp_Head_ApproachPlayer(EnTp* tp, PlayState* play);
void EnTp_Head_TakeOff(EnTp* tp, PlayState* play);
void EnTp_Head_BurrowReturnHome(EnTp* tp, PlayState* play);
void EnTp_Die(EnTp* tp, PlayState* play);

void EnTp_SetupDie(EnTp* tp);

}
typedef enum {
     TAILPASARAN_ACTION_FRAGMENT_FADE,
     TAILPASARAN_ACTION_DIE,
     TAILPASARAN_ACTION_TAIL_FOLLOWHEAD,
     TAILPASARAN_ACTION_HEAD_WAIT = 4,
     TAILPASARAN_ACTION_HEAD_APPROACHPLAYER = 7,
     TAILPASARAN_ACTION_HEAD_TAKEOFF,
     TAILPASARAN_ACTION_HEAD_BURROWRETURNHOME
} TailpasaranAction;
typedef enum {
     TAILPASARAN_DMGEFF_NONE,
     TAILPASARAN_DMGEFF_DEKUNUT,
     TAILPASARAN_DMGEFF_SHOCKING = 14,
     TAILPASARAN_DMGEFF_INSULATING
} TailpasaranDamageEffect;

namespace ZeldaOnline {

class TailpasaranController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTp* Typed() const {
        return reinterpret_cast<EnTp*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return params <= TAILPASARAN_HEAD;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using TpActionFunc = void (*)(EnTp*, PlayState*);
    static const TpActionFunc* ActionTable(size_t* count) {
        static const TpActionFunc sTable[] = {
            EnTp_Head_Wait,
            EnTp_Head_ApproachPlayer,
            EnTp_Head_TakeOff,
            EnTp_Head_BurrowReturnHome,
            EnTp_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    static constexpr u8 ID_DIE = 4;

    u8 CurrentActionIndex() const {
        size_t count;
        const TpActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        EnTp* tp = Typed();
        u8 roles = 0;
        if (tp->actionIndex >= TAILPASARAN_ACTION_TAIL_FOLLOWHEAD) {
            roles |= COLL_AT;
            if (tp->actor.colChkInfo.health != 0)
                roles |= COLL_AC;
        }
        if (tp->damageEffect == TAILPASARAN_DMGEFF_SHOCKING)
            roles |= COLL_AT;
        return roles;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        if (actorId == ACTOR_EN_TP && params >= 0)
            return false;
        return true;
    }

    bool TailChainWasHit() const {
        for (Actor* now = Typed()->actor.child; now != nullptr; now = now->child) {
            EnTp* seg = reinterpret_cast<EnTp*>(now);
            if (seg->collider.base.acFlags & AC_HIT) {
                if (seg->actor.colChkInfo.damageEffect != TAILPASARAN_DMGEFF_NONE)
                    return true;
            }
        }
        return false;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ACTION_INDEX,
        PROP_PARAMS,
        PROP_HEALTH,
        PROP_TIMERS,
        PROP_VISUAL,
        PROP_DAMAGE_STATE,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTp* tp = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ACTION_INDEX, PackedInt4(tp->actionIndex), out);
        PackProperty(PROP_PARAMS, PackedInt2(tp->actor.params), out);
        PackProperty(PROP_HEALTH, PackedUInt1(tp->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(tp->timer) << PackedInt2(tp->unk_15C) << PackedInt4(tp->unk_150), out);
        PackProperty(PROP_VISUAL, ByteStream() << PackedInt2(tp->red) << PackedInt2(tp->alpha), out);
        PackProperty(PROP_DAMAGE_STATE,
                     ByteStream() << PackedUInt1(tp->damageEffect) << PackedUInt1(tp->actor.freezeTimer), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTp* tp = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const TpActionFunc* table = ActionTable(&count);
                if (id < count)
                    tp->actionFunc = table[id];
                break;
            }
            case PROP_ACTION_INDEX:
                tp->actionIndex = data.Read<PackedInt4>().value();
                break;
            case PROP_PARAMS:
                tp->actor.params = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                tp->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                tp->timer = (s16)(data.Read<PackedInt2>().value());
                tp->unk_15C = (s16)(data.Read<PackedInt2>().value());
                tp->unk_150 = data.Read<PackedInt4>().value();
                break;
            case PROP_VISUAL:
                tp->red = (s16)(data.Read<PackedInt2>().value());
                tp->alpha = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_DAMAGE_STATE:
                tp->damageEffect = (u8)(data.Read<PackedUInt1>().value());
                tp->actor.freezeTimer = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (m_currentActionIndex == ID_DIE && !IsRunningLocally()) {
            EnTp_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnTp* tp = Typed();

        if (TailChainWasHit() || (tp->collider.base.acFlags & AC_HIT)) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        tp->collider.base.acFlags &= ~AC_HIT;
        tp->collider.base.atFlags &= ~AT_HIT;

        if (m_currentActionIndex != ID_DIE && tp->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        tp->actor.shape.rot.z += 0x800;

        tp->actor.focus.pos = tp->actor.world.pos;

        if (tp->actor.params != TAILPASARAN_TAIL_DYING) {
            tp->kiraSpawnTimer--;
            tp->kiraSpawnTimer &= 7;
        }

        if (m_roles != 0)
            RegisterColliderBase(play, &tp->collider.base, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AT | COLL_AC;
};

}

#endif
