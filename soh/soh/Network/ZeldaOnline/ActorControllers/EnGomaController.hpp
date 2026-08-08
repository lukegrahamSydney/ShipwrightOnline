#ifndef ENGOMACONTROLLERH
#define ENGOMACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Goma/z_en_goma.h"
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#include "objects/object_gol/object_gol.h"

void EnGoma_Egg(EnGoma* goma, PlayState* play);
void EnGoma_EggFallToGround(EnGoma* goma, PlayState* play);
void EnGoma_Hatch(EnGoma* goma, PlayState* play);
void EnGoma_Stand(EnGoma* goma, PlayState* play);
void EnGoma_ChasePlayer(EnGoma* goma, PlayState* play);
void EnGoma_PrepareJump(EnGoma* goma, PlayState* play);
void EnGoma_Jump(EnGoma* goma, PlayState* play);
void EnGoma_Land(EnGoma* goma, PlayState* play);
void EnGoma_Flee(EnGoma* goma, PlayState* play);
void EnGoma_Hurt(EnGoma* goma, PlayState* play);
void EnGoma_Stunned(EnGoma* goma, PlayState* play);
void EnGoma_Die(EnGoma* goma, PlayState* play);
void EnGoma_Dead(EnGoma* goma, PlayState* play);
void EnGoma_Debris(EnGoma* goma, PlayState* play);
void EnGoma_BossLimb(EnGoma* goma, PlayState* play);

}

namespace ZeldaOnline {

class EnGomaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnGoma* Typed() const {
        return reinterpret_cast<EnGoma*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return params < 10;
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ACTION_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const EnGomaActionFunc* ActionTable(size_t* count) {
        static const EnGomaActionFunc sTable[] = {
            EnGoma_Egg,         EnGoma_EggFallToGround,
            EnGoma_Hatch,       EnGoma_Stand,
            EnGoma_ChasePlayer, EnGoma_PrepareJump,
            EnGoma_Jump,        EnGoma_Land,
            EnGoma_Flee,        EnGoma_Hurt,
            EnGoma_Stunned,     EnGoma_Die,
            EnGoma_Dead,        EnGoma_Debris,
            EnGoma_BossLimb,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const EnGomaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ACTION_UNKNOWN;
    }

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return (const char*)&gObjectGolStandAnim;
            case 1:
                return (const char*)&gObjectGolRunningAnim;
            case 2:
                return (const char*)&gObjectGolJumpHeadbuttAnim;
            case 3:
                return (const char*)&gObjectGolPrepareJumpAnim;
            case 4:
                return (const char*)&gObjectGolLandFromJumpAnim;
            case 5:
                return (const char*)&gObjectGolDamagedAnim;
            case 6:
                return (const char*)&gObjectGolDeathAnim;
            case 7:
                return (const char*)&gObjectGolDeadTwitchingAnim;
            default:
                return nullptr;
        }
    }
    static constexpr int ANIM_COUNT = 8;

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelanime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    u8 Cyl1Roles() const {
        return (Typed()->invincibilityTimer == 0) ? (COLL_OC | COLL_AT) : 0;
    }
    u8 Cyl2Roles() const {
        return (Typed()->invincibilityTimer == 0) ? COLL_AC : 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_TIMERS,
        PROP_EYE_COLOR,
        PROP_GOMA_TYPE,
        PROP_EGG_VISUALS,
        PROP_SLOPE,
        PROP_SCALE_Z,
        PROP_GRAVITY,
        PROP_EYE,
        PROP_COLL_DIMS,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnGoma* goma = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(goma->actor.colChkInfo.health), out);

        PackProperty(PROP_TIMERS, ByteStream()
                                             << PackedInt2(goma->actionTimer) << PackedInt2(goma->invincibilityTimer)
                                             << PackedInt2(goma->stunTimer) << PackedInt2(goma->hatchState), out);

        PackProperty(PROP_EYE_COLOR, ByteStream() << PackedFloat4(goma->eyeEnvColor[0])
                                                         << PackedFloat4(goma->eyeEnvColor[1])
                                                         << PackedFloat4(goma->eyeEnvColor[2]), out);
        PackProperty(PROP_GOMA_TYPE, PackedInt2(goma->gomaType), out);

        PackProperty(PROP_EGG_VISUALS, ByteStream() << PackedFloat4(goma->eggSquishAngle) << PackedFloat4(goma->eggSquishAmount)
                                         << PackedFloat4(goma->eggPitch) << PackedFloat4(goma->eggYOffset), out);

        PackProperty(PROP_SLOPE, ByteStream() << PackedInt2(goma->slopePitch) << PackedInt2(goma->slopeRoll), out);

