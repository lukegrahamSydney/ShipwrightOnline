#ifndef ZL3CONTROLLERH
#define ZL3CONTROLLERH

#include <cstring>

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Zl3/z_en_zl3.h"
#include "objects/object_zl2_anime2/object_zl2_anime2.h"

void EnZl3_UpdateEyes(EnZl3*);
void func_80B56F10(EnZl3* thisx, PlayState* play);

extern EnZl3* sBossGanonZelda;
extern EnZl3* sBossGanon2Zelda;

}

namespace ZeldaOnline {

class Zl3Controller : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnZl3* Typed() const {
        return reinterpret_cast<EnZl3*>(m_actor);
    }

    static constexpr bool LOCK_CUR_FRAME = true;

  protected:
    static constexpr u8 ANIM_UNKNOWN = 0xFF;
    static constexpr int ANIM_COUNT = 35;

    static const char* AnimForIndex(u8 i) {
        static const char* sAnims[] = {
            gZelda2Anime2Anim_0001D8, gZelda2Anime2Anim_0004F4, gZelda2Anime2Anim_001110, gZelda2Anime2Anim_0014DC,
            gZelda2Anime2Anim_001D8C, gZelda2Anime2Anim_00210C, gZelda2Anime2Anim_002348, gZelda2Anime2Anim_002710,
            gZelda2Anime2Anim_002E54, gZelda2Anime2Anim_0038C0, gZelda2Anime2Anim_003D20, gZelda2Anime2Anim_003FF8,
            gZelda2Anime2Anim_004408, gZelda2Anime2Anim_00499C, gZelda2Anime2Anim_005248, gZelda2Anime2Anim_0054E0,
            gZelda2Anime2Anim_005A0C, gZelda2Anime2Anim_0061C4, gZelda2Anime2Anim_006508, gZelda2Anime2Anim_006AB0,
            gZelda2Anime2Anim_006F04, gZelda2Anime2Anim_007664, gZelda2Anime2Anim_007A78, gZelda2Anime2Anim_007C84,
            gZelda2Anime2Anim_008050, gZelda2Anime2Anim_0082F8, gZelda2Anime2Anim_008684, gZelda2Anime2Anim_008AD0,
            gZelda2Anime2Anim_0091D8, gZelda2Anime2Anim_0099A0, gZelda2Anime2Anim_009BE4, gZelda2Anime2Anim_009FBC,
            gZelda2Anime2Anim_00A334, gZelda2Anime2Anim_00A598, gZelda2Anime2Anim_00AACC,
        };
        if (i >= ANIM_COUNT)
            return nullptr;
        return sAnims[i];
    }

    void EnsurePathLinked() {
        EnZl3* zl = Typed();

        if (zl->unk_30C != nullptr || gPlayState == nullptr || gPlayState->setupPathList == nullptr)
            return;

        func_80B56F10(zl, gPlayState);
    }

