#ifndef HINTSCRUBCONTROLLERH
#define HINTSCRUBCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Hintnuts/z_en_hintnuts.h"
#include "objects/object_hintnuts/object_hintnuts.h"

extern s16 sPuzzleCounter;

void EnHintnuts_Wait(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_LookAround(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_Stand(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_ThrowNut(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_Burrow(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_BeginRun(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_Run(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_BeginFreeze(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_Freeze(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_Talk(EnHintnuts* hintnuts, PlayState* play);
void EnHintnuts_Leave(EnHintnuts* hintnuts, PlayState* play);
}

namespace ZeldaOnline {

class HintScrubController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnHintnuts* Typed() const {
        return reinterpret_cast<EnHintnuts*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return (params & 0xFF) != 0xA;
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

  protected:
    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_COLL,
        PROP_COLL_ROLES,
        PROP_HEALTH,
        PROP_ANIM_FLAG_TIMER,
        PROP_TURN_YAW,
        PROP_PUZZLE_COUNTER,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    static const void* const* ActionTable(size_t* count) {
        static const void* const sTable[] = {
            (void*)EnHintnuts_Wait,   (void*)EnHintnuts_LookAround,
            (void*)EnHintnuts_Stand,  (void*)EnHintnuts_ThrowNut,
            (void*)EnHintnuts_Burrow, (void*)EnHintnuts_BeginRun,
            (void*)EnHintnuts_Run,    (void*)EnHintnuts_BeginFreeze,
            (void*)EnHintnuts_Freeze, (void*)EnHintnuts_Talk,
            (void*)EnHintnuts_Leave,
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

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gHintNutsUpAnim;
            case 1:
                return gHintNutsLookAroundAnim;
            case 2:
                return gHintNutsStandAnim;
            case 3:
                return gHintNutsSpitAnim;
            case 4:
                return gHintNutsBurrowAnim;
            case 5:
                return gHintNutsUnburrowAnim;
            case 6:
                return gHintNutsRunAnim;
            case 7:
                return gHintNutsFreezeAnim;
            case 8:
                return gHintNutsTalkAnim;
            default:
                return gHintNutsStandAnim;
        }
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        for (u8 i = 0; i < 9; i++)
            if (cur == AnimForIndex(i) || (cur != nullptr && strcmp(cur, AnimForIndex(i)) == 0))
                return i;
        return 0xFF;
    }

    u8 CurrentColliderRoles() const {
        u8 roles = COLL_OC;
        if (Typed()->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        return roles;
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnHintnuts* scrub = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        u8 acConfig = scrub->collider.base.acFlags & ~(AC_HIT | AC_BOUNCED);
        PackProperty(PROP_COLL, ByteStream() << PackedUInt1(acConfig)
                                                    << PackedInt2((s16)(scrub->collider.dim.height)), out);

        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(scrub->actor.colChkInfo.health), out);
        PackProperty(PROP_ANIM_FLAG_TIMER, PackedInt2(scrub->animFlagAndTimer), out);
        PackProperty(PROP_TURN_YAW, PackedInt2(scrub->unk_196), out);

        if (m_puzzleCounter == sPuzzleCounter)
            PackProperty(PROP_PUZZLE_COUNTER, PackedInt2(m_puzzleCounter), out);
        else
            PackNullProperty(PROP_PUZZLE_COUNTER, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(scrub->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &scrub->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnHintnuts* scrub = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != 0xFF) {
                    size_t n;
                    const void* const* t = ActionTable(&n);
                    if (ai < n)
                        scrub->actionFunc = reinterpret_cast<EnHintnutsActionFunc>(const_cast<void*>(t[ai]));
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
            case PROP_TURN_YAW:
                scrub->unk_196 = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_PUZZLE_COUNTER:
                if (propLen != 0)
                    m_puzzleCounter = sPuzzleCounter = (s16)(data.Read<PackedInt2>().value());
                return true;
            default:
                return false;
        }
    }

    void UpdateLeader(PlayState* play) override {
        auto oldGlobalPuzzleCounter = sPuzzleCounter;
        AbstractActorController::UpdateLeader(play);

        //This deku changed it
        if (oldGlobalPuzzleCounter != sPuzzleCounter)
            m_puzzleCounter = sPuzzleCounter;       //Force us to send our value out
        
    }

    void UpdatePuppet(PlayState* play) override {
        EnHintnuts* scrub = Typed();

        UpdateAnimation(&scrub->skelAnime, LOCK_CUR_FRAME);

        if (scrub->actionFunc == EnHintnuts_Wait) {
            Actor_SetFocus(&scrub->actor, scrub->skelAnime.curFrame);
        } else if (scrub->actionFunc == EnHintnuts_Burrow) {
            Actor_SetFocus(&scrub->actor, 20.0f - ((scrub->skelAnime.curFrame * 20.0f) /
                                                   Animation_GetLastFrame((void*) &gHintNutsBurrowAnim)));
        } else {
            Actor_SetFocus(&scrub->actor, 20.0f);
        }

        if (scrub->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        u8 id = CurrentActionIndex();
        bool claimable = (id <= 3) || (id == 0xFF);
        if (claimable && scrub->actor.xzDistToPlayer < 250.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        RegisterCylinder(play, &scrub->collider, m_roles);
    }

  private:
    u8 m_roles = COLL_OC | COLL_AC;
    s16 m_puzzleCounter =-1;
};

}

#endif
