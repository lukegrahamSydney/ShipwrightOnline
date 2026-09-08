#ifndef LIZALFOSCONTROLLERH
#define LIZALFOSCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "objects/object_zf/object_zf.h"

void EnZf_DropIn(EnZf* zf, PlayState* play);
void func_80B4543C(EnZf* zf, PlayState* play);
void EnZf_ApproachPlayer(EnZf* zf, PlayState* play);
void EnZf_JumpForward(EnZf* zf, PlayState* play);
void func_80B46098(EnZf* zf, PlayState* play);
void func_80B463E4(EnZf* zf, PlayState* play);
void EnZf_Slash(EnZf* zf, PlayState* play);
void EnZf_RecoilFromBlockedSlash(EnZf* zf, PlayState* play);
void EnZf_JumpBack(EnZf* zf, PlayState* play);
void EnZf_Stunned(EnZf* zf, PlayState* play);
void EnZf_SheatheSword(EnZf* zf, PlayState* play);
void EnZf_HopAndTaunt(EnZf* zf, PlayState* play);
void EnZf_HopAway(EnZf* zf, PlayState* play);
void EnZf_DrawSword(EnZf* zf, PlayState* play);
void EnZf_Damaged(EnZf* zf, PlayState* play);
void EnZf_JumpUp(EnZf* zf, PlayState* play);
void EnZf_CircleAroundPlayer(EnZf* zf, PlayState* play);
void EnZf_Die(EnZf* zf, PlayState* play);

extern s16* gEnZfInactiveParams;
}

namespace ZeldaOnline {

class LizalfosController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnZf* Typed() const {
        return reinterpret_cast<EnZf*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_SLASH = 6;
    static constexpr u8 ID_STUNNED = 9;
    static constexpr u8 ID_DIE = 17;

    using ZfActionFunc = void (*)(EnZf*, PlayState*);
    static const ZfActionFunc* ActionTable(size_t* count) {
        static const ZfActionFunc sTable[] = {
            EnZf_DropIn,
            func_80B4543C,
            EnZf_ApproachPlayer,
            EnZf_JumpForward,
            func_80B46098,
            func_80B463E4,
            EnZf_Slash,
            EnZf_RecoilFromBlockedSlash,
            EnZf_JumpBack,
            EnZf_Stunned,
            EnZf_SheatheSword,
            EnZf_HopAndTaunt,
            EnZf_HopAway,
            EnZf_DrawSword,
            EnZf_Damaged,
            EnZf_JumpUp,
            EnZf_CircleAroundPlayer,
            EnZf_Die,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ZfActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_HOP_CROUCH = 0;
    static constexpr u8 ANIM_HOP_LEAP = 1;
    static constexpr u8 ANIM_HOP_LAND = 2;
    static constexpr u8 ANIM_CRYING = 3;
    static constexpr u8 ANIM_WALKING = 4;
    static constexpr u8 ANIM_SIDESTEP = 5;
    static constexpr u8 ANIM_SLASH = 6;
    static constexpr u8 ANIM_JUMPING = 7;
    static constexpr u8 ANIM_LANDING = 8;
    static constexpr u8 ANIM_KNOCKED_BACK = 9;
    static constexpr u8 ANIM_DYING = 10;
    static constexpr u8 ANIM_DRAW_SWORD = 11;
    static constexpr u8 ANIM_SHEATHE_SWORD = 12;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 13;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_HOP_CROUCH:
                return gZfHopCrouchingAnim;
            case ANIM_HOP_LEAP:
                return gZfHopLeapingAnim;
            case ANIM_HOP_LAND:
                return gZfHopLandingAnim;
            case ANIM_CRYING:
                return gZfCryingAnim;
            case ANIM_WALKING:
                return gZfWalkingAnim;
            case ANIM_SIDESTEP:
                return gZfSidesteppingAnim;
            case ANIM_SLASH:
                return gZfSlashAnim;
            case ANIM_JUMPING:
                return gZfJumpingAnim;
            case ANIM_LANDING:
                return gZfLandingAnim;
            case ANIM_KNOCKED_BACK:
                return gZfKnockedBackAnim;
            case ANIM_DYING:
                return gZfDyingAnim;
            case ANIM_DRAW_SWORD:
                return gZfDrawingSwordAnim;
            case ANIM_SHEATHE_SWORD:
                return gZfSheathingSwordAnim;
            default:
                return nullptr;
        }
    }
    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    bool IsInactiveOfPair() const {
        EnZf* zf = Typed();
        return zf->actor.params >= ENZF_TYPE_LIZALFOS_MINIBOSS_A && *gEnZfInactiveParams == zf->actor.params;
    }

