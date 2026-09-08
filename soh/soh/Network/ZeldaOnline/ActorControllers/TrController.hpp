#ifndef TRCONTROLLERH
#define TRCONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Tr/z_en_tr.h"
#include "objects/object_tr/object_tr.h"

void EnTr_DoNothing(EnTr*, PlayState* play);
void EnTr_ChooseAction1(EnTr*, PlayState* play);
void EnTr_ChooseAction2(EnTr*, PlayState* play);
void EnTr_CrySpellcast(EnTr*, PlayState* play);
void EnTr_FlyKidnapCutscene(EnTr*, PlayState* play);
void EnTr_ShrinkVanish(EnTr*, PlayState* play);
void EnTr_WaitToReappear(EnTr*, PlayState* play);
void EnTr_Reappear(EnTr*, PlayState* play);
void EnTr_TakeOff(EnTr*, PlayState* play);
void EnTr_TurnLookOverShoulder(EnTr*, PlayState* play);
}

namespace ZeldaOnline {

class TrController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnTr* Typed() const {
        return reinterpret_cast<EnTr*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    bool IsAnchor() const {
        return Typed()->actor.params == TR_KOUME;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 8;

    using TrActionFunc = void (*)(EnTr*, PlayState*);
    static const TrActionFunc* ActionTable(size_t* count) {
        static const TrActionFunc sTable[] = {
            EnTr_DoNothing,   EnTr_ChooseAction1, EnTr_ChooseAction2, EnTr_CrySpellcast,       EnTr_FlyKidnapCutscene,
            EnTr_ShrinkVanish, EnTr_WaitToReappear, EnTr_Reappear,     EnTr_TurnLookOverShoulder, EnTr_TakeOff,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const TrActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gKotakeKoumeStandingBroomOverRightShoulderAnim,
            gKotakeKoumeStandingBroomOverLeftShoulderAnim,
            gKotakeKoumeLookOverRightShoulderAnim,
            gKotakeKoumeLookOverLeftShoulderAnim,
            gKotakeKoumeLookingOverRightShoulderAnim,
            gKotakeKoumeLookingOverLeftShoulderAnim,
            gKotakeKoumeCastMagicAnim,
            gKotakeKoumeFlyAnim,
        };
        if (i >= ANIM_COUNT)
            return nullptr;
        return sAnims[i];
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++)
            if (strcmp(AnimForIndex(i), cur) == 0)
                return i;
        return ANIM_UNKNOWN;
    }

    void DragOthers(PlayState* play) {
        if (play->csCtx.state == CS_STATE_IDLE || m_actor->draw == NULL) {
            return;
        }

        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_BOSS].head; it != nullptr; it = it->next) {
            if (it == m_actor || it->zoController == nullptr)
                continue;
            if (it->id != ACTOR_EN_TR && it->id != ACTOR_EN_IK)
                continue;

            auto* other = reinterpret_cast<AbstractActorController*>(it->zoController);
            if (!other->IsLeader())
                other->ClaimLeadership(CLAIM_REASON_NOW);
        }

        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].head; it != nullptr; it = it->next) {
            if (it->id != ACTOR_EN_IK || it->zoController == nullptr)
                continue;

            auto* other = reinterpret_cast<AbstractActorController*>(it->zoController);
            if (!other->IsLeader())
                other->ClaimLeadership(CLAIM_REASON_NOW);
        }
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnTr* tr = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt2(tr->timer) << PackedInt2(tr->actionIndex) << PackedInt2(tr->unk_2D4),
                     out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(tr->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &tr->skelAnime), out);

        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnTr* tr = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const TrActionFunc* table = ActionTable(&count);
                if (id < count)
                    tr->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                tr->timer = (u16)(data.Read<PackedUInt2>().value());
                tr->actionIndex = (s16)(data.Read<PackedInt2>().value());
                tr->unk_2D4 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                tr->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &tr->skelAnime, LOCK_CUR_FRAME ? tr->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdateLeader(PlayState* play) override {
        if (IsAnchor())
            DragOthers(play);

        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnTr* tr = Typed();

        UpdateAnimation(&tr->skelAnime, LOCK_CUR_FRAME);

        if (tr->blinkTimer == 0) {
            tr->blinkTimer = (s16)Rand_ZeroFloat(60.0f) + 20;
            tr->eyeIndex = 0;
        } else {
            tr->blinkTimer--;
            tr->eyeIndex = (tr->blinkTimer < 3) ? (3 - tr->blinkTimer) : 0;
        }
    }
};

} // namespace ZeldaOnline

#endif
