#ifndef OCTOROKCONTROLLERH
#define OCTOROKCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Okuta/z_en_okuta.h"
#include "objects/object_okuta/object_okuta.h"

void EnOkuta_WaitToAppear(EnOkuta* ok, PlayState* play);
void EnOkuta_Appear(EnOkuta* ok, PlayState* play);
void EnOkuta_Hide(EnOkuta* ok, PlayState* play);
void EnOkuta_WaitToShoot(EnOkuta* ok, PlayState* play);
void EnOkuta_Shoot(EnOkuta* ok, PlayState* play);
void EnOkuta_WaitToDie(EnOkuta* ok, PlayState* play);
void EnOkuta_Die(EnOkuta* ok, PlayState* play);
void EnOkuta_Freeze(EnOkuta* ok, PlayState* play);
void EnOkuta_ProjectileFly(EnOkuta* ok, PlayState* play);
void EnOkuta_Draw(Actor* thisx, PlayState* play);
void EnOkuta_SetupDie(EnOkuta* thisx);
void EnOkuta_SetupWaitToDie(EnOkuta* thisx);
void EnOkuta_UpdateHeadScale(EnOkuta* ok);
}

namespace ZeldaOnline {

class OctorokController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnOkuta* Typed() const {
        return reinterpret_cast<EnOkuta*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr s16 ParamsProjectile = 0x10;
    static constexpr f32 ColliderHeight = 40.0f;

    bool IsProjectile() const {
        return Typed()->actor.params == ParamsProjectile;
    }

    using OkutaActionFunc = void (*)(EnOkuta*, PlayState*);
    static const OkutaActionFunc* ActionTable(size_t* count) {
        static const OkutaActionFunc sTable[] = {
            EnOkuta_WaitToAppear,
            EnOkuta_Appear,
            EnOkuta_Hide,
            EnOkuta_WaitToShoot,
            EnOkuta_Shoot,
            EnOkuta_WaitToDie,
            EnOkuta_Die,
            EnOkuta_Freeze,
            EnOkuta_ProjectileFly,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const OkutaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_APPEAR = 0;
    static constexpr u8 ANIM_HIDE = 1;
    static constexpr u8 ANIM_FLOAT = 2;
    static constexpr u8 ANIM_SHOOT = 3;
    static constexpr u8 ANIM_HIT = 4;
    static constexpr u8 ANIM_DIE = 5;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 6;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_APPEAR:
                return gOctorokAppearAnim;
            case ANIM_HIDE:
                return gOctorokHideAnim;
            case ANIM_FLOAT:
                return gOctorokFloatAnim;
            case ANIM_SHOOT:
                return gOctorokShootAnim;
            case ANIM_HIT:
                return gOctorokHitAnim;
            case ANIM_DIE:
                return gOctorokDieAnim;
            default:
                return nullptr;
        }
    }
    u8 CurrentAnimIndex() const {
        if (IsProjectile())
            return ANIM_UNKNOWN;
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

    bool HitWouldReact() const {
        EnOkuta* ok = Typed();
        if (!(ok->collider.base.acFlags & AC_HIT))
            return false;
        return ok->actor.colChkInfo.damageEffect != 0 || ok->actor.colChkInfo.damage != 0;
    }

    u8 CurrentColliderRoles() const {
        EnOkuta* ok = Typed();
        EnOkutaActionFunc fn = ok->actionFunc;
        bool excludedFromAll = (fn == EnOkuta_WaitToAppear);
        bool excludedFromAC = excludedFromAll || fn == EnOkuta_Die || fn == EnOkuta_WaitToDie || fn == EnOkuta_Freeze;
        u8 roles = 0;
        if (!excludedFromAll) {
            if (!excludedFromAC)
                roles |= COLL_AC;
            roles |= COLL_OC;
        }
        if (IsProjectile())
            roles |= COLL_AT;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_BATTERY,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnOkuta* ok = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(ok->actor.colChkInfo.health), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_BATTERY,
                     ByteStream() << PackedInt2(ok->timer) << PackedInt2(ok->numShots) << PackedFloat4(ok->jumpHeight),
                     out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        if (!IsProjectile()) {
            if (LOCK_CUR_FRAME)
                PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(ok->skelAnime.curFrame), out);
            PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &ok->skelAnime), out);
        }

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnOkuta* ok = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const OkutaActionFunc* table = ActionTable(&count);
                if (id < count)
                    ok->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                ok->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_BATTERY:
                ok->timer = (s16)(data.Read<PackedInt2>().value());
                ok->numShots = (s16)(data.Read<PackedInt2>().value());
                ok->jumpHeight = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                ok->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &ok->skelAnime, LOCK_CUR_FRAME ? ok->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnOkuta_Die || Typed()->actionFunc == EnOkuta_WaitToDie) {
            EnOkuta_SetupWaitToDie(Typed());
            GoLocal();
        }
    }

    void OnServerDestroy() override {
        if (!IsProjectile() || gPlayState == nullptr)
            return;
        EnOkuta* ok = Typed();
        Vec3f pos = ok->actor.world.pos;
        pos.y += 11.0f;
        EffectSsHahen_SpawnBurst(gPlayState, &pos, 6.0f, 0, 1, 2, 15, 7, 10, (Gfx*)gOctorokProjectileDL);
    }

    void EnsureDrawState() {
        EnOkuta* ok = Typed();
        if (!IsProjectile())
            ok->actor.draw = (ok->actionFunc == EnOkuta_WaitToAppear) ? nullptr : EnOkuta_Draw;
    }

    void UpdateLeader(PlayState* play) override {
        EnsureDrawState();
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnOkuta* ok = Typed();

        if (!IsProjectile())
            UpdateAnimation(&ok->skelAnime, LOCK_CUR_FRAME);

        if (!IsProjectile() && HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        ok->collider.base.acFlags &= ~AC_HIT;
        ok->collider.base.atFlags &= ~AT_HIT;

        if (!IsProjectile() && ok->actionFunc != EnOkuta_Die && ok->actor.colChkInfo.health > 0 &&
            ok->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        EnsureDrawState();

        if (IsProjectile())
            ok->actor.home.rot.z += 0x1554;

        EnOkuta_UpdateHeadScale(ok);

        Collider_UpdateCylinder(&ok->actor, &ok->collider);
        if (!IsProjectile() && (ok->actionFunc == EnOkuta_Appear || ok->actionFunc == EnOkuta_Hide)) {
            ok->collider.dim.pos.y = int16_t(ok->actor.world.pos.y + (ok->skelAnime.jointTable->y * ok->actor.scale.y));
            ok->collider.dim.height = (s16)(
                ((ColliderHeight * ok->headScale.y) - ok->collider.dim.yShift) * ok->actor.scale.y * 100.0f);
        }

        Actor_SetFocus(&ok->actor, 15.0f);

        if (m_roles != 0)
            RegisterColliderBase(play, &ok->collider.base, m_roles);
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
