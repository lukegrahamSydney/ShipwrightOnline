#ifndef POESISTERSCONTROLLERH
#define POESISTERSCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Po_Sisters/z_en_po_sisters.h"
#include "objects/object_po_sisters/object_po_sisters.h"

void func_80ADA4A8(EnPoSisters* po, PlayState* play);
void func_80ADA530(EnPoSisters* po, PlayState* play);
void func_80ADA6A0(EnPoSisters* po, PlayState* play);
void func_80ADA7F0(EnPoSisters* po, PlayState* play);
void func_80ADA8C0(EnPoSisters* po, PlayState* play);
void func_80ADA9E8(EnPoSisters* po, PlayState* play);
void func_80ADAAA4(EnPoSisters* po, PlayState* play);
void func_80ADAC70(EnPoSisters* po, PlayState* play);
void func_80ADAD54(EnPoSisters* po, PlayState* play);
void func_80ADAE6C(EnPoSisters* po, PlayState* play);
void func_80ADAFC0(EnPoSisters* po, PlayState* play);
void func_80ADB17C(EnPoSisters* po, PlayState* play);
void func_80ADB2B8(EnPoSisters* po, PlayState* play);
void func_80ADB4B0(EnPoSisters* po, PlayState* play);
void func_80ADB51C(EnPoSisters* po, PlayState* play);
void func_80ADB770(EnPoSisters* po, PlayState* play);
void func_80ADB9F0(EnPoSisters* po, PlayState* play);
void func_80ADBB6C(EnPoSisters* po, PlayState* play);
void func_80ADBBF4(EnPoSisters* po, PlayState* play);
void func_80ADBC88(EnPoSisters* po, PlayState* play);
void func_80ADBD38(EnPoSisters* po, PlayState* play);
void func_80ADBD8C(EnPoSisters* po, PlayState* play);
void func_80ADBEE8(EnPoSisters* po, PlayState* play);
void func_80ADBF58(EnPoSisters* po, PlayState* play);
extern s32 D_80ADD784;
}

namespace ZeldaOnline {

class PoeSistersController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnPoSisters* Typed() const {
        return reinterpret_cast<EnPoSisters*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return true;
    }

    static bool IsNetworkedVariant(s16 params) {
            
        //Intro meg allowed
        if (params & 0x1000)
            return true;

        //Do not sync fighting meg (we can sync intro meg though)
        return ((params >> 8) & 3) != 0;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using PoActionFunc = void (*)(EnPoSisters*, PlayState*);
    static const PoActionFunc* ActionTable(size_t* count) {
        static const PoActionFunc sTable[] = {
            func_80ADA4A8, func_80ADA530, func_80ADA6A0, func_80ADA7F0, func_80ADA8C0, func_80ADA9E8, func_80ADAAA4,
            func_80ADAC70, func_80ADAD54, func_80ADAE6C, func_80ADAFC0, func_80ADB17C, func_80ADB2B8, func_80ADB338,
            func_80ADB4B0, func_80ADB51C, func_80ADB770, func_80ADB9F0, func_80ADBB6C, func_80ADBBF4, func_80ADBC88,
            func_80ADBD38, func_80ADBD8C, func_80ADBEE8, func_80ADBF58,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const PoActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 7;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gPoeSistersFloatAnim;
            case 1:
                return gPoeSistersSwayAnim;
            case 2:
                return gPoeSistersAppearDisappearAnim;
            case 3:
                return gPoeSistersAttackAnim;
            case 4:
                return gPoeSistersDamagedAnim;
            case 5:
                return gPoeSistersFleeAnim;
            case 6:
                return gPoeSistersMegCryAnim;
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
        EnPoSisters* po = Typed();
        if (!(po->collider.base.acFlags & AC_HIT))
            return false;
        return po->actor.colChkInfo.damageEffect != 0 || po->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_HEALTH,
        PROP_TIMERS,
        PROP_ALPHA,
        PROP_SCALE_MOD,
        PROP_COLL_ROLES,
        PROP_ANIM,
        PROP_HOME_POS,
        PROP_APPEAR_MASK,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnPoSisters* po = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);


        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(po->unk_196) << PackedUInt1(po->unk_197) << PackedUInt1(po->unk_198)
                                  << PackedUInt1(po->unk_199),
                     out);
        PackProperty(PROP_HEALTH, PackedUInt1(po->actor.colChkInfo.health), out);
        PackProperty(PROP_TIMERS, ByteStream() << PackedInt2(po->unk_19A) << PackedInt2(po->unk_19C), out);
        PackProperty(PROP_ALPHA,
                     ByteStream() << PackedUInt1(po->unk_22E.r) << PackedUInt1(po->unk_22E.g)
                                  << PackedUInt1(po->unk_22E.b) << PackedUInt1(po->unk_22E.a),
                     out);
        PackProperty(PROP_SCALE_MOD, PackedFloat4(po->unk_294), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &po->skelAnime), out);
        PackProperty(PROP_HOME_POS,
                     ByteStream() << PackedFloat4(po->actor.home.pos.x) << PackedFloat4(po->actor.home.pos.y)
                                  << PackedFloat4(po->actor.home.pos.z),
                     out);