    void OnActorInit() override {
        sBossGanonZelda = Typed();
        sBossGanon2Zelda = Typed();
        EnsurePathLinked();
    }



    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++)
            if (strcmp(AnimForIndex(i), cur) == 0)
                return i;
        return ANIM_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_MOTION,
        PROP_FACE,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnZl3* zl = Typed();

        PackProperty(PROP_ACTION,
                     ByteStream() << PackedInt4(zl->action) << PackedInt4(zl->drawConfig) << PackedInt4(zl->alpha),
                     out);

        PackProperty(PROP_STATE,
                     ByteStream() << PackedInt4(zl->unk_2F0) << PackedInt4(zl->unk_2F8) << PackedInt4(zl->unk_2FC)
                                  << PackedInt4(zl->unk_308) << PackedInt4(zl->unk_310) << PackedInt4(zl->unk_314)
                                  << PackedInt4(zl->unk_328) << PackedInt4(zl->unk_36C) << PackedInt4(zl->unk_370)
                                  << PackedInt4(zl->unk_374) << PackedInt4(zl->unk_3C4) << PackedInt4(zl->unk_3D8)
                                  << PackedUInt1(zl->unk_3C8) << PackedInt2(zl->unk_3D0),
                     out);

        PackProperty(PROP_MOTION,
                     ByteStream() << PackedFloat4(zl->unk_2EC) << PackedFloat4(zl->unk_360)
                                  << PackedFloat4(zl->unk_364) << PackedFloat4(zl->unk_368)
                                  << PackedFloat4(zl->unk_3CC) << PackedFloat4(zl->unk_3DC)
                                  << PackedFloat4(zl->unk_3E0) << PackedFloat4(zl->unk_3E4)
                                  << PackedFloat4(zl->unk_3E8) << PackedFloat4(zl->unk_3EC)
                                  << PackedFloat4(zl->unk_3F0) << PackedFloat4(zl->unk_3F4)
                                  << PackedUInt2(zl->unk_344) << PackedUInt2(zl->unk_346),
                     out);

        PackProperty(PROP_FACE, ByteStream() << PackedInt2(zl->mouthTexIndex), out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(zl->skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &zl->skelAnime), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SHAPE_ROT, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnZl3* zl = Typed();

        switch (index) {
            case PROP_ACTION:
                zl->action = (s32)(data.Read<PackedInt4>().value());
                zl->drawConfig = (s32)(data.Read<PackedInt4>().value());
                zl->alpha = (s32)(data.Read<PackedInt4>().value());
                break;
            case PROP_STATE:
                zl->unk_2F0 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_2F8 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_2FC = (s32)(data.Read<PackedInt4>().value());
                zl->unk_308 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_310 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_314 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_328 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_36C = (s32)(data.Read<PackedInt4>().value());
                zl->unk_370 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_374 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_3C4 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_3D8 = (s32)(data.Read<PackedInt4>().value());
                zl->unk_3C8 = (u8)(data.Read<PackedUInt1>().value());
                zl->unk_3D0 = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_MOTION:
                zl->unk_2EC = data.Read<PackedFloat4>().value();
                zl->unk_360 = data.Read<PackedFloat4>().value();
                zl->unk_364 = data.Read<PackedFloat4>().value();
                zl->unk_368 = data.Read<PackedFloat4>().value();
                zl->unk_3CC = data.Read<PackedFloat4>().value();
                zl->unk_3DC = data.Read<PackedFloat4>().value();
                zl->unk_3E0 = data.Read<PackedFloat4>().value();
                zl->unk_3E4 = data.Read<PackedFloat4>().value();
                zl->unk_3E8 = data.Read<PackedFloat4>().value();
                zl->unk_3EC = data.Read<PackedFloat4>().value();
                zl->unk_3F0 = data.Read<PackedFloat4>().value();
                zl->unk_3F4 = data.Read<PackedFloat4>().value();
                zl->unk_344 = (u16)(data.Read<PackedUInt2>().value());
                zl->unk_346 = (u16)(data.Read<PackedUInt2>().value());
                break;
            case PROP_FACE:
                zl->mouthTexIndex = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_ANIM_CUR_FRAME:
                zl->skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* anim = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)anim, &zl->skelAnime, LOCK_CUR_FRAME ? zl->skelAnime.curFrame : 0.0f, data);
                break;
            }
            default:
                return false;
        }
        return true;
    }

    void OnPropertiesApplied(u64 changed) override {
        EnZl3* zl = Typed();

        if (zl->action == 27 && !m_startedSubTimer) {
            m_startedSubTimer = true;
            Interface_SetSubTimer(180);
            gSaveContext.healthAccumulator = 320;
            if (gPlayState != nullptr)
                Magic_Fill(gPlayState);
        }
    }

    void UpdatePuppet(PlayState* play) override {

        EnZl3* zl = Typed();

        if (zl->action == 28 && zl->actor.xzDistToPlayer < 150.0f && IsLocalPlayerClosest())
            ClaimLeadership(CLAIM_REASON_COOLDOWN);

        UpdateAnimation(&zl->skelAnime, LOCK_CUR_FRAME);

        EnZl3_UpdateEyes(zl);

        Collider_UpdateCylinder(&zl->actor, &zl->collider);
        RegisterColliderBase(play, &zl->collider.base, COLL_OC);
    }

    bool m_startedSubTimer = false;
};

} // namespace ZeldaOnline

#endif
