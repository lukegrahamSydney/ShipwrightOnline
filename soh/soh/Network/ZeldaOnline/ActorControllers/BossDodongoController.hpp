#ifndef BOSSDODONGOCONTROLLERH
#define BOSSDODONGOCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Boss_Dodongo/z_boss_dodongo.h"
#include "objects/object_kingdodongo/object_kingdodongo.h"

void BossDodongo_IntroCutscene(BossDodongo* boss, PlayState* play);
void BossDodongo_Walk(BossDodongo* boss, PlayState* play);
void BossDodongo_Inhale(BossDodongo* boss, PlayState* play);
void BossDodongo_BlowFire(BossDodongo* boss, PlayState* play);
void BossDodongo_Roll(BossDodongo* boss, PlayState* play);
void BossDodongo_Explode(BossDodongo* boss, PlayState* play);
void BossDodongo_LayDown(BossDodongo* boss, PlayState* play);
void BossDodongo_Vulnerable(BossDodongo* boss, PlayState* play);
void BossDodongo_GetUp(BossDodongo* boss, PlayState* play);
void BossDodongo_Damaged(BossDodongo* boss, PlayState* play);
void BossDodongo_DeathCutscene(BossDodongo* boss, PlayState* play);

void BossDodongo_SetupDeathCutscene(BossDodongo* boss);

void BossDodongo_UpdateEffects(PlayState* play);
}

namespace ZeldaOnline {

class BossDodongoController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BossDodongo* Typed() const {
        return reinterpret_cast<BossDodongo*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_DEATH_CUTSCENE = 10;

