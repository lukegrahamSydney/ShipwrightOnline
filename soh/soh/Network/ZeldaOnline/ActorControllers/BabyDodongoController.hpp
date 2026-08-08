#ifndef BABYDODONGOCONTROLLERH
#define BABYDODONGOCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Dodojr/z_en_dodojr.h"
#include "objects/object_dodojr/object_dodojr.h"

void EnDodojr_WaitUnderground(EnDodojr* d, PlayState* play);
void EnDodojr_DropItem(EnDodojr* d, PlayState* play);
void EnDodojr_EmergeFromGround(EnDodojr* d, PlayState* play);
void EnDodojr_CrawlTowardsTarget(EnDodojr* d, PlayState* play);
void EnDodojr_StunnedBounce(EnDodojr* d, PlayState* play);
void EnDodojr_JumpAttackBounce(EnDodojr* d, PlayState* play);
void EnDodojr_Stunned(EnDodojr* d, PlayState* play);
void EnDodojr_SwallowBomb(EnDodojr* d, PlayState* play);
void EnDodojr_SwallowedBombDeathBounce(EnDodojr* d, PlayState* play);
void EnDodojr_SwallowedBombDeathSequence(EnDodojr* d, PlayState* play);
void EnDodojr_StandardDeathBounce(EnDodojr* d, PlayState* play);
void EnDodojr_Despawn(EnDodojr* d, PlayState* play);
void EnDodojr_DeathSequence(EnDodojr* d, PlayState* play);
void EnDodojr_WaitFreezeFrames(EnDodojr* d, PlayState* play);
void EnDodojr_EatBomb(EnDodojr* d, PlayState* play);

s32 EnDodojr_CheckNearbyBombs(EnDodojr* d, PlayState* play);
}

namespace ZeldaOnline {

class BabyDodongoController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDodojr* Typed() const {
        return reinterpret_cast<EnDodojr*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_WAIT_UNDERGROUND = 0;
    static constexpr u8 ID_DROP_ITEM = 1;
    static constexpr u8 ID_EMERGE = 2;
    static constexpr u8 ID_CRAWL = 3;
    static constexpr u8 ID_STUNNED_BOUNCE = 4;
    static constexpr u8 ID_JUMP_ATTACK = 5;
    static constexpr u8 ID_STUNNED = 6;
    static constexpr u8 ID_SWALLOW_BOMB = 7;
    static constexpr u8 ID_DEATH_FIRST = 8;
    static constexpr u8 ID_DEATH_LAST = 12;
    static constexpr u8 ID_EAT_BOMB = 14;

