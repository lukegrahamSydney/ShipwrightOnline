#ifndef ABSTRACTBOSSCONTROLLERH
#define ABSTRACTBOSSCONTROLLERH

#include "AbstractActorController.hpp"

namespace ZeldaOnline {
class AbstractBossController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

  protected:
    u16 m_lastWantedBgm = 0xFFFF;
    bool m_touchedEnv = false;

    enum {
        PROP_BOSS_BGM = PROP_CUSTOM_START,
        PROP_BOSS_CAMERA,
        PROP_BOSS_TITLE_CARD,
        PROP_BOSS_LIGHTING,
        PROP_BOSS_ENV,
        PROP_BOSS_END

    };

    virtual const char* GetTitleCard() const {
        return nullptr;
    }

    virtual int16_t* GetCameraSubID() {
        return nullptr;
    }

    virtual Vec3f_* GetCameraAt() {
        return nullptr;
    }

    virtual Vec3f_* GetCameraEye() {
        return nullptr;
    }

    void EndCutsceneCamera() {
        auto* camSubID = GetCameraSubID();
        if (camSubID == nullptr || gPlayState == nullptr || *camSubID == MAIN_CAM || gPlayState->activeCamera == MAIN_CAM)
            return;

        Play_ClearCamera(gPlayState, *camSubID);
        *camSubID = MAIN_CAM;
        func_80064534(gPlayState, &gPlayState->csCtx);
        Play_ChangeCameraStatus(gPlayState, MAIN_CAM, CAM_STAT_ACTIVE);
        Player_SetCsActionWithHaltedActors(gPlayState, m_actor, 7);
    }

    void ResetEnvironment() {
        if (gPlayState == nullptr || !m_touchedEnv)
            return;

        EnvironmentContext* env = &gPlayState->envCtx;

        env->unk_BD = 0;
        env->unk_BE = 0;
        env->unk_BF = 0;
        env->unk_DC = 0;
        env->unk_D8 = 0.0f;
        env->fillScreen = false;
        env->customSkyboxFilter = 0;

        for (int i = 0; i < 4; i++) {
            env->screenFillColor[i] = 0;
            env->skyboxFilterColor[i] = 0;
        }

        for (int i = 0; i < 3; i++) {
            env->adjAmbientColor[i] = 0;
            env->adjLight1Color[i] = 0;
            env->adjFogColor[i] = 0;
        }

        env->adjFogNear = 0;
        env->adjFogFar = 0;

        m_touchedEnv = false;
    }

    bool BuildAllBossProperties(ByteStream& out) {
        for (unsigned int i = PROP_BOSS_BGM; i < PROP_BOSS_END; i++)
            BuildBossProperty(i, out);
        return true;
    }

    bool BuildBossProperty(unsigned int which, ByteStream& out) {
        switch (which) {
            case PROP_BOSS_BGM:
                PackProperty(PROP_BOSS_BGM, PackedUInt2(func_800FA0B4(SEQ_PLAYER_BGM_MAIN)), out);
                return true;

            case PROP_BOSS_TITLE_CARD:
                if (gPlayState->actorCtx.titleCtx.durationTimer > 0)
                    PackProperty(PROP_BOSS_TITLE_CARD, PackedUInt1(gPlayState->actorCtx.titleCtx.durationTimer), out);
                else
                    PackNullProperty(PROP_BOSS_TITLE_CARD, out);
                return true;

            case PROP_BOSS_CAMERA: {
                auto subCamera = GetCameraSubID();

                if (gPlayState && gPlayState->activeCamera != MAIN_CAM && subCamera != nullptr &&
                    *subCamera !=  MAIN_CAM) {
                    auto* eye = GetCameraEye();
                    auto* at = GetCameraAt();

                    if (eye && at) {
                        PackProperty(PROP_BOSS_CAMERA,
                                     ByteStream()
                                         << PackedFloat4(eye->x) << PackedFloat4(eye->y) << PackedFloat4(eye->z)
                                         << PackedFloat4(at->x) << PackedFloat4(at->y) << PackedFloat4(at->z),
                                     out);
                    } else
                        PackNullProperty(PROP_BOSS_CAMERA, out);
                } else
                    PackNullProperty(PROP_BOSS_CAMERA, out);
            }
            return true;

            case PROP_BOSS_LIGHTING: {
                if (gPlayState == nullptr) {
                    PackNullProperty(PROP_BOSS_LIGHTING, out);
                    return true;
                }

                EnvironmentContext* env = &gPlayState->envCtx;
                f32 intensity = env->unk_D8;

                if (intensity < 0.0f)
                    intensity = 0.0f;
                if (intensity > 1.0f)
                    intensity = 1.0f;

                u8 flags = 0;
                if (env->fillScreen)
                    flags |= 1;
                if (env->customSkyboxFilter)
                    flags |= 2;

                ByteStream body;
                body << PackedUInt1((u8)(env->unk_BD));
                body << PackedUInt1((u8)(env->unk_BE));
                body << PackedUInt1((u8)(env->unk_BF));
                body << PackedUInt1((u8)(env->unk_DC));
                body << PackedUInt1((u8)(intensity * 255.0f));
                body << PackedUInt1(flags);

                for (int i = 0; i < 4; i++)
                    body << PackedUInt1((u8)(env->screenFillColor[i]));
                for (int i = 0; i < 4; i++)
                    body << PackedUInt1((u8)(env->skyboxFilterColor[i]));

                PackProperty(PROP_BOSS_LIGHTING, body, out);
            }
                return true;

            case PROP_BOSS_ENV: {
                if (gPlayState == nullptr) {
                    PackNullProperty(PROP_BOSS_ENV, out);
                    return true;
                }

                EnvironmentContext* env = &gPlayState->envCtx;
                ByteStream body;

                for (int i = 0; i < 3; i++)
                    body << PackedUInt1((u8)(env->adjAmbientColor[i]));
                for (int i = 0; i < 3; i++)
                    body << PackedUInt1((u8)(env->adjLight1Color[i]));
                for (int i = 0; i < 3; i++)
                    body << PackedUInt1((u8)(env->adjFogColor[i]));

                body << PackedInt2(env->adjFogNear);
                body << PackedInt2(env->adjFogFar);

                PackProperty(PROP_BOSS_ENV, body, out);
            }
                return true;

            default:
                return false;
        }
    }

