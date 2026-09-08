#ifndef DAIKUCONTROLLERH
#define DAIKUCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Daiku/z_en_daiku.h"

extern AnimationFrameCountInfo* gEnDaikuAnimationInfo;

void EnDaiku_TentIdle(EnDaiku* thisx, PlayState* play);
void EnDaiku_Jailed(EnDaiku* thisx, PlayState* play);
void EnDaiku_WaitFreedom(EnDaiku* thisx, PlayState* play);
void EnDaiku_InitEscape(EnDaiku* thisx, PlayState* play);
void EnDaiku_EscapeRotate(EnDaiku* thisx, PlayState* play);
void EnDaiku_EscapeRun(EnDaiku* thisx, PlayState* play);
}

namespace ZeldaOnline {

class DaikuController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDaiku* Typed() const {
        return reinterpret_cast<EnDaiku*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr s32 STATEFLAG_1 = (1 << 1);
    static constexpr s32 STATEFLAG_2 = (1 << 2);
    static constexpr s32 STATEFLAG_GERUDOFIGHTING = (1 << 3);
    static constexpr s32 STATEFLAG_GERUDODEFEATED = (1 << 4);

    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 5;
    static constexpr s32 ANIM_RUN = 3;

    using DaikuActionFunc = void (*)(EnDaiku*, PlayState*);
    static const DaikuActionFunc* ActionTable(size_t* count) {
        static const DaikuActionFunc sTable[] = {
            EnDaiku_TentIdle,   EnDaiku_Jailed,       EnDaiku_WaitFreedom,
            EnDaiku_InitEscape, EnDaiku_EscapeRotate, EnDaiku_EscapeRun,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DaikuActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static void* AnimForIndex(u8 i) {
        if (i >= ANIM_COUNT)
            return nullptr;
        return (void*)gEnDaikuAnimationInfo[i].animation;
    }

    u8 CurrentAnimIndex() const {
        s32 i = Typed()->currentAnimIndex;
        if (i < 0 || i >= ANIM_COUNT)
            return ANIM_UNKNOWN;
        return (u8)(i);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_PATH,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnDaiku* dk = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE, ByteStream() << PackedInt4(dk->stateFlags) << PackedInt4(dk->currentAnimIndex), out);
        PackProperty(PROP_PATH,
                     ByteStream() << PackedInt4(dk->waypoint) << PackedInt2(dk->rotYtowardsPath)
                                  << PackedFloat4(dk->runSpeed),
                     out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(dk->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &dk->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDaiku* dk = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const DaikuActionFunc* table = ActionTable(&count);
                if (id < count)
                    dk->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                dk->stateFlags = (s32)(data.Read<PackedInt4>().value());
                dk->currentAnimIndex = (s32)(data.Read<PackedInt4>().value());
                break;
            case PROP_PATH:
                dk->waypoint = (s32)(data.Read<PackedInt4>().value());
                dk->rotYtowardsPath = (s16)(data.Read<PackedInt2>().value());
                dk->runSpeed = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                void* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty(anim, &dk->skelAnime, LOCK_CUR_FRAME ? dk->skelAnime.curFrame : 0.0f, data);
                break;
            }
            case PROP_ANIM_CUR_FRAME:
                dk->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnDaiku* dk = Typed();
        Player* player = GET_PLAYER(play);

        UpdateAnimation(&dk->skelAnime, LOCK_CUR_FRAME);

        if (dk->actor.xzDistToPlayer < 500.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        if (dk->currentAnimIndex == ANIM_RUN) {
            s32 curFrame = (s32)dk->skelAnime.curFrame;
            if (curFrame == 6 || curFrame == 15)
                Audio_PlayActorSound2(&dk->actor, NA_SE_EN_MORIBLIN_WALK);
        }

        RegisterColliderBase(play, &dk->collider.base, COLL_OC);

        if (dk->stateFlags & STATEFLAG_1) {
            dk->interactInfo.trackPos.x = player->actor.focus.pos.x;
            dk->interactInfo.trackPos.y = player->actor.focus.pos.y;
            dk->interactInfo.trackPos.z = player->actor.focus.pos.z;

            Npc_TrackPoint(&dk->actor, &dk->interactInfo, 0,
                           (dk->stateFlags & STATEFLAG_2) ? NPC_TRACKING_FULL_BODY : NPC_TRACKING_HEAD_AND_TORSO);
        }

    }
};

}

#endif
