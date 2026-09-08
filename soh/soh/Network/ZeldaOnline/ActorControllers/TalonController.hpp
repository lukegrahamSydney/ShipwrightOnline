#ifndef TALONCONTROLLERH
#define TALONCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ta/z_en_ta.h"
#include "objects/object_ta/object_ta.h"

void EnTa_IdleAsleepInCastle(EnTa* ta, PlayState* play);
void EnTa_IdleAsleepInLonLonHouse(EnTa* ta, PlayState* play);
void EnTa_IdleAsleepInKakariko(EnTa* ta, PlayState* play);
void EnTa_IdleAwakeInCastle(EnTa* ta, PlayState* play);
void EnTa_IdleAwakeInKakariko(EnTa* ta, PlayState* play);
void EnTa_IdleAtRanch(EnTa* ta, PlayState* play);
void EnTa_IdleSittingInLonLonHouse(EnTa* ta, PlayState* play);
void EnTa_IdleAfterCuccoGameFinished(EnTa* ta, PlayState* play);
void EnTa_RunCuccoGame(EnTa* ta, PlayState* play);
void EnTa_RunAwayStart(EnTa* ta, PlayState* play);
void EnTa_RunAwayRunSouth(EnTa* ta, PlayState* play);
void EnTa_RunAwayTurnWest(EnTa* ta, PlayState* play);
void EnTa_RunAwayRunWest(EnTa* ta, PlayState* play);
void EnTa_RunAwayTurnTowardsGate(EnTa* ta, PlayState* play);
void EnTa_RunAwayRunOutOfGate(EnTa* ta, PlayState* play);

void EnTa_AnimRepeatCurrent(EnTa* ta);
void EnTa_AnimSleeping(EnTa* ta);
void EnTa_AnimSitSleeping(EnTa* ta);
void EnTa_AnimRunToEnd(EnTa* ta);

void EnTa_BlinkWaitUntilNext(EnTa* ta);
void EnTa_BlinkAdvanceState(EnTa* ta);

s32 func_80038290(PlayState* play, Actor* actor, Vec3s* arg2, Vec3s* arg3, Vec3f arg4);
}

namespace ZeldaOnline {

class TalonController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTa* Typed() const {
        return reinterpret_cast<EnTa*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    static bool IsNetworkedVariant(s16 params) {
        return gPlayState->sceneNum == SCENE_HYRULE_CASTLE;
    }

    virtual bool ShouldLockActor() const {
        return IsMyOpenTextboxActor();
    }


  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_COUNT = 8;