    bool ApplyBossProperty(unsigned int index, ByteStream& data, unsigned int propLen) {
        switch (index) {
            case PROP_BOSS_BGM: {
                u16 wanted = (u16)(data.Read<PackedUInt2>().value());

                if (wanted != m_lastWantedBgm) {
                    m_lastWantedBgm = wanted;

                    if (func_800FA0B4(SEQ_PLAYER_BGM_MAIN) != wanted) {
                        Audio_QueueSeqCmd(SEQ_PLAYER_BGM_MAIN << 24 | wanted);
                    }
                }
            }
                return true;

            case PROP_BOSS_TITLE_CARD: {
                auto titleCard = GetTitleCard();
                if (propLen == 0 || titleCard == nullptr)
                    gPlayState->actorCtx.titleCtx.durationTimer = 0;
                else {
                    auto duration = data.Read<PackedUInt1>().value();
                    if (gPlayState->actorCtx.titleCtx.durationTimer == 0)
                        TitleCard_InitBossName(gPlayState, &gPlayState->actorCtx.titleCtx,
                                               SEGMENTED_TO_VIRTUAL((void*)titleCard), 160, 180, 128, 40, true);
                    gPlayState->actorCtx.titleCtx.durationTimer = duration;
                }
            }
                return true;

            case PROP_BOSS_CAMERA: {
                if (propLen == 0)
                    EndCutsceneCamera();
                else {
                    auto* eye = GetCameraEye();
                    auto* at = GetCameraAt();

                    if (at && eye) {
                        eye->x = data.Read<PackedFloat4>().value();
                        eye->y = data.Read<PackedFloat4>().value();
                        eye->z = data.Read<PackedFloat4>().value();
                        at->x = data.Read<PackedFloat4>().value();
                        at->y = data.Read<PackedFloat4>().value();
                        at->z = data.Read<PackedFloat4>().value();

                        if (gPlayState != nullptr) {

                            auto* cameraSubID = GetCameraSubID();
                            if (cameraSubID != nullptr) {
                                if (gPlayState->activeCamera == MAIN_CAM) {
                                    *cameraSubID = Play_CreateSubCamera(gPlayState);
                                    Play_ChangeCameraStatus(gPlayState, MAIN_CAM, CAM_STAT_WAIT);
                                    Play_ChangeCameraStatus(gPlayState, *cameraSubID, CAM_STAT_ACTIVE);
                                    func_80064520(gPlayState, &gPlayState->csCtx);
                                }
                                Play_CameraSetAtEye(gPlayState, *cameraSubID, at, eye);
                            }
                        }
                    }
                }
            }

                return true;

            case PROP_BOSS_LIGHTING: {
                if (propLen == 0)
                    return true;

                EnvironmentContext* env = &gPlayState->envCtx;

                u8 bd = (u8)(data.Read<PackedUInt1>().value());
                u8 be = (u8)(data.Read<PackedUInt1>().value());
                u8 bf = (u8)(data.Read<PackedUInt1>().value());
                u8 dc = (u8)(data.Read<PackedUInt1>().value());
                u8 intensity = (u8)(data.Read<PackedUInt1>().value());
                u8 flags = (u8)(data.Read<PackedUInt1>().value());

                env->unk_BD = (s8)(bd);
                env->unk_BE = (s8)(be);
                env->unk_BF = (s8)(bf);
                env->unk_DC = (s8)(dc);
                env->unk_D8 = intensity / 255.0f;
                env->fillScreen = (flags & 1) != 0;
                env->customSkyboxFilter = (flags & 2) != 0;

                for (int i = 0; i < 4; i++)
                    env->screenFillColor[i] = (s8)(u8)(data.Read<PackedUInt1>().value());
                for (int i = 0; i < 4; i++)
                    env->skyboxFilterColor[i] = (s8)(u8)(data.Read<PackedUInt1>().value());

                m_touchedEnv = true;
            }
                return true;

            case PROP_BOSS_ENV: {
                if (propLen == 0)
                    return true;

                EnvironmentContext* env = &gPlayState->envCtx;

                for (int i = 0; i < 3; i++)
                    env->adjAmbientColor[i] = (s8)(u8)(data.Read<PackedUInt1>().value());
                for (int i = 0; i < 3; i++)
                    env->adjLight1Color[i] = (s8)(u8)(data.Read<PackedUInt1>().value());
                for (int i = 0; i < 3; i++)
                    env->adjFogColor[i] = (s8)(u8)(data.Read<PackedUInt1>().value());

                env->adjFogNear = (s16)(data.Read<PackedInt2>().value());
                env->adjFogFar = (s16)(data.Read<PackedInt2>().value());

                m_touchedEnv = true;
            }
                return true;

            default:
                return false;
        }
    }
};
}; // namespace ZeldaOnline

#endif