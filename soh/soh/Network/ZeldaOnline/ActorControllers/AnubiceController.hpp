#ifndef ANUBICECONTROLLERH
#define ANUBICECONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"
#include "AnubiceTagController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Anubice/z_en_anubice.h"
#include "src/overlays/actors/ovl_En_Anubice_Tag/z_en_anubice_tag.h"
#include "objects/object_anubice/object_anubice.h"

void EnAnubice_FindFlameCircles(EnAnubice*, PlayState* play);
void EnAnubice_SetupIdle(EnAnubice*, PlayState* play);
void EnAnubice_Idle(EnAnubice*, PlayState* play);
void EnAnubice_GoToHome(EnAnubice*, PlayState* play);
void EnAnubice_SetupShootFireball(EnAnubice*, PlayState* play);
void EnAnubice_ShootFireball(EnAnubice*, PlayState* play);
void EnAnubice_SetupDie(EnAnubice*, PlayState* play);
void EnAnubice_Die(EnAnubice*, PlayState* play);
void EnAnubice_Hover(EnAnubice*, PlayState* play);
}

namespace ZeldaOnline {

class AnubiceController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnAnubice* Typed() const {
        return reinterpret_cast<EnAnubice*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 3;

    using AnubiceActionFunc = void (*)(EnAnubice*, PlayState*);
    static const AnubiceActionFunc* ActionTable(size_t* count) {
        static const AnubiceActionFunc sTable[] = {
            EnAnubice_FindFlameCircles, EnAnubice_SetupIdle,          EnAnubice_Idle, EnAnubice_GoToHome,
            EnAnubice_SetupShootFireball, EnAnubice_ShootFireball,    EnAnubice_SetupDie, EnAnubice_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    void OnActorInit() override {
        EnAnubice* an = Typed();
        Actor* parent = an->actor.parent;

        if (parent != nullptr && parent->id == ACTOR_EN_ANUBICE_TAG)
            reinterpret_cast<EnAnubiceTag*>(parent)->anubis = an;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const AnubiceActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gAnubiceIdleAnim,
            gAnubiceAttackingAnim,
            gAnubiceFallDownAnim,
        };
        if (i >= ANIM_COUNT)
            return nullptr;
        return sAnims[i];
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++)
            if (strcmp(AnimForIndex(i), cur) == 0)
                return i;
        return ANIM_UNKNOWN;
    }

    bool IsDying() const {
        EnAnubice* an = Typed();
        return an->actionFunc == EnAnubice_SetupDie || an->actionFunc == EnAnubice_Die;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_DEATH,
        PROP_FIREBALL,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnAnubice* an = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(an->timeAlive) << PackedInt2(an->knockbackTimer)
                                  << PackedInt2(an->isKnockedback) << PackedFloat4(an->headRot)
                                  << PackedFloat4(an->focusHeightOffset) << PackedFloat4(an->animLastFrame),
                     out);
        PackProperty(PROP_DEATH,
                     ByteStream() << PackedInt2(an->isFallingOver) << PackedInt2(an->fallTargetPitch)
                                  << PackedInt2(an->deathTimer),
                     out);
        PackProperty(PROP_FIREBALL,
                     ByteStream() << PackedFloat4(an->fireballPos.x) << PackedFloat4(an->fireballPos.y)
                                  << PackedFloat4(an->fireballPos.z) << PackedFloat4(an->fireballRot.x)
                                  << PackedFloat4(an->fireballRot.y) << PackedFloat4(an->fireballRot.z),
                     out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(an->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &an->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnAnubice* an = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const AnubiceActionFunc* table = ActionTable(&count);
                if (id < count)
                    an->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                an->timeAlive = (s16)(data.Read<PackedInt2>().value());
                an->knockbackTimer = (s16)(data.Read<PackedInt2>().value());
                an->isKnockedback = (s16)(data.Read<PackedInt2>().value());
                an->headRot = data.Read<PackedFloat4>().value();
                an->focusHeightOffset = data.Read<PackedFloat4>().value();
                an->animLastFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_DEATH:
                an->isFallingOver = (s16)(data.Read<PackedInt2>().value());
                an->fallTargetPitch = (s16)(data.Read<PackedInt2>().value());
                an->deathTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FIREBALL:
                an->fireballPos.x = data.Read<PackedFloat4>().value();
                an->fireballPos.y = data.Read<PackedFloat4>().value();
                an->fireballPos.z = data.Read<PackedFloat4>().value();
                an->fireballRot.x = data.Read<PackedFloat4>().value();
                an->fireballRot.y = data.Read<PackedFloat4>().value();
                an->fireballRot.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_CUR_FRAME:
                an->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &an->skelAnime, LOCK_CUR_FRAME ? an->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnAnubice* an = Typed();

        if ((changed & (1ull << PROP_ACTION)) && IsDying() && an->actor.category == ACTORCAT_ENEMY) {
            Actor_ChangeCategory(gPlayState, &gPlayState->actorCtx, &an->actor, ACTORCAT_PROP);
            an->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
        }
    }

    void OnBecomeLeader() override {
        EnAnubice* an = Typed();


        if (!an->hasSearchedForFlameCircles) {
            an->actionFunc = EnAnubice_FindFlameCircles;
        }
    }

    void DragSpawnerIntoLeadership() {
        Actor* parent = Typed()->actor.parent;

        if (parent == nullptr || parent->id != ACTOR_EN_ANUBICE_TAG)
            return;

        AnubiceTagController* tag = reinterpret_cast<AnubiceTagController*>(parent->zoController);

        if (tag != nullptr)
            tag->FollowAnubisLeadership();
    }

    void UpdateLeader(PlayState* play) override {
        DragSpawnerIntoLeadership();
        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnAnubice* an = Typed();

        UpdateAnimation(&an->skelAnime, LOCK_CUR_FRAME);

        if (IsDying())
            return;

        Actor_SetFocus(&an->actor, an->focusHeightOffset);

        if (an->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        an->collider.base.acFlags &= ~AC_HIT;


        if (an->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Collider_UpdateCylinder(&an->actor, &an->collider);
        RegisterColliderBase(play, &an->collider.base, COLL_OC);

        if (!an->isKnockedback && an->actor.shape.yOffset == 0.0f)
            RegisterColliderBase(play, &an->collider.base, COLL_AC);
    }
};

} // namespace ZeldaOnline

#endif
