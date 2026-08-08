#ifndef BARINADECONTROLLERH
#define BARINADECONTROLLERH

/*
The body controls all the logic. The body will also force all the parts (the little drones and supports, etc) to
become leader if the body is leader. So all the parts are always controlled by a single client
*/
#include <cstring>
#include <string>
#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Va/z_boss_va.h"

typedef struct BossVaEffect {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
    /* 0x24 */ u8 type;
    /* 0x26 */ u16 timer;
    /* 0x28 */ s16 mode;
    /* 0x2A */ Vec3s rot;
    /* 0x30 */ s16 primColor[4];
    /* 0x38 */ s16 envColor[4];
    /* 0x40 */ f32 scale;
    /* 0x44 */ f32 scaleMod;
    /* 0x48 */ Vec3f offset;
    /* 0x54 */ struct BossVa* parent;
    u32 epoch;
} BossVaEffect; // size = 0x58

void BossVa_BodyIntro(BossVa* va, PlayState* play);
void BossVa_BodyPhase1(BossVa* va, PlayState* play);
void BossVa_BodyPhase2(BossVa* va, PlayState* play);
void BossVa_BodyPhase3(BossVa* va, PlayState* play);
void BossVa_BodyPhase4(BossVa* va, PlayState* play);
void BossVa_BodyDeath(BossVa* va, PlayState* play);

void BossVa_SupportIntro(BossVa* va, PlayState* play);
void BossVa_SupportAttached(BossVa* va, PlayState* play);
void BossVa_SupportCut(BossVa* va, PlayState* play);

void BossVa_ZapperIntro(BossVa* va, PlayState* play);
void BossVa_ZapperAttack(BossVa* va, PlayState* play);
void BossVa_ZapperEnraged(BossVa* va, PlayState* play);
void BossVa_ZapperDamaged(BossVa* va, PlayState* play);
void BossVa_ZapperHold(BossVa* va, PlayState* play);
void BossVa_ZapperDeath(BossVa* va, PlayState* play);

void BossVa_Stump(BossVa* va, PlayState* play);
void BossVa_Door(BossVa* va, PlayState* play);

void BossVa_BariIntro(BossVa* va, PlayState* play);
void BossVa_BariPhase2Attack(BossVa* va, PlayState* play);
void BossVa_BariPhase3Attack(BossVa* va, PlayState* play);
void BossVa_BariPhase3Stunned(BossVa* va, PlayState* play);
void BossVa_BariDeath(BossVa* va, PlayState* play);

void BossVa_SetupAction(BossVa* thisx, BossVaActionFunc func);
void BossVa_SetupBodyDeath(BossVa* thisx, PlayState* play);
void BossVa_UpdateEffects(PlayState* play);
void BossVa_Spark(PlayState* play, BossVa* actor, s32 count, s16 scale, f32 xzSpread, f32 ySpread, u8 mode, f32 range,
                  u8 fixed);
void BossVa_SpawnZapperCharge(PlayState* play, BossVaEffect* effect, BossVa* actor, Vec3f* pos, Vec3s* rot, s16 scale,
                              u8 mode);

#include "objects/object_bv/object_bv.h"

extern u8 sBodyState;
extern u8 sFightPhase;
extern s8 sCsState;
extern s16 sDoorState;
extern u8 sPhase3StopMoving;
extern u16 sPhase2Timer;
extern s8 sPhase4HP;
extern u8 sKillBari;
extern u8 sBodyBari[10];
extern BossVaEffect sVaEffects[400];

extern s16 sCsCamera;
}

namespace ZeldaOnline {

typedef enum {
    /* 1 */ SPARK_TETHER = 1,
    /* 2 */ SPARK_BARI,
    /* 3 */ SPARK_BLAST,
    /* 4 */ SPARK_UNUSED,
    /* 5 */ SPARK_BODY,
    /* 6 */ SPARK_LINK
} BossVaSparkMode;

static constexpr s8 BOSSVA_CS_BATTLE = 13;
static constexpr s8 BOSSVA_CS_DEATH_START = 14;

class BarinadeController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BossVa* Typed() const {
        return reinterpret_cast<BossVa*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        if (params == BOSSVA_DOOR)
            return false;