    using DodojrActionFunc = void (*)(EnDodojr*, PlayState*);
    static const DodojrActionFunc* ActionTable(size_t* count) {
        static const DodojrActionFunc sTable[] = {
            EnDodojr_WaitUnderground,
            EnDodojr_DropItem,
            EnDodojr_EmergeFromGround,
            EnDodojr_CrawlTowardsTarget,
            EnDodojr_StunnedBounce,
            EnDodojr_JumpAttackBounce,
            EnDodojr_Stunned,
            EnDodojr_SwallowBomb,
            EnDodojr_SwallowedBombDeathBounce,
            EnDodojr_SwallowedBombDeathSequence,
            EnDodojr_StandardDeathBounce,
            EnDodojr_Despawn,
            EnDodojr_DeathSequence,
            EnDodojr_WaitFreezeFrames,
            EnDodojr_EatBomb,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DodojrActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_IDLE = 0;
    static constexpr u8 ANIM_CRAWL = 1;
    static constexpr u8 ANIM_FLIP = 2;
    static constexpr u8 ANIM_JUMP_EAT = 3;
    static constexpr u8 ANIM_BOMB_DEATH = 4;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 5;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_IDLE:
                return object_dodojr_Anim_0009D4;
            case ANIM_CRAWL:
                return object_dodojr_Anim_000860;
            case ANIM_FLIP:
                return object_dodojr_Anim_0004A0;
            case ANIM_JUMP_EAT:
                return object_dodojr_Anim_000724;
            case ANIM_BOMB_DEATH:
                return object_dodojr_Anim_0005F0;
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
        EnDodojr* d = Typed();
        if (d->actionFunc == EnDodojr_WaitUnderground || d->actionFunc == EnDodojr_DropItem)
            return 0;
        u8 roles = COLL_OC;
        if (d->actionFunc == EnDodojr_EmergeFromGround || d->actionFunc == EnDodojr_CrawlTowardsTarget ||
            d->actionFunc == EnDodojr_JumpAttackBounce)
            roles |= COLL_AT | COLL_AC;
        else if (d->actionFunc == EnDodojr_StunnedBounce || d->actionFunc == EnDodojr_Stunned)
            roles |= COLL_AC;
        return roles;
    }

    bool HitWouldReact() const {
        EnDodojr* d = Typed();
        if (!(d->collider.base.acFlags & AC_HIT))
            return false;
        return !(d->actionFunc == EnDodojr_SwallowBomb || d->actionFunc == EnDodojr_SwallowedBombDeathBounce ||
                 d->actionFunc == EnDodojr_SwallowedBombDeathSequence || d->actionFunc == EnDodojr_Despawn ||
                 d->actionFunc == EnDodojr_StandardDeathBounce || d->actionFunc == EnDodojr_DeathSequence ||
                 d->actionFunc == EnDodojr_DropItem);
    }

    static void SpawnCrawlDust(PlayState* play, EnDodojr* d) {
        Color_RGBA8 prim = { 170, 130, 90, 255 };
        Color_RGBA8 env = { 100, 60, 20, 0 };
        Vec3f velocity = { 0.0f, 0.0f, 0.0f };
        Vec3f accel = { 0.0f, 0.3f, 0.0f };
        Vec3f pos;
        s16 angle = (s16)((Rand_ZeroOne() - 0.5f) * 65536.0f);

        pos.y = d->actor.floorHeight;
        accel.x = (Rand_ZeroOne() - 0.5f) * 2;
        accel.z = (Rand_ZeroOne() - 0.5f) * 2;
        pos.x = (Math_SinS(angle) * 11.0f) + d->actor.world.pos.x;
        pos.z = (Math_CosS(angle) * 11.0f) + d->actor.world.pos.z;

        func_8002836C(play, &pos, &velocity, &accel, &prim, &env, 100, 60, 8);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_COLL_ROLES,
        PROP_COUNTER,
        PROP_TIMER1,
        PROP_TIMER2,
        PROP_TIMER3,
        PROP_TIMER4,
        PROP_ROOT_SCALE,
        PROP_SHADOW_ON,
        PROP_DUST_POS,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnDodojr* d = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(d->actor.colChkInfo.health), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        PackProperty(PROP_COUNTER, PackedInt2(d->counter), out);
        PackProperty(PROP_TIMER1, PackedInt2(d->timer1), out);
        PackProperty(PROP_TIMER2, PackedInt2(d->timer2), out);
        PackProperty(PROP_TIMER3, PackedInt2(d->timer3), out);
        PackProperty(PROP_TIMER4, PackedInt2(d->timer4), out);
        PackProperty(PROP_ROOT_SCALE, PackedFloat4(d->rootScale), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_SHADOW_ON, PackedUInt1(d->actor.shape.shadowDraw != nullptr ? 1 : 0), out);
        PackProperty(PROP_DUST_POS,
                     ByteStream() << PackedFloat4(d->dustPos.x) << PackedFloat4(d->dustPos.y)
                                  << PackedFloat4(d->dustPos.z),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(d->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &d->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDodojr* d = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const DodojrActionFunc* table = ActionTable(&count);
                if (id < count)
                    d->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                d->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COUNTER:
                d->counter = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER1:
                d->timer1 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER2:
                d->timer2 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER3:
                d->timer3 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER4:
                d->timer4 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ROOT_SCALE:
                d->rootScale = data.Read<PackedFloat4>().value();
                break;

            case PROP_SHADOW_ON:
                d->actor.shape.shadowDraw = (data.Read<PackedUInt1>().value() != 0) ? ActorShadow_DrawCircle : nullptr;
                break;
            case PROP_DUST_POS:
                d->dustPos.x = data.Read<PackedFloat4>().value();
                d->dustPos.y = data.Read<PackedFloat4>().value();
                d->dustPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_CUR_FRAME:
                d->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &d->skelAnime, LOCK_CUR_FRAME ? d->skelAnime.curFrame : 0.0f, data);
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
        EnDodojr* d = Typed();

        size_t count;
        const DodojrActionFunc* table = ActionTable(&count);
        if (m_currentActionIndex < count)
            d->actionFunc = table[m_currentActionIndex];

        if (m_currentActionIndex == ID_EAT_BOMB && d->bomb == nullptr) {
            EnDodojr_CheckNearbyBombs(d, gPlayState);
            if (d->bomb == nullptr) {
                m_currentActionIndex = ID_CRAWL;
                d->actionFunc = EnDodojr_CrawlTowardsTarget;
            }
        }
    }

    void OnPropertiesApplied(u64 changed) override {
        if (m_currentActionIndex >= ID_DEATH_FIRST && m_currentActionIndex <= ID_DEATH_LAST && !IsRunningLocally())
            GoLocal();
    }

    void UpdatePuppet(PlayState* play) override {
        EnDodojr* d = Typed();

        UpdateAnimation(&d->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }
        d->collider.base.acFlags &= ~AC_HIT;

        bool active = m_currentActionIndex == ID_WAIT_UNDERGROUND || m_currentActionIndex == ID_EMERGE ||
                      m_currentActionIndex == ID_CRAWL || m_currentActionIndex == ID_JUMP_ATTACK;
        if (active && d->actor.xzDistToPlayer < 200.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        Actor_SetFocus(&d->actor, 10.0f);

        {
            Vec3f rayOrigin = d->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            d->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &d->actor.floorPoly, &floorBgId,
                                                               &d->actor, &rayOrigin);
            d->actor.floorBgId = floorBgId;
        }

        if (m_currentActionIndex == ID_CRAWL)
            SpawnCrawlDust(play, d);

        RegisterCylinder(play, &d->collider, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = 0;
};

}

#endif
