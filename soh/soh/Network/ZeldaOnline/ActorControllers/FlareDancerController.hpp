#ifndef FLAREDANCERCONTROLLERH
#define FLAREDANCERCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Fd/z_en_fd.h"
#include "objects/object_fd/object_fd.h"
#include "objects/object_fw/object_fw.h"

void EnFd_WaitForCore(EnFd* fd, PlayState* play);
void EnFd_Reappear(EnFd* fd, PlayState* play);
void EnFd_SpinAndGrow(EnFd* fd, PlayState* play);
void EnFd_JumpToGround(EnFd* fd, PlayState* play);
void EnFd_Land(EnFd* fd, PlayState* play);
void EnFd_SpinAndSpawnFire(EnFd* fd, PlayState* play);
void EnFd_Run(EnFd* fd, PlayState* play);

void EnFd_SpawnDot(EnFd* fd, PlayState* play);
void EnFd_UpdateFlames(EnFd* fd);
void EnFd_UpdateDots(EnFd* fd);
}

namespace ZeldaOnline {

class FlareDancerController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnFd* Typed() const {
        return reinterpret_cast<EnFd*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    static const EnFdActionFunc* ActionTable(size_t* count) {
        static const EnFdActionFunc sTable[] = {
            EnFd_WaitForCore, EnFd_Reappear,         EnFd_SpinAndGrow, EnFd_JumpToGround,
            EnFd_Land,        EnFd_SpinAndSpawnFire, EnFd_Run,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const EnFdActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 5;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gFlareDancerTwirlAnim;
            case 1:
                return gFlareDancerGettingUpAnim;
            case 2:
                return gFlareDancerChasingAnim;
            case 3:
                return gFlareDancerCastingFireAnim;
            case 4:
                return gFlareDancerBackflipAnim;
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
        EnFd* fd = Typed();
        return (fd->collider.base.acFlags & AC_HIT) && fd->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_RUN,
        PROP_CORE_POS,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnFd* fd = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(fd->coreActive) << PackedInt2(fd->spinTimer)
                                  << PackedInt2(fd->circlesToComplete) << PackedInt2(fd->invincibilityTimer)
                                  << PackedInt2(fd->attackTimer) << PackedUInt1(fd->actor.colChkInfo.health),
                     out);


        PackProperty(PROP_RUN,
                     ByteStream() << PackedInt2(fd->initYawToInitPos) << PackedInt2(fd->curYawToInitPos)
                                  << PackedInt2(fd->runDir) << PackedFloat4(fd->runRadius)
                                  << PackedFloat4(fd->fadeAlpha),
                     out);
        PackProperty(PROP_CORE_POS,
                     ByteStream() << PackedFloat4(fd->corePos.x) << PackedFloat4(fd->corePos.y)
                                  << PackedFloat4(fd->corePos.z),
                     out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(fd->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &fd->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnFd* fd = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const EnFdActionFunc* table = ActionTable(&count);
                if (id < count)
                    fd->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                fd->coreActive = (u8)(data.Read<PackedUInt1>().value());
                fd->spinTimer = (s16)(data.Read<PackedInt2>().value());
                fd->circlesToComplete = (s16)(data.Read<PackedInt2>().value());
                fd->invincibilityTimer = (s16)(data.Read<PackedInt2>().value());
                fd->attackTimer = (s16)(data.Read<PackedInt2>().value());
                fd->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_RUN:
                fd->initYawToInitPos = (s16)(data.Read<PackedInt2>().value());
                fd->curYawToInitPos = (s16)(data.Read<PackedInt2>().value());
                fd->runDir = (s16)(data.Read<PackedInt2>().value());
                fd->runRadius = data.Read<PackedFloat4>().value();
                fd->fadeAlpha = data.Read<PackedFloat4>().value();
                break;
            case PROP_CORE_POS:
                fd->corePos.x = data.Read<PackedFloat4>().value();
                fd->corePos.y = data.Read<PackedFloat4>().value();
                fd->corePos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_CUR_FRAME:
                fd->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &fd->skelAnime, LOCK_CUR_FRAME ? fd->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnFd* fd = Typed();

        UpdateAnimation(&fd->skelAnime, LOCK_CUR_FRAME);


        if (fd->actionFunc != EnFd_Reappear)
            EnFd_SpawnDot(fd, play);
        EnFd_UpdateDots(fd);
        EnFd_UpdateFlames(fd);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        fd->collider.base.acFlags &= ~AC_HIT;

        if (fd->actor.colChkInfo.health > 0 && fd->actor.xzDistToPlayer < 500.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);


        if (fd->actionFunc != EnFd_Reappear && fd->actionFunc != EnFd_SpinAndGrow &&
            fd->actionFunc != EnFd_WaitForCore) {

            Collider_UpdateSpheres(0, &fd->collider);

            u8 roles = COLL_OC;
            if (fd->attackTimer == 0 && fd->invincibilityTimer == 0)
                roles |= COLL_AT;
            if (fd->actionFunc == EnFd_Run || fd->actionFunc == EnFd_SpinAndSpawnFire)
                roles |= COLL_AC;

            RegisterColliderBase(play, &fd->collider.base, roles);
        }
    }
};

} // namespace ZeldaOnline

#endif