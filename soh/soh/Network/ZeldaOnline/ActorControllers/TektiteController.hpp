#ifndef TEKTITECONTROLLERH
#define TEKTITECONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Tite/z_en_tite.h"
#include "objects/object_tite/object_tite.h"

void EnTite_Idle(EnTite* tite, PlayState* play);
void EnTite_Attack(EnTite* tite, PlayState* play);
void EnTite_TurnTowardPlayer(EnTite* tite, PlayState* play);
void EnTite_MoveTowardPlayer(EnTite* tite, PlayState* play);
void EnTite_Recoil(EnTite* tite, PlayState* play);
void EnTite_Stunned(EnTite* tite, PlayState* play);
void EnTite_DeathCry(EnTite* tite, PlayState* play);
void EnTite_FallApart(EnTite* tite, PlayState* play);
void EnTite_FlipOnBack(EnTite* tite, PlayState* play);
void EnTite_FlipUpright(EnTite* tite, PlayState* play);

void EnTite_SetupDeathCry(EnTite* tite);
}

namespace ZeldaOnline {

class TektiteController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTite* Typed() const {
        return reinterpret_cast<EnTite*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_ATTACK = 1;
    static constexpr u8 ID_DEATH_CRY = 6;
    static constexpr u8 ID_FALL_APART = 7;
    static constexpr u8 ID_FLIP_ON_BACK = 8;

    static constexpr s16 ATTACK_STATE_MID_LUNGE = 1;

    using TiteActionFunc = void (*)(EnTite*, PlayState*);
    static const TiteActionFunc* ActionTable(size_t* count) {
        static const TiteActionFunc sTable[] = {
            EnTite_Idle,
            EnTite_Attack,
            EnTite_TurnTowardPlayer,
            EnTite_MoveTowardPlayer,
            EnTite_Recoil,
            EnTite_Stunned,
            EnTite_DeathCry,
            EnTite_FallApart,
            EnTite_FlipOnBack,
            EnTite_FlipUpright,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TiteActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static f32 CanonicalPlaySpeed(u8 actionIndex) {
        return (actionIndex == ID_FLIP_ON_BACK) ? 1.5f
                                                 : 1.0f;
    }

    static constexpr u8 ANIM_IDLE = 0;
    static constexpr u8 ANIM_LUNGE_BEGIN = 1;
    static constexpr u8 ANIM_LUNGE_MID = 2;
    static constexpr u8 ANIM_LUNGE_LANDED = 3;
    static constexpr u8 ANIM_TURN = 4;
    static constexpr u8 ANIM_MOVE = 5;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 6;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_IDLE:
                return object_tite_Anim_0012E4;
            case ANIM_LUNGE_BEGIN:
                return object_tite_Anim_00083C;
            case ANIM_LUNGE_MID:
                return object_tite_Anim_0004F8;
            case ANIM_LUNGE_LANDED:
                return object_tite_Anim_00069C;
            case ANIM_TURN:
                return object_tite_Anim_000A14;
            case ANIM_MOVE:
                return object_tite_Anim_000C70;
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
        EnTite* tite = Typed();
        u8 roles = COLL_AC | COLL_OC;
        if (tite->actionFunc == EnTite_Attack && tite->actionVar1 == ATTACK_STATE_MID_LUNGE &&
            !(tite->collider.base.atFlags & AT_HIT))
            roles |= COLL_AT;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ACTION_STATE,
        PROP_HEALTH,
        PROP_COLL_ROLES,
        PROP_FLIP_STATE,
        PROP_ACTION_VAR1,
        PROP_ACTION_VAR2,
        PROP_DAMAGE_EFFECT,
        PROP_SPAWN_ICE_TIMER,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTite* tite = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ACTION_STATE, PackedUInt1(tite->action), out);
        PackProperty(PROP_HEALTH, PackedUInt1(tite->actor.colChkInfo.health), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        PackProperty(PROP_FLIP_STATE, PackedUInt1(tite->flipState), out);
        PackProperty(PROP_ACTION_VAR1, PackedInt2(tite->actionVar1), out);
        PackProperty(PROP_ACTION_VAR2, PackedUInt1(tite->actionVar2), out);
        PackProperty(PROP_DAMAGE_EFFECT, PackedUInt1(tite->damageEffect), out);
        PackProperty(PROP_SPAWN_ICE_TIMER, PackedUInt1(tite->spawnIceTimer), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(tite->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &tite->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTite* tite = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const TiteActionFunc* table = ActionTable(&count);
                if (id < count)
                    tite->actionFunc = table[id];
                break;
            }
            case PROP_ACTION_STATE:
                tite->action = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEALTH:
                tite->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FLIP_STATE:
                tite->flipState = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ACTION_VAR1:
                tite->actionVar1 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ACTION_VAR2:
                tite->actionVar2 = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_DAMAGE_EFFECT:
                tite->damageEffect = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_SPAWN_ICE_TIMER:
                tite->spawnIceTimer = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                tite->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &tite->skelAnime, LOCK_CUR_FRAME ? tite->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void OnBecomeLeader() override {
        EnTite* tite = Typed();

        size_t count;
        const TiteActionFunc* table = ActionTable(&count);
        if (m_currentActionIndex < count)
            tite->actionFunc = table[m_currentActionIndex];

        if (tite->skelAnime.playSpeed == 0.0f)
            tite->skelAnime.playSpeed = CanonicalPlaySpeed(m_currentActionIndex);
    }

    void OnPropertiesApplied(u64 changed) override {
        if ((m_currentActionIndex == ID_DEATH_CRY || m_currentActionIndex == ID_FALL_APART)) {
            EnTite_SetupDeathCry(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnTite* tite = Typed();

        UpdateAnimation(&tite->skelAnime, LOCK_CUR_FRAME);

        if (tite->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        tite->collider.base.acFlags &= ~AC_HIT;

        bool hunting = m_currentActionIndex <= 3 || m_currentActionIndex == ID_UNKNOWN;
        if (hunting && tite->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        tite->actor.focus.pos = tite->actor.world.pos;
        tite->actor.focus.pos.y += 20.0f;

        {
            Vec3f rayOrigin = tite->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            tite->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &tite->actor.floorPoly,
                                                                  &floorBgId, &tite->actor, &rayOrigin);
            tite->actor.floorBgId = floorBgId;
        }

        RegisterColliderBase(play, &tite->collider.base, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AC | COLL_OC;
};

}

#endif
