#ifndef FLAREDANCERCORECONTROLLERH
#define FLAREDANCERCORECONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Fw/z_en_fw.h"
#include "objects/object_fw/object_fw.h"

void EnFw_Bounce(EnFw* fw, PlayState* play);
void EnFw_Run(EnFw* fw, PlayState* play);
void EnFw_TurnToParentInitPos(EnFw* fw, PlayState* play);
void EnFw_JumpToParentInitPos(EnFw* fw, PlayState* play);

void EnFw_UpdateDust(EnFw* fw);
}

namespace ZeldaOnline {

class FlareDancerCoreController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnFw* Typed() const {
        return reinterpret_cast<EnFw*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    static const EnFwActionFunc* ActionTable(size_t* count) {
        static const EnFwActionFunc sTable[] = {
            EnFw_Bounce,
            EnFw_Run,
            EnFw_TurnToParentInitPos,
            EnFw_JumpToParentInitPos,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const EnFwActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 3;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gFlareDancerCoreInitRunCycleAnim;
            case 1:
                return gFlareDancerCoreRunCycleAnim;
            case 2:
                return gFlareDancerCoreEndRunCycleAnim;
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

    bool HitWouldReact() const {
        EnFw* fw = Typed();
        return (fw->collider.base.acFlags & AC_HIT) && fw->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_RUN,
        PROP_BOMP_POS,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnFw* fw = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(fw->lastDmgHook) << PackedInt2(fw->bounceCnt)
                                  << PackedInt2(fw->damageTimer) << PackedInt2(fw->explosionTimer)
                                  << PackedInt2(fw->slideTimer) << PackedInt2(fw->returnToParentTimer)
                                  << PackedInt2(fw->turnAround) << PackedUInt1(fw->actor.colChkInfo.health),
                     out);
        PackProperty(PROP_RUN, ByteStream() << PackedInt2(fw->runDirection) << PackedFloat4(fw->runRadius), out);
        PackProperty(PROP_BOMP_POS,
                     ByteStream() << PackedFloat4(fw->bompPos.x) << PackedFloat4(fw->bompPos.y)
                                  << PackedFloat4(fw->bompPos.z),
                     out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(fw->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &fw->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnFw* fw = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const EnFwActionFunc* table = ActionTable(&count);
                if (id < count)
                    fw->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                fw->lastDmgHook = (u8)(data.Read<PackedUInt1>().value());
                fw->bounceCnt = (s16)(data.Read<PackedInt2>().value());
                fw->damageTimer = (s16)(data.Read<PackedInt2>().value());
                fw->explosionTimer = (s16)(data.Read<PackedInt2>().value());
                fw->slideTimer = (s16)(data.Read<PackedInt2>().value());
                fw->returnToParentTimer = (s16)(data.Read<PackedInt2>().value());
                fw->turnAround = (s16)(data.Read<PackedInt2>().value());
                fw->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_RUN:
                fw->runDirection = (s16)(data.Read<PackedInt2>().value());
                fw->runRadius = data.Read<PackedFloat4>().value();
                break;
            case PROP_BOMP_POS:
                fw->bompPos.x = data.Read<PackedFloat4>().value();
                fw->bompPos.y = data.Read<PackedFloat4>().value();
                fw->bompPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_CUR_FRAME:
                fw->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &fw->skelAnime, LOCK_CUR_FRAME ? fw->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnFw* fw = Typed();

        UpdateAnimation(&fw->skelAnime, LOCK_CUR_FRAME);

        EnFw_UpdateDust(fw);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        fw->collider.base.acFlags &= ~AC_HIT;

        if (fw->actor.colChkInfo.health > 0 && fw->actor.xzDistToPlayer < 500.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        // Same gates as EnFw_Update.
        if (!CHECK_FLAG_ALL(fw->actor.flags, ACTOR_FLAG_HOOKSHOT_ATTACHED)) {
            Collider_UpdateSpheres(0, &fw->collider);

            u8 roles = COLL_OC;
            if (fw->damageTimer == 0 && fw->explosionTimer == 0 && fw->actionFunc == EnFw_Run)
                roles |= COLL_AC;

            RegisterColliderBase(play, &fw->collider.base, roles);
        }
    }
};

} // namespace ZeldaOnline

#endif