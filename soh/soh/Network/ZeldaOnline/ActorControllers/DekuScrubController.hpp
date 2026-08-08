#ifndef DEKUSCRUBCONTROLLERH
#define DEKUSCRUBCONTROLLERH

#include <cstring>
#include "../AbstractActorController.hpp"
#include "src/overlays/actors/ovl_En_Dekunuts/z_en_dekunuts.h"

#include "objects/object_dekunuts/object_dekunuts.h"

extern "C" {
void EnDekunuts_Wait(EnDekunuts*, PlayState*);
void EnDekunuts_LookAround(EnDekunuts*, PlayState*);
void EnDekunuts_Stand(EnDekunuts*, PlayState*);
void EnDekunuts_ThrowNut(EnDekunuts*, PlayState*);
void EnDekunuts_Gasp(EnDekunuts*, PlayState*);
void EnDekunuts_BeginRun(EnDekunuts*, PlayState*);
void EnDekunuts_Run(EnDekunuts*, PlayState*);
void EnDekunuts_Burrow(EnDekunuts*, PlayState*);
void EnDekunuts_BeDamaged(EnDekunuts*, PlayState*);
void EnDekunuts_BeStunned(EnDekunuts*, PlayState*);
void EnDekunuts_Die(EnDekunuts*, PlayState*);
void EnDekunuts_SetupDie(EnDekunuts* );
}

namespace ZeldaOnline {

class DekuScrubController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDekunuts* Typed() const {
        return reinterpret_cast<EnDekunuts*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return params != 10;
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_COLL,
        PROP_COLL_ROLES,
        PROP_HEALTH,
        PROP_ANIM_FLAG_TIMER,
        PROP_RUN_DIRECTION,
        PROP_RUN_AWAY_COUNT,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    static const void* const* ActionTable(size_t* count) {
        static const void* const sTable[] = {
            (void*)EnDekunuts_Wait,      (void*)EnDekunuts_LookAround, (void*)EnDekunuts_Stand,
            (void*)EnDekunuts_ThrowNut,  (void*)EnDekunuts_Gasp,       (void*)EnDekunuts_BeginRun,
            (void*)EnDekunuts_Run,       (void*)EnDekunuts_Burrow,     (void*)EnDekunuts_BeDamaged,
            (void*)EnDekunuts_BeStunned, (void*)EnDekunuts_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    u8 CurrentActionIndex() const {
        size_t n;
        const void* const* t = ActionTable(&n);
        for (size_t i = 0; i < n; i++)
            if (t[i] == (void*)Typed()->actionFunc)
                return (u8)(i);
        return 0xFF;
    }

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 1:
                return gDekuNutsUpAnim;
            case 2:
                return gDekuNutsLookAroundAnim;
            case 3:
                return gDekuNutsStandAnim;
            case 4:
                return gDekuNutsSpitAnim;
            case 5:
                return gDekuNutsGaspAnim;
            case 6:
                return gDekuNutsUnburrowAnim;
            case 7:
                return gDekuNutsRunAnim;
            case 8:
                return gDekuNutsBurrowAnim;
            case 9:
                return gDekuNutsDamageAnim;
            case 10:
                return gDekuNutsDieAnim;
            default:
                return gDekuNutsStandAnim;
        }
    }

    u8 CurrentAnimIndex() const {
        const char* a = (const char*)Typed()->skelAnime.animation;
        if (!a)
            return 0;
        if (strcmp(a, gDekuNutsUpAnim) == 0)
            return 1;
        if (strcmp(a, gDekuNutsLookAroundAnim) == 0)
            return 2;
        if (strcmp(a, gDekuNutsStandAnim) == 0)
            return 3;
        if (strcmp(a, gDekuNutsSpitAnim) == 0)
            return 4;
        if (strcmp(a, gDekuNutsGaspAnim) == 0)
            return 5;
        if (strcmp(a, gDekuNutsUnburrowAnim) == 0)
            return 6;
        if (strcmp(a, gDekuNutsRunAnim) == 0)
            return 7;
        if (strcmp(a, gDekuNutsBurrowAnim) == 0)
            return 8;
        if (strcmp(a, gDekuNutsDamageAnim) == 0)
            return 9;
        if (strcmp(a, gDekuNutsDieAnim) == 0)
            return 10;
        return 0;
    }

