#ifndef KAREBABACONTROLLERH
#define KAREBABACONTROLLERH

#include "../AbstractActorController.hpp"
#include "src/overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"

extern "C" {
void EnKarebaba_Grow(EnKarebaba*, PlayState*);
void EnKarebaba_Idle(EnKarebaba*, PlayState*);
void EnKarebaba_Awaken(EnKarebaba*, PlayState*);
void EnKarebaba_Spin(EnKarebaba*, PlayState*);
void EnKarebaba_Dying(EnKarebaba*, PlayState*);
void EnKarebaba_DeadItemDrop(EnKarebaba*, PlayState*);
void EnKarebaba_Retract(EnKarebaba*, PlayState*);
void EnKarebaba_Dead(EnKarebaba*, PlayState*);
void EnKarebaba_Regrow(EnKarebaba*, PlayState*);
void EnKarebaba_Upright(EnKarebaba*, PlayState*);

}

namespace ZeldaOnline {

class KareBabaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnKarebaba* Typed() const {
        return reinterpret_cast<EnKarebaba*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_BODY_COLL,
        PROP_HEAD_COLL,
        PROP_COLL_ROLES,
        PROP_PARAMS,
        PROP_SHADOW_SCALE,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    static const void* const* ActionTable(size_t* count) {
        static const void* const sTable[] = {
            (void*)EnKarebaba_Grow,   (void*)EnKarebaba_Idle,         (void*)EnKarebaba_Awaken,  (void*)EnKarebaba_Spin,
            (void*)EnKarebaba_Dying,  (void*)EnKarebaba_DeadItemDrop, (void*)EnKarebaba_Retract, (void*)EnKarebaba_Dead,
            (void*)EnKarebaba_Regrow, (void*)EnKarebaba_Upright,
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

    void CurrentColliderRoles(u8* bodyRoles, u8* headRoles) const {
        EnKarebabaActionFunc af = Typed()->actionFunc;
        *bodyRoles = 0;
        *headRoles = 0;

        if (af == EnKarebaba_Dead)
            return;

        if (af != EnKarebaba_Dying && af != EnKarebaba_DeadItemDrop) {
            *headRoles |= COLL_OC;

            if (af != EnKarebaba_Regrow && af != EnKarebaba_Grow) {
                *headRoles |= COLL_AT;
                *bodyRoles |= COLL_AC;
            }
        }
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnKarebaba* kb = Typed();

        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        u8 acConfig = kb->bodyCollider.base.acFlags & ~(AC_HIT | AC_BOUNCED);
        PackProperty(PROP_BODY_COLL, ByteStream()
                                                << PackedInt2(kb->bodyCollider.dim.radius)
                                                << PackedInt2(kb->bodyCollider.dim.height)
                                                << PackedUInt1(kb->bodyCollider.base.colType) << PackedUInt1(acConfig)
                                                << PackedUInt4(kb->bodyCollider.info.bumper.dmgFlags), out);

        PackProperty(PROP_HEAD_COLL, ByteStream() << PackedInt2(kb->headCollider.dim.radius)
                                                         << PackedInt2(kb->headCollider.dim.height), out);
        PackProperty(PROP_PARAMS, PackedInt2(kb->actor.params), out);

        u8 bodyRoles, headRoles;
        CurrentColliderRoles(&bodyRoles, &headRoles);
        PackProperty(PROP_COLL_ROLES, ByteStream() << PackedUInt1(bodyRoles) << PackedUInt1(headRoles), out);
        PackProperty(PROP_SHADOW_SCALE, PackedFloat4(kb->actor.shape.shadowScale), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(kb->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(0, &kb->skelAnime), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnKarebaba* kb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != 0xFF) {
                    size_t n;
                    const void* const* t = ActionTable(&n);
                    if (ai < n)
                        kb->actionFunc = reinterpret_cast<EnKarebabaActionFunc>(const_cast<void*>(t[ai]));
                }
                return true;
            }
            case PROP_ANIM_CUR_FRAME:
                kb->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                return true;
            case PROP_ANIM: {
                data.Read<PackedUInt1>();
                ApplyAnimProperty(nullptr, &kb->skelAnime, LOCK_CUR_FRAME ? kb->skelAnime.curFrame : 0.0f, data);
                return true;
            }
            case PROP_BODY_COLL: {
                kb->bodyCollider.dim.radius = (s16)(data.Read<PackedInt2>().value());
                kb->bodyCollider.dim.height = (s16)(data.Read<PackedInt2>().value());
                kb->bodyCollider.base.colType = (u8)(data.Read<PackedUInt1>().value());
                u8 acConfig = (u8)(data.Read<PackedUInt1>().value());
                kb->bodyCollider.base.acFlags = (kb->bodyCollider.base.acFlags & (AC_HIT | AC_BOUNCED)) | acConfig;
                kb->bodyCollider.info.bumper.dmgFlags = data.Read<PackedUInt4>().value();
                return true;
            }

            case PROP_HEAD_COLL: {
                kb->headCollider.dim.radius = (s16)(data.Read<PackedInt2>().value());
                kb->headCollider.dim.height = (s16)(data.Read<PackedInt2>().value());
                return true;
            }
            case PROP_COLL_ROLES: {
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_headRoles = (u8)(data.Read<PackedUInt1>().value());
                return true;
            }

            case PROP_PARAMS:
                kb->actor.params = (s16)(data.Read<PackedInt2>().value());
                return true;

            case PROP_SHADOW_SCALE:
                kb->actor.shape.shadowScale = data.Read<PackedFloat4>().value();
                return true;
            default:
                return false;
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnKarebaba* kb = Typed();

        UpdateAnimation(&kb->skelAnime, LOCK_CUR_FRAME);

        if (kb->bodyCollider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        if (kb->actor.xzDistToPlayer < 240.0f && IsLocalPlayerClosest()) {
            ClaimLeadership(CLAIM_REASON_PROXIMITY);
        }

        if (kb->actionFunc != EnKarebaba_Dying && kb->actionFunc != EnKarebaba_DeadItemDrop) {
            Actor_SetFocus(&kb->actor, (kb->actor.scale.x * 10.0f) / 0.01f);
        }

        RegisterColliderBase(play, &kb->bodyCollider.base, m_bodyRoles);
        RegisterColliderBase(play, &kb->headCollider.base, m_headRoles);
    }

  private:
    u8 m_bodyRoles = 0;
    u8 m_headRoles = 0;
};

}

#endif
