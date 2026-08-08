#ifndef SKULLWALLTULACONTROLLERH
#define SKULLWALLTULACONTROLLERH

#include "../AbstractActorController.hpp"
#include "src/overlays/actors/ovl_En_Sw/z_en_sw.h"

extern "C" {
void func_80B0D364(EnSw* sw, PlayState* play);
void func_80B0D3AC(EnSw* sw, PlayState* play);
void func_80B0D590(EnSw* sw, PlayState* play);
void func_80B0D878(EnSw* sw, PlayState* play);
void func_80B0DB00(EnSw* sw, PlayState* play);
void func_80B0DC7C(EnSw* sw, PlayState* play);
void func_80B0E5E0(EnSw* sw, PlayState* play);
void func_80B0E728(EnSw* sw, PlayState* play);
void func_80B0E90C(EnSw* sw, PlayState* play);
void func_80B0E9BC(EnSw* sw, PlayState* play);

extern AnimationInfo* gEnSwAnimationInfo;
}

namespace ZeldaOnline {

class SkullWalltulaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnSw* Typed() const {
        return reinterpret_cast<EnSw*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = false;

  protected:
    static constexpr u8 ACTION_UNKNOWN = 0xFF;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 4;

    bool IsGoldSkulltula() const {
        return ((Typed()->actor.params & 0xE000) >> 13) != 0;
    }

