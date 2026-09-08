#ifndef GELDBCONTROLLERH
#define GELDBCONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_GeldB/z_en_geldb.h"
#include "objects/object_geldb/object_geldb.h"

void EnGeldB_Wait(EnGeldB*, PlayState* play);
void EnGeldB_Flee(EnGeldB*, PlayState* play);
void EnGeldB_Ready(EnGeldB*, PlayState* play);
void EnGeldB_Advance(EnGeldB*, PlayState* play);
void EnGeldB_RollForward(EnGeldB*, PlayState* play);
void EnGeldB_Pivot(EnGeldB*, PlayState* play);
void EnGeldB_Circle(EnGeldB*, PlayState* play);
void EnGeldB_SpinDodge(EnGeldB*, PlayState* play);
void EnGeldB_Slash(EnGeldB*, PlayState* play);
void EnGeldB_SpinAttack(EnGeldB*, PlayState* play);
void EnGeldB_RollBack(EnGeldB*, PlayState* play);
void EnGeldB_Stunned(EnGeldB*, PlayState* play);
void EnGeldB_Damaged(EnGeldB*, PlayState* play);
void EnGeldB_Jump(EnGeldB*, PlayState* play);
void EnGeldB_Block(EnGeldB*, PlayState* play);
void EnGeldB_Sidestep(EnGeldB*, PlayState* play);
void EnGeldB_Defeated(EnGeldB*, PlayState* play);
void EnGeldB_SetupDefeated(EnGeldB*);
void EnGeldB_SetupWait(EnGeldB*);

extern ColliderCylinderInit* gEnGeldBBodyCylInit;
extern ColliderTrisInit* gEnGeldBBlockTrisInit;
extern ColliderQuadInit* gEnGeldBSwordQuadInit;
extern DamageTable* gEnGeldBDamageTable;
}

namespace ZeldaOnline {

class GeldBController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnGeldB* Typed() const {
        return reinterpret_cast<EnGeldB*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

    static void RegisterHooks(s16 actorID, bool enabled) {
        COND_ID_HOOK(ShouldActorInit, actorID, enabled, [&](void* actorRef, bool*) {
            Actor* actor = (Actor*)actorRef;

            AbstractActorController::InstallCustomInit(actor, GeldBController::ReplacementInit);
        });
    }

    static void ReplacementInit(Actor* thisx, PlayState* play) {
        EffectBlureInit1 blureInit;
        EnGeldB* self = reinterpret_cast<EnGeldB*>(thisx);

        Actor_SetFocus(thisx, 0.0f);
        thisx->targetArrowOffset = 2000.0f;
        Actor_SetScale(thisx, 0.01f);
        thisx->gravity = -3.0f;

        thisx->colChkInfo.damageTable = gEnGeldBDamageTable;
        ActorShape_Init(&thisx->shape, 0.0f, ActorShadow_DrawFeet, 0.0f);
        self->actor.colChkInfo.mass = MASS_HEAVY;
        thisx->colChkInfo.health = 20;
        thisx->colChkInfo.cylRadius = 50;
        thisx->colChkInfo.cylHeight = 100;
        thisx->naviEnemyId = 0x54;
        self->keyFlag = thisx->params & 0xFF00;
        thisx->params &= 0xFF;
        self->blinkState = 0;
        self->unkFloat = 10.0f;

        SkelAnime_InitFlex(play, &self->skelAnime, (FlexSkeletonHeader*)gGerudoRedSkel,
                           (AnimationHeader*)gGerudoRedNeutralAnim, self->jointTable, self->morphTable, GELDB_LIMB_MAX);

        Collider_InitCylinder(play, &self->bodyCollider);
        Collider_SetCylinder(play, &self->bodyCollider, thisx, gEnGeldBBodyCylInit);
        Collider_InitTris(play, &self->blockCollider);
        Collider_SetTris(play, &self->blockCollider, thisx, gEnGeldBBlockTrisInit, self->blockElements);
        Collider_InitQuad(play, &self->swordCollider);
        Collider_SetQuad(play, &self->swordCollider, thisx, gEnGeldBSwordQuadInit);

        blureInit.p1StartColor[0] = blureInit.p1StartColor[1] = blureInit.p1StartColor[2] = blureInit.p1StartColor[3] =
            blureInit.p2StartColor[0] = blureInit.p2StartColor[1] = blureInit.p2StartColor[2] =
                blureInit.p1EndColor[0] = blureInit.p1EndColor[1] = blureInit.p1EndColor[2] = blureInit.p2EndColor[0] =
                    blureInit.p2EndColor[1] = blureInit.p2EndColor[2] = 255;
        blureInit.p2StartColor[3] = 64;
        blureInit.p1EndColor[3] = blureInit.p2EndColor[3] = 0;
        blureInit.elemDuration = 8;
        blureInit.unkFlag = 0;
        blureInit.calcMode = 2;

        Effect_Add(play, &self->blureIndex, EFFECT_BLURE1, 0, 0, &blureInit);
        Actor_SetScale(thisx, 0.012499999f);
        EnGeldB_SetupWait(self);

        // DO NOT KILL SELF IF ALREADY GOT THE KEY (moved to OnActorInit)
    }

