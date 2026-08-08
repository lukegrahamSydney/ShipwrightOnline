#ifndef FLYINGSKULLCONTROLLERH
#define FLYINGSKULLCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bb/z_en_bb.h"
#include "assets/objects/object_Bb/object_Bb.h"

void EnBb_FlameTrail(EnBb* bb, PlayState* play);
void EnBb_Death(EnBb* bb, PlayState* play);
void EnBb_Damage(EnBb* bb, PlayState* play);
void EnBb_Blue(EnBb* bb, PlayState* play);
void EnBb_Down(EnBb* bb, PlayState* play);
void EnBb_Red(EnBb* bb, PlayState* play);
void EnBb_White(EnBb* bb, PlayState* play);
void EnBb_Green(EnBb* bb, PlayState* play);
void EnBb_Stunned(EnBb* bb, PlayState* play);

void EnBb_SetupDeath(EnBb* bb, PlayState* play);
}

namespace ZeldaOnline {

typedef enum {
     BB_DAMAGE,
     BB_KILL,
     BB_FLAME_TRAIL,
     BB_DOWN,
     BB_STUNNED,
     BB_UNUSED,
     BB_BLUE,
     BB_RED,
     BB_WHITE,
     BB_GREEN
} EnBbAction;

typedef enum {
     BBMOVE_NORMAL,
     BBMOVE_NOCLIP,
     BBMOVE_HIDDEN
} EnBbMoveMode;

class FlyingSkullController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBb* Typed() const {
        return reinterpret_cast<EnBb*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return (s8)(params) <= ENBB_BLUE;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return false;
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr u8 ID_DEATH = 1;

