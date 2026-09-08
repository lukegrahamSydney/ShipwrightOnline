#ifndef RUTOCONTROLLERH
#define RUTOCONTROLLERH

//A bit of a difficult controller. I;ve had to replace the vanilla init function with one that doesnt call the Actor_Kills
//On the server side, there is special code to deal with ruto to make sure there is only ever one instance of her in the entire
//Jabu scene
#include <cstring>
#include "../AbstractActorController.hpp"
#include "BdanObjectsController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Ru1/z_en_ru1.h"
#include "objects/object_ru1/object_ru1.h"

void func_80AEC0B4(EnRu1*, PlayState*);
void func_80AEC100(EnRu1*, PlayState*);
void func_80AEC130(EnRu1*, PlayState*);
void func_80AEC17C(EnRu1*, PlayState*);
void func_80AEC1D4(EnRu1*, PlayState*);
void func_80AEC244(EnRu1*, PlayState*);
void func_80AEC2C0(EnRu1*, PlayState*);
void func_80AECA94(EnRu1*, PlayState*);
void func_80AECAB4(EnRu1*, PlayState*);
void func_80AECAD4(EnRu1*, PlayState*);
void func_80AECB18(EnRu1*, PlayState*);
void func_80AECB60(EnRu1*, PlayState*);
void func_80AECBB8(EnRu1*, PlayState*);
void func_80AECC1C(EnRu1*, PlayState*);
void func_80AECC84(EnRu1*, PlayState*);
void func_80AED304(EnRu1*, PlayState*);
void func_80AED324(EnRu1*, PlayState*);
void func_80AED344(EnRu1*, PlayState*);
void func_80AED374(EnRu1*, PlayState*);
void func_80AED3A4(EnRu1*, PlayState*);
void func_80AED3E0(EnRu1*, PlayState*);
void func_80AED414(EnRu1*, PlayState*);
void func_80AEF29C(EnRu1*, PlayState*);
void func_80AEF2AC(EnRu1*, PlayState*);
void func_80AEF2D0(EnRu1*, PlayState*);
void func_80AEF354(EnRu1*, PlayState*);
void func_80AEF3A8(EnRu1*, PlayState*);
void func_80AEEBD4(EnRu1*, PlayState*);
void func_80AEEC5C(EnRu1*, PlayState*);
void func_80AEECF0(EnRu1*, PlayState*);
void func_80AEED58(EnRu1*, PlayState*);
void func_80AEEDCC(EnRu1*, PlayState*);
void func_80AEEE34(EnRu1*, PlayState*);
void func_80AEEE9C(EnRu1*, PlayState*);
void func_80AEEF08(EnRu1*, PlayState*);
void func_80AEEF5C(EnRu1*, PlayState*);
void func_80AEF9D8(EnRu1*, PlayState*);
void func_80AEFA2C(EnRu1*, PlayState*);
void func_80AEFAAC(EnRu1*, PlayState*);
void func_80AEFB04(EnRu1*, PlayState*);
void func_80AEFB68(EnRu1*, PlayState*);
void func_80AEFCE8(EnRu1*, PlayState*);
void func_80AEFBC8(EnRu1*, PlayState*);
void func_80AEFC24(EnRu1*, PlayState*);
void func_80AEFECC(EnRu1*, PlayState*);
void func_80AEFF40(EnRu1*, PlayState*);

s32 func_8002F2F4(Actor* actor, PlayState* play);
void func_80AEF2D0(EnRu1* thisx, PlayState* play);
void func_80AEF354(EnRu1* thisx, PlayState* play);
void func_80AEAD20(Actor* thisx, PlayState* play);

void func_80AEB264(EnRu1*, AnimationHeader* animation, u8 arg2, f32 transitionRate, s32 arg4);

void EnRu1_SetEyeIndex(EnRu1*, s16 eyeIndex);
void EnRu1_SetMouthIndex(EnRu1*, s16 mouthIndex);
void EnRu1_UpdateEyes(EnRu1*);

BgBdanObjects* EnRu1_FindSwitch(PlayState* play);
void func_80AEB0EC(EnRu1*, s32 cameraSetting);
}

namespace ZeldaOnline {

class RutoController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnRu1* Typed() const {
        return reinterpret_cast<EnRu1*>(m_actor);
    }

    static void RegisterHooks(s16 actorID, bool enabled)
    {
        COND_ID_HOOK(ShouldActorInit, ACTOR_EN_RU1, enabled, [&](void* actorRef, bool*) {
            Actor* actor = (Actor*)actorRef;

            AbstractActorController::InstallCustomInit(actor, RutoController::EnRu1_Init);
        });
    }

