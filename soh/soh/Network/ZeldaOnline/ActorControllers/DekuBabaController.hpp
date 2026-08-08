#ifndef DEKUBABACONTROLLERH
#define DEKUBABACONTROLLERH

#include <cstring>
#include "../AbstractActorController.hpp"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "objects/object_dekubaba/object_dekubaba.h"

extern "C" {
void EnDekubaba_Wait(EnDekubaba*, PlayState*);
void EnDekubaba_Grow(EnDekubaba*, PlayState*);
void EnDekubaba_Retract(EnDekubaba*, PlayState*);
void EnDekubaba_DecideLunge(EnDekubaba*, PlayState*);
void EnDekubaba_PrepareLunge(EnDekubaba*, PlayState*);
void EnDekubaba_Lunge(EnDekubaba*, PlayState*);
void EnDekubaba_PullBack(EnDekubaba*, PlayState*);
void EnDekubaba_Recover(EnDekubaba*, PlayState*);
void EnDekubaba_Hit(EnDekubaba*, PlayState*);
void EnDekubaba_StunnedVertical(EnDekubaba*, PlayState*);
void EnDekubaba_Sway(EnDekubaba*, PlayState*);
void EnDekubaba_PrunedSomersault(EnDekubaba*, PlayState*);
void EnDekubaba_ShrinkDie(EnDekubaba*, PlayState*);
void EnDekubaba_DeadStickDrop(EnDekubaba*, PlayState*);
}

namespace ZeldaOnline {

class DekuBabaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDekubaba* Typed() const {
        return reinterpret_cast<EnDekubaba*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_COLL,
        PROP_COLL_ROLES,
        PROP_POSE,
        PROP_HEALTH,
        PROP_TIMER,
    };

    static const void* const* ActionTable(size_t* count) {
        static const void* const sTable[] = {
            (void*)EnDekubaba_Wait,         (void*)EnDekubaba_Grow,
            (void*)EnDekubaba_Retract,      (void*)EnDekubaba_DecideLunge,
            (void*)EnDekubaba_PrepareLunge, (void*)EnDekubaba_Lunge,
            (void*)EnDekubaba_PullBack,     (void*)EnDekubaba_Recover,
            (void*)EnDekubaba_Hit,          (void*)EnDekubaba_StunnedVertical,
            (void*)EnDekubaba_Sway,         (void*)EnDekubaba_PrunedSomersault,
            (void*)EnDekubaba_ShrinkDie,    (void*)EnDekubaba_DeadStickDrop,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t n;
        const void* const* t = ActionTable(&n);
        for (size_t i = 0; i < n; i++)
            if (t[i] == (void*)Typed()->actionFunc)
                return (u8)(i);
        return 0xFF;
    }

    u8 CurrentAnimIndex() const {
        const char* a = (const char*)Typed()->skelAnime.animation;
        if (a && strcmp(a, gDekuBabaPauseChompAnim) == 0)
            return 1;
        return 0;
    }

    static const char* AnimForIndex(u8 id) {
        return (id == 1) ? gDekuBabaPauseChompAnim : gDekuBabaFastChompAnim;
    }

    u8 CurrentColliderRoles() const {
        EnDekubabaActionFunc af = Typed()->actionFunc;
        u8 roles = 0;

        if (af == EnDekubaba_Lunge)
            roles |= COLL_AT;

        if (Typed()->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;

        if (af != EnDekubaba_DeadStickDrop)
            roles |= COLL_OC;

        return roles;
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnDekubaba* db = Typed();

        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(db->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &db->skelAnime), out);

        u8 acConfig = db->collider.base.acFlags & ~(AC_HIT | AC_BOUNCED);
        u8 bumpMask = 0, ocMask = 0;
        for (int i = 0; i < 7; i++) {
            if (db->collider.elements[i].info.bumperFlags & BUMP_ON)
                bumpMask |= (1 << i);
            if (db->collider.elements[i].info.ocElemFlags & OCELEM_ON)
                ocMask |= (1 << i);
        }
        PackProperty(PROP_COLL, ByteStream() << PackedUInt1(acConfig) << PackedUInt1(db->collider.base.colType)
                                                    << PackedUInt1(bumpMask) << PackedUInt1(ocMask), out);

        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        {
            ByteStream pose;
            for (int i = 0; i < 3; i++)
                pose << PackedInt2(db->stemSectionAngle[i]);
            pose << PackedInt2(db->targetSwayAngle);
            PackProperty(PROP_POSE, pose, out);
        }

        PackProperty(PROP_HEALTH, PackedUInt1(db->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMER, PackedInt2(db->timer), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDekubaba* db = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != 0xFF) {
                    size_t n;
                    const void* const* t = ActionTable(&n);
                    if (ai < n)
                        db->actionFunc = reinterpret_cast<EnDekubabaActionFunc>(const_cast<void*>(t[ai]));
                }
                return true;
            }
            case PROP_ANIM_CUR_FRAME:
                db->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                return true;
            case PROP_ANIM: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                ApplyAnimProperty((void*)AnimForIndex(ai), &db->skelAnime, LOCK_CUR_FRAME ? db->skelAnime.curFrame : 0.0f,
                                 data);
                return true;
            }
            case PROP_COLL: {
                u8 acConfig = (u8)(data.Read<PackedUInt1>().value());
                db->collider.base.acFlags = (db->collider.base.acFlags & (AC_HIT | AC_BOUNCED)) | acConfig;
                db->collider.base.colType = (u8)(data.Read<PackedUInt1>().value());
                u8 bumpMask = (u8)(data.Read<PackedUInt1>().value());
                u8 ocMask = (u8)(data.Read<PackedUInt1>().value());
                for (int i = 0; i < 7; i++) {
                    if (bumpMask & (1 << i))
                        db->collider.elements[i].info.bumperFlags |= BUMP_ON;
                    else
                        db->collider.elements[i].info.bumperFlags &= ~BUMP_ON;
                    if (ocMask & (1 << i))
                        db->collider.elements[i].info.ocElemFlags |= OCELEM_ON;
                    else
                        db->collider.elements[i].info.ocElemFlags &= ~OCELEM_ON;
                }
                return true;
            }
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_POSE: {
                for (int i = 0; i < 3; i++)
                    db->stemSectionAngle[i] = (s16)(data.Read<PackedInt2>().value());
                db->targetSwayAngle = (s16)(data.Read<PackedInt2>().value());
                return true;
            }
            case PROP_HEALTH:
                db->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_TIMER:
                db->timer = (s16)(data.Read<PackedInt2>().value());
                return true;
            default:
                return false;
        }
    }

    void OnPropertiesApplied(u64 changed) override {
        if (CurrentActionIndex() == ID_SHRINK_DIE && !IsRunningLocally()) {
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnDekubaba* db = Typed();

        UpdateAnimation(&db->skelAnime, LOCK_CUR_FRAME);

        if (db->actionFunc != EnDekubaba_DeadStickDrop && db->boundFloor == NULL) {
            Actor_UpdateBgCheckInfo(play, &db->actor, 0.0f, 0.0f, 0.0f, 4);
            db->boundFloor = db->actor.floorPoly;
        }

        if (db->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        if (db->actionFunc == EnDekubaba_Wait && db->actor.xzDistToPlayer < 150.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        RegisterColliderBase(play, &db->collider.base, m_roles);
    }

  private:
    static constexpr u8 ID_SHRINK_DIE = 12;

    u8 m_roles = COLL_OC | COLL_AC;
};

}

#endif