    using DodongoActionFunc = void (*)(BossDodongo*, PlayState*);
    static const DodongoActionFunc* ActionTable(size_t* count) {
        static const DodongoActionFunc sTable[] = {
            BossDodongo_IntroCutscene,
            BossDodongo_Walk,
            BossDodongo_Inhale,
            BossDodongo_BlowFire,
            BossDodongo_Roll,
            BossDodongo_Explode,
            BossDodongo_LayDown,
            BossDodongo_Vulnerable,
            BossDodongo_GetUp,
            BossDodongo_Damaged,
            BossDodongo_DeathCutscene,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DodongoActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 13;
    static constexpr u8 ANIM_ROLL_CURL = 5;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case 0:
                return object_kingdodongo_Anim_00F0D8;
            case 1:
                return object_kingdodongo_Anim_008EEC;
            case 2:
                return object_kingdodongo_Anim_001074;
            case 3:
                return object_kingdodongo_Anim_00E848;
            case 4:
                return object_kingdodongo_Anim_01D934;
            case 5:
                return object_kingdodongo_Anim_00DF38;
            case 6:
                return object_kingdodongo_Anim_0061D4;
            case 7:
                return object_kingdodongo_Anim_004E0C;
            case 8:
                return object_kingdodongo_Anim_0042A8;
            case 9:
                return object_kingdodongo_Anim_009D10;
            case 10:
                return object_kingdodongo_Anim_01CAE0;
            case 11:
                return object_kingdodongo_Anim_002D0C;
            case 12:
                return object_kingdodongo_Anim_003CF8;
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

    u8 CurrentColliderRoles() const {
        BossDodongo* b = Typed();
        if (b->unk_1BC != 0)
            return 0;
        u8 roles = COLL_OC;
        if (b->actionFunc != BossDodongo_DeathCutscene)
            roles |= COLL_AC;
        if (b->actionFunc == BossDodongo_Roll)
            roles |= COLL_AT;
        return roles;
    }

    void ApplyElementScales() {
        BossDodongo* b = Typed();
        b->collider.elements[0].dim.scale = (b->actionFunc == BossDodongo_Inhale) ? 0.0f : 1.0f;
        for (int i = 6; i < 19; i++) {
            if (i != 12)
                b->collider.elements[i].dim.scale = (b->actionFunc == BossDodongo_Roll) ? 0.0f : 1.0f;
        }
    }

    bool HitWouldReact() const {
        BossDodongo* b = Typed();
        if (b->unk_1C0 != 0)
            return false;

        if (b->actionFunc == BossDodongo_Inhale) {
            for (int i = 0; i < 19; i++) {
                if (b->collider.elements[i].info.bumperFlags & 2) {
                    ColliderInfo* hit = b->collider.elements[i].info.acHitInfo;
                    if (hit != nullptr && ((hit->toucher.dmgFlags & 0x10) || (hit->toucher.dmgFlags & 4)))
                        return true;
                }
            }
        }

        if ((b->collider.elements[0].info.bumperFlags & 2) &&
            (b->actionFunc == BossDodongo_Vulnerable || b->actionFunc == BossDodongo_LayDown))
            return true;

        return false;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_CS_STATE,
        PROP_TIMERS,
        PROP_PHASE,
        PROP_SUBSTATE,
        PROP_PLAYER_CHECKS,
        PROP_DRAW_ANGLE,
        PROP_COLOR_FILTERRGB,
        PROP_FLOATS_A,
        PROP_FLOATS_B,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
        PROP_JOINT_TABLE,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BossDodongo* b = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedInt2(b->health), out);
        PackProperty(PROP_CS_STATE, PackedInt2(b->csState), out);

        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(b->unk_19E) << PackedInt2(b->unk_1DA) << PackedInt2(b->unk_1DC)
                                  << PackedInt2(b->unk_1DE) << PackedInt2(b->unk_1C0) << PackedInt2(b->unk_1C8),
                     out);
        PackProperty(PROP_PHASE,
                     ByteStream() << PackedInt2(b->unk_196) << PackedInt2(b->unk_198) << PackedInt2(b->unk_19A)
                                  << PackedInt2(b->unk_1A0) << PackedInt2(b->unk_1A2)
                                  << PackedInt2(b->numWallCollisions),
                     out);
        PackProperty(PROP_SUBSTATE,
                     ByteStream() << PackedInt2(b->unk_1AA) << PackedInt2(b->unk_1AC) << PackedInt2(b->unk_1AE)
                                  << PackedInt2(b->unk_1B6) << PackedInt2(b->unk_1BC),
                     out);

        PackProperty(PROP_PLAYER_CHECKS,
                     ByteStream() << PackedInt2(b->unk_1A4) << PackedInt2(b->unk_1A6) << PackedInt2(b->playerYawInRange)
                                  << PackedInt2(b->playerPosInRange),
                     out);

        PackProperty(PROP_DRAW_ANGLE, PackedInt2(b->unk_1C4), out);

        PackProperty(PROP_COLOR_FILTERRGB,
                     ByteStream() << PackedFloat4(b->colorFilterR) << PackedFloat4(b->colorFilterG)
                                  << PackedFloat4(b->colorFilterB) << PackedFloat4(b->colorFilterMin)
                                  << PackedFloat4(b->colorFilterMax) << PackedInt2(b->unk_1BE),
                     out);

        PackProperty(PROP_FLOATS_A,
                     ByteStream() << PackedFloat4(b->unk_1E4) << PackedFloat4(b->unk_1E8) << PackedFloat4(b->unk_1EC)
                                  << PackedFloat4(b->unk_1F8),
                     out);
        PackProperty(PROP_FLOATS_B,
                     ByteStream() << PackedFloat4(b->unk_204) << PackedFloat4(b->unk_208) << PackedFloat4(b->unk_20C)
                                  << PackedFloat4(b->unk_228) << PackedFloat4(b->unk_230),
                     out);

        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(b->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &b->skelAnime), out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BossDodongo* b = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const DodongoActionFunc* table = ActionTable(&count);
                if (id < count)
                    b->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                b->health = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_CS_STATE:
                b->csState = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_TIMERS:
                b->unk_19E = (s16)(data.Read<PackedInt2>().value());
                b->unk_1DA = (s16)(data.Read<PackedInt2>().value());
                b->unk_1DC = (s16)(data.Read<PackedInt2>().value());
                b->unk_1DE = (s16)(data.Read<PackedInt2>().value());
                b->unk_1C0 = (s16)(data.Read<PackedInt2>().value());
                b->unk_1C8 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_PHASE:
                b->unk_196 = (s16)(data.Read<PackedInt2>().value());
                b->unk_198 = (s16)(data.Read<PackedInt2>().value());
                b->unk_19A = (s16)(data.Read<PackedInt2>().value());
                b->unk_1A0 = (s16)(data.Read<PackedInt2>().value());
                b->unk_1A2 = (s16)(data.Read<PackedInt2>().value());
                b->numWallCollisions = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SUBSTATE:
                b->unk_1AA = (s16)(data.Read<PackedInt2>().value());
                b->unk_1AC = (s16)(data.Read<PackedInt2>().value());
                b->unk_1AE = (s16)(data.Read<PackedInt2>().value());
                b->unk_1B6 = (s16)(data.Read<PackedInt2>().value());
                b->unk_1BC = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_PLAYER_CHECKS:
                b->unk_1A4 = (s16)(data.Read<PackedInt2>().value());
                b->unk_1A6 = (s16)(data.Read<PackedInt2>().value());
                b->playerYawInRange = (s16)(data.Read<PackedInt2>().value());
                b->playerPosInRange = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_DRAW_ANGLE:
                b->unk_1C4 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLOR_FILTERRGB:
                b->colorFilterR = data.Read<PackedFloat4>().value();
                b->colorFilterG = data.Read<PackedFloat4>().value();
                b->colorFilterB = data.Read<PackedFloat4>().value();
                b->colorFilterMin = data.Read<PackedFloat4>().value();
                b->colorFilterMax = data.Read<PackedFloat4>().value();
                b->unk_1BE = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_FLOATS_A:
                b->unk_1E4 = data.Read<PackedFloat4>().value();
                b->unk_1E8 = data.Read<PackedFloat4>().value();
                b->unk_1EC = data.Read<PackedFloat4>().value();
                b->unk_1F8 = data.Read<PackedFloat4>().value();
                break;
            case PROP_FLOATS_B:
                b->unk_204 = data.Read<PackedFloat4>().value();
                b->unk_208 = data.Read<PackedFloat4>().value();
                b->unk_20C = data.Read<PackedFloat4>().value();
                b->unk_228 = data.Read<PackedFloat4>().value();
                b->unk_230 = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                b->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentAnimIndex = id;
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &b->skelAnime, LOCK_CUR_FRAME ? b->skelAnime.curFrame : 0.0f, data);
                break;
            }