    u8 CurrentBodyRoles() const {
        EnZf* zf = Typed();
        if (zf->actor.colChkInfo.health <= 0 || zf->alpha != 255)
            return 0;
        u8 roles = 0;
        if (zf->actor.world.pos.y == zf->actor.floorHeight && zf->action <= ENZF_ACTION_DAMAGED)
            roles |= COLL_OC;
        if (!IsInactiveOfPair() && (zf->actor.colorFilterTimer == 0 || !(zf->actor.colorFilterParams & 0x4000)))
            roles |= COLL_AC;
        return roles;
    }

    u8 CurrentSwordRoles() const {
        EnZf* zf = Typed();
        if (zf->action != ENZF_ACTION_SLASH)
            return 0;
        if (zf->skelAnime.curFrame < 14.0f || zf->skelAnime.curFrame > 20.0f)
            return 0;
        if ((zf->swordCollider.base.atFlags & AT_BOUNCED) || (zf->swordCollider.base.acFlags & AC_HIT))
            return 0;
        return COLL_AT;
    }

    bool HitWouldReact() const {
        EnZf* zf = Typed();
        return (zf->bodyCollider.base.acFlags & AC_HIT) && zf->action <= ENZF_ACTION_STUNNED && !IsInactiveOfPair();
    }

    LizalfosController* Partner(PlayState* play) {
        EnZf* self = Typed();
        if (self->actor.params < ENZF_TYPE_LIZALFOS_MINIBOSS_A)
            return nullptr;

        static const u8 cats[] = { ACTORCAT_ENEMY, ACTORCAT_PROP };
        for (u8 cat : cats) {
            for (Actor* it = play->actorCtx.actorLists[cat].head; it != nullptr; it = it->next) {
                if (it == &self->actor || it->id != self->actor.id)
                    continue;
                if (it->params < ENZF_TYPE_LIZALFOS_MINIBOSS_A || it->params == self->actor.params)
                    continue;

                if (it->zoController)
                    return reinterpret_cast<LizalfosController*>(it->zoController);
            }
        }
        return nullptr;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ACTION_STATE,
        PROP_HEALTH,
        PROP_COLL_ROLES,
        PROP_HOP_LAUNCH,
        PROP_HOP_ANIM,
        PROP_TIMERS,
        PROP_HEAD_ROT,
        PROP_SWORD_SHEATHED,
        PROP_DAMAGE_EFFECT,
        PROP_PLATFORMS,
        PROP_ALPHA,
        PROP_PAIR_TAG,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnZf* zf = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ACTION_STATE, PackedInt2(zf->action), out);
        PackProperty(PROP_HEALTH, PackedUInt1(zf->actor.colChkInfo.health), out);
        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentBodyRoles()) << PackedUInt1(CurrentSwordRoles()), out);

        PackProperty(PROP_HOP_LAUNCH, ByteStream() << PackedFloat4(zf->unk_408) << PackedFloat4(zf->unk_40C), out);
        PackProperty(PROP_HOP_ANIM, PackedInt2((s16)(zf->hopAnimIndex)), out);

        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2((s16)(zf->unk_3F0)) << PackedInt2(zf->unk_3F4)
                                  << PackedInt2(zf->iceTimer) << PackedInt2(zf->unk_3F8),
                     out);

        PackProperty(PROP_HEAD_ROT, ByteStream() << PackedInt2(zf->headRot) << PackedInt2(zf->headRotTemp), out);

        PackProperty(PROP_SWORD_SHEATHED, PackedInt2(zf->swordSheathed), out);
        PackProperty(PROP_DAMAGE_EFFECT, PackedUInt1(zf->damageEffect), out);
        PackProperty(PROP_PLATFORMS,
                     ByteStream() << PackedInt2(zf->curPlatform) << PackedInt2(zf->homePlatform)
                                  << PackedInt2(zf->nextPlatform),
                     out);
        PackProperty(PROP_ALPHA, PackedUInt1(zf->alpha), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        if (zf->actor.params == ENZF_TYPE_LIZALFOS_MINIBOSS_B) {
            PackProperty(PROP_PAIR_TAG, PackedInt2(*gEnZfInactiveParams), out);
        } else {
            PackNullProperty(PROP_PAIR_TAG, out);
        }

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(zf->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &zf->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnZf* zf = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const ZfActionFunc* table = ActionTable(&count);
                if (id < count)
                    zf->actionFunc = table[id];
                break;
            }
            case PROP_ACTION_STATE:
                zf->action = (s32)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                zf->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_COLL_ROLES:
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_swordRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HOP_LAUNCH:
                zf->unk_408 = data.Read<PackedFloat4>().value();
                zf->unk_40C = data.Read<PackedFloat4>().value();
                break;
            case PROP_HOP_ANIM:
                zf->hopAnimIndex = (s32)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                zf->unk_3F0 = (s32)(data.Read<PackedInt2>().value());
                zf->unk_3F4 = (s16)(data.Read<PackedInt2>().value());
                zf->iceTimer = (s16)(data.Read<PackedInt2>().value());
                zf->unk_3F8 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEAD_ROT:
                zf->headRot = (s16)(data.Read<PackedInt2>().value());
                zf->headRotTemp = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SWORD_SHEATHED:
                zf->swordSheathed = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_DAMAGE_EFFECT:
                zf->damageEffect = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PLATFORMS:
                zf->curPlatform = (s16)(data.Read<PackedInt2>().value());
                zf->homePlatform = (s16)(data.Read<PackedInt2>().value());
                zf->nextPlatform = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ALPHA:
                zf->alpha = (u8)(data.Read<PackedUInt1>().value());
                zf->actor.shape.shadowAlpha = zf->alpha;
                break;
            case PROP_PAIR_TAG:
                if (propLen == 0)
                    break;
                *gEnZfInactiveParams = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                zf->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &zf->skelAnime, LOCK_CUR_FRAME ? zf->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;

    }

    void UpdateLeader(PlayState* play) override {
        if (m_actor->params != *gEnZfInactiveParams)
        {
            auto partner = Partner(gPlayState);
            if (partner && !partner->IsLeader())
                partner->ClaimLeadership(CLAIM_REASON_NOW);
        }
        AbstractActorController::UpdateLeader(play);

    }
    void UpdatePuppet(PlayState* play) override {
        EnZf* zf = Typed();

        UpdateAnimation(&zf->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        zf->bodyCollider.base.acFlags &= ~AC_HIT;
        zf->swordCollider.base.atFlags &= ~AT_BOUNCED;
        zf->swordCollider.base.acFlags &= ~AC_HIT;

        zf->actor.focus.pos = zf->actor.world.pos;
        zf->actor.focus.pos.y += 40.0f;

        {
            Vec3f rayOrigin = zf->actor.world.pos;
            rayOrigin.y += 50.0f;
            s32 floorBgId = BGCHECK_SCENE;
            zf->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &zf->actor.floorPoly, &floorBgId,
                                                                &zf->actor, &rayOrigin);
            zf->actor.floorBgId = floorBgId;
        }

        RegisterCylinder(play, &zf->bodyCollider, m_bodyRoles);
        RegisterColliderBase(play, &zf->swordCollider.base, m_swordRoles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_bodyRoles = COLL_OC | COLL_AC;
    u8 m_swordRoles = 0;

};

}

#endif
