#include "ZeldaOnlineClient.hpp"

#include "src/overlays/actors/ovl_En_Kusa/z_en_kusa.h"
#include "src/overlays/actors/ovl_En_Kanban/z_en_kanban.h"
#include "src/overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "src/overlays/actors/ovl_Obj_Syokudai/z_obj_syokudai.h"
#include "src/overlays/actors/ovl_Bg_Ddan_Kd/z_bg_ddan_kd.h"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "z64.h"
 #include "src/overlays/actors/ovl_Bg_Dodoago/z_bg_dodoago.h"
#include "src/overlays/actors/ovl_Bg_Spot02_Objects/z_bg_spot02_objects.h"

extern PlayState* gPlayState;
extern SaveContext gSaveContext;
extern s32 sLitTorchCount;
void BgDdanKd_LowerStairs(BgDdanKd*, PlayState* play);
void BgDdanKd_CheckForExplosions(BgDdanKd*, PlayState* play);

void BgDodoago_WaitExplosives(BgDodoago* self, PlayState* play);
void BgDodoago_LightOneEye(BgDodoago* self, PlayState* play);
void BgDodoago_OpenJaw(BgDodoago* self, PlayState* play);
void BgDodoago_DoNothing(BgDodoago* self, PlayState* play);
void EnArrow_Fly(EnArrow*, PlayState* play);
void func_808ACA08(BgSpot02Objects* thisx, PlayState* play);
}

namespace ZeldaOnline
{
    void ZeldaOnlineClient::OnSceneTrigger(AbstractActorController* sendingPlayer, const std::string& name, ByteStream& data)
    {
        static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
        if (name == "spawn" || name == "spawnC")
        {
            auto actorID = (int16_t)data.Read<PackedUInt2>().value();
            auto x = data.Read<PackedFloat4>().value();
            auto y = data.Read<PackedFloat4>().value();
            auto z = data.Read<PackedFloat4>().value();
            auto rotX = data.Read<PackedInt2>().value();
            auto rotY = data.Read<PackedInt2>().value();
            auto rotZ = data.Read<PackedInt2>().value();
            int16_t params = data.Read<PackedInt2>().value();

            if (name == "spawnC")
            {
                auto parentController = GetNetworkController(data.Read<PackedUInt2>().value());
                if (parentController) {
                    m_applyingRemoteSpawn = true;
                    Actor_SpawnAsChildDirect(&gPlayState->actorCtx, parentController->GetActor(), gPlayState, actorID, x, y, z, rotX,
                                             rotY, rotZ, x,
                                             y, z, rotX, rotY, rotZ, params, 0);
                    m_applyingRemoteSpawn = false;
                    return;
                }
            }
            m_applyingRemoteSpawn = true;
            Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, actorID, x, y, z, rotX, rotY, rotZ, x, y, z, rotX,
                                rotY, rotZ, params, 0);
            m_applyingRemoteSpawn = false;

        } else if (name == "hit") {
            struct HitSyncEntry {
                u8 category;
                u16 colliderOffset;
                u16 infoOffset;
                u32 fakeDmgFlags;
            };
            static const std::unordered_map<s16, HitSyncEntry> sHitSyncTable = {
                { ACTOR_EN_KANBAN,  { ACTORCAT_PROP, offsetof(EnKanban, collider), offsetof(EnKanban, collider.info), DMG_SLASH_KOKIRI } },
                { ACTOR_OBJ_SYOKUDAI, { ACTORCAT_PROP, offsetof(ObjSyokudai, colliderFlame), offsetof(ObjSyokudai, colliderFlame.info), 0 } },
            };
            auto actorID = data.Read<PackedUInt2>().value();

            if (data.BytesLeft() < 6 || gPlayState == NULL) {
                return;
            }
            s16 x = (s16)(data.Read<PackedInt2>().value());
            s16 y = (s16)(data.Read<PackedInt2>().value());
            s16 z = (s16)(data.Read<PackedInt2>().value());

            auto entry = sHitSyncTable.find((s16)(actorID));
            if (entry == sHitSyncTable.end()) {
                return;
            }

            for (Actor* a = gPlayState->actorCtx.actorLists[entry->second.category].head; a != NULL; a = a->next) {
                if (a->id != (s16)(actorID) || a->update == NULL) {
                    continue;
                }
                f32 dx = a->world.pos.x - x, dy = a->world.pos.y - y, dz = a->world.pos.z - z;
                if (dx * dx + dy * dy + dz * dz < 25.0f) {
                    Collider* collider =
                        reinterpret_cast<Collider*>(reinterpret_cast<u8*>(a) + entry->second.colliderOffset);

                    if (entry->second.infoOffset != 0) {
                        static ColliderInfo sRemoteHitInfo;
                        sRemoteHitInfo.toucher.dmgFlags = entry->second.fakeDmgFlags;
                        ;
                        sRemoteHitInfo.toucher.damage = 1;
                        ColliderInfo* info =
                            reinterpret_cast<ColliderInfo*>(reinterpret_cast<u8*>(a) + entry->second.infoOffset);
                        info->acHitInfo = &sRemoteHitInfo;
                    }
                    a->flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
                    collider->acFlags |= AC_HIT | AC_TYPE_OTHER;
                    break;
                }
            }
        }

