#ifndef POECONTROLLERH
#define POECONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Poh/z_en_poh.h"
#include "objects/object_poh/object_poh.h"
#include "objects/object_po_composer/object_po_composer.h"

void EnPoh_Idle(EnPoh* poh, PlayState* play);
void EnPoh_Attack(EnPoh* poh, PlayState* play);
void EnPoh_ComposerAppear(EnPoh* poh, PlayState* play);
void EnPoh_Death(EnPoh* poh, PlayState* play);
void EnPoh_Disappear(EnPoh* poh, PlayState* play);
void EnPoh_Appear(EnPoh* poh, PlayState* play);
void EnPoh_TalkComposer(EnPoh* poh, PlayState* play);
void EnPoh_TalkRegular(EnPoh* poh, PlayState* play);
void func_80ADEAC4(EnPoh* poh, PlayState* play);
void func_80ADEC9C(EnPoh* poh, PlayState* play);
void func_80ADEECC(EnPoh* poh, PlayState* play);
void func_80ADEF38(EnPoh* poh, PlayState* play);
void func_80ADF15C(EnPoh* poh, PlayState* play);
void func_80ADF574(EnPoh* poh, PlayState* play);
void func_80ADF5E0(EnPoh* poh, PlayState* play);
void func_80ADF894(EnPoh* poh, PlayState* play);
void func_80ADFE28(EnPoh* poh, PlayState* play);
void func_80ADFE80(EnPoh* poh, PlayState* play);
void func_80AE009C(EnPoh* poh, PlayState* play);
void EnPoh_DrawComposer(Actor* thisx, PlayState* play);
void EnPoh_DrawRegular(Actor* thisx, PlayState* play);

void EnPoh_UpdateLiving(Actor* thisx, PlayState* play);
void EnPoh_UpdateDead(Actor* thisx, PlayState* play);

void EnPoh_SetupDeath(EnPoh* poh, PlayState* play);

void func_80AE089C(EnPoh* poh);
}

