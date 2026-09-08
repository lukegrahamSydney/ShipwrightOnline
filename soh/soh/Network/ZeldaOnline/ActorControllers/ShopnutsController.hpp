#ifndef SHOPNUTSCONTROLLERH
#define SHOPNUTSCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Shopnuts/z_en_shopnuts.h"
#include "objects/object_shopnuts/object_shopnuts.h"

void EnShopnuts_Wait(EnShopnuts* sn, PlayState* play);
void EnShopnuts_LookAround(EnShopnuts* sn, PlayState* play);
void EnShopnuts_Stand(EnShopnuts* sn, PlayState* play);
void EnShopnuts_ThrowNut(EnShopnuts* sn, PlayState* play);
void EnShopnuts_Burrow(EnShopnuts* sn, PlayState* play);
void EnShopnuts_SpawnSalesman(EnShopnuts* sn, PlayState* play);
}

namespace ZeldaOnline {

class ShopnutsController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnShopnuts* Typed() const {
        return reinterpret_cast<EnShopnuts*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using ShopnutsActionFunc = void (*)(EnShopnuts*, PlayState*);
    static const ShopnutsActionFunc* ActionTable(size_t* count) {
        static const ShopnutsActionFunc sTable[] = {
            EnShopnuts_Wait,
            EnShopnuts_LookAround,
            EnShopnuts_Stand,
            EnShopnuts_ThrowNut,
            EnShopnuts_Burrow,
            EnShopnuts_SpawnSalesman,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ShopnutsActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_WAIT = 0;
    static constexpr u8 ANIM_LOOK_AROUND = 1;
    static constexpr u8 ANIM_THROW = 2;
    static constexpr u8 ANIM_STAND = 3;
    static constexpr u8 ANIM_BURROW = 4;
    static constexpr u8 ANIM_ROTATE = 5;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 6;

    static const char* AnimForIndex(u8 i) {
        switch (i) {
            case ANIM_WAIT:
                return gBusinessScrubAnim_139C;
            case ANIM_LOOK_AROUND:
                return gBusinessScrubLookAroundAnim;
            case ANIM_THROW:
                return gBusinessScrubAnim_1EC;
            case ANIM_STAND:
                return gBusinessScrubAnim_4574;
            case ANIM_BURROW:
                return gBusinessScrubAnim_39C;
            case ANIM_ROTATE:
                return gBusinessScrubRotateAnim;
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
        EnShopnuts* sn = Typed();
        u8 roles = COLL_OC;
        if (sn->collider.base.acFlags & AC_ON)
            roles |= COLL_AC;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_ANIM_FLAG_TIMER,
        PROP_COLL_ROLES,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnShopnuts* sn = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_ANIM_FLAG_TIMER, PackedInt2(sn->animFlagAndTimer), out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(sn->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &sn->skelAnime), out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnShopnuts* sn = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const ShopnutsActionFunc* table = ActionTable(&count);
                if (id < count)
                    sn->actionFunc = table[id];
                break;
            }
            case PROP_ANIM_FLAG_TIMER:
                sn->animFlagAndTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                sn->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &sn->skelAnime, LOCK_CUR_FRAME ? sn->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnShopnuts* sn = Typed();

        UpdateAnimation(&sn->skelAnime, LOCK_CUR_FRAME);

        if (sn->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        sn->collider.base.acFlags &= ~AC_HIT;

        if (sn->actionFunc != EnShopnuts_Burrow && sn->actionFunc != EnShopnuts_SpawnSalesman &&
            sn->actor.xzDistToPlayer < 480.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        if (sn->actionFunc == EnShopnuts_Wait) {
            Actor_SetFocus(&sn->actor, sn->skelAnime.curFrame);
        } else if (sn->actionFunc == EnShopnuts_Burrow) {
            Actor_SetFocus(&sn->actor, 20.0f - ((sn->skelAnime.curFrame * 20.0f) /
                                                Animation_GetLastFrame((void*)gBusinessScrubAnim_39C)));
        } else {
            Actor_SetFocus(&sn->actor, 20.0f);
        }

        RegisterColliderBase(play, &sn->collider.base, m_roles);
    }

  private:
    u8 m_roles = COLL_OC;
};

}

#endif