        u8 myBit = (u8)(1 << po->unk_194);
        PackProperty(PROP_APPEAR_MASK, PackedUInt1((D_80ADD784 & myBit) ? 1u : 0u), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    u8 CurrentColliderRoles() const {
        EnPoSisters* po = Typed();
        u8 roles = 0;
        if (po->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        if (po->collider.base.ocFlags1 & OC1_ON)
            roles |= COLL_OC;
        if (po->collider.base.atFlags & AT_ON)
            roles |= COLL_AT;
        return roles;
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnPoSisters* po = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                m_currentActionIndex = id;
                size_t count;
                const PoActionFunc* table = ActionTable(&count);
                if (id < count)
                    po->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                po->unk_196 = (u8)(data.Read<PackedUInt1>().value());
                po->unk_197 = (u8)(data.Read<PackedUInt1>().value());
                po->unk_198 = (u8)(data.Read<PackedUInt1>().value());
                po->unk_199 = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_HEALTH:
                po->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                po->unk_19A = (s16)(data.Read<PackedInt2>().value());
                po->unk_19C = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ALPHA:
                po->unk_22E.r = (u8)(data.Read<PackedUInt1>().value());
                po->unk_22E.g = (u8)(data.Read<PackedUInt1>().value());
                po->unk_22E.b = (u8)(data.Read<PackedUInt1>().value());
                po->unk_22E.a = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_SCALE_MOD:
                po->unk_294 = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &po->skelAnime, LOCK_CUR_FRAME ? po->skelAnime.curFrame : 0.0f, data);
                break;
            }

            case PROP_APPEAR_MASK: {
                u8 set = (u8)(data.Read<PackedUInt1>().value());
                u8 myBit = (u8)(1 << po->unk_194);

                if (set)
                    D_80ADD784 |= myBit;
                else
                    D_80ADD784 &= ~myBit;
                break;
            }

             case PROP_HOME_POS:
                po->actor.home.pos.x = data.Read<PackedFloat4>().value();
                po->actor.home.pos.y = data.Read<PackedFloat4>().value();
                po->actor.home.pos.z = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

     void ClaimSisters(PlayState* play) {
        for (Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].head; it != nullptr; it = it->next) {
            if (it == m_actor || it->id != ACTOR_EN_PO_SISTERS || it->zoController == nullptr)
                continue;
            if (it->colChkInfo.health == 0)
                continue;

            auto* sister = reinterpret_cast<PoeSistersController*>(it->zoController);
            if (!sister->IsLeader())
                sister->ClaimLeadership(CLAIM_REASON_NOW);
        }
    }

    void UpdateLeader(PlayState* play) override {
        ClaimSisters(play);

       EnPoSisters* po = Typed();

       //Make sure our fake megs follow the same leader
       if (po->unk_194 == 0) {
            for (Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].head; it != nullptr; it = it->next) {
                if (it->id != ACTOR_EN_PO_SISTERS || it == m_actor || it->zoController == nullptr)
                    continue;
                if (reinterpret_cast<EnPoSisters*>(it)->unk_194 != 0)
                    continue;

                auto* other = reinterpret_cast<PoeSistersController*>(it->zoController);
                if (!other->IsLeader())
                    other->ClaimLeadership(CLAIM_REASON_NOW);
            }
        }

        //If we are the fake megs, make sure meg exist then set her as parent
        if (po->actor.parent == nullptr && po->unk_195 != 0) {
            for (Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].head; it != nullptr; it = it->next) {
                if (it->id != ACTOR_EN_PO_SISTERS || it == m_actor)
                    continue;
                EnPoSisters* other = reinterpret_cast<EnPoSisters*>(it);
                if (other->unk_194 == po->unk_194 && other->unk_195 == 0) {
                    po->actor.parent = it;
                    break;
                }
            }
            if (po->actor.parent == nullptr)
                return;
        }

        AbstractActorController::UpdateLeader(play);
    }

    void UpdatePuppet(PlayState* play) override {
        EnPoSisters* po = Typed();

        UpdateAnimation(&po->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        po->collider.base.acFlags &= ~AC_HIT;
        po->collider.base.atFlags &= ~AT_HIT;

        if (po->unk_195 == 0 && po->actor.colChkInfo.health > 0 && po->actor.xzDistToPlayer < 400.0f &&
            IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        Actor_SetFocus(&po->actor, 40.0f);
        po->actor.shape.shadowAlpha = po->unk_22E.a;

        if (m_roles != 0) {
            Collider_UpdateCylinder(&po->actor, &po->collider);
            RegisterColliderBase(play, &po->collider.base, m_roles);
        }
    }

  private:
    u8 m_currentActionIndex = ID_UNKNOWN;
    u8 m_roles = COLL_AC | COLL_OC;
};

} // namespace ZeldaOnline

#endif