        if (params >= BOSSVA_STUMP_1 && params <= BOSSVA_STUMP_3)
            return false;
        return true;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {

        return actorId == ACTOR_BOSS_VA;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_BODY_PHASE2 = 2;
    static constexpr u8 ID_BODY_PHASE3 = 3;
    static constexpr u8 ID_BODY_PHASE4 = 4;
    static constexpr u8 ID_BODY_DEATH = 5;
    static constexpr u8 ID_SUPPORT_INTRO = 6;
    static constexpr u8 ID_SUPPORT_ATTACHED = 7;
    static constexpr u8 ID_ZAPPER_ATTACK = 10;
    static constexpr u8 ID_ZAPPER_ENRAGED = 11;
    static constexpr u8 ID_BARI_PHASE2 = 18;
    static constexpr u8 ID_BARI_PHASE3 = 19;
    static constexpr u8 ID_BARI_STUNNED = 20;

    bool IsBody() const {
        return Typed()->actor.params == BOSSVA_BODY;
    }
    bool IsSupport() const {
        s16 p = Typed()->actor.params;
        return p >= BOSSVA_SUPPORT_1 && p <= BOSSVA_SUPPORT_3;
    }
    bool IsZapper() const {
        s16 p = Typed()->actor.params;
        return p >= BOSSVA_ZAPPER_1 && p <= BOSSVA_ZAPPER_3;
    }
    bool IsBari() const {
        s16 p = Typed()->actor.params;
        return p >= BOSSVA_BARI_UPPER_1 && p <= BOSSVA_BARI_LOWER_5;
    }

    static bool IsIntroPlaying() {
        return sCsState < BOSSVA_CS_BATTLE;
    }

    using VaActionFunc = void (*)(BossVa*, PlayState*);
    static const VaActionFunc* ActionTable(size_t* count) {
        static const VaActionFunc sTable[] = {
            BossVa_BodyIntro,
            BossVa_BodyPhase1,
            BossVa_BodyPhase2,
            BossVa_BodyPhase3,
            BossVa_BodyPhase4,
            BossVa_BodyDeath,
            BossVa_SupportIntro,
            BossVa_SupportAttached,
            BossVa_SupportCut,
            BossVa_ZapperIntro,
            BossVa_ZapperAttack,
            BossVa_ZapperEnraged,
            BossVa_ZapperDamaged,
            BossVa_ZapperHold,
            BossVa_ZapperDeath,
            BossVa_Stump,
            BossVa_Door,
            BossVa_BariIntro,
            BossVa_BariPhase2Attack,
            BossVa_BariPhase3Attack,
            BossVa_BariPhase3Stunned,
            BossVa_BariDeath,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const VaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr bool LOCK_CUR_FRAME = true;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 11;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case 0:
                return gBarinadeBodyAnim;
            case 1:
                return gBarinadeSupportAttachedAnim;
            case 2:
                return gBarinadeSupportCutAnim;
            case 3:
                return gBarinadeSupportDetachedAnim;
            case 4:
                return gBarinadeSupportDamage1Anim;
            case 5:
                return gBarinadeSupportDamage2Anim;
            case 6:
                return gBarinadeZapperIdleAnim;
            case 7:
                return gBarinadeZapperDamage1Anim;
            case 8:
                return gBarinadeZapperDamage2Anim;
            case 9:
                return gBarinadeStumpAnim;
            case 10:
                return gBarinadeBariAnim;
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

    void BodyColliderRoles(u8* roles) const {
        BossVa* va = Typed();
        *roles = COLL_OC;

        switch (m_currentActionIndex) {
            case ID_BODY_PHASE2:
                if (va->actor.colorFilterTimer == 0)
                    *roles |= COLL_AT;
                if ((va->actor.colorFilterTimer == 0) || !(va->actor.colorFilterParams & 0x4000))
                    *roles |= COLL_AC;
                break;
            case ID_BODY_PHASE3:
                *roles |= COLL_AT;
                if (va->timer == 0)
                    *roles |= COLL_AC;
                break;
            case ID_BODY_PHASE4:
                if (va->invincibilityTimer == 0)
                    *roles |= COLL_AC;
                if ((va->headRot.y > 0x3E8) || (va->actor.shape.yOffset < -1200.0f))
                    *roles |= COLL_AT;
                break;
            default:
                break;
        }
    }

    void PartColliderRoles(u8* sphRoles, u8* lightningRoles) const {
        BossVa* va = Typed();
        *sphRoles = 0;
        *lightningRoles = 0;

        switch (m_currentActionIndex) {
            case ID_SUPPORT_ATTACHED:
                *sphRoles = COLL_AC;
                break;
            case ID_ZAPPER_ATTACK:
            case ID_ZAPPER_ENRAGED:
                if (va->burst && va->timer2 >= 32)
                    *lightningRoles = COLL_AT;
                break;
            case ID_BARI_PHASE3:
                *lightningRoles = COLL_AT;
                *sphRoles = COLL_AT;
                if (!sPhase3StopMoving)
                    *sphRoles |= COLL_AC;
                break;
            case ID_BARI_STUNNED:
                *lightningRoles = COLL_AT;
                *sphRoles = COLL_AT;
                break;
            case ID_BARI_PHASE2:
                *sphRoles = COLL_AC;
                break;
            default:
                break;
        }
    }

    bool HitWouldReact() const {
        BossVa* va = Typed();
        return (va->colliderBody.base.acFlags & AC_HIT) || (va->colliderSph.base.acFlags & AC_HIT);
    }

    void ClearHitFlags() {
        BossVa* va = Typed();
        va->colliderBody.base.acFlags &= ~AC_HIT;
        va->colliderSph.base.acFlags &= ~AC_HIT;
        va->colliderBody.base.atFlags &= ~AT_HIT;
        va->colliderSph.base.atFlags &= ~AT_HIT;
        va->colliderLightning.base.atFlags &= ~AT_HIT;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_DAMAGE,
        PROP_TIMERS,
        PROP_GLOW,
        PROP_PULSE,
        PROP_ZAP_POS,
        PROP_ZAP_ROT,
        PROP_HEAD_ROT,
        PROP_COLL_ROLES,
        PROP_ATTACH_POINT,
        PROP_BODY_TILT,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_FIGHT_STATE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BossVa* va = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_DAMAGE,
                     ByteStream() << PackedUInt1(va->actor.colChkInfo.health) << PackedInt1(va->invincibilityTimer)
                                  << PackedUInt1(va->isDead) << PackedUInt1(va->burst),
                     out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt4(va->timer) << PackedInt2(va->timer2) << PackedUInt1(va->onCeiling),
                     out);
        PackProperty(PROP_GLOW, ByteStream() << PackedInt2(va->bodyGlow) << PackedInt2(va->unk_1B0), out);
        PackProperty(PROP_PULSE,
                     ByteStream() << PackedFloat4(va->unk_1A0) << PackedFloat4(va->unk_1A4) << PackedInt2(va->unk_1AC),
                     out);
        PackProperty(PROP_ZAP_POS,
                     ByteStream() << PackedFloat4(va->armTip.x) << PackedFloat4(va->armTip.y)
                                  << PackedFloat4(va->armTip.z) << PackedFloat4(va->zapNeckPos.x)
                                  << PackedFloat4(va->zapNeckPos.y) << PackedFloat4(va->zapNeckPos.z)
                                  << PackedFloat4(va->zapHeadPos.x) << PackedFloat4(va->zapHeadPos.y)
                                  << PackedFloat4(va->zapHeadPos.z),
                     out);
        PackProperty(PROP_ZAP_ROT,
                     ByteStream() << PackedInt2(va->unk_1E4) << PackedInt2(va->unk_1E6) << PackedInt2(va->unk_1E8)
                                  << PackedInt2(va->unk_1EA) << PackedInt2(va->unk_1EC) << PackedInt2(va->unk_1EE)
                                  << PackedInt2(va->unk_1F0) << PackedInt2(va->unk_1F2) << PackedInt2(va->unk_1F4),
                     out);
        PackProperty(
            PROP_HEAD_ROT,
            ByteStream() << PackedInt2(va->headRot.x) << PackedInt2(va->headRot.y) << PackedInt2(va->headRot.z), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(
            PROP_COLL_ROLES,
            ByteStream() << PackedUInt1(m_bodyRoles) << PackedUInt1(m_sphRoles) << PackedUInt1(m_lightningRoles), out);

        if (IsBody() || IsZapper()) {
            PackProperty(PROP_ATTACH_POINT,
                         ByteStream() << PackedFloat4(va->unk_1D8.x) << PackedFloat4(va->unk_1D8.y)
                                      << PackedFloat4(va->unk_1D8.z),
                         out);
        }

        if (IsBody()) {
            PackProperty(PROP_BODY_TILT,
                         ByteStream() << PackedInt2(va->actor.world.rot.x) << PackedInt2(va->actor.world.rot.z)
                                      << PackedFloat4(va->actor.shape.yOffset),
                         out);
        }

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(va->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &va->skelAnime), out);

        if (IsBody()) {
            ByteStream fight;
            fight << PackedUInt1(sBodyState) << PackedUInt1(sFightPhase) << PackedInt1(sCsState)
                  << PackedInt2(sDoorState) << PackedUInt1(sPhase3StopMoving) << PackedUInt2(sPhase2Timer)
                  << PackedInt1(sPhase4HP) << PackedUInt1(sKillBari);
            for (int i = 0; i < 10; i++)
                fight << PackedUInt1(sBodyBari[i]);
            PackProperty(PROP_FIGHT_STATE, fight, out);
        }

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        if (!IsBody())
            BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        if (!IsBody())
            BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossVa* va = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const VaActionFunc* table = ActionTable(&count);
                if (id < count)
                    va->actionFunc = table[id];
                break;
            }
            case PROP_DAMAGE:
                va->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                va->invincibilityTimer = (s8)(data.Read<PackedInt1>().value());
                va->isDead = (u8)(data.Read<PackedUInt1>().value());
                va->burst = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                va->timer = data.Read<PackedInt4>().value();
                va->timer2 = (s16)(data.Read<PackedInt2>().value());
                va->onCeiling = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_GLOW:
                va->bodyGlow = (s16)(data.Read<PackedInt2>().value());
                va->unk_1B0 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_PULSE:
                va->unk_1A0 = data.Read<PackedFloat4>().value();
                va->unk_1A4 = data.Read<PackedFloat4>().value();
                va->unk_1AC = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ZAP_POS:
                va->armTip.x = data.Read<PackedFloat4>().value();
                va->armTip.y = data.Read<PackedFloat4>().value();
                va->armTip.z = data.Read<PackedFloat4>().value();
                va->zapNeckPos.x = data.Read<PackedFloat4>().value();
                va->zapNeckPos.y = data.Read<PackedFloat4>().value();
                va->zapNeckPos.z = data.Read<PackedFloat4>().value();
                va->zapHeadPos.x = data.Read<PackedFloat4>().value();
                va->zapHeadPos.y = data.Read<PackedFloat4>().value();
                va->zapHeadPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_ZAP_ROT:
                va->unk_1E4 = (s16)(data.Read<PackedInt2>().value());
                va->unk_1E6 = (s16)(data.Read<PackedInt2>().value());
                va->unk_1E8 = (s16)(data.Read<PackedInt2>().value());
                va->unk_1EA = (s16)(data.Read<PackedInt2>().value());
                va->unk_1EC = (s16)(data.Read<PackedInt2>().value());
                va->unk_1EE = (s16)(data.Read<PackedInt2>().value());
                va->unk_1F0 = (s16)(data.Read<PackedInt2>().value());
                va->unk_1F2 = (s16)(data.Read<PackedInt2>().value());
                va->unk_1F4 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEAD_ROT:
                va->headRot.x = (s16)(data.Read<PackedInt2>().value());
                va->headRot.y = (s16)(data.Read<PackedInt2>().value());
                va->headRot.z = (s16)(data.Read<PackedInt2>().value());
                break;

            case PROP_COLL_ROLES:
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_sphRoles = (u8)(data.Read<PackedUInt1>().value());
                m_lightningRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ATTACH_POINT:
                va->unk_1D8.x = data.Read<PackedFloat4>().value();
                va->unk_1D8.y = data.Read<PackedFloat4>().value();
                va->unk_1D8.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_BODY_TILT:
                m_tiltRotX = (s16)(data.Read<PackedInt2>().value());
                m_tiltRotZ = (s16)(data.Read<PackedInt2>().value());
                m_tiltYOffset = data.Read<PackedFloat4>().value();
                m_tiltKnown = true;
                break;
            case PROP_ANIM_CUR_FRAME:
                va->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &va->skelAnime, LOCK_CUR_FRAME ? va->skelAnime.curFrame : 0.0f, data);
                break;
            }
            case PROP_FIGHT_STATE:
                sBodyState = (u8)(data.Read<PackedUInt1>().value());
                sFightPhase = (u8)(data.Read<PackedUInt1>().value());
                sCsState = (s8)(data.Read<PackedInt1>().value());
                sDoorState = (s16)(data.Read<PackedInt2>().value());
                sPhase3StopMoving = (u8)(data.Read<PackedUInt1>().value());
                sPhase2Timer = (u16)(data.Read<PackedUInt2>().value());
                sPhase4HP = (s8)(data.Read<PackedInt1>().value());
                sKillBari = (u8)(data.Read<PackedUInt1>().value());
                for (int i = 0; i < 10; i++)
                    sBodyBari[i] = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == BossVa_BodyDeath && !IsRunningLocally()) {
            BossVa* va = Typed();

            va->isDead = 0;
            va->burst = 0;
            sCsState = 13;
            BossVa_SetupBodyDeath(va, gPlayState);
            GoLocal();
        }
    }
    void OnTrigger(const std::string& name, ByteStream& data) override {
        if (name != "hit")
            return;

        BossVa* va = Typed();
        va->colliderSph.base.acFlags |= AC_HIT;
        va->colliderSph.base.ac = &GET_PLAYER(gPlayState)->actor;
    }

    void OnBecomeLeader() override {
        if (gPlayState == nullptr || sCsState >= BOSSVA_CS_BATTLE) {
            return;
        }

        if (sCsCamera == SUBCAM_FREE) {
            sCsCamera = Play_CreateSubCamera(gPlayState);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        BossVa* va = Typed();

        //If this is not the main body
        //Then check if the main body is the leader
        //If yes, then we must also become leader
        if (!IsBody()) {
            Actor* bodyActor = va->actor.parent;
            if (bodyActor != nullptr && bodyActor->zoController != nullptr) {
                auto* bodyCtl = static_cast<AbstractActorController*>(bodyActor->zoController);

                if (bodyCtl->IsRunningLocally()) {
                    GoLocal();
                    m_originalUpdate(&va->actor, play);
                    return;
                }

                if (bodyCtl->IsLeader() && !IsIntroPlaying()) {
                    ClaimLeadership(CLAIM_REASON_HIT);
                    UpdateLeader(play);
                    return;
                }
            }
        }

        //When you cut its supports it doesnt just change animation, the whole skeleton
        //is swapped.
        if (IsSupport() && !va->onCeiling && !m_supportSkelSwapped) {
            m_supportSkelSwapped = true;
            SkelAnime_Free(&va->skelAnime, play);
            SkelAnime_InitFlex(play, &va->skelAnime, (FlexSkeletonHeader*)&gBarinadeCutSupportSkel,
                               (AnimationHeader*)&gBarinadeSupportCutAnim, NULL, NULL, 0);
        }

        if (HitWouldReact() && !IsIntroPlaying()) {
            if (IsBody()) {
                ClaimLeadership(CLAIM_REASON_HIT);
                UpdateLeader(play);
                ClearHitFlags();
                return;
            } else {
                SendTriggerToLeader("hit", ByteStream());
            }
        }
        ClearHitFlags();

        if (IsBody())
            va->actor.shape.rot.y += va->headRot.y;

        UpdateAnimation(&va->skelAnime, LOCK_CUR_FRAME);

        if (IsBody() && m_tiltKnown) {
            va->actor.world.rot.x = m_tiltRotX;
            va->actor.world.rot.z = m_tiltRotZ;
            va->actor.shape.yOffset = m_tiltYOffset;
        }

        {
            f32 focusY = 45.0f;
            if (IsBody()) {
                if (m_currentActionIndex == ID_BODY_PHASE3)
                    focusY = 20.0f;
                else if (m_currentActionIndex == ID_BODY_PHASE4)
                    focusY = 60.0f;
            }
            va->actor.focus.pos = va->actor.world.pos;
            va->actor.focus.pos.y += focusY;
        }

        if (IsBari()) {
            va->unk_1D8.y = (Math_CosS(va->timer2 * 0xFA4) * 0.24f) + 0.76f;
            va->unk_1D8.x = (Math_SinS(va->timer2 * 0xFA4) * 0.2f) + 1.0f;
        }

        if (IsBody()) {
            BossVa_UpdateEffects(play);

            play->envCtx.unk_BF = 1;

            play->envCtx.adjFogNear = 0;
            play->envCtx.adjFogFar = 0;
        }

        if (IsSupport() && (va->timer % 2) == 0) {
            f32 range = static_cast<f32>(((va->timer & 0x20) >> 5) + 1);
            if (m_currentActionIndex == ID_SUPPORT_INTRO)
                BossVa_Spark(play, va, 2, 90, 5.0f, 0.0f, SPARK_BODY, range, true);
            else if (m_currentActionIndex == ID_SUPPORT_ATTACHED)
                BossVa_Spark(play, va, 1, 100, 5.0f, 0.0f, SPARK_BODY, range, true);
        }

        if (IsZapper() && va->burst) {
            if (va->timer2 >= 32) {
                BossVa_Spark(play, va, 2, 110, 15.0f, 15.0f, SPARK_BLAST, 5.0f, true);
                BossVa_Spark(play, va, 2, 110, 15.0f, 15.0f, SPARK_BLAST, 6.0f, true);
                BossVa_Spark(play, va, 2, 110, 15.0f, 15.0f, SPARK_BLAST, 7.0f, true);
            } else {
                BossVa_Spark(play, va, 2, 50, 15.0f, 0.0f, SPARK_BODY, (va->timer2 >> 3) + 1.0f, true);
                if (va->timer2 == 20) {
                    Vec3f chargePos = va->zapHeadPos;
                    BossVa_SpawnZapperCharge(play, sVaEffects, va, &chargePos, &va->headRot, 100, 0);
                }
            }
        }

        if (IsBari()) {
            BossVa* body = reinterpret_cast<BossVa*>(va->actor.parent);

            if (m_currentActionIndex == ID_BARI_PHASE2) {
                if (!(sPhase2Timer & 0x100) && body != nullptr && body->actor.colorFilterTimer == 0)
                    BossVa_Spark(play, va, 1, 125, 15.0f, 7.0f, SPARK_TETHER, 1.0f, true);
            } else if (m_currentActionIndex == ID_BARI_PHASE3) {
                if (!sPhase3StopMoving)
                    BossVa_Spark(play, va, 1, 75, 15.0f, 7.0f, SPARK_TETHER, 1.0f, true);
            } else if (m_currentActionIndex == ID_BARI_STUNNED) {
                if (va->timer < 0)
                    BossVa_Spark(play, va, 1, 85, 15.0f, 0.0f, SPARK_TETHER, 1.0f, true);
            }

            if (m_currentActionIndex == ID_BARI_PHASE2 && (play->gameplayFrames % 8) == 0)
                BossVa_Spark(play, va, 1, va->unk_1F0, 25.0f, 20.0f, SPARK_BARI, 2.0f, true);
            else if (m_currentActionIndex == ID_BARI_STUNNED && (play->gameplayFrames % 4) == 0)
                BossVa_Spark(play, va, 1, va->unk_1F0, 25.0f, 20.0f, SPARK_BARI, 2.0f, true);
        }

        if (IsBody()) {
            u8 bodyRoles;
            BodyColliderRoles(&bodyRoles);
            if (bodyRoles != 0) {
                Collider_UpdateCylinder(&va->actor, &va->colliderBody);
                RegisterColliderBase(play, &va->colliderBody.base, bodyRoles);
            }
        }

        if (!va->isDead) {
            u8 sphRoles, lightningRoles;
            PartColliderRoles(&sphRoles, &lightningRoles);
            if (sphRoles != 0)
                RegisterColliderBase(play, &va->colliderSph.base, sphRoles);
            if (lightningRoles != 0)
                RegisterColliderBase(play, &va->colliderLightning.base, lightningRoles);
        }
    }

    void UpdateLeader(PlayState* play) override {
        BossVa* va = Typed();

        AbstractActorController::UpdateLeader(play);

        m_bodyRoles = 0;
        if (va->colliderBody.base.atFlags & AT_ON)
            m_bodyRoles |= COLL_AT;
        if (va->colliderBody.base.acFlags & AC_ON)
            m_bodyRoles |= COLL_AC;
        if (va->colliderBody.base.ocFlags1 & OC1_ON)
            m_bodyRoles |= COLL_OC;

        m_sphRoles = 0;
        if (va->colliderSph.base.atFlags & AT_ON)
            m_sphRoles |= COLL_AT;
        if (va->colliderSph.base.acFlags & AC_ON)
            m_sphRoles |= COLL_AC;
        if (va->colliderSph.base.ocFlags1 & OC1_ON)
            m_sphRoles |= COLL_OC;

        m_lightningRoles = (va->colliderLightning.base.atFlags & AT_ON) ? COLL_AT : 0;
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    s16 m_tiltRotX = 0;
    s16 m_tiltRotZ = 0;
    f32 m_tiltYOffset = 0.0f;
    bool m_tiltKnown = false;
    u8 m_bodyRoles = COLL_AC;
    u8 m_sphRoles = COLL_AC;
    u8 m_lightningRoles = 0;
    bool m_supportSkelSwapped = false;
};

}

#endif
