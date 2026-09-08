#ifndef PHANTOMHORSECONTROLLERH
#define PHANTOMHORSECONTROLLERH

#include <cstring>

#include "../AbstractBossController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_fHG/z_en_fhg.h"
#include "src/overlays/actors/ovl_Boss_Ganondrof/z_boss_ganondrof.h"
#include "objects/object_fhg/object_fhg.h"
#include "assets/textures/boss_title_cards/object_fhg.h"

void EnfHG_Intro(EnfHG* fhg, PlayState* play);
void EnfHG_Approach(EnfHG* fhg, PlayState* play);
void EnfHG_Attack(EnfHG* fhg, PlayState* play);
void EnfHG_Damage(EnfHG* fhg, PlayState* play);
void EnfHG_Retreat(EnfHG* fhg, PlayState* play);
void EnfHG_Done(EnfHG* fhg, PlayState* play);

u16 func_800FA0B4(u8 seqPlayerIndex);
}

namespace ZeldaOnline {

class PhantomHorseController : public AbstractBossController {
  public:
    using AbstractBossController::AbstractBossController;

    EnfHG* Typed() const {
        return reinterpret_cast<EnfHG*>(m_actor);
    }

    static bool IsNetworkedVariant(s16 params) {
        return params < GND_FAKE_BOSS;
    }