    static void EnRu1_Init(Actor* thisx, PlayState* play) {

        EnRu1* ru = reinterpret_cast<EnRu1*>(thisx);
        s16 rawParams = ru->actor.params;
        s16 params = rawParams & 0x00FF;
        bool hasMetRutoAlready = (rawParams & 0x0100) != 0;

        ActorShape_Init(&ru->actor.shape, 0.0f, ActorShadow_DrawCircle, 30.0f);
        SkelAnime_InitFlex(play, &ru->skelAnime, (FlexSkeletonHeader*)&gRutoChildSkel, NULL, ru->jointTable,
                           ru->morphTable, 17);
        func_80AEAD20(&ru->actor, play);

        printf("RUTO CREATED PARAMS: %i\n", params);
        switch (params) {
            case 0:
                func_80AEB264(ru, (AnimationHeader*)&gRutoChildWaitHandsOnHipsAnim, 0, 0, 0);
                ru->action = 15;
                ru->actor.shape.yOffset = -10000.0f;
                EnRu1_SetEyeIndex(ru, 5);
                EnRu1_SetMouthIndex(ru, 2);
                break;
            case 1:
                func_80AEB264(ru, (AnimationHeader*)&gRutoChildWaitHandsBehindBackAnim, 0, 0, 0);
                ru->action = 0;
                ru->drawConfig = 1;
                EnRu1_SetEyeIndex(ru, 4);
                EnRu1_SetMouthIndex(ru, 0);
                break;
            case 2: {
                bool metRutoAlready = hasMetRutoAlready;
                if (!metRutoAlready) {
                    func_80AEB264(ru, (AnimationHeader*)&gRutoChildWait2Anim, 0, 0, 0);
                    ru->action = 7;
                    EnRu1_SetMouthIndex(ru, 1);
                } else {
                    func_80AEB264(ru, (AnimationHeader*)&gRutoChildWait2Anim, 0, 0, 0);
                    s8 actorRoom = (s8)(ru->actor.room);
                    ru->action = 22;
                    ru->actor.room = -1;
                    ru->drawConfig = 0;
                    ru->roomNum1 = actorRoom;
                    ru->roomNum3 = actorRoom;
                    ru->roomNum2 = actorRoom;
                }
                break;
            }
            case 3: {
                func_80AEB264(ru, (AnimationHeader*)&gRutoChildWait2Anim, 0, 0, 0);
                s8 actorRoom = (s8)(ru->actor.room);
                ru->action = 22;
                ru->actor.room = -1;
                ru->roomNum1 = actorRoom;
                ru->roomNum3 = actorRoom;
                ru->roomNum2 = actorRoom;
                break;
            }
            case 4:
                func_80AEB264(ru, (AnimationHeader*)&gRutoChildWait2Anim, 0, 0, 0);
                ru->action = 41;
                ru->unk_28C = EnRu1_FindSwitch(play);
                func_80AEB0EC(ru, 1);
                ru->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY);
                break;
            case 5:
                if (Flags_GetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP) && LINK_IS_CHILD) {
                    func_80AEB264(ru, (AnimationHeader*)&gRutoChildWait2Anim, 0, 0, 0);
                    ru->actor.flags &= ~ACTOR_FLAG_UPDATE_CULLING_DISABLED;
                    ru->action = 44;
                    ru->drawConfig = 1;
                } else {
                    Actor_Kill(&ru->actor);
                }
                break;
            case 6: {
                func_80AEB264(ru, (AnimationHeader*)&gRutoChildWait2Anim, 0, 0, 0);
                s8 actorRoom = (s8)(ru->actor.room);
                ru->action = 22;
                ru->actor.room = -1;
                ru->drawConfig = 0;
                ru->roomNum1 = actorRoom;
                ru->roomNum3 = actorRoom;
                ru->roomNum2 = actorRoom;
                break;
            }
            case 10:
                func_80AEB264(ru, (AnimationHeader*)&gRutoChildWait2Anim, 0, 0, 0);
                ru->action = 36;
                ru->roomNum1 = (s8)(ru->actor.room);
                ru->unk_28C = EnRu1_FindSwitch(play);
                ru->actor.room = -1;
                break;
            default:
                Actor_Kill(&ru->actor);
                break;
        }
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr int kActionCount = 45;

    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 21;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case 0:
                return (const char*)&gRutoChildBringArmsUpAnim;
            case 1:
                return (const char*)&gRutoChildBringHandsDownAnim;
            case 2:
                return (const char*)&gRutoChildHoldArmsUpAnim;
            case 3:
                return (const char*)&gRutoChildSeesSapphireAnim;
            case 4:
                return (const char*)&gRutoChildShutterAnim;
            case 5:
                return (const char*)&gRutoChildSitAnim;
            case 6:
                return (const char*)&gRutoChildSittingAnim;
            case 7:
                return (const char*)&gRutoChildSquirmAnim;
            case 8:
                return (const char*)&gRutoChildSwimOnBackAnim;
            case 9:
                return (const char*)&gRutoChildTransitionFromSwimOnBackAnim;
            case 10:
                return (const char*)&gRutoChildTransitionHandsOnHipToCrossArmsAndLegsAnim;
            case 11:
                return (const char*)&gRutoChildTransitionToSwimOnBackAnim;
            case 12:
                return (const char*)&gRutoChildTreadWaterAnim;
            case 13:
                return (const char*)&gRutoChildTurnAroundAnim;
            case 14:
                return (const char*)&gRutoChildWait2Anim;
            case 15:
                return (const char*)&gRutoChildWaitHandsBehindBackAnim;
            case 16:
                return (const char*)&gRutoChildWaitHandsOnHipsAnim;
            case 17:
                return (const char*)&gRutoChildWaitInBlueWarpAnim;
            case 18:
                return (const char*)&gRutoChildWaitSittingAnim;
            case 19:
                return (const char*)&gRutoChildWalkAnim;
            case 20:
                return (const char*)&gRutoChildWalkToAndHoldUpSapphireAnim;
            default:
                return nullptr;
        }
    }
    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = (const char*)AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_MOUTH,
        PROP_ROOMS,
        PROP_MOTION,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_INTERACT_FLAGS,
        PROP_TEXT_ID,
    };

    u8 CurrentCollider1Roles() const {
        EnRu1* ru = Typed();
        u8 roles = 0;
        if (ru->collider.base.ocFlags1 & OC1_ON)
            roles |= COLL_OC;
        return roles;
    }
    u8 CurrentCollider2Roles() const {
        EnRu1* ru = Typed();
        u8 roles = 0;
        if (ru->collider2.base.ocFlags1 & OC1_ON)
            roles |= COLL_OC;
        if (ru->collider2.base.atFlags & AT_ON)
            roles |= COLL_AT;
        return roles;
    }

    bool ShouldLockActor() const override {
        return false;
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnRu1* ru = Typed();

        PackProperty(PROP_ACTION, PackedUInt1((u8)(ru->action)), out);
        PackProperty(PROP_MOUTH, PackedUInt1((u8)(ru->mouthIndex)), out);
        PackProperty(PROP_ROOMS,
                     ByteStream() << PackedInt4(ru->drawConfig) << PackedUInt1((u8)(ru->roomNum1))
                                  << PackedUInt1((u8)(ru->roomNum2))
                                  << PackedUInt1((u8)(ru->roomNum3)) << PackedInt4(ru->isFalling)
                                  << PackedInt4(ru->waterState) << PackedInt4(ru->isSittingOCActive),
                     out);
        PackProperty(PROP_MOTION,
                     ByteStream() << PackedFloat4(ru->walkingFrame) << PackedFloat4(ru->treadTimer)
                                  << PackedInt4(ru->alpha) << PackedInt2(ru->headTurnSpeed)
                                  << PackedInt2(ru->headRotTimer) << PackedInt4(ru->headRotDirection)
                                  << PackedFloat4(ru->unk_288) << PackedInt4(ru->unk_298)
                                  << PackedInt4(ru->preLimbDrawIndex),
                     out);
        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentCollider1Roles()) << PackedUInt1(CurrentCollider2Roles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(ru->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &ru->skelAnime), out);

        PackProperty(PROP_INTERACT_FLAGS,
                     PackedUInt1((((ru->actor.flags & ACTOR_FLAG_ATTENTION_ENABLED) != 0) ? 1 : 0) |
                                 (((ru->actor.flags & ACTOR_FLAG_FRIENDLY) != 0) ? 2 : 0)),
                     out);

        PackProperty(PROP_TEXT_ID, PackedUInt2(ru->actor.textId), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnRu1* ru = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                if (id < kActionCount)
                    ru->action = id;
                break;
            }
            case PROP_MOUTH:
                ru->mouthIndex = (s16)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ROOMS:
                ru->drawConfig = data.Read<PackedInt4>().value();
                ru->roomNum1 = (s8)(data.Read<PackedUInt1>().value());
                ru->roomNum2 = (s8)(data.Read<PackedUInt1>().value());
                ru->roomNum3 = (s8)(data.Read<PackedUInt1>().value());
                ru->isFalling = data.Read<PackedInt4>().value();
                ru->waterState = data.Read<PackedInt4>().value();
                ru->isSittingOCActive = data.Read<PackedInt4>().value();
                break;
            case PROP_MOTION:
                ru->walkingFrame = data.Read<PackedFloat4>().value();
                ru->treadTimer = data.Read<PackedFloat4>().value();
                ru->alpha = data.Read<PackedInt4>().value();
                ru->headTurnSpeed = (s16)(data.Read<PackedInt2>().value());
                ru->headRotTimer = (s16)(data.Read<PackedInt2>().value());
                ru->headRotDirection = data.Read<PackedInt4>().value();
                ru->unk_288 = data.Read<PackedFloat4>().value();
                ru->unk_298 = data.Read<PackedInt4>().value();
                ru->preLimbDrawIndex = data.Read<PackedInt4>().value();
                break;
            case PROP_COLL_ROLES:
                m_collider1Roles = (u8)(data.Read<PackedUInt1>().value());
                m_collider2Roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                ru->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &ru->skelAnime, LOCK_CUR_FRAME ? ru->skelAnime.curFrame : 0.0f, data);
                break;
            }

            case PROP_INTERACT_FLAGS: {
                u8 bits = (u8)(data.Read<PackedUInt1>().value());
                if (bits & 1)
                    ru->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
                else
                    ru->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
                if (bits & 2)
                    ru->actor.flags |= ACTOR_FLAG_FRIENDLY;
                else
                    ru->actor.flags &= ~ACTOR_FLAG_FRIENDLY;
                break;
            }
            case PROP_TEXT_ID:
                ru->actor.textId = (u16)(data.Read<PackedUInt2>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    static BdanObjectsController* FindPlatformController(PlayState* play) {
        static const u8 cats[] = { ACTORCAT_BG };
        for (u8 cat : cats) {
            for (Actor* it = play->actorCtx.actorLists[cat].head; it != nullptr; it = it->next)
            {
                if (it->id == ACTOR_BG_BDAN_OBJECTS) {
                    if (it->zoController)
                        return reinterpret_cast<BdanObjectsController*>(it->zoController);
                }
            }
        }
        return nullptr;
    }

    void UpdateLeader(PlayState* play) override {
        auto platform = FindPlatformController(play);
        if (platform && !platform->IsLeader())
            platform->ClaimLeadership(CLAIM_REASON_NOW);

        auto ru = Typed();
        ru->roomNum2 = play->roomCtx.curRoom.num;

        AbstractActorController::UpdateLeader(play);

        //Kill Ruto after she falls down the hole. When cut scenes were skipped, this didnt happen
        if (ru->action == 13)
        {
            if (ru->actor.world.pos.y < ru->actor.home.pos.y - 60)
            {
                Actor_Kill(&ru->actor);
                return;
            }
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnRu1* ru = Typed();

        ru->roomNum2 = play->roomCtx.curRoom.num;
        if (ru->actor.parent == &GET_PLAYER(play)->actor) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        if (ru->action == 27)
            Actor_OfferCarry(&ru->actor, play);

        m_conversationHandled = true;
        if (ru->action == 24)
        {
            func_80AEF2D0(ru, play);
            if (ru->action != 24) {
                ClaimLeadership(CLAIM_REASON_NOW);
                return;
            }
        }
        else if (ru->action == 25)
        {
            if (IsMyOpenTextboxActor())
                func_80AEF354(ru, play);
            ru->action = 25;

        } else if (IsMyOpenTextboxActor())
            m_conversationHandled = false;

        UpdateAnimation(&ru->skelAnime, LOCK_CUR_FRAME);

        EnRu1_UpdateEyes(ru);

        if (ru->action == 41 || ru->action == 36)
            ru->unk_28C = EnRu1_FindSwitch(play);

        {
            Vec3f savedPos = ru->actor.world.pos;
            Actor_UpdateBgCheckInfo(play, &ru->actor, 26.0f, 10.0f, 0.0f, 5);
            ru->actor.world.pos = savedPos;
        }

        Collider_UpdateCylinder(&ru->actor, &ru->collider);
        Collider_UpdateCylinder(&ru->actor, &ru->collider2);
        if (m_collider1Roles != 0)
            RegisterColliderBase(play, &ru->collider.base, m_collider1Roles);
        if (m_collider2Roles != 0)
            RegisterColliderBase(play, &ru->collider2.base, m_collider2Roles);

    }

  private:
    u8 m_collider1Roles = 0;
    u8 m_collider2Roles = 0;
};

}

#endif
