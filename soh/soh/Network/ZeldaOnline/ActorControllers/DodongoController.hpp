#ifndef DODONGOCONTROLLERH
#define DODONGOCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Dodongo/z_en_dodongo.h"
#include "objects/object_dodongo/object_dodongo.h"

void EnDodongo_Idle(EnDodongo* dd, PlayState* play);
void EnDodongo_Walk(EnDodongo* dd, PlayState* play);
void EnDodongo_BreatheFire(EnDodongo* dd, PlayState* play);
void EnDodongo_EndBreatheFire(EnDodongo* dd, PlayState* play);
void EnDodongo_SwallowBomb(EnDodongo* dd, PlayState* play);
void EnDodongo_SweepTail(EnDodongo* dd, PlayState* play);
void EnDodongo_Stunned(EnDodongo* dd, PlayState* play);
void EnDodongo_Death(EnDodongo* dd, PlayState* play);

void EnDodongo_ShiftVecRadial(s16 yaw, f32 radius, Vec3f* vec);
void EffectSsDFire_SpawnFixedScale(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* accel, s16 alpha, s16 scale);
}

namespace ZeldaOnline {

class DodongoController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDodongo* Typed() const {
        return reinterpret_cast<EnDodongo*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    static constexpr s32 STATE_SWEEP_TAIL = 0;
    static constexpr s32 STATE_SWALLOW_BOMB = 1;
    static constexpr s32 STATE_DEATH = 2;
    static constexpr s32 STATE_BREATHE_FIRE = 3;
    static constexpr s32 STATE_IDLE = 4;
    static constexpr s32 STATE_END_BREATHE_FIRE = 5;
    static constexpr s32 STATE_UNUSED = 6;
    static constexpr s32 STATE_STUNNED = 7;
    static constexpr s32 STATE_WALK = 8;