  protected:
    static constexpr s32 ACT_WAIT = 0;
    static constexpr s32 ACT_READY = 5;
    static constexpr s32 ACT_BLOCK = 6;
    static constexpr s32 ACT_STUNNED = 15;

    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    using GeldBActionFunc = void (*)(EnGeldB*, PlayState*);
    static const GeldBActionFunc* ActionTable(size_t* count) {
        static const GeldBActionFunc sTable[] = {
            EnGeldB_Wait,    EnGeldB_Flee,      EnGeldB_Ready, EnGeldB_Advance,    EnGeldB_RollForward, EnGeldB_Pivot,
            EnGeldB_Circle,  EnGeldB_SpinDodge, EnGeldB_Slash, EnGeldB_SpinAttack, EnGeldB_RollBack,    EnGeldB_Stunned,
            EnGeldB_Damaged, EnGeldB_Jump,      EnGeldB_Block, EnGeldB_Sidestep,   EnGeldB_Defeated,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const GeldBActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gGerudoRedNeutralAnim,  gGerudoRedJumpAnim,   gGerudoRedWalkAnim,       gGerudoRedFlipAnim,
            gGerudoRedSidestepAnim, gGerudoRedSlashAnim,  gGerudoRedSpinAttackAnim, gGerudoRedDamageAnim,
            gGerudoRedBlockAnim,    gGerudoRedDefeatAnim,
        };
        if (i >= (sizeof(sAnims) / sizeof(sAnims[0])))
            return nullptr;
        return sAnims[i];
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < 10; i++)
            if (strcmp(AnimForIndex(i), cur) == 0)
                return i;
        return ANIM_UNKNOWN;
    }

    void CurrentColliderRoles(u8& body, u8& block, u8& sword) const {
        EnGeldB* gb = Typed();

        body = COLL_OC;
        block = 0;
        sword = 0;

        if ((gb->action >= ACT_READY) && (gb->spinAttackState < 2) &&
            ((gb->actor.colorFilterTimer == 0) || !(gb->actor.colorFilterParams & 0x4000)))
            body |= COLL_AC;

        if ((gb->action == ACT_BLOCK) && (gb->skelAnime.curFrame == 0.0f))
            block |= COLL_AC;

        if (gb->meleeWeaponState > 0)
            sword |= COLL_AT;
    }

    bool HitWouldReact() const {
        EnGeldB* gb = Typed();

        if (gb->blockCollider.base.acFlags & AC_BOUNCED)
            return false;

        return (gb->bodyCollider.base.acFlags & AC_HIT) && (gb->action >= ACT_READY) && (gb->spinAttackState < 2) &&
               (gb->actor.colChkInfo.damageEffect != 0 || gb->actor.colChkInfo.damage != 0);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_TIMERS,
        PROP_HEALTH,
        PROP_HEAD,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnGeldB* gb = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt4(gb->action) << PackedInt2(gb->invisible)
                                  << PackedInt2(gb->meleeWeaponState) << PackedInt2(gb->spinAttackState)
                                  << PackedUInt1(gb->damageEffect),
                     out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt4(gb->timer) << PackedInt2(gb->unkTimer) << PackedInt2(gb->lookTimer)
                                  << PackedInt2(gb->iceTimer) << PackedFloat4(gb->approachRate)
                                  << PackedFloat4(gb->unkFloat),
                     out);
        PackProperty(PROP_HEALTH, PackedInt1(gb->actor.colChkInfo.health), out);
        PackProperty(
            PROP_HEAD,
            ByteStream() << PackedInt2(gb->headRot.x) << PackedInt2(gb->headRot.y) << PackedInt2(gb->headRot.z), out);

