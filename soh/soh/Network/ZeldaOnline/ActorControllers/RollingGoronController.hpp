#ifndef ROLLINGGORONCONTROLLERH
#define ROLLINGGORONCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Go2/z_en_go2.h"

extern AnimationInfo* gEnGo2AnimationInfo;

void EnGo2_CurledUp(EnGo2* go, PlayState* play);
void EnGo2_GoronRollingBigContinueRolling(EnGo2* go, PlayState* play);
void EnGo2_ContinueRolling(EnGo2* go, PlayState* play);
void EnGo2_SlowRolling(EnGo2* go, PlayState* play);
void EnGo2_GroundRolling(EnGo2* go, PlayState* play);
void EnGo2_ReverseRolling(EnGo2* go, PlayState* play);
void EnGo2_GoronLinkStopRolling(EnGo2* go, PlayState* play);
void EnGo2_SetupGetItem(EnGo2* thisx, PlayState* play);
void EnGo2_SetGetItem(EnGo2* thisx, PlayState* play);
void EnGo2_CheckCollision(EnGo2* go, PlayState* play);
void EnGo2_GoronFireGenericAction(EnGo2* thisx, PlayState* play);
void EnGo2_GoronDmtBombFlowerAnimation(EnGo2* thisx, PlayState* play);
void EnGo2_BiggoronEyedrops(EnGo2* thisx, PlayState* play);
void EnGo2_GetDustData(EnGo2* go, s32 index);
}

namespace ZeldaOnline {

class RollingGoronController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnGo2* Typed() const {
        return reinterpret_cast<EnGo2*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        u8 t = params & 0x1F;
        return t == GORON_CITY_ROLLING_BIG || t == GORON_CITY_LINK || t == GORON_DMT_ROLLING_SMALL ||
               t == GORON_FIRE_GENERIC;
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 14;

    u8 GoronType() const {
        return (u8)(Typed()->actor.params & 0x1F);
    }

    bool IsRolling() const {
        u8 t = GoronType();
        return t == GORON_CITY_ROLLING_BIG || t == GORON_CITY_LINK || t == GORON_DMT_ROLLING_SMALL;
    }

    using Go2ActionFunc = void (*)(EnGo2*, PlayState*);
    static const Go2ActionFunc* ActionTable(size_t* count) {
        static const Go2ActionFunc sTable[] = {
            EnGo2_CurledUp,
            EnGo2_GoronRollingBigContinueRolling,
            EnGo2_ContinueRolling,
            EnGo2_SlowRolling,
            EnGo2_GroundRolling,
            EnGo2_ReverseRolling,
            EnGo2_SetupGetItem,
            EnGo2_GoronLinkStopRolling,
            EnGo2_GoronFireGenericAction,
            EnGo2_GoronDmtBombFlowerAnimation,
            EnGo2_BiggoronEyedrops,
            EnGo2_SetGetItem,
            func_80A46B40,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const Go2ActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static void* AnimForIndex(u8 i) {
        if (i >= ANIM_COUNT)
            return nullptr;
        return (void*)gEnGo2AnimationInfo[i].animation;
    }

    u8 CurrentAnimIndex() const {
        for (int i = 0; i < ANIM_COUNT; i++)
            if (Typed()->skelAnime.animation == gEnGo2AnimationInfo[i].animation)
                return (u8)(i);
        return ANIM_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ROLL_STATE,
        PROP_TIMERS,
        PROP_ALPHA,
        PROP_COLL_DIM,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnGo2* go = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ROLL_STATE,
                     ByteStream() << PackedInt1(go->waypoint) << PackedUInt1(go->reverse) << PackedUInt1(go->isAwake)
                                  << PackedUInt1(go->goronState) << PackedUInt1(go->unk_211),
                     out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(go->animTimer) << PackedInt2(go->unk_590) << PackedInt2(go->unk_59C),
                     out);
        PackProperty(PROP_ALPHA, PackedFloat4(go->alpha), out);
        PackProperty(PROP_COLL_DIM,
                     ByteStream() << PackedInt2(go->collider.dim.radius) << PackedInt2(go->collider.dim.height), out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(go->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &go->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnGo2* go = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const Go2ActionFunc* table = ActionTable(&count);
                if (id < count)
                    go->actionFunc = table[id];
                break;
            }
            case PROP_ROLL_STATE:
                go->waypoint = (s8)(data.Read<PackedInt1>().value());
                go->reverse = (u8)(data.Read<PackedUInt1>().value());
                go->isAwake = (u8)(data.Read<PackedUInt1>().value());
                go->goronState = (u8)(data.Read<PackedUInt1>().value());
                go->unk_211 = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                go->animTimer = (s16)(data.Read<PackedInt2>().value());
                go->unk_590 = (s16)(data.Read<PackedInt2>().value());
                go->unk_59C = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ALPHA:
                go->alpha = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_DIM:
                go->collider.dim.radius = (s16)(data.Read<PackedInt2>().value());
                go->collider.dim.height = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                void* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty(anim, &go->skelAnime, LOCK_CUR_FRAME ? go->skelAnime.curFrame : 0.0f, data);
                break;
            }

            case PROP_ANIM_CUR_FRAME:
                go->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnGo2* go = Typed();

        UpdateAnimation(&go->skelAnime, LOCK_CUR_FRAME);

        if (go->actor.xzDistToPlayer < 500.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        go->actor.shape.shadowAlpha = (u8)(go->alpha);

        if (go->actionFunc == EnGo2_GoronRollingBigContinueRolling) {
            if (Animation_OnFrame(&go->skelAnime, go->skelAnime.endFrame))
                EnGo2_GetDustData(go, 1);
        } else if (go->actionFunc == EnGo2_ContinueRolling) {
            EnGo2_GetDustData(go, 2);
        } else if (go->actionFunc == EnGo2_SlowRolling) {
            EnGo2_GetDustData(go, 3);
        } else if (go->actionFunc == EnGo2_GroundRolling) {
            EnGo2_GetDustData(go, 0);
        } else if (go->actionFunc == EnGo2_ReverseRolling) {
            if (go->actor.speedXZ >= 1.0f)
                EnGo2_GetDustData(go, 3);
        }

        EnGo2_CheckCollision(go, play);
    }

  private:
};

} // namespace ZeldaOnline

#endif