#ifndef GUAYCONTROLLERH
#define GUAYCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Crow/z_en_crow.h"

void EnCrow_FlyIdle(EnCrow* crow, PlayState* play);
void EnCrow_Respawn(EnCrow* crow, PlayState* play);
void EnCrow_DiveAttack(EnCrow* crow, PlayState* play);
void EnCrow_Die(EnCrow* crow, PlayState* play);
void EnCrow_TurnAway(EnCrow* crow, PlayState* play);
void EnCrow_Damaged(EnCrow* crow, PlayState* play);
}

namespace ZeldaOnline {

class GuayController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnCrow* Typed() const {
        return reinterpret_cast<EnCrow*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using CrowActionFunc = void (*)(EnCrow*, PlayState*);
    static const CrowActionFunc* ActionTable(size_t* count) {
        static const CrowActionFunc sTable[] = {
            EnCrow_FlyIdle,
            EnCrow_Respawn,
            EnCrow_DiveAttack,
            EnCrow_Die,
            EnCrow_TurnAway,
            EnCrow_Damaged,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const CrowActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool HitWouldReact() const {
        EnCrow* crow = Typed();
        if (!(crow->collider.base.acFlags & AC_HIT))
            return false;
        return crow->actor.colChkInfo.damageEffect != 0 || crow->actor.colChkInfo.damage != 0;
    }

    u8 CurrentColliderRoles() const {
        EnCrow* crow = Typed();
        u8 roles = 0;
        if (crow->actionFunc == EnCrow_DiveAttack)
            roles |= COLL_AT;
        if (crow->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        if (crow->actionFunc != EnCrow_Respawn)
            roles |= COLL_OC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_AIM,
        PROP_TIMER,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnCrow* crow = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(crow->actor.colChkInfo.health), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_AIM, ByteStream() << PackedInt2(crow->aimRotX) << PackedInt2(crow->aimRotY), out);
        PackProperty(PROP_TIMER, PackedInt2(crow->timer), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(crow->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(0, &crow->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnCrow* crow = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const CrowActionFunc* table = ActionTable(&count);
                if (id < count)
                    crow->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                crow->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_AIM:
                crow->aimRotX = (s16)(data.Read<PackedInt2>().value());
                crow->aimRotY = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER:
                crow->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                crow->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM:
                data.Read<PackedUInt1>();
                ApplyAnimProperty(nullptr, &crow->skelAnime, LOCK_CUR_FRAME ? crow->skelAnime.curFrame : 0.0f, data);
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnCrow* crow = Typed();
        bool nowDying = crow->actionFunc == EnCrow_Die;
        if (nowDying && m_prevAction == EnCrow_Damaged && gPlayState != nullptr) {
            static Vec3f sZero = { 0.0f, 0.0f, 0.0f };
            EffectSsDeadDb_Spawn(gPlayState, &crow->actor.world.pos, &sZero, &sZero, (int16_t)crow->actor.scale.x * 10000, 0,
                                 255, 255, 255, 255, 255, 0, 0, 1, 9, 1);
        }
        m_prevAction = crow->actionFunc;
    }

    void UpdatePuppet(PlayState* play) override {
        EnCrow* crow = Typed();

        UpdateAnimation(&crow->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        crow->collider.base.acFlags &= ~AC_HIT;
        crow->collider.base.atFlags &= ~AT_HIT;

        if (crow->actionFunc != EnCrow_Die && crow->actionFunc != EnCrow_Respawn &&
            crow->actionFunc != EnCrow_Damaged && crow->actor.xzDistToPlayer < 300.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        crow->actor.world.rot.y = crow->actor.shape.rot.y;
        crow->actor.world.rot.x = -crow->actor.shape.rot.x;

        f32 height = (crow->actor.colChkInfo.health != 0) ? 20.0f * (crow->actor.scale.x * 100.0f) : 0.0f;
        crow->collider.elements[0].dim.worldSphere.center.x = (int16_t)crow->actor.world.pos.x;
        crow->collider.elements[0].dim.worldSphere.center.y = (int16_t)(crow->actor.world.pos.y + height);
        crow->collider.elements[0].dim.worldSphere.center.z = (int16_t)crow->actor.world.pos.z;

        Actor_SetFocus(&crow->actor, height);

        RegisterColliderBase(play, &crow->collider.base, m_roles);
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;

    CrowActionFunc m_prevAction = nullptr;
};

}

#endif
