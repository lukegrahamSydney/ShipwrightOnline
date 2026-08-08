#ifndef DEADHANDCONTROLLERH
#define DEADHANDCONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Dh/z_en_dh.h"
#include "assets/objects/object_dh/object_dh.h"

void EnDh_Wait(EnDh* dh, PlayState* play);
void EnDh_Walk(EnDh* dh, PlayState* play);
void EnDh_Retreat(EnDh* dh, PlayState* play);
void EnDh_Attack(EnDh* dh, PlayState* play);
void EnDh_Burrow(EnDh* dh, PlayState* play);
void EnDh_Damage(EnDh* dh, PlayState* play);
void EnDh_Death(EnDh* dh, PlayState* play);

void EnDh_SetupDeath(EnDh* dh);
}

namespace ZeldaOnline {

typedef enum {
     DH_WAIT,
     DH_RETREAT,
     DH_BURROW,
     DH_WALK,
     DH_ATTACK,
     DH_DEATH,
     DH_DAMAGE
} EnDhAction;

class DeadHandController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDh* Typed() const {
        return reinterpret_cast<EnDh*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using DhActionFunc = void (*)(EnDh*, PlayState*);
    static const DhActionFunc* ActionTable(size_t* count) {
        static const DhActionFunc sTable[] = {
            EnDh_Wait,
            EnDh_Retreat,
            EnDh_Burrow,
            EnDh_Walk,
            EnDh_Attack,
            EnDh_Death,
            EnDh_Damage,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DhActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 8;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case 0:
                return object_dh_Anim_001A3C;
            case 1:
                return object_dh_Anim_002148;
            case 2:
                return object_dh_Anim_0032BC;
            case 3:
                return object_dh_Anim_00375C;
            case 4:
                return object_dh_Anim_003A8C;
            case 5:
                return object_dh_Anim_003D6C;
            case 6:
                return object_dh_Anim_004658;
            case 7:
                return object_dh_Anim_005880;
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

    bool HitWouldReact() const {
        EnDh* dh = Typed();
        if (!(dh->collider2.base.acFlags & AC_HIT) || dh->retreat)
            return false;
        u8 effect = dh->actor.colChkInfo.damageEffect;
        return effect != 0 && effect != 6;
    }

    bool AnyHit() const {
        EnDh* dh = Typed();
        return (dh->collider1.base.acFlags & AC_HIT) || (dh->collider2.base.acFlags & AC_HIT);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_CUR_ACTION,
        PROP_PARAMS,
        PROP_HEALTH,
        PROP_STATE,
        PROP_DIRT_WAVE,
        PROP_BITE,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnDh* dh = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_CUR_ACTION, ByteStream() << PackedUInt1(dh->curAction) << PackedUInt1(dh->actionState), out);
        PackProperty(PROP_PARAMS, PackedInt2(dh->actor.params), out);
        PackProperty(PROP_HEALTH, PackedUInt1(dh->actor.colChkInfo.health), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(dh->retreat) << PackedUInt1(dh->unk_258) << PackedUInt1(dh->alpha)
                                  << PackedInt2(dh->timer),
                     out);
        PackProperty(PROP_DIRT_WAVE,
                     ByteStream() << PackedUInt1(dh->drawDirtWave) << PackedInt2(dh->dirtWavePhase)
                                  << PackedFloat4(dh->dirtWaveSpread) << PackedFloat4(dh->dirtWaveHeight)
                                  << PackedFloat4(dh->dirtWaveAlpha),
                     out);
        PackProperty(PROP_BITE,
                     ByteStream() << PackedUInt1(dh->collider2.base.atFlags)
                                  << PackedUInt1(dh->collider2.elements[0].info.toucherFlags)
                                  << PackedUInt4(dh->collider2.elements[0].info.toucher.dmgFlags)
                                  << PackedUInt1(dh->collider2.elements[0].info.toucher.damage),
                     out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(dh->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &dh->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDh* dh = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const DhActionFunc* table = ActionTable(&count);
                if (id < count)
                    dh->actionFunc = table[id];
                break;
            }
            case PROP_CUR_ACTION:
                dh->curAction = (u8)(data.Read<PackedUInt1>().value());
                dh->actionState = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_PARAMS:
                dh->actor.params = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HEALTH:
                dh->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_STATE:
                dh->retreat = (u8)(data.Read<PackedUInt1>().value());
                dh->unk_258 = (u8)(data.Read<PackedUInt1>().value());
                dh->alpha = (u8)(data.Read<PackedUInt1>().value());
                dh->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_DIRT_WAVE:
                dh->drawDirtWave = (u8)(data.Read<PackedUInt1>().value());
                dh->dirtWavePhase = (s16)(data.Read<PackedInt2>().value());
                dh->dirtWaveSpread = data.Read<PackedFloat4>().value();
                dh->dirtWaveHeight = data.Read<PackedFloat4>().value();
                dh->dirtWaveAlpha = data.Read<PackedFloat4>().value();
                break;
            case PROP_BITE:
                dh->collider2.base.atFlags = (u8)(data.Read<PackedUInt1>().value());
                dh->collider2.elements[0].info.toucherFlags = (u8)(data.Read<PackedUInt1>().value());
                dh->collider2.elements[0].info.toucher.dmgFlags = data.Read<PackedUInt4>().value();
                dh->collider2.elements[0].info.toucher.damage = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_ANIM_CUR_FRAME:
                dh->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &dh->skelAnime, LOCK_CUR_FRAME ? dh->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnDh_Death) {
            EnDh_SetupDeath(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnDh* dh = Typed();

        UpdateAnimation(&dh->skelAnime, LOCK_CUR_FRAME);

        if (AnyHit()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        dh->collider1.base.acFlags &= ~AC_HIT;
        dh->collider2.base.acFlags &= ~AC_HIT;
        dh->collider2.base.atFlags &= ~(AT_HIT | AT_BOUNCED);

        dh->actor.focus.pos = dh->headPos;

        Collider_UpdateCylinder(&dh->actor, &dh->collider1);

        if (dh->actor.colChkInfo.health > 0) {
            if (dh->curAction == DH_WAIT) {
                RegisterColliderBase(play, &dh->collider1.base, COLL_AC);
            } else {
                RegisterColliderBase(play, &dh->collider1.base, COLL_OC);
            }

            Player* player = GET_PLAYER(play);
            if ((dh->curAction != DH_DAMAGE && dh->actor.shape.yOffset == 0.0f) ||
                (player->unk_844 != 0 && player->unk_845 != dh->unk_258)) {
                RegisterColliderBase(play, &dh->collider2.base, COLL_AC | COLL_AT);
                RegisterColliderBase(play, &dh->collider1.base, COLL_AT);
            }
        } else {
            RegisterColliderBase(play, &dh->collider1.base, COLL_OC);
            RegisterColliderBase(play, &dh->collider2.base, COLL_OC);
        }
    }
};

}

#endif