    bool CanSpawnActorOverNetwork(s16 actorId, s16 params) override {
        return actorId == ACTOR_EN_FHG_FIRE || actorId == ACTOR_BOSS_GANONDROF;
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using FhgActionFunc = void (*)(EnfHG*, PlayState*);
    static const FhgActionFunc* ActionTable(size_t* count) {
        static const FhgActionFunc sTable[] = {
            EnfHG_Intro, EnfHG_Approach, EnfHG_Attack, EnfHG_Damage, EnfHG_Retreat, EnfHG_Done,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    static constexpr u8 ANIM_COUNT = 6;
    static constexpr u8 ANIM_UNKNOWN = 0xFF;

    static const char* AnimForIndex(u8 idx) {
        switch (idx) {
            case 0:
                return gPhantomHorseIdleAnim;
            case 1:
                return gPhantomHorseRunningAnim;
            case 2:
                return gPhantomHorseLeapAnim;
            case 3:
                return gPhantomHorseAirAnim;
            case 4:
                return gPhantomHorseLandAnim;
            case 5:
                return gPhantomHorseRearingAnim;
            default:
                return nullptr;
        }
    }

    const char* GetTitleCard() const override {
        return gPhantomGanonTitleCardENGTex;
    }

    int16_t* GetCameraSubID() override {
        return &Typed()->cutsceneCamera;
    }

    Vec3f_* GetCameraAt() override {
        return &Typed()->cameraAt;
    }

    Vec3f_* GetCameraEye() override {
        return &Typed()->cameraEye;
    }

    u8 CurrentAnimIndex() const {
        const char* cur = (const char*)Typed()->skin.skelAnime.animation;
        if (cur == nullptr)
            return ANIM_UNKNOWN;
        for (u8 i = 0; i < ANIM_COUNT; i++) {
            const char* a = AnimForIndex(i);
            if (a != nullptr && strcmp(cur, a) == 0)
                return i;
        }
        return ANIM_UNKNOWN;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const FhgActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_BOSS_END,
        PROP_SIGNAL,
        PROP_TIMERS,
        PROP_PAINTING,
        PROP_MOTION,
        PROP_WARP_FILTER,
        PROP_ANIM_CUR_FRAME,
        PROP_ANIM,
    };

    void OnActorInit() override {

        if (Flags_GetClear(gPlayState, gPlayState->roomCtx.curRoom.num))
            Actor_Kill(m_actor);
    }

    void BuildCustomProperties(ByteStream& out) override {
        EnfHG* fhg = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);


        PackProperty(PROP_SIGNAL,
                     ByteStream() << PackedUInt1(fhg->bossGndSignal) << PackedUInt1(fhg->bossGndInPainting)
                                  << PackedUInt1(fhg->killActor) << PackedUInt1(fhg->fhgFireKillWarp),
                     out);

        ByteStream timers;
        for (s32 i = 0; i < 5; i++)
            timers << PackedInt2(fhg->timers[i]);
        timers << PackedInt2(fhg->hitTimer) << PackedInt2(fhg->gallopTimer);
        PackProperty(PROP_TIMERS, timers, out);

        PackProperty(PROP_PAINTING,
                     ByteStream() << PackedInt2(fhg->curPainting) << PackedInt2(fhg->targetPainting)
                                  << PackedInt2(fhg->turnTarget) << PackedInt2(fhg->turnRot)
                                  << PackedInt2(fhg->spawnedWarp) << PackedInt2(fhg->cutsceneState),
                     out);

        PackProperty(PROP_MOTION,
                     ByteStream() << PackedFloat4(fhg->inPaintingPos.x) << PackedFloat4(fhg->inPaintingPos.y)
                                  << PackedFloat4(fhg->inPaintingPos.z) << PackedFloat4(fhg->inPaintingVelX)
                                  << PackedFloat4(fhg->inPaintingVelZ) << PackedFloat4(fhg->damageSpeedMod)
                                  << PackedFloat4(fhg->approachRate),
                     out);

        PackProperty(PROP_WARP_FILTER,
                     ByteStream() << PackedFloat4(fhg->warpColorFilterR) << PackedFloat4(fhg->warpColorFilterG)
                                  << PackedFloat4(fhg->warpColorFilterB) << PackedFloat4(fhg->warpColorFilterUnk1)
                                  << PackedFloat4(fhg->warpColorFilterUnk2),
                     out);

        PackProperty(PROP_ANIM_CUR_FRAME, PackedFloat4(fhg->skin.skelAnime.curFrame), out);
        PackProperty(PROP_ANIM, BuildAnimProperty(CurrentAnimIndex(), &fhg->skin.skelAnime), out);

        BuildBossProperty(PROP_BOSS_CAMERA, out);
        BuildBossProperty(PROP_BOSS_BGM, out);
        BuildBossProperty(PROP_BOSS_TITLE_CARD, out);
        BuildBossProperty(PROP_BOSS_LIGHTING, out);

        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_SCALE, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnfHG* fhg = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const FhgActionFunc* table = ActionTable(&count);
                if (id < count)
                    fhg->actionFunc = table[id];
                break;
            }
            case PROP_SIGNAL:
                fhg->bossGndSignal = (u8)(data.Read<PackedUInt1>().value());
                fhg->bossGndInPainting = (u8)(data.Read<PackedUInt1>().value());
                fhg->killActor = (u8)(data.Read<PackedUInt1>().value());
                fhg->fhgFireKillWarp = (u8)(data.Read<PackedUInt1>().value());
                break;
            case PROP_TIMERS:
                for (s32 i = 0; i < 5; i++)
                    fhg->timers[i] = (s16)(data.Read<PackedInt2>().value());
                fhg->hitTimer = (s16)(data.Read<PackedInt2>().value());
                fhg->gallopTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_PAINTING:
                fhg->curPainting = (s16)(data.Read<PackedInt2>().value());
                fhg->targetPainting = (s16)(data.Read<PackedInt2>().value());
                fhg->turnTarget = (s16)(data.Read<PackedInt2>().value());
                fhg->turnRot = (s16)(data.Read<PackedInt2>().value());
                fhg->spawnedWarp = (s16)(data.Read<PackedInt2>().value());
                fhg->cutsceneState = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_MOTION:
                fhg->inPaintingPos.x = data.Read<PackedFloat4>().value();
                fhg->inPaintingPos.y = data.Read<PackedFloat4>().value();
                fhg->inPaintingPos.z = data.Read<PackedFloat4>().value();
                fhg->inPaintingVelX = data.Read<PackedFloat4>().value();
                fhg->inPaintingVelZ = data.Read<PackedFloat4>().value();
                fhg->damageSpeedMod = data.Read<PackedFloat4>().value();
                fhg->approachRate = data.Read<PackedFloat4>().value();
                break;
            case PROP_WARP_FILTER:
                fhg->warpColorFilterR = data.Read<PackedFloat4>().value();
                fhg->warpColorFilterG = data.Read<PackedFloat4>().value();
                fhg->warpColorFilterB = data.Read<PackedFloat4>().value();
                fhg->warpColorFilterUnk1 = data.Read<PackedFloat4>().value();
                fhg->warpColorFilterUnk2 = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM_CUR_FRAME:
                fhg->skin.skelAnime.curFrame = data.Read<PackedFloat4>().value();
                break;
            case PROP_ANIM: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                const char* a = (id != ANIM_UNKNOWN) ? AnimForIndex(id) : nullptr;
                ApplyAnimProperty((void*)a, &fhg->skin.skelAnime, fhg->skin.skelAnime.curFrame, data);
                break;
            }

            default:
                return ApplyBossProperty(index, data, propLen);
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnfHG* fhg = Typed();

        UpdateAnimation(&fhg->skin.skelAnime, true);
    }

  private:

};

} // namespace ZeldaOnline

#endif