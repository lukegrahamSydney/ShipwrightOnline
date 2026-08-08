#ifndef BIGOKUTACONTROLLERH
#define BIGOKUTACONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bigokuta/z_en_bigokuta.h"
#include "objects/object_bigokuta/object_bigokuta.h"

void func_809BD84C(EnBigokuta* bo, PlayState* play);
void func_809BD8DC(EnBigokuta* bo, PlayState* play);
void func_809BDAE8(EnBigokuta* bo, PlayState* play);
void func_809BDB90(EnBigokuta* bo, PlayState* play);
void func_809BDC08(EnBigokuta* bo, PlayState* play);
void func_809BDF34(EnBigokuta* bo, PlayState* play);
void func_809BDFC8(EnBigokuta* bo, PlayState* play);
void func_809BE058(EnBigokuta* bo, PlayState* play);
void func_809BE180(EnBigokuta* bo, PlayState* play);
void func_809BE26C(EnBigokuta* bo, PlayState* play);
void func_809BE3E4(EnBigokuta* bo, PlayState* play);
void func_809BE4A4(EnBigokuta* bo, PlayState* play);
void func_809BE518(EnBigokuta* bo, PlayState* play);

void func_809BD658(EnBigokuta* bo);
}

namespace ZeldaOnline {

class BigokutaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBigokuta* Typed() const {
        return reinterpret_cast<EnBigokuta*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    static constexpr f32 kCylinderOffsetX[2] = { 30.0f, -30.0f };
    static constexpr f32 kCylinderOffsetZ[2] = { 12.0f, 12.0f };