    using BbActionFunc = void (*)(EnBb*, PlayState*);
    static const BbActionFunc* ActionTable(size_t* count) {
        static const BbActionFunc sTable[] = {
            EnBb_FlameTrail,
            EnBb_Death,
            EnBb_Damage,
            EnBb_Blue,
            EnBb_Down,
            EnBb_Red,
            EnBb_White,
            EnBb_Green,
            EnBb_Stunned,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BbActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_A = 0;
    static constexpr u8 ANIM_B = 1;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 2;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_A:
                return object_Bb_Anim_000444;
            case ANIM_B:
                return object_Bb_Anim_000184;
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
        EnBb* bb = Typed();
        u8 roles = 0;

        if (bb->action > BB_KILL && (bb->actor.speedXZ != 0.0f || bb->action == BB_GREEN))
            roles |= COLL_AT;

        if (bb->action > BB_FLAME_TRAIL &&
            (bb->actor.colorFilterTimer == 0 || !(bb->actor.colorFilterParams & 0x4000)) &&
            bb->moveMode != BBMOVE_HIDDEN)
            roles |= COLL_AC | COLL_OC;

        return roles;
    }

    bool HitWouldReact() const {
        EnBb* bb = Typed();
        if (!(bb->collider.base.acFlags & AC_HIT))
            return false;
        return bb->actor.colChkInfo.damageEffect != 0xD;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ACTION_NUM,
        PROP_MOVE_MODE,
        PROP_HEALTH,
        PROP_STATE,
        PROP_BOB,
        PROP_FLAME,
        PROP_REACTION,
        PROP_PATH,
        PROP_HOME_POS,
        PROP_BODY_BREAK,
        PROP_COLL_ROLES,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBb* bb = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ACTION_NUM, PackedInt4(bb->action), out);
        PackProperty(PROP_MOVE_MODE, PackedInt4(bb->moveMode), out);
        PackProperty(PROP_HEALTH, PackedUInt1(bb->actor.colChkInfo.health), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt2(bb->actionState) << PackedInt2(bb->charge) << PackedInt2(bb->actionVar1)
                                  << PackedInt2(bb->actionVar2) << PackedInt4(bb->timer),
                     out);
        PackProperty(PROP_BOB,
                     ByteStream() << PackedFloat4(bb->bobPhase) << PackedFloat4(bb->bobSize)
                                  << PackedFloat4(bb->bobSpeedMod) << PackedFloat4(bb->flyHeightMod)
                                  << PackedFloat4(bb->maxSpeed),
                     out);
        PackProperty(PROP_FLAME,
                     ByteStream() << PackedFloat4(bb->flameScaleY) << PackedFloat4(bb->flameScaleX)
                                  << PackedInt2(bb->flameScrollMod) << PackedUInt1(bb->flamePrimBlue)
                                  << PackedUInt1(bb->flamePrimAlpha) << PackedUInt1(bb->flameEnvColor.r)
                                  << PackedUInt1(bb->flameEnvColor.g) << PackedUInt1(bb->flameEnvColor.b),
                     out);
        PackProperty(PROP_REACTION, ByteStream() << PackedInt2(bb->fireIceTimer) << PackedUInt1(bb->dmgEffect), out);
        PackProperty(PROP_PATH,
                     ByteStream() << PackedUInt1(bb->waypoint) << PackedFloat4(bb->waypointPos.x)
                                  << PackedFloat4(bb->waypointPos.y) << PackedFloat4(bb->waypointPos.z),
                     out);
        PackProperty(PROP_HOME_POS,
                     ByteStream() << PackedFloat4(bb->actor.home.pos.x) << PackedFloat4(bb->actor.home.pos.y)
                                  << PackedFloat4(bb->actor.home.pos.z),
                     out);
        PackProperty(PROP_BODY_BREAK, PackedInt2(bb->bodyBreak.val), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &bb->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBb* bb = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const BbActionFunc* table = ActionTable(&count);
                if (id < count)
                    bb->actionFunc = table[id];
                break;
            }
            case PROP_ACTION_NUM:
                bb->action = data.Read<PackedInt4>().value();
                break;
            case PROP_MOVE_MODE:
                bb->moveMode = data.Read<PackedInt4>().value();
                break;
            case PROP_HEALTH:
                bb->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_STATE:
                bb->actionState = (s16)(data.Read<PackedInt2>().value());
                bb->charge = (s16)(data.Read<PackedInt2>().value());
                bb->actionVar1 = (s16)(data.Read<PackedInt2>().value());
                bb->actionVar2 = (s16)(data.Read<PackedInt2>().value());
                bb->timer = data.Read<PackedInt4>().value();
                break;
            case PROP_BOB:
                bb->bobPhase = data.Read<PackedFloat4>().value();
                bb->bobSize = data.Read<PackedFloat4>().value();
                bb->bobSpeedMod = data.Read<PackedFloat4>().value();
                bb->flyHeightMod = data.Read<PackedFloat4>().value();
                bb->maxSpeed = data.Read<PackedFloat4>().value();
                break;
            case PROP_FLAME:
                bb->flameScaleY = data.Read<PackedFloat4>().value();
                bb->flameScaleX = data.Read<PackedFloat4>().value();
                bb->flameScrollMod = (s16)(data.Read<PackedInt2>().value());
                bb->flamePrimBlue = (u8)(data.Read<PackedUInt1>().value());
                bb->flamePrimAlpha = (u8)(data.Read<PackedUInt1>().value());
                bb->flameEnvColor.r = (u8)(data.Read<PackedUInt1>().value());
                bb->flameEnvColor.g = (u8)(data.Read<PackedUInt1>().value());
                bb->flameEnvColor.b = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_REACTION:
                bb->fireIceTimer = (s16)(data.Read<PackedInt2>().value());
                bb->dmgEffect = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PATH:
                bb->waypoint = (u8)(data.Read<PackedUInt1>().value());
                bb->waypointPos.x = data.Read<PackedFloat4>().value();
                bb->waypointPos.y = data.Read<PackedFloat4>().value();
                bb->waypointPos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_HOME_POS:
                bb->actor.home.pos.x = data.Read<PackedFloat4>().value();
                bb->actor.home.pos.y = data.Read<PackedFloat4>().value();
                bb->actor.home.pos.z = data.Read<PackedFloat4>().value();
                break;
            case PROP_BODY_BREAK:
                bb->bodyBreak.val = (s16)(data.Read<PackedInt2>().value());
                break;

            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &bb->skelAnime, LOCK_CUR_FRAME ? bb->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnBb* bb = Typed();

        UpdateAnimation(&bb->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        bb->collider.base.acFlags &= ~AC_HIT;
        bb->collider.base.atFlags &= ~AT_HIT;

        if (m_currentActionIndex != ID_DEATH && bb->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        bb->actor.focus.pos = bb->actor.world.pos;

        bb->collider.elements->dim.worldSphere.center.x = (s16)(bb->actor.world.pos.x);
        bb->collider.elements->dim.worldSphere.center.y =
            (s16)(bb->actor.world.pos.y + (bb->actor.shape.yOffset * bb->actor.scale.y));
        bb->collider.elements->dim.worldSphere.center.z = (s16)(bb->actor.world.pos.z);

        if (m_roles != 0)
            RegisterColliderBase(play, &bb->collider.base, m_roles);
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AC | COLL_OC;
};

}

#endif
