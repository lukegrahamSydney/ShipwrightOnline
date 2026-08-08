#ifndef SHOPSCRUBCONTROLLERH
#define SHOPSCRUBCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Dns/z_en_dns.h"
#include "objects/object_shopnuts/object_shopnuts.h"

void EnDns_SetupIdle(EnDns* dns, PlayState* play);
void EnDns_Idle(EnDns* dns, PlayState* play);
void EnDns_Talk(EnDns* dns, PlayState* play);
void EnDns_SetupSale(EnDns* dns, PlayState* play);
void EnDns_Sale(EnDns* dns, PlayState* play);
void EnDns_SetupBurrow(EnDns* dns, PlayState* play);
void EnDns_SetupNoSaleBurrow(EnDns* dns, PlayState* play);
void EnDns_Burrow(EnDns* dns, PlayState* play);
void EnDns_PostBurrow(EnDns* dns, PlayState* play);

void func_80028990(PlayState* play, f32 arg1, Vec3f* pos);

extern AnimationInfo* gEnDnsAnimationInfo;
}

namespace ZeldaOnline {

class ShopScrubController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnDns* Typed() const {
        return reinterpret_cast<EnDns*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;
    static constexpr f32 CLAIM_RANGE = 220.0f;

    using DnsActionFunc = void (*)(EnDns*, PlayState*);
    static const DnsActionFunc* ActionTable(size_t* count) {
        static const DnsActionFunc sTable[] = {
            EnDns_SetupIdle,
            EnDns_Idle,
            EnDns_Talk,
            EnDns_SetupSale,
            EnDns_Sale,
            EnDns_SetupBurrow,
            EnDns_SetupNoSaleBurrow,
            EnDns_Burrow,
            EnDns_PostBurrow,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const DnsActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TEXT_ID,
        PROP_BURROW_STATE,
        PROP_Y_INIT_POS,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnDns* dns = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TEXT_ID, PackedUInt2(dns->actor.textId), out);
        PackProperty(PROP_BURROW_STATE,
                     ByteStream() << PackedUInt1(dns->maintainCollider) << PackedUInt1(dns->standOnGround)
                                  << PackedUInt1(dns->dropCollectible),
                     out);
        PackProperty(PROP_Y_INIT_POS, PackedFloat4(dns->yInitPos), out);
        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(dns->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(dns->animIndex, &dns->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnDns* dns = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const DnsActionFunc* table = ActionTable(&count);
                if (id < count)
                    dns->actionFunc = table[id];
                break;
            }
            case PROP_ANIM_CUR_FRAME:
                dns->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                dns->animIndex = id;
                f32 playSpeed = data.Read<PackedFloat4>().value();
                f32 startFrame = static_cast<f32>(data.Read<PackedInt2>().value());
                f32 endFrame = static_cast<f32>(data.Read<PackedInt2>().value());
                f32 animLength = data.Read<PackedFloat4>().value();
                u8 mode = (u8)(data.Read<PackedUInt1>().value());

                if (dns->skelAnime.animation != gEnDnsAnimationInfo[id].animation) {
                    f32 fallbackCurFrame = LOCK_CUR_FRAME ? dns->skelAnime.curFrame : 0.0f;
                    Animation_ChangeByInfo(&dns->skelAnime, gEnDnsAnimationInfo, id);
                    dns->skelAnime.curFrame = fallbackCurFrame;
                }
                dns->skelAnime.playSpeed = playSpeed;
                dns->skelAnime.startFrame = startFrame;
                dns->skelAnime.endFrame = endFrame;
                dns->skelAnime.animLength = animLength;
                dns->skelAnime.mode = mode;
                break;
            }
            case PROP_TEXT_ID:
                dns->actor.textId = (u16)(data.Read<PackedUInt2>().value());
                break;
            case PROP_BURROW_STATE:
                dns->maintainCollider = (u8)(data.Read<PackedUInt1>().value());
                dns->standOnGround = (u8)(data.Read<PackedUInt1>().value());
                dns->dropCollectible = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_Y_INIT_POS:
                dns->yInitPos = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnDns* dns = Typed();

        if (dns->actor.xzDistToPlayer < CLAIM_RANGE && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        dns->dustTimer++;
        Actor_SetFocus(&dns->actor, 60.0f);
        Actor_SetScale(&dns->actor, 0.01f);
        UpdateAnimation(&dns->skelAnime, LOCK_CUR_FRAME);

        if (dns->actionFunc == EnDns_Burrow && dns->skelAnime.animation != nullptr &&
            strcmp((const char*)dns->skelAnime.animation, gBusinessScrubLeaveBurrowAnim) != 0) {
            SetAnimation(&dns->skelAnime, (void*)gBusinessScrubLeaveBurrowAnim);
        }

        if (dns->actionFunc == EnDns_PostBurrow && (dns->dustTimer & 3) == 0) {
            Vec3f dustPos;
            dustPos.x = dns->actor.world.pos.x;
            dustPos.y = dns->yInitPos;
            dustPos.z = dns->actor.world.pos.z;
            func_80028990(play, 20.0f, &dustPos);
        }

        if (dns->maintainCollider) {
            Collider_UpdateCylinder(&dns->actor, &dns->collider);
            RegisterColliderBase(play, &dns->collider.base, COLL_OC);
        }
    }

};

}

#endif