        {
            u8 body, block, sword;
            CurrentColliderRoles(body, block, sword);
            PackProperty(PROP_COLL_ROLES, ByteStream() << PackedUInt1(body) << PackedUInt1(block) << PackedUInt1(sword),
                         out);
        }

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(gb->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &gb->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnGeldB* gb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const GeldBActionFunc* table = ActionTable(&count);
                if (id < count)
                    gb->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                gb->action = (s32)(data.Read<PackedInt4>().value());
                gb->invisible = (s16)(data.Read<PackedInt2>().value());
                gb->meleeWeaponState = (s16)(data.Read<PackedInt2>().value());
                gb->spinAttackState = (s16)(data.Read<PackedInt2>().value());
                gb->damageEffect = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                gb->timer = (s32)(data.Read<PackedInt4>().value());
                gb->unkTimer = (s16)(data.Read<PackedInt2>().value());
                gb->lookTimer = (s16)(data.Read<PackedInt2>().value());
                gb->iceTimer = (s16)(data.Read<PackedInt2>().value());
                gb->approachRate = data.Read<PackedFloat4>().value();
                gb->unkFloat = data.Read<PackedFloat4>().value();
                break;
            case PROP_HEALTH:
                gb->actor.colChkInfo.health = (s8)(data.Read<PackedInt1>().value());
                break;
            case PROP_HEAD:
                gb->headRot.x = (s16)(data.Read<PackedInt2>().value());
                gb->headRot.y = (s16)(data.Read<PackedInt2>().value());
                gb->headRot.z = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_blockRoles = (u8)(data.Read<PackedUInt1>().value());
                m_swordRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                gb->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &gb->skelAnime, LOCK_CUR_FRAME ? gb->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnGeldB* gb = Typed();

        if (gb->spinAttackState >= 2)
            gb->spinAttackState = 1;

        if (!(changed & (1ull << PROP_ACTION)) || gb->actionFunc != EnGeldB_Defeated)
            return;

        if (gb->keyFlag != 0 && !Flags_GetCollectible(gPlayState, gb->keyFlag >> 8)) {
            EnItem00* key = Item_DropCollectible(gPlayState, &gb->actor.world.pos, gb->keyFlag | ITEM00_SMALL_KEY);
            if (key != NULL) {
                key->actor.world.rot.y = Math_Vec3f_Yaw(&key->actor.world.pos, &gb->actor.home.pos);
                key->actor.speedXZ = 6.0f;
                Audio_PlaySoundGeneral(NA_SE_SY_TRE_BOX_APPEAR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                       &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            }
        }

        EnGeldB_SetupDefeated(gb);
        GoLocal();
    }

    void OnActorInit() override {
        EnGeldB* gb = Typed();

        if (gb->keyFlag != 0 && Flags_GetCollectible(gPlayState, gb->keyFlag >> 8)) {
            // GO LOCAL before killing self. This kill should only apply for us. GoLocal first ensures the leader wont
            //  kill it for everyone
            //  GoLocal(); Actor_Kill(&gb->actor);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnGeldB* gb = Typed();

        UpdateAnimation(&gb->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        gb->actor.focus.pos = gb->actor.world.pos;
        gb->actor.focus.pos.y += 40.0f;

        gb->bodyCollider.base.acFlags &= ~AC_HIT;
        gb->blockCollider.base.acFlags &= ~(AC_HIT | AC_BOUNCED);
        gb->swordCollider.base.atFlags &= ~AT_HIT;

        if (gb->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Collider_UpdateCylinder(&gb->actor, &gb->bodyCollider);

        if (m_bodyRoles != 0)
            RegisterColliderBase(play, &gb->bodyCollider.base, m_bodyRoles);
        if (m_blockRoles != 0)
            RegisterColliderBase(play, &gb->blockCollider.base, m_blockRoles);
        if (m_swordRoles != 0)
            RegisterColliderBase(play, &gb->swordCollider.base, m_swordRoles);

        if (gb->blinkState == 0) {
            if ((Rand_ZeroOne() < 0.1f) && ((play->gameplayFrames % 4) == 0))
                gb->blinkState++;
        } else {
            gb->blinkState = (gb->blinkState + 1) & 3;
        }
    }

  private:
    u8 m_bodyRoles = 0;
    u8 m_blockRoles = 0;
    u8 m_swordRoles = 0;
};

} // namespace ZeldaOnline

#endif