            default:
                return false;
        }
        return true;
    }

    void OnActorInit() override {
        Typed()->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    }

    void OnPropertiesApplied(u64 changed) override {

        if (Typed()->actionFunc == BossDodongo_DeathCutscene && !IsRunningLocally()) {
            BossDodongo_SetupDeathCutscene(Typed());
            GoLocal();
        }
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId == ACTOR_EN_BDFIRE;
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
        if (Typed()->actionFunc == BossDodongo_DeathCutscene && !IsRunningLocally()) {
            GoLocal();
        }
    }

    void OnBecomeLeader() override {
        if (gPlayState == nullptr) {
            return;
        }

        BossDodongo* b = Typed();
        if (b->actionFunc == BossDodongo_IntroCutscene && b->cutsceneCamera == MAIN_CAM) {
            b->cutsceneCamera = Play_CreateSubCamera(gPlayState);
        }
    }

    void UpdatePuppet(PlayState* play) override {
        BossDodongo* b = Typed();

        UpdateAnimation(&b->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        b->collider.base.acFlags &= ~AC_HIT;
        for (int i = 0; i < 19; i++)
            b->collider.elements[i].info.bumperFlags &= ~2;

        if (b->actionFunc != BossDodongo_IntroCutscene && b->actionFunc != BossDodongo_DeathCutscene && b->actionFunc != BossDodongo_Roll &&
            b->actor.xzDistToPlayer < 600.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        for (int i = 0; i < 50; i++)
            b->unk_25C[i] += b->unk_324[i];
        if ((b->unk_19E % 128) == 0) {
            for (int i = 0; i < 50; i++)
                b->unk_324[i] = (Rand_ZeroOne() * 0.25f) + 0.5f;
        }

        BossDodongo_UpdateEffects(play);

        ApplyElementScales();

        RegisterColliderBase(play, &b->collider.base, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AC | COLL_OC;
    u8 m_currentAnimIndex = ANIM_UNKNOWN;
};

}

#endif