    static const EnSwActionFunc* ActionTable(size_t* count) {
        static const EnSwActionFunc sTable[] = {
            func_80B0D364, func_80B0D3AC, func_80B0D590, func_80B0D878, func_80B0DB00,
            func_80B0DC7C, func_80B0E5E0, func_80B0E728, func_80B0E90C, func_80B0E9BC,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const EnSwActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ACTION_UNKNOWN;
    }

    static void* AnimForIndex(u8 i) {
        if (i >= ANIM_COUNT)
            return nullptr;
        return (void*)gEnSwAnimationInfo[i].animation;
    }

    u8 CurrentAnimIndex() const {
        for (int i = 0; i < ANIM_COUNT; i++)
            if (Typed()->skelAnime.animation == gEnSwAnimationInfo[i].animation)
                return (u8)(i);
        return ANIM_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        EnSw* sw = Typed();
        if (IsGoldSkulltula() && sw->actionFunc != func_80B0D590)
            return 0;
        u8 roles = COLL_OC;
        if (sw->unk_390 == 0 && sw->actor.colChkInfo.health != 0)
            roles |= COLL_AT;
        if (sw->unk_392 == 0 && sw->actor.colChkInfo.health != 0)
            roles |= COLL_AC;
        return roles;
    }

    void RebuildOrientation() {
        EnSw* sw = Typed();

        sw->unk_3D8.xx = sw->unk_370.x;
        sw->unk_3D8.yx = sw->unk_370.y;
        sw->unk_3D8.zx = sw->unk_370.z;
        sw->unk_3D8.wx = 0.0f;
        sw->unk_3D8.xy = sw->unk_364.x;
        sw->unk_3D8.yy = sw->unk_364.y;
        sw->unk_3D8.zy = sw->unk_364.z;
        sw->unk_3D8.wy = 0.0f;
        sw->unk_3D8.xz = sw->unk_37C.x;
        sw->unk_3D8.yz = sw->unk_37C.y;
        sw->unk_3D8.zz = sw->unk_37C.z;
        sw->unk_3D8.wz = 0.0f;
        sw->unk_3D8.xw = 0.0f;
        sw->unk_3D8.yw = 0.0f;
        sw->unk_3D8.zw = 0.0f;
        sw->unk_3D8.ww = 1.0f;
        Matrix_MtxFToYXZRotS(&sw->unk_3D8, &sw->actor.world.rot, 0);
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_HEALTH,
        PROP_BASIS,
        PROP_TIMERS,
        PROP_UNK_420,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnSw* sw = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_HEALTH, PackedUInt1(sw->actor.colChkInfo.health), out);

        PackProperty(
            PROP_BASIS,
            ByteStream() << PackedFloat4(sw->unk_364.x) << PackedFloat4(sw->unk_364.y) << PackedFloat4(sw->unk_364.z)
                         << PackedFloat4(sw->unk_370.x) << PackedFloat4(sw->unk_370.y) << PackedFloat4(sw->unk_370.z)
                         << PackedFloat4(sw->unk_37C.x) << PackedFloat4(sw->unk_37C.y) << PackedFloat4(sw->unk_37C.z),
            out);

        PackProperty(PROP_TIMERS,
                     ByteStream() << PackedInt2(sw->unk_388) << PackedInt2(sw->unk_38A) << PackedInt2(sw->unk_38C)
                                  << PackedInt2(sw->unk_38E) << PackedInt2(sw->unk_390) << PackedInt2(sw->unk_392)
                                  << PackedInt2(sw->unk_394),
                     out);

        PackProperty(PROP_UNK_420, PackedFloat4(sw->unk_420), out);

        if (LOCK_CUR_FRAME)
            PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(sw->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &sw->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
        BuildStandardExtendedProperty(PROP_COLOR_FILTER, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnSw* sw = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                if (ai != ACTION_UNKNOWN) {
                    size_t count;
                    const EnSwActionFunc* table = ActionTable(&count);
                    if (ai < count)
                        sw->actionFunc = table[ai];
                }
                return true;
            }
            case PROP_ANIM_CUR_FRAME:
                sw->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                return true;
            case PROP_ANIM: {
                u8 ai = (u8)(data.Read<PackedUInt1>().value());
                void* anim = (ai != ANIM_UNKNOWN) ? AnimForIndex(ai) : nullptr;
                ApplyAnimProperty(anim, &sw->skelAnime, LOCK_CUR_FRAME ? sw->skelAnime.curFrame : 0.0f, data);
                return true;
            }
            case PROP_HEALTH:
                sw->actor.colChkInfo.health = (u8)(data.Read<PackedUInt1>().value());
                return true;
            case PROP_BASIS:
                sw->unk_364.x = data.Read<PackedFloat4>().value();
                sw->unk_364.y = data.Read<PackedFloat4>().value();
                sw->unk_364.z = data.Read<PackedFloat4>().value();
                sw->unk_370.x = data.Read<PackedFloat4>().value();
                sw->unk_370.y = data.Read<PackedFloat4>().value();
                sw->unk_370.z = data.Read<PackedFloat4>().value();
                sw->unk_37C.x = data.Read<PackedFloat4>().value();
                sw->unk_37C.y = data.Read<PackedFloat4>().value();
                sw->unk_37C.z = data.Read<PackedFloat4>().value();
                return true;
            case PROP_TIMERS:
                sw->unk_388 = (s16)(data.Read<PackedInt2>().value());
                sw->unk_38A = (s16)(data.Read<PackedInt2>().value());
                sw->unk_38C = (s16)(data.Read<PackedInt2>().value());
                sw->unk_38E = (s16)(data.Read<PackedInt2>().value());
                sw->unk_390 = (s16)(data.Read<PackedInt2>().value());
                sw->unk_392 = (s16)(data.Read<PackedInt2>().value());
                sw->unk_394 = (s16)(data.Read<PackedInt2>().value());
                return true;
            case PROP_UNK_420:
                sw->unk_420 = data.Read<PackedFloat4>().value();
                return true;
            default:
                return false;
        }
    }

    void OnPropertiesApplied(u64 changed) override {
        if (changed & (1ull << PROP_BASIS))
            RebuildOrientation();

        if (Typed()->actionFunc == func_80B0D590 && !IsRunningLocally())
            GoLocal();
    }

    void OnBecomeLeader() override {
        RebuildOrientation();
    }

    void UpdatePuppet(PlayState* play) override {
        EnSw* sw = Typed();

        UpdateAnimation(&sw->skelAnime, LOCK_CUR_FRAME);

        if (sw->collider.base.acFlags & AC_HIT) {
            ClaimLeadership(CLAIM_REASON_HIT);
            m_originalUpdate(m_actor, play);
            return;
        }

        if (!IsGoldSkulltula() && sw->actor.xzDistToPlayer < 200.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_PROXIMITY);

        RegisterColliderBase(play, &sw->collider.base, CurrentColliderRoles());
    }
};

}

#endif