    u8 CurrentColliderRoles() const {
        u8 roles = COLL_OC;
        if (Typed()->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        return roles;
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnDekunuts* scrub = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        u8 acConfig = scrub->collider.base.acFlags & ~(AC_HIT | AC_BOUNCED);
        PackProperty(PROP_COLL, ByteStream() << PackedUInt1(acConfig)
                                                    << PackedInt2((s16)(scrub->actor.colChkInfo.mass))
                                                    << PackedInt2((s16)(scrub->collider.dim.height)), out);

        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        PackProperty(PROP_HEALTH, PackedUInt1(scrub->actor.colChkInfo.health), out);
        PackProperty(PROP_ANIM_FLAG_TIMER, PackedInt2(scrub->animFlagAndTimer), out);
        PackProperty(PROP_RUN_DIRECTION, PackedInt2(scrub->runDirection), out);
        PackProperty(PROP_RUN_AWAY_COUNT, PackedUInt1(scrub->runAwayCount), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(scrub->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &scrub->skelAnime), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDekunuts* scrub = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != 0xFF) {
                    size_t n;
                    const void* const* t = ActionTable(&n);
                    if (ai < n)
                        scrub->actionFunc = reinterpret_cast<EnDekunutsActionFunc>(const_cast<void*>(t[ai]));
                }
                return true;
            }
            case PROP_ANIM_CUR_FRAME:
                scrub->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                return true;
            case PROP_ANIM: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                ApplyAnimProperty((void*)AnimForIndex(ai), &scrub->skelAnime,
                                 LOCK_CUR_FRAME ? scrub->skelAnime.curFrame : 0.0f, data);
                return true;
            }
            case PROP_COLL: {
                u8 acConfig = (u8)(data.Read<PackedUInt1>().value());
                scrub->collider.base.acFlags = (scrub->collider.base.acFlags & (AC_HIT | AC_BOUNCED)) | acConfig;
                scrub->actor.colChkInfo.mass = (u8)(data.Read<PackedInt2>().value());
                scrub->collider.dim.height = (s16)(data.Read<PackedInt2>().value());
                return true;
            }
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_HEALTH:
                scrub->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_ANIM_FLAG_TIMER:
                scrub->animFlagAndTimer = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_RUN_DIRECTION:
                scrub->runDirection = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_RUN_AWAY_COUNT:
                scrub->runAwayCount = (u8)(data.Read<PackedUInt1>().value());
                return true;
            default:
                return false;
        }
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnDekunuts_Die) {
            EnDekunuts_SetupDie(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnDekunuts* scrub = Typed();

        UpdateAnimation(&scrub->skelAnime, LOCK_CUR_FRAME);

        if (scrub->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        u8 id = CurrentActionIndex();
        bool claimable = (id <= 3) || (id == 0xFF);
        if (claimable && scrub->actor.xzDistToPlayer < 200.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        if (scrub->actionFunc == EnDekunuts_Wait) {
            Actor_SetFocus(&scrub->actor, scrub->skelAnime.curFrame);
        } else if (scrub->actionFunc == EnDekunuts_Burrow) {
            Actor_SetFocus(&scrub->actor, 20.0f - ((scrub->skelAnime.curFrame * 20.0f) /
                                                   Animation_GetLastFrame((void*) &gDekuNutsBurrowAnim)));
        } else {
            Actor_SetFocus(&scrub->actor, 20.0f);
        }

        RegisterCylinder(play, &scrub->collider, m_roles);
    }

  private:

    u8 m_roles = COLL_OC;
};

}

#endif
