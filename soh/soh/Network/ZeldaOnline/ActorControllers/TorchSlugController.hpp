#ifndef TORCHSLUGCONTROLLERH
#define TORCHSLUGCONTROLLERH
#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bw/z_en_bw.h"
#include "objects/object_bw/object_bw.h"

void func_809CE884(EnBw* bw, PlayState* play);
void func_809CEA24(EnBw* bw, PlayState* play);
void func_809CF7AC(EnBw* bw, PlayState* play);
void func_809CF984(EnBw* bw, PlayState* play);
void func_809CFC4C(EnBw* bw, PlayState* play);
void func_809CFF98(EnBw* bw, PlayState* play);
void func_809D014C(EnBw* bw, PlayState* play);
void func_809D0268(EnBw* bw, PlayState* play);
void func_809D0424(EnBw* bw, PlayState* play);
}

namespace ZeldaOnline {

class TorchSlugController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBw* Typed() const {
        return reinterpret_cast<EnBw*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using BwActionFunc = void (*)(EnBw*, PlayState*);
    static const BwActionFunc* ActionTable(size_t* count) {
        static const BwActionFunc sTable[] = {
            func_809CE884, func_809CEA24, func_809CF7AC, func_809CF984, func_809CFC4C,
            func_809CFF98, func_809D014C, func_809D0268, func_809D0424,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const BwActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    static constexpr u8 ANIM_COUNT = 3;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0: return gTorchSlugEyestalkWaveAnim;
            case 1: return gTorchSlugEyestalkRaiseAnim;
            case 2: return gTorchSlugEyestalkFlailAnim;
            default: return nullptr;
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
        EnBw* bw = Typed();
        return (bw->collider2.base.acFlags & AC_HIT) && bw->actor.colChkInfo.damage != 0;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_TIMERS,
        PROP_SIZE,
        PROP_WOBBLE,
        PROP_COLOR,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBw* bw = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedUInt1(bw->unk_220) << PackedUInt1(bw->unk_221)
                                  << PackedUInt1(bw->unk_230) << PackedUInt1(bw->unk_232)
                                  << PackedUInt1(bw->unk_23A) << PackedUInt1(bw->unk_23C)
                                  << PackedUInt1(bw->iceTimer) << PackedUInt1(bw->actor.colChkInfo.health),
                     out);
        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(bw->unk_222) << PackedInt2(bw->unk_224)
                                  << PackedInt2(bw->unk_236) << PackedInt2(bw->unk_238),
                     out);


        PackProperty(PROP_SIZE,
                     ByteStream() << PackedFloat4(bw->unk_240) << PackedFloat4(bw->unk_244)
                                  << PackedFloat4(bw->unk_248) << PackedFloat4(bw->unk_24C)
                                  << PackedFloat4(bw->unk_250),
                     out);
        PackProperty(PROP_WOBBLE,
                     ByteStream() << PackedFloat4(bw->unk_258) << PackedFloat4(bw->unk_25C)
                                  << PackedFloat4(bw->unk_260),
                     out);
        PackProperty(PROP_COLOR,
                     ByteStream() << PackedUInt1(bw->color1.r) << PackedUInt1(bw->color1.g)
                                  << PackedUInt1(bw->color1.b) << PackedUInt1(bw->color1.a),
                     out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(bw->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &bw->skelAnime), out);

        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBw* bw = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const BwActionFunc* table = ActionTable(&count);
                if (id < count)
                    bw->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                bw->unk_220 = (u8)(data.Read<PackedUInt1>().value());
                bw->unk_221 = (u8)(data.Read<PackedUInt1>().value());
                bw->unk_230 = (u8)(data.Read<PackedUInt1>().value());
                bw->unk_232 = (u8)(data.Read<PackedUInt1>().value());
                bw->unk_23A = (u8)(data.Read<PackedUInt1>().value());
                bw->unk_23C = (u8)(data.Read<PackedUInt1>().value());
                bw->iceTimer = (u8)(data.Read<PackedUInt1>().value());
                bw->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                bw->unk_222 = (s16)(data.Read<PackedInt2>().value());
                bw->unk_224 = (s16)(data.Read<PackedInt2>().value());
                bw->unk_236 = (s16)(data.Read<PackedInt2>().value());
                bw->unk_238 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_SIZE:
                bw->unk_240 = data.Read<PackedFloat4>().value();
                bw->unk_244 = data.Read<PackedFloat4>().value();
                bw->unk_248 = data.Read<PackedFloat4>().value();
                bw->unk_24C = data.Read<PackedFloat4>().value();
                bw->unk_250 = data.Read<PackedFloat4>().value();
                break;
            case PROP_WOBBLE:
                bw->unk_258 = data.Read<PackedFloat4>().value();
                bw->unk_25C = data.Read<PackedFloat4>().value();
                bw->unk_260 = data.Read<PackedFloat4>().value();
                break;
            case PROP_COLOR:
                bw->color1.r = (u8)(data.Read<PackedUInt1>().value());
                bw->color1.g = (u8)(data.Read<PackedUInt1>().value());
                bw->color1.b = (u8)(data.Read<PackedUInt1>().value());
                bw->color1.a = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                bw->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &bw->skelAnime, LOCK_CUR_FRAME ? bw->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnBw* bw = Typed();

        UpdateAnimation(&bw->skelAnime, LOCK_CUR_FRAME);

        if (HitWouldReact()) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }
        bw->collider1.base.atFlags &= ~AT_HIT;
        bw->collider2.base.acFlags &= ~AC_HIT;

        if (bw->actor.colChkInfo.health > 0 && bw->actor.xzDistToPlayer < 400.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        bw->actor.focus.pos = bw->actor.world.pos;
        bw->actor.focus.pos.y += 5.0f;

        Collider_UpdateCylinder(&bw->actor, &bw->collider2);

        u8 bodyRoles = COLL_OC;
        if (bw->unk_220 != 0 && (bw->actor.colorFilterTimer == 0 || !(bw->actor.colorFilterParams & 0x4000)))
            bodyRoles |= COLL_AC;
        RegisterColliderBase(play, &bw->collider2.base, bodyRoles);

        //the burn hitbox only exists while it is hot enough.
        if (bw->unk_221 != 1 && bw->unk_220 < 5 && bw->unk_248 > 0.4f) {
            bw->collider1.info.toucher.effect = 1;
            Collider_UpdateCylinder(&bw->actor, &bw->collider1);
            RegisterColliderBase(play, &bw->collider1.base, COLL_AT);
        } else {
            bw->collider1.info.toucher.effect = 0;
        }
    }
};

}

#endif