        else if (name == "arrow") {
            if (data.BytesLeft() < 26 || gPlayState == NULL)
                return;

            f32 x = data.Read<PackedFloat4>().value();
            f32 y = data.Read<PackedFloat4>().value();
            f32 z = data.Read<PackedFloat4>().value();
            s16 rx = (s16)(data.Read<PackedInt2>().value());
            s16 ry = (s16)(data.Read<PackedInt2>().value());
            s16 params = (s16)(data.Read<PackedInt2>().value());
            f32 speedXZ = data.Read<PackedFloat4>().value();
            f32 velY = data.Read<PackedFloat4>().value();

            if (params == ARROW_NUT) {
                Player* localPlayer = GET_PLAYER(gPlayState);
                if (localPlayer != NULL) {
                    f32 dx = localPlayer->actor.world.pos.x - x;
                    f32 dz = localPlayer->actor.world.pos.z - z;
                    if (dx * dx + dz * dz > 400.0f * 400.0f) {
                        return;
                    }
                }
            }

            m_applyingRemoteSpawn = true;
            Actor* arrowActor = Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, ACTOR_EN_ARROW, x, y, z, rx, ry, 0,
                                                  x, y, z, rx, ry, 0, params, 0);
            m_applyingRemoteSpawn = false;

            if (arrowActor != NULL) {
                arrowActor->flags |= ACTOR_FLAG_ZO_USER1;

                EnArrow* arrow = reinterpret_cast<EnArrow*>(arrowActor);

                if (arrowActor != NULL) {
                    arrowActor->flags |= ACTOR_FLAG_ZO_USER1;
                    EnArrow* arrow = reinterpret_cast<EnArrow*>(arrowActor);

                    if (params != ARROW_NUT) {
                        arrow->actionFunc = EnArrow_Fly;
                        Math_Vec3f_Copy(&arrow->unk_210, &arrow->actor.world.pos);
                        arrow->actor.speedXZ = speedXZ;
                        arrow->actor.velocity.y = velY;
                        arrow->timer = (params >= ARROW_SEED) ? 15 : 12;
                        if (params >= ARROW_SEED) {
                            arrow->actor.shape.rot.x = arrow->actor.shape.rot.y = arrow->actor.shape.rot.z = 0;
                        }

                        if (true) {
                            arrow->collider.base.atFlags =
                                (arrow->collider.base.atFlags & ~AT_TYPE_ALL) | AT_TYPE_ENEMY;
                        } else {
                            arrow->collider.base.atFlags &= ~AT_ON;
                        }
                    }
                }
            }
        }

        else if (name == "dodongo_lowerStairs") {
            PlayState* play = gPlayState;
            if (play == nullptr) {
                return;
            }

            Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head;
            while (actor != nullptr) {
                if (actor->id == ACTOR_BG_DDAN_KD) {
                    BgDdanKd* stairs = reinterpret_cast<BgDdanKd*>(actor);

                    if (stairs->actionFunc == BgDdanKd_CheckForExplosions) {
                        stairs->dyna.actor.flags |= ACTOR_FLAG_ZO_USER2;
                        stairs->actionFunc = BgDdanKd_LowerStairs;
                        OnePointCutscene_Init(play, 3050, 999, &stairs->dyna.actor, MAIN_CAM);
                    }
                }
                actor = actor->next;
            }
        } else if (name == "dodongo_lightEye") {
            PlayState* play = gPlayState;
            if (play == nullptr) {
                return;
            }

            s16 eye = data.Read<PackedInt2>().value();

            Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head;
            while (actor != nullptr) {
                if (actor->id == ACTOR_BG_DODOAGO) {
                    BgDodoago* skull = reinterpret_cast<BgDodoago*>(actor);

                    if (skull->actionFunc == BgDodoago_WaitExplosives) {
                        skull->dyna.actor.flags |= ACTOR_FLAG_ZO_USER1;
                        skull->state = eye;
                        skull->actionFunc = BgDodoago_LightOneEye;
                    }
                }
                actor = actor->next;
            }
        } else if (name == "dodongo_openJaw") {
            PlayState* play = gPlayState;
            if (play == nullptr) {
                return;
            }

            Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head;
            while (actor != nullptr) {
                if (actor->id == ACTOR_BG_DODOAGO) {
                    BgDodoago* skull = reinterpret_cast<BgDodoago*>(actor);

                    if (skull->actionFunc != BgDodoago_OpenJaw && skull->actionFunc != BgDodoago_DoNothing) {
                        skull->dyna.actor.flags |= ACTOR_FLAG_ZO_USER2;
                        play->roomCtx.unk_74[BGDODOAGO_EYE_LEFT] = 255;
                        play->roomCtx.unk_74[BGDODOAGO_EYE_RIGHT] = 255;
                        skull->state = 0;
                        skull->actionFunc = BgDodoago_OpenJaw;
                        break;
                    }
                }
                actor = actor->next;
            }
        }

        else if (name == "destroyroyalgrave") {
            PlayState* play = gPlayState;
            if (play == nullptr) {
                return;
            }

            Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head;
            while (actor != nullptr) {
                if (actor->id == ACTOR_BG_SPOT02_OBJECTS && actor->params == 2) {
                    BgSpot02Objects* grave = reinterpret_cast<BgSpot02Objects*>(actor);

                    if (grave->actionFunc == func_808AC908)
                    {
                        grave->dyna.actor.flags |= ACTOR_FLAG_ZO_USER1;
                        Vec3f pos;

                        Audio_PlayActorSound2(&grave->dyna.actor, NA_SE_EV_GRAVE_EXPLOSION);
                        grave->timer = 25;
                        pos.x = (Math_SinS(grave->dyna.actor.shape.rot.y) * 50.0f) + grave->dyna.actor.world.pos.x;
                        pos.y = grave->dyna.actor.world.pos.y + 30.0f;
                        pos.z = (Math_CosS(grave->dyna.actor.shape.rot.y) * 50.0f) + grave->dyna.actor.world.pos.z;
                        EffectSsBomb2_SpawnLayered(gPlayState, &pos, &zeroVec, &zeroVec, 70, 30);
                        grave->actionFunc = func_808ACA08;
                        break;
                    }
                }
                actor = actor->next;
            }

        }

	}

 }
