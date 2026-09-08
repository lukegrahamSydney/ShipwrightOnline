#ifndef GE2CONTROLLERH
#define GE2CONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ge2/z_en_ge2.h"
#include "objects/object_gla/object_gla.h"

void EnGe2_ChangeAction(EnGe2* thisx, s32 i);
void EnGe2_Walk(EnGe2* thisx, PlayState* play);
void EnGe2_AboutTurn(EnGe2* thisx, PlayState* play);
void EnGe2_TurnPlayerSpotted(EnGe2* thisx, PlayState* play);
void EnGe2_KnockedOut(EnGe2* thisx, PlayState* play);
void EnGe2_CaptureTurn(EnGe2* thisx, PlayState* play);
void EnGe2_CaptureCharge(EnGe2* thisx, PlayState* play);
void EnGe2_CaptureClose(EnGe2* thisx, PlayState* play);
void EnGe2_Stand(EnGe2* thisx, PlayState* play);
void EnGe2_WaitLookAtPlayer(EnGe2* thisx, PlayState* play);
}

namespace ZeldaOnline {

class Ge2Controller : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnGe2* Typed() const {
        return reinterpret_cast<EnGe2*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    // GE2_STATE_*, GE2_ACTION_* and GE2_TYPE_* are defined in z_en_ge2.c, not the header
    static constexpr u16 STATE_ANIMCOMPLETE = (1 << 1);
    static constexpr u16 STATE_KO = (1 << 2);
    static constexpr u16 STATE_CAPTURING = (1 << 3);
    static constexpr u16 STATE_TALKED = (1 << 4);

    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_WALK = 0;
    static constexpr u8 ID_ABOUTTURN = 1;
    static constexpr u8 ID_TURNPLAYERSPOTTED = 2;
    static constexpr u8 ID_KNOCKEDOUT = 3;
    static constexpr u8 ID_CAPTURETURN = 4;
    static constexpr u8 ID_CAPTURECHARGE = 5;
    static constexpr u8 ID_CAPTURECLOSE = 6;
    static constexpr u8 ID_STAND = 7;
    static constexpr u8 ID_WAITLOOKATPLAYER = 8;

    static constexpr u8 TYPE_PATROLLING = 0;
    static constexpr u8 TYPE_STATIONARY = 1;
    static constexpr u8 TYPE_GERUDO_CARD_GIVER = 2;

    u8 Type() const {
        return (u8)(Typed()->actor.params & 0xFF);
    }

    using Ge2ActionFunc = void (*)(EnGe2*, PlayState*);
    static const Ge2ActionFunc* ActionTable(size_t* count) {
        static const Ge2ActionFunc sTable[] = {
            EnGe2_Walk,         EnGe2_AboutTurn,   EnGe2_TurnPlayerSpotted,
            EnGe2_KnockedOut,   EnGe2_CaptureTurn, EnGe2_CaptureCharge,
            EnGe2_CaptureClose, EnGe2_Stand,       EnGe2_WaitLookAtPlayer,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const Ge2ActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    bool IsCapturing() const {
        EnGe2* ge = Typed();
        return (ge->stateFlags & STATE_CAPTURING) || ge->actionFunc == EnGe2_CaptureTurn ||
               ge->actionFunc == EnGe2_CaptureCharge || ge->actionFunc == EnGe2_CaptureClose;
    }

    u8 CalmActionIndex() const {
        if (!IsCapturing())
            return CurrentActionIndex();
        return (Type() == TYPE_STATIONARY) ? ID_STAND : ID_WALK;
    }

    u16 CalmStateFlags() const {
        return (u16)(Typed()->stateFlags & ~STATE_CAPTURING);
    }

    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gGerudoPurpleWalkingAnim,         gGerudoPurpleLookingAboutAnim, gGerudoPurpleLookingAboutAnim,
            gGerudoPurpleFallingToGroundAnim, gGerudoPurpleLookingAboutAnim, gGerudoPurpleChargingAnim,
            gGerudoPurpleLookingAboutAnim,    gGerudoPurpleLookingAboutAnim, gGerudoPurpleLookingAboutAnim,
        };
        if (i >= (sizeof(sAnims) / sizeof(sAnims[0])))
            return gGerudoPurpleLookingAboutAnim;
        return sAnims[i];
    }

    u8 CurrentAnimIndex() const {
        return CalmActionIndex();
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_WALK,
        PROP_HEAD,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnGe2* ge = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CalmActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt2(CalmStateFlags()) << PackedUInt1(ge->timer)
                                  << PackedUInt1(ge->playerSpottedParam),
                     out);
        PackProperty(PROP_WALK,
                     ByteStream() << PackedInt2(ge->walkDirection) << PackedUInt2(ge->walkTimer)
                                  << PackedInt2(ge->yawTowardsPlayer),
                     out);
        PackProperty(PROP_HEAD,
                     ByteStream() << PackedInt2(ge->headRot.x) << PackedInt2(ge->headRot.y) << PackedInt2(ge->headRot.z)
                                  << PackedInt2(ge->unk_2EE.x) << PackedInt2(ge->unk_2EE.y)
                                  << PackedInt2(ge->unk_2EE.z),
                     out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(ge->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &ge->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnGe2* ge = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const Ge2ActionFunc* table = ActionTable(&count);
                if (id < count)
                    ge->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                ge->stateFlags = (u16)(data.Read<PackedUInt2>().value());
                ge->timer = (u8)(data.Read<PackedUInt1>().value());
                ge->playerSpottedParam = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_WALK:
                ge->walkDirection = (s16)(data.Read<PackedInt2>().value());
                ge->walkTimer = (u16)(data.Read<PackedUInt2>().value());
                ge->yawTowardsPlayer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEAD:
                ge->headRot.x = (s16)(data.Read<PackedInt2>().value());
                ge->headRot.y = (s16)(data.Read<PackedInt2>().value());
                ge->headRot.z = (s16)(data.Read<PackedInt2>().value());
                ge->unk_2EE.x = (s16)(data.Read<PackedInt2>().value());
                ge->unk_2EE.y = (s16)(data.Read<PackedInt2>().value());
                ge->unk_2EE.z = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                ge->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                ApplyAnimProperty((void*)AnimForIndex(id), &ge->skelAnime,
                                  LOCK_CUR_FRAME ? ge->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }


    void OnActorInit() override {
        ReinstallUpdate();
    }

    void UpdateLeader(PlayState* play) override {
        bool wasCalm = !IsCapturing();

        AbstractActorController::UpdateLeader(play);

        if (wasCalm && IsCapturing()) {
            GoLocal();
        }

        ReinstallUpdate();
    }

    void OnBecomeLeader() override {
        EnGe2* ge = Typed();

        if (!IsCapturing())
            return;

        ge->stateFlags &= ~STATE_CAPTURING;
        ge->actor.speedXZ = 0.0f;
        EnGe2_ChangeAction(ge, (Type() == TYPE_STATIONARY) ? ID_STAND : ID_WALK);
    }

    void UpdatePuppet(PlayState* play) override {
        EnGe2* ge = Typed();

        UpdateAnimation(&ge->skelAnime, LOCK_CUR_FRAME);

        if (!IsCapturing() && !(ge->stateFlags & STATE_KO) && ge->actor.xzDistToPlayer < 300.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);
    }
};

}

#endif