        PackProperty(PROP_SCALE_Z, PackedFloat4(goma->actor.scale.z), out);

        PackProperty(PROP_GRAVITY, PackedFloat4(goma->actor.gravity), out);

        PackProperty(PROP_EYE, ByteStream() << PackedInt2(goma->eyePitch) << PackedInt2(goma->eyeYaw), out);

        PackProperty(PROP_COLL_DIMS, ByteStream() << PackedInt2(goma->colCyl2.dim.radius)
                                                         << PackedInt2(goma->colCyl2.dim.height)
                                                         << PackedInt2(goma->colCyl2.dim.yShift), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(goma->skelanime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &goma->skelanime), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnGoma* goma = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != ACTION_UNKNOWN) {
                    size_t count;
                    const EnGomaActionFunc* table = ActionTable(&count);
                    if (ai < count)
                        goma->actionFunc = table[ai];
                }
                return true;
            }
            case PROP_ANIM_CUR_FRAME:
                goma->skelanime.curFrame = data.Read<PackedFloat4>().value();
                return true;
            case PROP_ANIM: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (ai != ANIM_UNKNOWN && ai < ANIM_COUNT) ? AnimForIndex(ai) : nullptr;
                ApplyAnimProperty((void*)a, &goma->skelanime, LOCK_CUR_FRAME ? goma->skelanime.curFrame : 0.0f, data);
                return true;
            }
            case PROP_HEALTH:
                goma->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_TIMERS:
                goma->actionTimer = (s16)(data.Read<PackedInt2>().value());
                goma->invincibilityTimer = (s16)(data.Read<PackedInt2>().value());
                goma->stunTimer = (s16)(data.Read<PackedInt2>().value());
                goma->hatchState = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_EYE_COLOR:
                goma->eyeEnvColor[0] = data.Read<PackedFloat4>().value();
                goma->eyeEnvColor[1] = data.Read<PackedFloat4>().value();
                goma->eyeEnvColor[2] = data.Read<PackedFloat4>().value();
                return true;
            case PROP_GOMA_TYPE:
                goma->gomaType = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_EGG_VISUALS:
                goma->eggSquishAngle = data.Read<PackedFloat4>().value();
                goma->eggSquishAmount = data.Read<PackedFloat4>().value();
                goma->eggPitch = data.Read<PackedFloat4>().value();
                goma->eggYOffset = data.Read<PackedFloat4>().value();
                return true;
            case PROP_SLOPE:
                goma->slopePitch = (s16)(data.Read<PackedInt2>().value());
                goma->slopeRoll = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_SCALE_Z:
                Actor_SetScale(m_actor, data.Read<PackedFloat4>().value());
                return true;
            case PROP_GRAVITY:
                goma->actor.gravity = data.Read<PackedFloat4>().value();
                return true;
            case PROP_EYE:
                goma->eyePitch = (s16)(data.Read<PackedInt2>().value());
                goma->eyeYaw = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_COLL_DIMS:
                goma->colCyl2.dim.radius = (s16)(data.Read<PackedInt2>().value());
                goma->colCyl2.dim.height = (s16)(data.Read<PackedInt2>().value());
                goma->colCyl2.dim.yShift = (s16)(data.Read<PackedInt2>().value());
                return true;
            default:
                return false;
        }
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnGoma_Die && !IsRunningLocally())
            GoLocal();
    }

    void UpdatePuppet(PlayState* play) override {
        EnGoma* goma = Typed();

        UpdateAnimation(&goma->skelanime, LOCK_CUR_FRAME);

        if (IsNetworkedVariant(goma->actor.params)) {
            Actor_SetFocus(&goma->actor, 20.0f);
        }

        if ((goma->colCyl1.base.acFlags & AC_HIT) || (goma->colCyl2.base.acFlags & AC_HIT)) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        u8 action = CurrentActionIndex();
        bool engaged = action >= 3 && action <= 7;
        if (engaged && goma->actor.xzDistToPlayer < 200.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        RegisterCylinder(play, &goma->colCyl1, Cyl1Roles());
        RegisterCylinder(play, &goma->colCyl2, Cyl2Roles());
    }

    void OnServerDestroy() override {
        EnGoma* goma = Typed();
        if (goma->actor.params >= 3) {
            AbstractActorController::OnServerDestroy();
            return;
        }
        Actor* boss = goma->actor.parent;
        if (boss != NULL && boss->id == ACTOR_BOSS_GOMA && boss->update != NULL)
            ((BossGoma*)boss)->childrenGohmaState[goma->actor.params] = -1;
        AbstractActorController::OnServerDestroy();
    }
};

}

#endif
