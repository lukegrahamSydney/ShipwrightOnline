#ifndef CLAMCONTROLLERH
#define CLAMCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Sb/z_en_sb.h"
#include "objects/object_sb/object_sb.h"

void EnSb_WaitClosed(EnSb* sb, PlayState* play);
void EnSb_Open(EnSb* sb, PlayState* play);
void EnSb_WaitOpen(EnSb* sb, PlayState* play);
void EnSb_TurnAround(EnSb* sb, PlayState* play);
void EnSb_Lunge(EnSb* sb, PlayState* play);
void EnSb_Bounce(EnSb* sb, PlayState* play);
void EnSb_Cooldown(EnSb* sb, PlayState* play);

void EnSb_SpawnBubbles(PlayState* play, EnSb* sb);
}

namespace ZeldaOnline {

class ClamController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnSb* Typed() const {
        return reinterpret_cast<EnSb*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using SbActionFunc = decltype(&EnSb_WaitClosed);
    static const SbActionFunc* ActionTable(size_t* count) {
        static const SbActionFunc sTable[] = {
            EnSb_WaitClosed, EnSb_Open,   EnSb_WaitOpen, EnSb_TurnAround,
            EnSb_Lunge,      EnSb_Bounce, EnSb_Cooldown,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const SbActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 5;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0: return object_sb_Anim_00004C;
            case 1: return object_sb_Anim_0000B4;
            case 2: return object_sb_Anim_000124;
            case 3: return object_sb_Anim_000194;
            case 4: return object_sb_Anim_002C8C;
            default: return nullptr;
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

    bool HitWouldReact() const {
        EnSb* sb = Typed();
        return !sb->isDead && (sb->collider.base.acFlags & AC_HIT);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnSb* sb = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);


        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(sb->fire) << PackedInt2(sb->behavior) << PackedInt2(sb->isDead)
                                  << PackedInt2(sb->timer) << PackedInt2(sb->attackYaw)
                                  << PackedInt2(sb->bouncesLeft) << PackedUInt1(sb->hitByWindArrow)
                                  << PackedUInt1(sb->actor.colChkInfo.health),
                     out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(sb->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &sb->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnSb* sb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const SbActionFunc* table = ActionTable(&count);
                if (id < count)
                    sb->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                sb->fire = (s16)(data.Read<PackedInt2>().value());
                sb->behavior = (s16)(data.Read<PackedInt2>().value());
                sb->isDead = (s16)(data.Read<PackedInt2>().value());
                sb->timer = (s16)(data.Read<PackedInt2>().value());
                sb->attackYaw = (s16)(data.Read<PackedInt2>().value());
                sb->bouncesLeft = (s16)(data.Read<PackedInt2>().value());
                sb->hitByWindArrow = (u8)(data.Read<PackedUInt1>().value());
                sb->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                sb->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &sb->skelAnime, LOCK_CUR_FRAME ? sb->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnSb* sb = Typed();


        if (sb->isDead) {
            BodyBreak_Alloc(&sb->bodyBreak, 8, gPlayState);
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnSb* sb = Typed();

        UpdateAnimation(&sb->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        sb->collider.base.acFlags &= ~AC_HIT;
        sb->collider.base.atFlags &= ~AT_HIT;

        if (!sb->isDead && sb->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Actor_SetFocus(&sb->actor, 20.0f);

        //the bubbles come from EnSb_Open, which a puppet never runs.
        if (sb->actionFunc == EnSb_Open && sb->actor.yDistToWater > 0.0f) {
            EnSb_SpawnBubbles(play, sb);
        }

        if (!sb->isDead) {
            Collider_UpdateCylinder(&sb->actor, &sb->collider);
            RegisterColliderBase(play, &sb->collider.base, COLL_AT | COLL_AC | COLL_OC);
        }
    }
};

}

#endif