    using TaAction = void (*)(EnTa*, PlayState*);
    static const TaAction* ActionTable(size_t* count) {
        static const TaAction sTable[] = {
            EnTa_IdleAsleepInCastle,
            EnTa_IdleAsleepInLonLonHouse,
            EnTa_IdleAsleepInKakariko,
            EnTa_IdleAwakeInCastle,
            EnTa_IdleAwakeInKakariko,
            EnTa_IdleAtRanch,
            EnTa_IdleSittingInLonLonHouse,
            EnTa_IdleAfterCuccoGameFinished,
            EnTa_RunCuccoGame,
            EnTa_RunAwayStart,
            EnTa_RunAwayRunSouth,
            EnTa_RunAwayTurnWest,
            EnTa_RunAwayRunWest,
            EnTa_RunAwayTurnTowardsGate,
            EnTa_RunAwayRunOutOfGate,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    using TaAnim = void (*)(EnTa*);
    static const TaAnim* AnimFuncTable(size_t* count) {
        static const TaAnim sTable[] = {
            EnTa_AnimRepeatCurrent,
            EnTa_AnimSleeping,
            EnTa_AnimSitSleeping,
            EnTa_AnimRunToEnd,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    using TaBlink = void (*)(EnTa*);
    static const TaBlink* BlinkFuncTable(size_t* count) {
        static const TaBlink sTable[] = {
            EnTa_BlinkWaitUntilNext,
            EnTa_BlinkAdvanceState,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gTalonStandAnim;
            case 1:
                return gTalonSleepAnim;
            case 2:
                return gTalonWakeUpAnim;
            case 3:
                return gTalonSitSleepingAnim;
            case 4:
                return gTalonSitWakeUpAnim;
            case 5:
                return gTalonSitHandsUpAnim;
            case 6:
                return gTalonRunTransitionAnim;
            case 7:
                return gTalonRunAnim;
            default:
                return gTalonStandAnim;
        }
    }

    static u8 IndexForAnim(const char* anim) {
        if (anim == nullptr)
            return ID_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++)
            if (strcmp(anim, AnimForIndex(i)) == 0)
                return i;
        return ID_UNKNOWN;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TaAction* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentAnimFuncIndex() const {
        size_t count;
        const TaAnim* table = AnimFuncTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->animFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentBlinkFuncIndex() const {
        size_t count;
        const TaBlink* table = BlinkFuncTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->blinkFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentAnimIndex() const {
        return IndexForAnim((const char*)Typed()->skelAnime.animation);
    }

    u8 CurrentAnimationFieldIndex() const {
        return IndexForAnim((const char*)Typed()->currentAnimation);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ANIM_FUNC,
        PROP_TIMER,
        PROP_CURRENT_ANIMATION,
        PROP_EYE_INDEX,
        PROP_STATE_FLAGS,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTa* ta = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ANIM_FUNC, PackedUInt1(CurrentAnimFuncIndex()), out);
        PackProperty(PROP_TIMER, ByteStream() << PackedInt2(ta->timer) << PackedInt2(ta->nodOffTimer), out);
        PackProperty(PROP_CURRENT_ANIMATION, PackedUInt1(CurrentAnimationFieldIndex()), out);
        PackProperty(PROP_EYE_INDEX, PackedInt2(ta->eyeIndex), out);
        PackProperty(PROP_STATE_FLAGS, PackedUInt2(ta->stateFlags), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &ta->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTa* ta = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TaAction* table = ActionTable(&count);
                if (id < count)
                    ta->actionFunc = table[id];
                break;
            }
            case PROP_ANIM_FUNC: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TaAnim* table = AnimFuncTable(&count);
                if (id < count)
                    ta->animFunc = table[id];
                break;
            }

            case PROP_TIMER:
                ta->timer = (s16)(data.Read<PackedInt2>().value());
                ta->nodOffTimer = (s16)(data.Read<PackedInt2>().value());
                break;

            case PROP_CURRENT_ANIMATION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                ta->currentAnimation = (AnimationHeader*)AnimForIndex(id);
                break;
            }

            case PROP_EYE_INDEX:
                ta->eyeIndex = (s16)(data.Read<PackedInt2>().value());
                break;

            case PROP_STATE_FLAGS:
                ta->stateFlags = (u16)(data.Read<PackedUInt2>().value());
                break;

            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                ApplyAnimProperty((void*)AnimForIndex(id), &ta->skelAnime,
                                  LOCK_CUR_FRAME ? ta->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnTa* ta = Typed();

        if (ta->actor.xzDistToPlayer < 150.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        ta->animFunc(ta);

        Collider_UpdateCylinder(&ta->actor, &ta->collider);
        RegisterColliderBase(play, &ta->collider.base, COLL_AC | COLL_OC);

        if (m_trackLocalPlayer) {
            func_80038290(play, &ta->actor, &ta->headRot, &ta->torsoRot, ta->actor.focus.pos);
        } else {
            Math_SmoothStepToS(&ta->headRot.x, 0, 6, 6200, 100);
            Math_SmoothStepToS(&ta->headRot.y, 0, 6, 6200, 100);
            Math_SmoothStepToS(&ta->torsoRot.x, 0, 6, 6200, 100);
            Math_SmoothStepToS(&ta->torsoRot.y, 0, 6, 6200, 100);
        }

        m_trackLocalPlayer = ta->actor.xzDistToPlayer < 170.0f;
    }

  private:
    bool m_trackLocalPlayer = false;
};

} // namespace ZeldaOnline

#endif