    using BigokutaActionFunc = void (*)(EnBigokuta*, PlayState*);
    static const BigokutaActionFunc* ActionTable(size_t* count) {
        static const BigokutaActionFunc sTable[] = {
            func_809BD84C,
            func_809BD8DC,
            func_809BDAE8,
            func_809BDB90,
            func_809BDC08,
            func_809BDF34,
            func_809BDFC8,
            func_809BE058,
            func_809BE180,
            func_809BE26C,
            func_809BE3E4,
            func_809BE4A4,
            func_809BE518,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BigokutaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_0 = 0;
    static constexpr u8 ANIM_1 = 1;
    static constexpr u8 ANIM_2 = 2;
    static constexpr u8 ANIM_3 = 3;
    static constexpr u8 ANIM_4 = 4;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 5;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_0:
                return object_bigokuta_Anim_000444;
            case ANIM_1:
                return object_bigokuta_Anim_000A74;
            case ANIM_2:
                return object_bigokuta_Anim_000D1C;
            case ANIM_3:
                return object_bigokuta_Anim_0014B8;
            case ANIM_4:
                return object_bigokuta_Anim_001CA4;
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
        EnBigokuta* bo = Typed();
        if (!(bo->collider.base.acFlags & AC_HIT))
            return false;
        if (bo->actor.colChkInfo.damageEffect == 0 && bo->actor.colChkInfo.damage == 0)
            return false;
        return true;
    }

    u8 CurrentBodyRoles() const {
        EnBigokuta* bo = Typed();
        return (bo->collider.base.acFlags & AC_ON) ? (COLL_AC | COLL_OC) : COLL_AT;
    }
    u8 CurrentTentacleRoles() const {
        EnBigokuta* bo = Typed();
        if (!(bo->cylinder[0].base.atFlags & AT_ON))
            return 0;
        u8 roles = COLL_AC;
        roles |= (bo->actionFunc == func_809BE058) ? COLL_OC : COLL_AT;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_BATTERY,
        PROP_HOME_HEIGHT,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBigokuta* bo = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(bo->actor.colChkInfo.health), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_BATTERY,
                     ByteStream() << PackedUInt1((u8)(bo->unk_194)) << PackedUInt1(bo->unk_195)
                                  << PackedInt2(bo->unk_196) << PackedInt2(bo->unk_198) << PackedInt2(bo->unk_19A),
                     out);
        PackProperty(PROP_HOME_HEIGHT, PackedFloat4(bo->actor.home.pos.y), out);
        PackProperty(PROP_COLL_ROLES,
                     ByteStream() << PackedUInt1(CurrentBodyRoles()) << PackedUInt1(CurrentTentacleRoles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(bo->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &bo->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBigokuta* bo = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BigokutaActionFunc* table = ActionTable(&count);
                if (id < count)
                    bo->actionFunc = table[id];
                break;
            }
            case PROP_HEALTH:
                bo->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_BATTERY:
                bo->unk_194 = (s8)(data.Read<PackedUInt1>().value());
                bo->unk_195 = (u8)(data.Read<PackedUInt1>().value());
                bo->unk_196 = (s16)(data.Read<PackedInt2>().value());
                bo->unk_198 = (s16)(data.Read<PackedInt2>().value());
                bo->unk_19A = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_HOME_HEIGHT:
                bo->actor.home.pos.y = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_bodyRoles = (u8)(data.Read<PackedUInt1>().value());
                m_tentacleRoles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                bo->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &bo->skelAnime, LOCK_CUR_FRAME ? bo->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == func_809BE26C) {
            func_809BD658(Typed());
            GoLocal();
        }
    }

    void UpdatePuppet(PlayState* play) override {
        EnBigokuta* bo = Typed();

        UpdateAnimation(&bo->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_HIT);
            UpdateLeader(play);
            return;
        }
        bo->collider.base.acFlags &= ~AC_HIT;
        bo->cylinder[0].base.atFlags &= ~AT_HIT;
        bo->cylinder[1].base.atFlags &= ~AT_HIT;

        if (bo->actionFunc != func_809BE26C && bo->actor.colChkInfo.health > 0 && bo->actor.xzDistToPlayer < 400.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        Actor_SetFocus(&bo->actor, bo->actor.scale.y * 25.0f * 100.0f);

        {
            EnBigokuta* b = bo;
            f32 sinRot = Math_SinS(b->actor.shape.rot.y);
            f32 cosRot = Math_CosS(b->actor.shape.rot.y);
            f32 modelCenterX = b->collider.elements->dim.modelSphere.center.x;
            f32 modelCenterY = b->collider.elements->dim.modelSphere.center.y;
            f32 modelCenterZ = b->collider.elements->dim.modelSphere.center.z;

            b->collider.elements->dim.worldSphere.center.x =
                (s16)((modelCenterZ * sinRot) + (b->actor.world.pos.x + (modelCenterX * cosRot)));
            b->collider.elements->dim.worldSphere.center.z =
                (s16)((b->actor.world.pos.z + (modelCenterZ * cosRot)) - (modelCenterX * sinRot));
            b->collider.elements->dim.worldSphere.center.y = (s16)(modelCenterY + b->actor.world.pos.y);
            b->collider.elements->dim.worldSphere.radius =
                (s16)(b->collider.elements->dim.modelSphere.radius * b->collider.elements->dim.scale);

            for (s32 i = 0; i < 2; i++) {
                b->cylinder[i].dim.pos.x = (s16)(b->actor.world.pos.x + kCylinderOffsetZ[i] * sinRot +
                                                            kCylinderOffsetX[i] * cosRot);
                b->cylinder[i].dim.pos.z = (s16)(b->actor.world.pos.z + kCylinderOffsetZ[i] * cosRot -
                                                            kCylinderOffsetX[i] * sinRot);
                b->cylinder[i].dim.pos.y = (s16)(b->actor.world.pos.y);
            }
        }

        if (m_bodyRoles & COLL_AC)
            bo->collider.base.acFlags |= AC_ON;
        else
            bo->collider.base.acFlags &= ~AC_ON;

        RegisterColliderBase(play, &bo->collider.base, m_bodyRoles);
        if (m_tentacleRoles != 0) {
            RegisterColliderBase(play, &bo->cylinder[0].base, m_tentacleRoles);
            RegisterColliderBase(play, &bo->cylinder[1].base, m_tentacleRoles);
        }
    }

  private:
    u8 m_bodyRoles = COLL_AC | COLL_OC;
    u8 m_tentacleRoles = 0;
};

}

#endif