    using DodongoActionFunc = void (*)(EnDodongo*, PlayState*);
    static const DodongoActionFunc* ActionTable(size_t* count) {
        static const DodongoActionFunc sTable[] = {
            EnDodongo_Idle,
            EnDodongo_Walk,
            EnDodongo_BreatheFire,
            EnDodongo_EndBreatheFire,
            EnDodongo_SwallowBomb,
            EnDodongo_SweepTail,
            EnDodongo_Stunned,
            EnDodongo_Death,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DodongoActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_WAIT = 0;
    static constexpr u8 ANIM_WALK = 1;
    static constexpr u8 ANIM_BREATHE_FIRE = 2;
    static constexpr u8 ANIM_AFTER_BREATHE = 3;
    static constexpr u8 ANIM_DAMAGE = 4;
    static constexpr u8 ANIM_DIE = 5;
    static constexpr u8 ANIM_SWEEP_LEFT = 6;
    static constexpr u8 ANIM_SWEEP_RIGHT = 7;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 8;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_WAIT:
                return gDodongoWaitAnim;
            case ANIM_WALK:
                return gDodongoWalkAnim;
            case ANIM_BREATHE_FIRE:
                return gDodongoBreatheFireAnim;
            case ANIM_AFTER_BREATHE:
                return gDodongoAfterBreatheFireAnim;
            case ANIM_DAMAGE:
                return gDodongoDamageAnim;
            case ANIM_DIE:
                return gDodongoDieAnim;
            case ANIM_SWEEP_LEFT:
                return gDodongoSweepTailLeftAnim;
            case ANIM_SWEEP_RIGHT:
                return gDodongoSweepTailRightAnim;
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

    u8 CurrentBodyRoles() const {
        EnDodongo* dd = Typed();
        u8 roles = COLL_OC;
        if (dd->actionState > STATE_DEATH)
            roles |= COLL_AC;
        return roles;
    }
    u8 CurrentHardRoles() const {
        return (Typed()->actionState != STATE_DEATH) ? COLL_AC : 0;
    }
    u8 CurrentFireRoles() const {
        EnDodongo* dd = Typed();
        if (dd->actionState != STATE_BREATHE_FIRE)
            return 0;
        return (dd->skelAnime.curFrame > 29.0f && dd->skelAnime.curFrame < 43.0f) ? COLL_AT : 0;
    }

    bool TailArmed() const {
        return (Typed()->sphElements[1].info.toucherFlags & TOUCH_ON) != 0;
    }
    void ApplyTailArmed(bool armed) {
        EnDodongo* dd = Typed();
        for (int i = 1; i <= 2; i++) {
            dd->sphElements[i].info.toucherFlags = armed ? (TOUCH_ON | TOUCH_SFX_WOOD) : TOUCH_NONE;
            dd->sphElements[i].info.toucher.dmgFlags = armed ? 0xFFCFFFFF : 0;
            dd->sphElements[i].info.toucher.damage = armed ? 8 : 0;
        }
        dd->colliderBody.base.atFlags = armed ? (AT_ON | AT_TYPE_ENEMY) : AT_NONE;
    }

    bool HitWouldReact() const {
        EnDodongo* dd = Typed();
        if (dd->colliderHard.base.acFlags & AC_BOUNCED)
            return false;
        return (dd->colliderBody.base.acFlags & AC_HIT) && dd->actionState > STATE_DEATH;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ACTION_STATE,
        PROP_HEALTH,
        PROP_COLL_ROLES,
        PROP_TAIL_ARMED,
        PROP_TIMERS,
        PROP_RIGHT_FOOT,
        PROP_BODY_SCALE,
        PROP_SMOKE_COLORS,
        PROP_DAMAGE_EFFECT,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    static u32 PackColor(const Color_RGBA8& c) {
        return ((u32)(c.r) << 24) | ((u32)(c.g) << 16) | ((u32)(c.b) << 8) | c.a;
    }
    static void UnpackColor(u32 v, Color_RGBA8& c) {
        c.r = (u8)(v >> 24);
        c.g = (u8)(v >> 16);
        c.b = (u8)(v >> 8);
        c.a = (u8)(v);
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnDodongo* dd = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ACTION_STATE, PackedInt2((s16)(dd->actionState)), out);
        PackProperty(PROP_HEALTH, PackedUInt1(dd->actor.colChkInfo.health), out);
        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentBodyRoles()) << PackedUInt1(CurrentHardRoles())
                                  << PackedUInt1(CurrentFireRoles()),
                     out);
        PackProperty(PROP_TAIL_ARMED, PackedUInt1(TailArmed() ? 1 : 0), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(dd->timer) << PackedInt2(dd->retreatTimer)
                                  << PackedInt2(dd->tailSwipeSpeed) << PackedInt2(dd->iceTimer),
                     out);
        PackProperty(PROP_RIGHT_FOOT, PackedUInt1((u8)(dd->rightFootStep)), out);
        PackProperty(PROP_BODY_SCALE,
                     ByteStream() << PackedFloat4(dd->bodyScale.x) << PackedFloat4(dd->bodyScale.y)
                                  << PackedFloat4(dd->bodyScale.z),
                     out);
        PackProperty(PROP_SMOKE_COLORS,
                     ByteStream() << PackedUInt4(PackColor(dd->bombSmokePrimColor))
                                  << PackedUInt4(PackColor(dd->bombSmokeEnvColor)),
                     out);
        PackProperty(PROP_DAMAGE_EFFECT, PackedUInt1(dd->damageEffect), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(dd->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &dd->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDodongo* dd = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const DodongoActionFunc* table = ActionTable(&count);
                if (id < count)
                    dd->actionFunc = table[id];
                break;
            }
            case PROP_ACTION_STATE:
                dd->actionState = (s32)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                dd->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_hardRoles = (u8)(data.Read<PackedUInt1>().value());
                m_fireRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TAIL_ARMED:
                ApplyTailArmed(data.Read<PackedUInt1>().value() != 0);
                break;
            case PROP_TIMERS:
                dd->timer = (s16)(data.Read<PackedInt2>().value());
                dd->retreatTimer = (s16)(data.Read<PackedInt2>().value());
                dd->tailSwipeSpeed = (s16)(data.Read<PackedInt2>().value());
                dd->iceTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_RIGHT_FOOT:
                dd->rightFootStep = (s16)(data.Read<PackedUInt1>().value());
                break;
            case PROP_BODY_SCALE:
                dd->bodyScale.x = data.Read<PackedFloat4>().value();
                dd->bodyScale.y = data.Read<PackedFloat4>().value();
                dd->bodyScale.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_SMOKE_COLORS:
                UnpackColor(data.Read<PackedUInt4>().value(), dd->bombSmokePrimColor);
                UnpackColor(data.Read<PackedUInt4>().value(), dd->bombSmokeEnvColor);
                break;
            case PROP_DAMAGE_EFFECT:
                dd->damageEffect = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_ANIM_CUR_FRAME:
                dd->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &dd->skelAnime, LOCK_CUR_FRAME ? dd->skelAnime.curFrame : 0.0f, data);
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

    void UpdatePuppet(PlayState* play) override {
        EnDodongo* dd = Typed();

        UpdateAnimation(&dd->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }
        dd->colliderBody.base.acFlags &= ~AC_HIT;
        dd->colliderHard.base.acFlags &= ~AC_BOUNCED;
        dd->colliderAT.base.atFlags &= ~AT_HIT;

        if (dd->actionState > STATE_DEATH && dd->actor.colChkInfo.health > 0 && dd->actor.xzDistToPlayer < 400.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        dd->actor.focus.pos.x = dd->actor.world.pos.x + Math_SinS(dd->actor.shape.rot.y) * -30.0f;
        dd->actor.focus.pos.y = dd->actor.world.pos.y + 20.0f;
        dd->actor.focus.pos.z = dd->actor.world.pos.z + Math_CosS(dd->actor.shape.rot.y) * -30.0f;

        {
            Vec3f rayOrigin = dd->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            dd->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &dd->actor.floorPoly, &floorBgId,
                                                                &dd->actor, &rayOrigin);
            dd->actor.floorBgId = floorBgId;
        }

        if (dd->actionState == STATE_BREATHE_FIRE && dd->skelAnime.curFrame >= 29.0f &&
            dd->skelAnime.curFrame <= 43.0f) {
            Vec3f velocity = { 0.0f, 0.0f, 0.0f };
            Vec3f accel = { 0.0f, 0.0f, 0.0f };
            Vec3f pos = dd->actor.world.pos;
            s16 fireFrame = (s16)(dd->skelAnime.curFrame - 29.0f);

            pos.y += 35.0f;
            EnDodongo_ShiftVecRadial(dd->actor.world.rot.y, 30.0f, &pos);
            EnDodongo_ShiftVecRadial(dd->actor.world.rot.y, 2.5f, &accel);
            EffectSsDFire_SpawnFixedScale(play, &pos, &velocity, &accel, 255 - (fireFrame * 10), fireFrame + 3);
        }

        RegisterColliderBase(play, &dd->colliderBody.base, m_bodyRoles);
        RegisterColliderBase(play, &dd->colliderHard.base, m_hardRoles);
        RegisterColliderBase(play, &dd->colliderAT.base, m_fireRoles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_bodyRoles = COLL_OC | COLL_AC;
    u8 m_hardRoles = COLL_AC;
    u8 m_fireRoles = 0;
};

}

#endif
