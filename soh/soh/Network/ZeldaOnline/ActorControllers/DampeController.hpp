#ifndef GRAVEDIGGERCONTROLLERH
#define GRAVEDIGGERCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Tk/z_en_tk.h"
#include "objects/object_tk/object_tk.h"

void EnTk_Rest(EnTk* tk, PlayState* play);
void EnTk_Walk(EnTk* tk, PlayState* play);
void EnTk_Dig(EnTk* tk, PlayState* play);

void EnTk_RestAnim(EnTk* tk, PlayState* play);
void EnTk_WalkAnim(EnTk* tk, PlayState* play);
void EnTk_DigAnim(EnTk* tk, PlayState* play);

void EnTk_UpdateEyes(EnTk* tk);
void EnTk_CheckCurrentSpot(EnTk* tk);

u16 EnTk_GetTextId(PlayState* play, Actor* thisx);
s16 EnTk_UpdateTalkState(PlayState* play, Actor* thisx);
}

namespace ZeldaOnline {

class DampeController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTk* Typed() const {
        return reinterpret_cast<EnTk*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using TkActionFunc = void (*)(EnTk*, PlayState*);
    static const TkActionFunc* ActionTable(size_t* count) {
        static const TkActionFunc sTable[] = {
            EnTk_Rest,
            EnTk_Walk,
            EnTk_Dig,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TkActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_REST = 0;
    static constexpr u8 ANIM_WALK = 1;
    static constexpr u8 ANIM_DIG = 2;


    void ApplyAnimForAction(PlayState* play, u8 id) {
        EnTk* tk = Typed();

        switch (id) {
            case 0:
                EnTk_RestAnim(tk, play);
                break;
            case 1:
                EnTk_WalkAnim(tk, play);
                break;
            case 2:
                EnTk_DigAnim(tk, play);
                break;
            default:
                break;
        }
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_WAYPOINT,
        PROP_TIMERS,
        PROP_HEAD_ROT,
        PROP_DIG,
        PROP_REWARDS,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTk* tk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_WAYPOINT, PackedInt2(tk->currentWaypoint), out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(tk->actionCountdown) << PackedInt2(tk->rewardTimer)
                                  << PackedInt2(tk->h_21E),
                     out);
        PackProperty(PROP_HEAD_ROT, PackedInt2(tk->headRot), out);
        PackProperty(PROP_DIG,
                     ByteStream() << PackedUInt1(tk->validDigHere) << PackedInt4(tk->currentReward)
                                  << PackedUInt1(tk->heartPieceSpawned) << PackedFloat4(tk->v3f_304.x)
                                  << PackedFloat4(tk->v3f_304.y) << PackedFloat4(tk->v3f_304.z),
                     out);
        PackProperty(PROP_REWARDS,
                     ByteStream() << PackedUInt1(tk->rewardCount[0]) << PackedUInt1(tk->rewardCount[1])
                                  << PackedUInt1(tk->rewardCount[2]) << PackedUInt1(tk->rewardCount[3]),
                     out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentActionIndex(), &tk->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTk* tk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TkActionFunc* table = ActionTable(&count);
                if (id < count && tk->actionFunc != table[id]) {
                    tk->actionFunc = table[id];
                    ApplyAnimForAction(gPlayState, id);
                }
                break;
            }
            case PROP_WAYPOINT:
                tk->currentWaypoint = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                tk->actionCountdown = (s16)(data.Read<PackedInt2>().value());
                tk->rewardTimer = (s16)(data.Read<PackedInt2>().value());
                tk->h_21E = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEAD_ROT:
                tk->headRot = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_DIG:
                tk->validDigHere = (u8)(data.Read<PackedUInt1>().value());
                tk->currentReward = data.Read<PackedInt4>().value();
                tk->heartPieceSpawned = (u8)(data.Read<PackedUInt1>().value());
                tk->v3f_304.x = data.Read<PackedFloat4>().value();
                tk->v3f_304.y = data.Read<PackedFloat4>().value();
                tk->v3f_304.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_REWARDS:
                tk->rewardCount[0] = (u8)(data.Read<PackedUInt1>().value());
                tk->rewardCount[1] = (u8)(data.Read<PackedUInt1>().value());
                tk->rewardCount[2] = (u8)(data.Read<PackedUInt1>().value());
                tk->rewardCount[3] = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM:
                data.Read<PackedUInt1>(); 
                ApplyAnimProperty(nullptr, &tk->skelAnime, LOCK_CUR_FRAME ? tk->skelAnime.curFrame : 0.0f, data);
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnTk* tk = Typed();

        UpdateAnimation(&tk->skelAnime, LOCK_CUR_FRAME);

        if (GET_PLAYER(play)->talkActor == &tk->actor) {
            ClaimLeadership(CLAIM_REASON_NOW);
        }

        Npc_UpdateTalking(play, &tk->actor, &tk->interactInfo.talkState, tk->collider.dim.radius + 30.0f,
                          EnTk_GetTextId, EnTk_UpdateTalkState);

        m_conversationHandled = true;

        EnTk_UpdateEyes(tk);
        EnTk_CheckCurrentSpot(tk);

        Collider_UpdateCylinder(&tk->actor, &tk->collider);
        RegisterColliderBase(play, &tk->collider.base, COLL_OC);
    }
};

} // namespace ZeldaOnline

#endif