namespace ZeldaOnline {

class PoeController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnPoh* Typed() const {
        return reinterpret_cast<EnPoh*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using PohActionFunc = void (*)(EnPoh*, PlayState*);
    static const PohActionFunc* ActionTable(size_t* count) {
        static const PohActionFunc sTable[] = {
            EnPoh_Idle,
            EnPoh_Attack,
            EnPoh_ComposerAppear,
            EnPoh_Death,
            EnPoh_Disappear,
            EnPoh_Appear,
            EnPoh_TalkComposer,
            EnPoh_TalkRegular,
            func_80ADEAC4,
            func_80ADEC9C,
            func_80ADEECC,
            func_80ADEF38,
            func_80ADF15C,
            func_80ADF574,
            func_80ADF5E0,
            func_80ADF894,
            func_80ADFE28,
            func_80ADFE80,
            func_80AE009C,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const PohActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_IDLE = 0;
    static constexpr u8 ANIM_IDLE2 = 1;
    static constexpr u8 ANIM_FLEE = 2;
    static constexpr u8 ANIM_ATTACK = 3;
    static constexpr u8 ANIM_DAMAGED = 4;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AttackAnimFor(EnPoh* poh) {
        return (poh->infoIdx == EN_POH_INFO_NORMAL) ? gPoeAttackAnim : gPoeComposerAttackAnim;
    }
    static const char* DamagedAnimFor(EnPoh* poh) {
        return (poh->infoIdx == EN_POH_INFO_NORMAL) ? gPoeDamagedAnim : gPoeComposerDamagedAnim;
    }

    u8 CurrentAnimIndex() const {
        EnPoh* poh = Typed();
        void* cur = poh->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        if (cur == (void*)poh->info->idleAnim)
            return ANIM_IDLE;
        if (cur == (void*)poh->info->idleAnim2)
            return ANIM_IDLE2;
        if (cur == (void*)poh->info->fleeAnim)
            return ANIM_FLEE;
        if (strcmp((const char*)cur, AttackAnimFor(poh)) == 0)
            return ANIM_ATTACK;
        if (strcmp((const char*)cur, DamagedAnimFor(poh)) == 0)
            return ANIM_DAMAGED;
        return ANIM_UNKNOWN;
    }
    static AnimationHeader* AnimForIndex(EnPoh* poh, u8 i) {
        switch (i) {
            case ANIM_IDLE:
                return poh->info->idleAnim;
            case ANIM_IDLE2:
                return poh->info->idleAnim2;
            case ANIM_FLEE:
                return poh->info->fleeAnim;
            case ANIM_ATTACK:
                return (AnimationHeader*)AttackAnimFor(poh);
            case ANIM_DAMAGED:
                return (AnimationHeader*)DamagedAnimFor(poh);
            default:
                return nullptr;
        }
    }

    u8 CurrentColliderRoles() const {
        EnPoh* poh = Typed();
        u8 cylRoles = COLL_OC;
        if ((poh->colliderCyl.base.acFlags & AC_ON) && poh->lightColor.a == 255)
            cylRoles |= COLL_AC;
        u8 sphRoles = COLL_OC;
        if (poh->actionFunc == EnPoh_Attack && poh->unk_198 < 10)
            sphRoles |= COLL_AT;
        return (u8)((cylRoles << 4) | sphRoles);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_LIGHT_COLOR,
        PROP_PHASE,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnPoh* poh = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(poh->actor.colChkInfo.health), out);
        PackProperty(PROP_LIGHT_COLOR,
                     ByteStream() << PackedUInt1(poh->lightColor.r) << PackedUInt1(poh->lightColor.g)
                                  << PackedUInt1(poh->lightColor.b) << PackedUInt1(poh->lightColor.a),
                     out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        PackProperty(PROP_PHASE,
                     ByteStream() << PackedInt2(poh->visibilityTimer) << PackedUInt1(poh->unk_194)
                                  << PackedInt2(poh->unk_198) << PackedInt2(poh->unk_19C),
                     out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(poh->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &poh->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnPoh* poh = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const PohActionFunc* table = ActionTable(&count);
                if (id < count)
                    poh->actionFunc = table[id];
                break;
            }
            case PROP_ANIM_CUR_FRAME:
                poh->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                AnimationHeader* a = (id != ANIM_UNKNOWN) ? AnimForIndex(poh, id) : nullptr;
                ApplyAnimProperty((void*)a, &poh->skelAnime, LOCK_CUR_FRAME ? poh->skelAnime.curFrame : 0.0f, data);
                break;
            }
            case PROP_HEALTH:
                poh->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_LIGHT_COLOR:
                poh->lightColor.r = (u8)(data.Read<PackedUInt1>().value());
                poh->lightColor.g = (u8)(data.Read<PackedUInt1>().value());
                poh->lightColor.b = (u8)(data.Read<PackedUInt1>().value());
                poh->lightColor.a = (u8)(data.Read<PackedUInt1>().value());
                break;

            case PROP_PHASE:
                poh->visibilityTimer = (s16)(data.Read<PackedInt2>().value());
                poh->unk_194 = (u8)(data.Read<PackedUInt1>().value());
                poh->unk_198 = (s16)(data.Read<PackedInt2>().value());
                poh->unk_19C = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        if (Typed()->actionFunc == EnPoh_Death) {
            EnPoh_SetupDeath(Typed(), gPlayState);
            GoLocal();
        }
    }

    void UpdateLeader(PlayState* play) override {
        AbstractActorController::UpdateLeader(play);
        if (m_actor->update != &AbstractActorController::DispatchUpdate) {
            m_originalUpdate = m_actor->update;
            m_actor->update = &AbstractActorController::DispatchUpdate;
        }
    }

    void EnsureLocalSkeletonReady(PlayState* play) {
        EnPoh* poh = Typed();
        if (m_localSkelReady)
            return;
        if (!Object_IsLoaded(&play->objectCtx, poh->objectIdx))
            return;

        poh->actor.objBankIndex = poh->objectIdx;
        Actor_SetObjectDependency(play, &poh->actor);
        if (poh->infoIdx == EN_POH_INFO_NORMAL) {
            SkelAnime_Init(play, &poh->skelAnime, (SkeletonHeader*)&gPoeSkel, (AnimationHeader*)&gPoeFloatAnim,
                           poh->jointTable, poh->morphTable, 21);
            poh->actor.draw = EnPoh_DrawRegular;
        } else {
            SkelAnime_InitFlex(play, &poh->skelAnime, (FlexSkeletonHeader*)&gPoeComposerSkel,
                               (AnimationHeader*)&gPoeComposerFloatAnim, poh->jointTable, poh->morphTable, 12);
            poh->actor.draw = EnPoh_DrawComposer;
            poh->colliderSph.elements[0].dim.limb = 9;
            poh->colliderSph.elements[0].dim.modelSphere.center.y *= -1;
            poh->actor.shape.rot.y = poh->actor.world.rot.y = -0x4000;
            poh->colliderCyl.dim.radius = 20;
            poh->colliderCyl.dim.height = 55;
            poh->colliderCyl.dim.yShift = 15;
        }
        poh->actor.flags &= ~ACTOR_FLAG_UPDATE_CULLING_DISABLED;

        m_localSkelReady = true;

    }

    void UpdatePuppet(PlayState* play) override {
        EnPoh* poh = Typed();

        EnsureLocalSkeletonReady(play);
        if (!m_localSkelReady)
            return;

        UpdateAnimation(&poh->skelAnime, LOCK_CUR_FRAME);

        if (poh->colliderCyl.base.acFlags & AC_HIT) {
            bool wouldReact = poh->actor.colChkInfo.damageEffect != 0 || poh->actor.colChkInfo.damage != 0;
            poh->colliderCyl.base.acFlags &= ~AC_HIT;
            if (wouldReact) {
                ClaimLeadership(CLAIM_REASON_HIT);
                UpdateLeader(play);
                return;
            }
        }
        poh->colliderSph.base.atFlags &= ~AT_HIT;

        if (poh->actionFunc != EnPoh_Death && poh->actor.colChkInfo.health > 0 && poh->actor.xzDistToPlayer < 300.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        func_80AE089C(poh);

        Actor_SetFocus(&poh->actor, 42.0f);

        if (poh->actionFunc != func_80ADEECC && poh->actionFunc != func_80ADF574) {
            if (poh->actionFunc == func_80ADF894)
                poh->actor.shape.rot.y = poh->actor.world.rot.y + 0x8000;
            else
                poh->actor.shape.rot.y = poh->actor.world.rot.y;
        }

        {
            Vec3f rayOrigin = poh->actor.world.pos;
            rayOrigin.y += 20.0f;
            s32 floorBgId = BGCHECK_SCENE;
            poh->actor.floorHeight = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &poh->actor.floorPoly, &floorBgId,
                                                                 &poh->actor, &rayOrigin);
            poh->actor.floorBgId = floorBgId;
        }
        poh->actor.shape.shadowAlpha = poh->lightColor.a;

        u8 cylRoles = (m_roles >> 4) & 0xF;
        u8 sphRoles = m_roles & 0xF;
        Collider_UpdateCylinder(&poh->actor, &poh->colliderCyl);
        RegisterColliderBase(play, &poh->colliderCyl.base, cylRoles);
        RegisterColliderBase(play, &poh->colliderSph.base, sphRoles);
    }

  private:
    u8 m_roles = (COLL_OC << 4) | COLL_OC;

    bool m_localSkelReady = false;
};

}

#endif
