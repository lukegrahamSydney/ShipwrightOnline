#ifndef BOMBFLOWERCONTROLLERH
#define BOMBFLOWERCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bombf/z_en_bombf.h"

void EnBombf_GrowBomb(EnBombf* bf, PlayState* play);
void EnBombf_Move(EnBombf* bf, PlayState* play);
void EnBombf_WaitForRelease(EnBombf* bf, PlayState* play);
void EnBombf_Explode(EnBombf* bf, PlayState* play);
}

namespace ZeldaOnline {

class BombFlowerController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBombf* Typed() const {
        return reinterpret_cast<EnBombf*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using BombfActionFunc = void (*)(EnBombf*, PlayState*);
    static const BombfActionFunc* ActionTable(size_t* count) {
        static const BombfActionFunc sTable[] = {
            EnBombf_GrowBomb,
            EnBombf_Move,
            EnBombf_WaitForRelease,
            EnBombf_Explode,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BombfActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsGrownFlower() const {
        EnBombf* bf = Typed();
        return bf->actionFunc == EnBombf_GrowBomb && bf->flowerBombScale >= 1.0f;
    }

    bool HitWouldReact() const {
        EnBombf* bf = Typed();

        if (IsGrownFlower()) {
            return (bf->bombCollider.base.acFlags & AC_HIT) && bf->bombCollider.base.ac != nullptr &&
                   bf->bombCollider.base.ac->category != ACTORCAT_BOSS;
        }

        if (bf->actor.params != BOMBFLOWER_BODY)
            return false;
        if (bf->bombCollider.base.acFlags & AC_HIT)
            return true;
        return (bf->bombCollider.base.ocFlags1 & OC1_HIT) && bf->bombCollider.base.oc != nullptr &&
               bf->bombCollider.base.oc->category == ACTORCAT_ENEMY;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_PARAMS,
        PROP_TIMER,
        PROP_FUSE_ENABLED,
        PROP_FLOWER_SCALE,
        PROP_BUMP_ON,
        PROP_FLASH_SPEED,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBombf* bf = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_PARAMS, PackedInt2(bf->actor.params), out);
        PackProperty(PROP_TIMER, PackedInt2(bf->timer), out);
        PackProperty(PROP_FUSE_ENABLED, PackedInt4(bf->isFuseEnabled), out);
        PackProperty(PROP_FLOWER_SCALE, PackedFloat4(bf->flowerBombScale), out);
        PackProperty(PROP_BUMP_ON, PackedUInt1(bf->bumpOn), out);
        PackProperty(PROP_FLASH_SPEED, PackedInt2(bf->flashSpeedScale), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBombf* bf = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BombfActionFunc* table = ActionTable(&count);
                if (id < count)
                    bf->actionFunc = table[id];
                break;
            }
            case PROP_PARAMS:
                bf->actor.params = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMER:
                bf->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FUSE_ENABLED:
                bf->isFuseEnabled = data.Read<PackedInt4>().value();
                break;
            case PROP_FLOWER_SCALE:
                bf->flowerBombScale = data.Read<PackedFloat4>().value();
                break;
            case PROP_BUMP_ON:
                bf->bumpOn = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_FLASH_SPEED:
                bf->flashSpeedScale = (s16)(data.Read<PackedInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        auto bf = Typed();

        if (bf->actionFunc == EnBombf_Explode) {
            bf->actor.params = BOMBFLOWER_BODY;
            bf->actionFunc = EnBombf_Move;
            bf->isFuseEnabled = 1;
            bf->timer = 0;

            m_originalUpdate(m_actor, gPlayState);
            GoLocal();
            return;
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnBombf* bf = Typed();

        if (bf->actor.parent == &GET_PLAYER(play)->actor && !IsRunningLocally()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        bf->bombCollider.base.acFlags &= ~AC_HIT;
        bf->bombCollider.base.ocFlags1 &= ~OC1_HIT;

        if (bf->actor.parent != nullptr)
            bf->actor.room = -1;

        if (IsGrownFlower() && bf->actor.parent == nullptr)
            Actor_OfferCarry(&bf->actor, play);

        if (bf->actor.params == BOMBFLOWER_BODY && bf->isFuseEnabled != 0) {
            if (bf->timer < 127) {
                Vec3f effVelocity = { 0.0f, 0.0f, 0.0f };
                Vec3f effAccel = { 0.0f, 0.0f, 0.0f };
                Vec3f dustAccel = { 0.0f, 0.2f, 0.0f };
                Color_RGBA8 dustColor = { 255, 255, 255, 255 };
                Vec3f effPos = bf->actor.world.pos;

                effPos.y += 25.0f;
                if ((play->gameplayFrames % 2) == 0)
                    EffectSsGSpk_SpawnFuse(play, &bf->actor, &effPos, &effVelocity, &effAccel);

                effPos.y += 3.0f;
                func_8002829C(play, &effPos, &effVelocity, &dustAccel, &dustColor, &dustColor, 50, 5);
            }

            if ((bf->timer < 100) && ((bf->timer & (bf->flashSpeedScale + 1)) != 0))
                Math_SmoothStepToF(&bf->flashIntensity, 150.0f, 1.0f, 150.0f / bf->flashSpeedScale, 0.0f);
            else
                Math_SmoothStepToF(&bf->flashIntensity, 0.0f, 1.0f, 150.0f / bf->flashSpeedScale, 0.0f);
        }

        bf->actor.focus.pos = bf->actor.world.pos;
        bf->actor.focus.pos.y += 10.0f;

        if (bf->actor.params <= BOMBFLOWER_BODY) {
            Collider_UpdateCylinder(&bf->actor, &bf->bombCollider);

            u8 roles = COLL_AC;
            if (bf->flowerBombScale >= 1.0f && bf->bumpOn)
                roles |= COLL_OC;

            RegisterColliderBase(play, &bf->bombCollider.base, roles);
        }
    }
};

}

#endif
