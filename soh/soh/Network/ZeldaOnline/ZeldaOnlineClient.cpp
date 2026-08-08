#include "ZeldaOnlineClient.hpp"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/frame_interpolation.h"
#include "Packet.hpp"
#include "PacketTypes.hpp"
#include "ActorControllerFactory.hpp"
#include "ActorControllers/PlayerPuppetController.hpp"
#include "z64actor_enum.h"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/nametag.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "src/overlays/actors/ovl_En_Horse/z_en_horse.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <cstdio>
#include "HorsePuppet.hpp"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

#ifdef _WIN32
static void DbgPrintf(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    OutputDebugStringA(buf);
}
#endif

extern "C" {
#include "variables.h"
#include "functions.h"
#include "z64.h"
#include "src/overlays/actors/ovl_En_Door/z_en_door.h"
extern f32 D_80130F28;
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
extern MapData* gMapData;
extern int gMapLoading;
u8 gZeldaOnlineEngineCleanup = 0;
float OTRGetDimensionFromLeftEdge(float v);
float OTRGetDimensionFromRightEdge(float v);

void FrameInterpolation_RecordOpenChild(const void* a, int b);
void FrameInterpolation_RecordCloseChild(void);
}

namespace ZeldaOnline {

#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#endif

//Some locations are using the same scene. We can make these locations unique based on some other data
//Grttos/Fairy locations
static int GetSceneVariant() {
    switch (gPlayState->sceneNum) {
        case SCENE_GROTTOS:
            return gSaveContext.respawn[RESPAWN_MODE_RETURN].data & 0xFF;

        case SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC:
        case SCENE_FAIRYS_FOUNTAIN:
        case SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS:
        case SCENE_GRAVE_WITH_FAIRYS_FOUNTAIN:
            return gSaveContext.entranceIndex & 0xFFFF;

        default:
            return 0;
    }
}

//Can we call ReloadSceneInPlace? Not while paused or in a cutscene
static bool CanReloadSceneNow(PlayState* play) {
    Player* player = GET_PLAYER(play);
    return play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->msgCtx.msgMode == MSGMODE_NONE && play->pauseCtx.state == 0 && play->csCtx.state == CS_STATE_IDLE &&
           player != NULL && !(player->stateFlags1 & (PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE));
}

//Reload the current scene and keep the same position.
//Used for day/night transitions and disconnects/reconnects
static void ReloadSceneInPlace(PlayState* play) {
    Player* player = GET_PLAYER(play);
    if (player == NULL)
        return;

    if ((player->stateFlags1 & PLAYER_STATE1_ON_HORSE) && player->rideActor != NULL) {
        gSaveContext.horseData.scene = play->sceneNum;
        gSaveContext.horseData.pos.x = (int16_t)player->rideActor->world.pos.x;
        gSaveContext.horseData.pos.y = (int16_t)player->rideActor->world.pos.y;
        gSaveContext.horseData.pos.z = (int16_t)player->rideActor->world.pos.z;
        gSaveContext.horseData.angle = player->rideActor->world.rot.y;
    }

    gSaveContext.respawnFlag = 1;
    play->nextEntranceIndex = gSaveContext.entranceIndex;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = play->nextEntranceIndex;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].roomIndex = play->roomCtx.curRoom.num;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = player->actor.world.pos;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = player->actor.shape.rot.y;
    if (play->roomCtx.curRoom.behaviorType2 < 4) {
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = 0x0DFF;
    } else {
        Camera* camera = GET_ACTIVE_CAM(play);
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = 0x0D00 | camera->camDataIdx;
    }
    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_INSTANT;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK_FAST;

    static int hookId = 0;
    hookId = REGISTER_VB_SHOULD(VB_INFLICT_VOID_DAMAGE, {
        *should = false;
        GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnVanillaBehavior>(hookId);
    });
}

ZeldaOnlineClient::ZeldaOnlineClient() {
    m_blockedParentSpawners.insert(ACTOR_BG_SPOT01_OBJECTS2);
}

void ZeldaOnlineClient::Enable() {
    m_host = CVarGetString("gZeldaOnline.Host", "107.175.79.45");
    m_port = CVarGetInteger("gZeldaOnline.Port", 21050);

    ZNetworking::Enable(m_host.c_str(), m_port);

    RegisterNetworkingHook(true);

    CVarSetString("gZeldaOnline.Host", m_host.c_str());
    CVarSetInteger("gZeldaOnline.Port", m_port);
}

void ZeldaOnlineClient::OnConnected() {
    DbgPrintf("OnConnected()\n");
    m_sendFullPlayerProps = true;
    m_lastAppearanceBlob.Clear();
    m_lastPlayerProps.Clear();
    RegisterHooks(true);
    recvBuffer.Clear();

    outgoingBuffer.Clear();
    m_myNetworkID = 0;
    WritePacket(newPacket(0));

    if (m_didDisconnect)
        ReloadSceneInPlace(gPlayState);

}

void ZeldaOnlineClient::OnDisconnected() {
    DbgPrintf("OnDisconnected()\n");
    m_didDisconnect = true;
    m_hasWorldTime = false;
    printf("PLAYER DISCONNECTED\n");
    RegisterNetworkingHook(false);
    RegisterHooks(false);

    ZeldaOnlineClient::Enable();
}

void ZeldaOnlineClient::OnConnectionClosedBeforeConnect() {
    OnDisconnected();
}

#include "src/overlays/actors/ovl_Obj_Switch/z_obj_switch.h"
extern "C" {
void ObjSwitch_FloorPressInit(ObjSwitch* objSwitch);
void ObjSwitch_FloorReleaseInit(ObjSwitch* objSwitch);
}
static void NudgeFloorSwitches(int flag, u8 pressed) {
    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_SWITCH].head;
    for (; actor != NULL; actor = actor->next) {
        if (actor->id != ACTOR_OBJ_SWITCH)
            continue;
        ObjSwitch* sw = (ObjSwitch*)actor;
        s32 type = sw->dyna.actor.params & 7;
        if (type != OBJSWITCH_TYPE_FLOOR && type != OBJSWITCH_TYPE_FLOOR_RUSTY)
            continue;
        if (((sw->dyna.actor.params >> 8) & 0x3F) != flag)
            continue;
        if (pressed && sw->dyna.actor.scale.y >= (33.0f / 200.0f))
            ObjSwitch_FloorPressInit(sw);
        else if (!pressed && sw->dyna.actor.scale.y <= (33.0f / 2000.0f))
            ObjSwitch_FloorReleaseInit(sw);
    }
}

//Anchor put puppet players into a different category. This caused problems when holding objects (the object would lag behind)
//We are going to correctly put them in the player category but fix the ordering so the 
//local player is always at head
static void RelinkPuppetBehindPlayer(Actor* actor) {
    ActorListEntry* list = &gPlayState->actorCtx.actorLists[ACTORCAT_PLAYER];
    if (list->head == actor && actor->next != NULL) {
        Actor* realPlayer = actor->next;

        list->head = realPlayer;
        realPlayer->prev = NULL;

        actor->next = realPlayer->next;
        actor->prev = realPlayer;

        if (realPlayer->next != NULL) {
            realPlayer->next->prev = actor;
        }
        realPlayer->next = actor;
    }
}

void ZeldaOnlineClient::OnIncomingPacket(ByteStream& packet) {
    auto serverPacketID = packet.Read<PackedUInt1>().value();

    switch (serverPacketID) {
        case SERVER_PACKET_UPDATE_PLAYER: {

        } break;

        //old code
        case SERVER_PACKET_UPDATE_ACTOR: {
            if (packet.BytesLeft() < 2) {
                SPDLOG_ERROR("[ZeldaOnline] malformed SERVER_PACKET_UPDATE_ACTOR ({} bytes)", packet.BytesLeft());
                break;
            }

            int networkID = (int)(packet.Read<PackedUInt2>().value());

            auto it = m_networkedActors.find(networkID);
            if (it == m_networkedActors.end()) {
                break;
            }
            if (it->second->IsLeader()) {
                break;
            }
        } break;

        //new code
        case SERVER_PACKET_ACTOR_PROPERTIES: {
            if (packet.BytesLeft() < 2)
                break;

            int networkID = (int)(packet.Read<PackedUInt2>().value());

            auto it = m_networkedActors.find(networkID);
            if (it == m_networkedActors.end())
                break;

            AbstractActorController* controller = it->second;
            if (controller->IsLeader())
                return;
            ByteStream props = packet.Read(packet.BytesLeft());
            if (controller->GetActor()) {
                if (controller->GetActor()->init) {
                    controller->AppendPendingProperties(props);
                } else
                    controller->ReadProperties(props);
            }

        } break;

        case SERVER_PACKET_ACTOR_SPAWN:
        case SERVER_PACKET_ACTOR_SPAWN_AS_CHILD: {

            int parentID = serverPacketID == SERVER_PACKET_ACTOR_SPAWN_AS_CHILD
                               ? (s16)(packet.Read<PackedUInt2>().value())
                               : 0;

            auto sceneKey = (int)(packet.Read<PackedUInt4>().value());
            int roomIndex = (int)(packet.Read<PackedInt1>().value());
            int networkID = (int)(packet.Read<PackedUInt2>().value());
            s16 actorId = (s16)(packet.Read<PackedUInt2>().value());

            printf("SERVER_PACKET_SPAWN_ACTOR ACTOR: %i\n", int(actorId));
            s16 params = (s16)(packet.Read<PackedInt2>().value());

            PosRot posRot = { { packet.Read<PackedFloat4>().value(),
                                packet.Read<PackedFloat4>().value(),
                                packet.Read<PackedFloat4>().value() },
                              { (s16)(packet.Read<PackedInt2>().value()),
                                (s16)(packet.Read<PackedInt2>().value()),
                                (s16)(packet.Read<PackedInt2>().value()) } };

            PosRot home = { { packet.Read<PackedFloat4>().value(),
                              packet.Read<PackedFloat4>().value(),
                              packet.Read<PackedFloat4>().value() },
                            { (s16)(packet.Read<PackedInt2>().value()),
                              (s16)(packet.Read<PackedInt2>().value()),
                              (s16)(packet.Read<PackedInt2>().value())

                            } };

            if (gPlayState == NULL) {
                break;
            }

            auto existing = m_networkedActors.find(networkID);
            if (existing != m_networkedActors.end()) {
                SPDLOG_INFO("[ZeldaOnline] re-introduction of netID {}: replacing predecessor", networkID);
                DetachAndKill(existing->second);
            }

            auto localKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());

            int leaderID = packet.Read<PackedUInt2>().value();

            int isActorLeader = leaderID == m_myNetworkID;

            if (actorId == ACTOR_PLAYER) {
                if (sceneKey != localKey) {
                    printf("MISMATCH SCENE KEY %i:%i\n", localKey, sceneKey);
                    break;
                }

                auto appearanceLen = packet.ReadVarUInt();
                ByteStream appearance = packet.Read(appearanceLen);
                auto propertiesLen = packet.BytesLeft();
                ByteStream properties = packet.Read(propertiesLen);

                gZeldaOnlinePuppetSpawn.linkAge = 1;
                gZeldaOnlinePuppetSpawn.name[0] = '\0';
                {
                    ByteStream peek = appearance;
                    if (peek.BytesLeft() >= 6) {
                        gZeldaOnlinePuppetSpawn.linkAge =
                            (u8)(peek.Read<PackedUInt1>().value());
                        peek.Read<PackedUInt1>();
                        peek.Read<PackedUInt1>();
                        peek.Read<PackedUInt1>();
                        peek.Read<PackedUInt1>();
                        unsigned int nameLen = peek.Read<PackedUInt1>().value();
                        if (nameLen > 16 || peek.BytesLeft() < nameLen)
                            nameLen = 0;
                        std::string name = peek.ReadString(nameLen);
                        std::memcpy(gZeldaOnlinePuppetSpawn.name, name.c_str(), nameLen);
                        gZeldaOnlinePuppetSpawn.name[nameLen] = '\0';
                    }
                }

                m_spawningPuppetPlayer = true;

                Actor* actor =
                    Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, ACTOR_PLAYER, posRot.pos.x, posRot.pos.y,
                                      posRot.pos.z, posRot.rot.x, posRot.rot.y, posRot.rot.z, home.pos.x, home.pos.y,
                                      home.pos.z, home.rot.x, home.rot.y, home.rot.z, 0, 0);
                m_spawningPuppetPlayer = false;

                if (actor == NULL || actor->update == NULL) {
                    break;
                }

                RelinkPuppetBehindPlayer(actor);

                actor->world = posRot;
                Math_Vec3f_Copy(&actor->prevPos, &actor->world.pos);
                actor->room = -1;

                auto controller = new PlayerPuppetController(actor, networkID, sceneKey, appearance);
                m_networkedActors[networkID] = controller;
                controller->ReadProperties(properties);
                break;
            }

            if (sceneKey != localKey ||
                (roomIndex != -1 && roomIndex != gPlayState->roomCtx.curRoom.num)) {
                break;
            }

            const bool movedFromHome =
                posRot.pos.x != home.pos.x || posRot.pos.y != home.pos.y || posRot.pos.z != home.pos.z;
            Actor* actor = nullptr;

            if (parentID == 0)
                actor = Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, actorId, home.pos.x, home.pos.y,
                                          home.pos.z, home.rot.x, home.rot.y, home.rot.z, home.pos.x, home.pos.y,
                                          home.pos.z, home.rot.x, home.rot.y, home.rot.z, params, 1);
            else {
                auto it = m_networkedActors.find(parentID);
                if (it != m_networkedActors.end()) {
                    auto parentActor = it->second->GetActor();

                    if (parentActor != nullptr) {
                        actor = Actor_SpawnAsChildDirect(&gPlayState->actorCtx, parentActor, gPlayState, actorId,
                                                         home.pos.x, home.pos.y, home.pos.z, home.rot.x, home.rot.y,
                                                         home.rot.z, home.pos.x, home.pos.y, home.pos.z, home.rot.x,
                                                         home.rot.y, home.rot.z, params, 1);
                    } else {
                        SPDLOG_WARN("[ZeldaOnline] parent {} missing for child spawn, degrading", parentID);
                        actor =
                            Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, actorId, home.pos.x, home.pos.y,
                                              home.pos.z, home.rot.x, home.rot.y, home.rot.z, home.pos.x, home.pos.y,
                                              home.pos.z, home.rot.x, home.rot.y, home.rot.z, params, 1);
                    }
                }
            }
            if (actor == NULL || actor->update == NULL) {
                break;
            }
            actor->room = roomIndex;

            if (actor->init == nullptr) {

                if (movedFromHome)
                    actor->world = posRot;
                Math_Vec3f_Copy(&actor->prevPos, &actor->world.pos);
            }

            AbstractActorController* controller = ActorControllerFactory::Instance().Create(
                actorId, actor, networkID, sceneKey, roomIndex, isActorLeader);

            if (controller == nullptr) {
                SPDLOG_ERROR("[ZeldaOnline] no controller registered for actor {:#06x}", actor->id);
                break;
            }
            controller->SetSpawnPosRot(posRot);

            auto isLocked = packet.Read<PackedUInt1>().value();
            auto isCreator = packet.Read<PackedUInt1>().value();

            controller->SetCreator(isCreator);
            ByteStream statePacket = packet.Read(packet.BytesLeft());

            if (actor->init)
                controller->AppendPendingProperties(statePacket);
            else {
                controller->ActorInit(gPlayState);
                controller->ReadProperties(statePacket);
            }

            controller->SetLocked(isLocked);

            m_networkedActors[networkID] = controller;

        } break;

        case SERVER_PACKET_UPDATE_APPEARANCE: {
            if (packet.BytesLeft() < 2)
                break;
            int networkID = (int)(packet.Read<PackedUInt2>().value());
            auto it = m_networkedActors.find(networkID);
            if (it == m_networkedActors.end())
                break;
            if (!it->second->IsPlayer())
                break;
            static_cast<PlayerPuppetController*>(it->second)->ApplyAppearance(packet.Read(packet.BytesLeft()));
        } break;

        case SERVER_PACKET_AUTH_RESPONSE: {
            if (packet.BytesLeft() < 2)
                break;
            m_myNetworkID = (int)(packet.Read<PackedUInt2>().value());

            UpdateAppearance();
            printf("MY NETWORK ID SET TO %i\n", int(m_myNetworkID));
        } break;

        case SERVER_PACKET_DESTROY_ACTOR: {
            if (packet.BytesLeft() < 2)
                break;
            int networkID = (int)(packet.Read<PackedUInt2>().value());

            auto it = m_networkedActors.find(networkID);
            if (it == m_networkedActors.end())
                break;

            AbstractActorController* controller = it->second;

            if (controller->IsRunningLocally()) {
                controller->Detach();
                RemoveNetworkedActor(controller->NetworkID(), controller);
                break;
            }

            Player* player = (gPlayState != NULL) ? GET_PLAYER(gPlayState) : NULL;
            if (player != NULL && player->talkActor == controller->GetActor()) {
                //Removed this. Only caused crashes and doesnt seem to be needed
                //player->talkActor = nullptr;
            }

            controller->OnServerDestroy();
            DetachAndKill(controller);
        } break;

        case SERVER_PACKET_ACTOR_TRIGGER: {
            if (packet.BytesLeft() < 3)
                break;

            int networkID = (int)(packet.Read<PackedUInt2>().value());
            bool fromPuppet = packet.Read<PackedUInt1>().value() == 1;

            auto it = m_networkedActors.find(networkID);
            if (it == m_networkedActors.end())
                break;

            if (it->second == nullptr)
                break;

            if (fromPuppet && !it->second->IsLeader())
                break;

            else if (!fromPuppet && it->second->IsLeader())
                break;

            unsigned int nameLen = packet.Read<PackedUInt1>().value();
            if (packet.BytesLeft() < nameLen)
                break;
            std::string name = packet.ReadString(nameLen);

            it->second->OnTrigger(name, packet);
        } break;

        case SERVER_PACKET_SET_ACTOR_LEADER: {
            if (packet.BytesLeft() < 4)
                break;
            int networkID = (int)(packet.Read<PackedUInt2>().value());
            int ownerID = (int)(packet.Read<PackedUInt2>().value());
            auto it = m_networkedActors.find(networkID);
            if (it == m_networkedActors.end()) {
                printf("SERVER_PACKET_SET_ACTOR_LEADER: Invalid network ID: %i\n", networkID);
                if (ownerID == m_myNetworkID) {
                    SendDeclineActorLeader(networkID);
                }

                break;
            }

            auto controller = it->second;

            if (ownerID == m_myNetworkID && !controller->CanAcceptLeadership()) {
                SendDeclineActorLeader(networkID);
                break;
            }

            if (!it->second->IsLocked() || ownerID == m_myNetworkID)
                it->second->SetLeader(ownerID != 0 && ownerID == m_myNetworkID);
        } break;

        case SERVER_PACKET_SCENE_FLAG: {
            if (packet.BytesLeft() < 2)
                break;

            u8 flagType = packet.Read<PackedUInt1>().value();
            u16 flag = packet.Read<PackedUInt2>().value();
            if (gPlayState == NULL)
                break;

            u8 setFlag = 1;
            if (packet.BytesLeft() >= 1)
                setFlag = packet.Read<PackedUInt1>().value();

            printf("SCENE FLAG PACKET: %i:%i\n", int(flagType), int(setFlag));

            if (setFlag) {
                if (flagType == FLAG_SCENE_SWITCH || flagType == FLAG_SCENE_CLEAR) {
                    GameInteractionEffect::SetSceneFlag effect;
                    effect.parameters[0] = gPlayState->sceneNum;
                    effect.parameters[1] = flagType;
                    effect.parameters[2] = flag;
                    effect.Apply();

                    if (flagType == FLAG_SCENE_SWITCH)
                        NudgeFloorSwitches(flag, setFlag);
                }

                else if (flagType == FLAG_INF_TABLE || flagType == FLAG_EVENT_CHECK_INF) {
                    GameInteractionEffect::SetFlag effect;
                    effect.parameters[0] = flagType;
                    effect.parameters[1] = flag;
                    effect.Apply();
                }
            } else {
                if (flagType == FLAG_SCENE_SWITCH || flagType == FLAG_SCENE_CLEAR) {
                    GameInteractionEffect::UnsetSceneFlag effect;
                    effect.parameters[0] = gPlayState->sceneNum;
                    effect.parameters[1] = flagType;
                    effect.parameters[2] = flag;
                    effect.Apply();
                }

                else if (flagType == FLAG_INF_TABLE || flagType == FLAG_EVENT_CHECK_INF) {
                    GameInteractionEffect::UnsetFlag effect;
                    effect.parameters[0] = flagType;
                    effect.parameters[1] = flag;
                    effect.Apply();
                }
            }

        } break;

        case SERVER_PACKET_WORLD_TIME: {
            if (packet.BytesLeft() < 2)
                break;

            m_serverDayTime = (u16)(packet.Read<PackedUInt2>().value());

            s16 drift = (s16)(m_serverDayTime - gSaveContext.dayTime);
            if (!m_hasWorldTime || drift > 600 || drift < -600) {
                gSaveContext.skyboxTime = gSaveContext.dayTime = m_serverDayTime;
            }
            m_hasWorldTime = true;

            if (packet.BytesLeft() >= 1)
                packet.Read<PackedUInt1>();
            if (packet.BytesLeft() >= 2)
                m_serverTimeRate = (u16)(packet.Read<PackedUInt2>().value());

        } break;

        case SERVER_PACKET_SCENE_TRIGGER: {
            if (packet.BytesLeft() < 6 || gPlayState == NULL) {
                break;
            }

            auto sceneKey = packet.Read<PackedUInt4>().value();
            auto localKey = MakeSceneKey(gPlayState->sceneNum, !LINK_IS_ADULT ? 0 : 1, 0);
            if (localKey != sceneKey) {
                break;
            }

            auto fromID = packet.Read<PackedUInt2>().value();

            auto fromPlayer = fromID != 0 ? GetNetworkController(fromID) : nullptr;

            u8 nameLen = packet.Read<PackedUInt1>().value();
            if (packet.BytesLeft() < nameLen) {
                break;
            }
            std::string name = packet.ReadString(nameLen);
            OnSceneTrigger(fromPlayer, name, packet);
        } break;

        case SERVER_PACKET_ROOM_TEMP_FLAGS: {
            if (packet.BytesLeft() < 9 || gPlayState == NULL) {
                break;
            }
            auto sceneKey = packet.Read<PackedUInt4>().value();
            s8 roomIndex = (packet.Read<PackedInt1>().value());
            u32 mask = packet.Read<PackedUInt4>().value();

            auto localKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());
            if (localKey != sceneKey || roomIndex != (gPlayState->roomCtx.curRoom.num)) {
                break;
            }
            gPlayState->actorCtx.flags.tempSwch = m_currentRoomTempMask = mask;
        } break;

        case SERVER_PACKET_SET_ACTOR_LOCKED: {
            if (packet.BytesLeft() < 3)
                break;
            int networkID = (int)(packet.Read<PackedUInt2>().value());
            bool locked = packet.Read<PackedUInt1>().value() != 0;
            auto it = m_networkedActors.find(networkID);
            if (it != m_networkedActors.end())
                it->second->SetLocked(locked);
        } break;

        case SERVER_PACKET_ASSIGN_ACTOR_ID: {
            auto sceneKey = packet.Read<PackedUInt4>().value();
            s8 roomIndex = (packet.Read<PackedInt1>().value());
            unsigned int localID = packet.Read<PackedUInt4>().value();
            int networkID = (int)(packet.Read<PackedUInt2>().value());

            if (gPlayState == NULL) {
                break;
            }
            if (localID == 0)
                break;

            Actor* match = nullptr;
            for (int cat = 0; cat < ACTORCAT_MAX && match == nullptr; cat++) {
                for (Actor* a = gPlayState->actorCtx.actorLists[cat].head; a != NULL; a = a->next) {
                    if (a->zoLocalId == localID) {
                        match = a;
                        break;
                    }
                }
            }

            if (match == nullptr || match->update == nullptr) {
                SPDLOG_DEBUG("[ZeldaOnline] ASSIGN_ACTOR_ID: no local actor for localID {}", localID);
                break;
            }

            if (networkID == 0) {
                DetachAndKill(static_cast<AbstractActorController*>(match->zoController));
                break;
            }

            if (match->zoController == nullptr) {
                AbstractActorController* controller = ActorControllerFactory::Instance().Create(
                    match->id, match, networkID, (int)(sceneKey), gPlayState->roomCtx.curRoom.num, true);
                if (controller) {
                    controller->SetCreator(true);
                    m_networkedActors[networkID] = controller;
                }
                break;
            }
            AbstractActorController* controller = static_cast<AbstractActorController*>(match->zoController);
            controller->SetNetworkID(networkID);

            m_networkedActors[networkID] = controller;

        } break;

        case SERVER_PACKET_ACTOR_SOUND: {
            int networkID = (int)(packet.Read<PackedUInt2>().value());
            u16 sfxId = (u16)(packet.Read<PackedUInt2>().value());

            if (gPlayState == nullptr) {
                break;
            }
            AbstractActorController* ctl = GetNetworkController(networkID);

            if (ctl && ctl->GetActor()) {
                if (ctl->GetActor()->id != ACTOR_PLAYER && !ctl->ShouldSuppressSounds()) {
                    break;
                }

                Audio_PlaySoundGeneral(sfxId, &ctl->GetActor()->projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                       &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            }
            break;
        }

        //Anchor
        case SERVER_PACKET_OCARINA_SFX: {
            int networkID = packet.Read<PackedUInt2>().value();
            u8 note = packet.Read<PackedUInt1>().value();
            f32 modulator = packet.Read<PackedFloat4>().value();
            s8 bend = (s8)(packet.Read<PackedInt1>().value());

            AbstractActorController* ctl = GetNetworkController(networkID);
            if (ctl != nullptr && ctl->IsPlayer() && ctl->GetActor()) {
                PlayerPuppetController* playerController = static_cast<PlayerPuppetController*>(ctl);
                auto puppetState = playerController->PuppetState();
                puppetState->ocarinaModulator = modulator;
                puppetState->ocarinaBend = bend;

                if (note != 0xFF && puppetState->ocarinaNote != note) {
                    Audio_QueueCmdS8(0x6 << 24 | SEQ_PLAYER_SFX << 16 | 0xD07, puppetState->ocarinaBend - 1);
                    Audio_QueueCmdS8(0x6 << 24 | SEQ_PLAYER_SFX << 16 | 0xD05, note);

                    Audio_PlaySoundGeneral(NA_SE_OC_OCARINA, &ctl->GetActor()->projectedPos, 4,
                                           &puppetState->ocarinaModulator, &D_80130F28, &gSfxDefaultReverb);
                } else if (puppetState->ocarinaNote != 0xFF && note == 0xFF) {
                    Audio_StopSfxById(NA_SE_OC_OCARINA);
                }
                puppetState->ocarinaNote = note;
            }
            break;
        }

        default:
            SPDLOG_DEBUG("[ZeldaOnline] unhandled server packet id {}", serverPacketID);
            break;
    }
}

bool ZeldaOnlineClient::WritePacket(const ByteStream& data) {
    if (!isConnected) {
        return false;
    }
    if (data.Length() > MAX_FRAME_PAYLOAD) {
        SPDLOG_ERROR("[FramedNetwork] Packet payload too large: {} bytes", data.Length());
        return false;
    }

    outgoingBuffer.Reserve(FRAME_HEADER_SIZE + data.Length());
    outgoingBuffer << PackedUInt2(data.Length());
    outgoingBuffer.Write(data.Text(), data.Length());

    m_bytesSent += FRAME_HEADER_SIZE + data.Length();
    return true;
}


void ZeldaOnlineClient::SendSceneTrigger(const std::string& name, const ByteStream& payload) {

    auto sceneKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());
    ByteStream packet = newPacket(CLIENT_PACKET_SCENE_TRIGGER);
    packet << PackedUInt4(sceneKey);
    packet << PackedUInt1(0);
    packet << PackedUInt1((unsigned int)(name.length())) << name << payload;
    WritePacket(packet);
}

void ZeldaOnlineClient::SendRoomTrigger(const std::string& name, const ByteStream& payload) {
    auto sceneKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());
    ByteStream packet = newPacket(CLIENT_PACKET_SCENE_TRIGGER);
    packet << PackedUInt4(sceneKey);
    packet << PackedUInt1(1);
    packet << PackedUInt1((unsigned int)(name.length())) << name << payload;
    WritePacket(packet);
}

void ZeldaOnlineClient::SendHitTrigger(ActorID actorID, f32 posX, f32 posY, f32 posZ) {
    ByteStream payload;
    payload << PackedUInt2(actorID) << PackedInt2(int(posX)) << PackedInt2(int(posY)) << PackedInt2(int(posZ));
    SendSceneTrigger("hit", payload);
}

void ZeldaOnlineClient::ProcessOutgoingPackets() {
    if (outgoingBuffer.Length() == 0) {
        return;
    }
    ByteStream toSend = std::move(outgoingBuffer);
    outgoingBuffer.Clear();

    SendDataToRemote(toSend.Text(), (int)(toSend.Length()));
}

void ZeldaOnlineClient::OnIncomingData(const char* payload, int length) {
    recvBuffer.Write(payload, (unsigned int)(length));

    m_bytesReceived += static_cast<uint64_t>(length);

    ByteStream packet;
    while (NextPacket(packet)) {
        OnIncomingPacket(packet);
    }

    recvBuffer.Compact();
}

bool ZeldaOnlineClient::NextPacket(ByteStream& out) {
    if (recvBuffer.BytesLeft() < FRAME_HEADER_SIZE) {
        return false;
    }

    PackedUInt2 lenField;
    memcpy(&lenField, recvBuffer.Text() + recvBuffer.TellRead(), FRAME_HEADER_SIZE);
    unsigned int payloadLen = lenField.value();

    if (recvBuffer.BytesLeft() < FRAME_HEADER_SIZE + payloadLen) {
        return false;
    }

    recvBuffer.SeekRead(FRAME_HEADER_SIZE, ByteStream::ORIGIN_CUR);
    out = recvBuffer.Read(payloadLen);
    return true;
}

static Actor* FindLocalHorse(Player* player) {
    if ((player->stateFlags1 & PLAYER_STATE1_ON_HORSE) && player->rideActor != NULL &&
        player->rideActor->id == ACTOR_EN_HORSE) {
        return player->rideActor;
    }

    if (0) {
        if (!Flags_GetEventChkInf(EVENTCHKINF_EPONA_OBTAINED)) {
            return NULL;
        }
    }
    for (Actor* a = gPlayState->actorCtx.actorLists[ACTORCAT_BG].head; a != NULL; a = a->next) {
        if (a->id == ACTOR_EN_HORSE && a->update != HorsePuppet_Update && ((EnHorse*)a)->type == HORSE_EPONA) {
            return a;
        }
    }
    return NULL;
}

void ZeldaOnlineClient::SendPacket_PlayerUpdate() {
    if (!isConnected || gPlayState == NULL) {
        return;
    }

    Player* player = GET_PLAYER(gPlayState);
    if (player == NULL) {
        return;
    }

    if (gPlayState->roomCtx.curRoom.num < 0) {
        return;
    }

    auto horseActor = FindLocalHorse(player);

    ByteStream frame;
    PlayerPuppetController::BuildLocalPlayerProperties(player, horseActor, frame);

    unsigned int changedCount = 0;
    ByteStream delta =
        AbstractActorController::DiffProperties(m_lastPlayerProps, frame, m_sendFullPlayerProps, &changedCount);
    m_lastPlayerProps = frame;

    if (m_sendFullPlayerProps)
        DbgPrintf("Sent playe props full!\n");
    m_sendFullPlayerProps = false;

    if (delta.Length())
        WritePacket(newPacket(CLIENT_PACKET_UPDATE_PLAYER) << delta);
}

void ZeldaOnlineClient::UpdateAppearance() {
    if (!isConnected || m_myNetworkID == 0) {
        return;
    }

    ByteStream blob;
    blob << PackedUInt1(LINK_IS_ADULT ? 0u : 1u);
    blob << PackedUInt1((u8)(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC)));
    blob << PackedUInt1((u8)(CUR_EQUIP_VALUE(EQUIP_TYPE_BOOTS)));
    blob << PackedUInt1((u8)(CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD)));
    blob << PackedUInt1(gSaveContext.equips.buttonItems[0]);

    std::string name = CVarGetString("gZeldaOnline.PlayerName", "Player");
    blob << PackedUInt1((int)name.length()) << name;

    if (blob.compare(m_lastAppearanceBlob) == 0)
        return;

    m_lastAppearanceBlob = blob;

    WritePacket(newPacket(CLIENT_PACKET_UPDATE_APPEARANCE) << blob);
}

void ZeldaOnlineClient::RegisterNetworkingHook(bool enabled) {
    COND_HOOK(OnGameFrameUpdate, enabled, [&] { ZNetworking::Poll(); });
}

void ZeldaOnlineClient::RegisterHooks(bool enabled) {
    if (enabled == m_hooksEnabled)
        return;

    m_hooksEnabled = enabled;
    COND_HOOK(OnPlayDrawEnd, enabled,[&]() { SendPacket_PlayerUpdate(); });

    COND_HOOK(OnSceneInit, enabled,[&](int16_t) {
        m_sceneLoadedAtNight = gSaveContext.nightFlag != 0;
        UpdateAppearance();
    });

    COND_HOOK(OnTransitionEnd, enabled,[&](int16_t) {
        if (m_boundaryCueOnLoad != 0) {
            Sfx_PlaySfxCentered2(m_boundaryCueOnLoad == 1 ? NA_SE_EV_CHICKEN_CRY_M : NA_SE_EV_DOG_CRY_EVENING);
            m_boundaryCueOnLoad = 0;
        }
    });

    COND_HOOK(OnGameFrameUpdate, enabled,[&] { OnGameFrameUpdate(); });

    COND_HOOK(OnActorKill, enabled,[&](void* refActor) {
        Actor* actor = static_cast<Actor*>(refActor);

        OnActorKill(actor);
    });

    COND_HOOK(OnActorInit, enabled,[&](void* refActor) {
        Actor* actor = static_cast<Actor*>(refActor);

        if (actor->zoController) {
            auto controller = static_cast<AbstractActorController*>(actor->zoController);
            if (controller->ShouldReinstateInit()) {
                actor->init = controller->DispatchInit;
            }
        }
    });

    COND_ID_HOOK(ShouldActorInit, ACTOR_PLAYER, enabled,[&](void* actorRef, bool*) {
        if (!m_spawningPuppetPlayer) {
            return;
        }

        Actor* actor = (Actor*)actorRef;

        InitPuppetPlayer(actor);
    });

    COND_ID_HOOK(ShouldActorInit, ACTOR_EN_HORSE, enabled,[&](void* actorRef, bool*) {
        Actor* actor = (Actor*)actorRef;
        if (!(actor->params & ZO_HORSE_PUPPET)) {
            return;
        }
        actor->params &= ~ZO_HORSE_PUPPET;
        actor->init = HorsePuppet_Init;
        actor->update = HorsePuppet_Update;
        actor->destroy = HorsePuppet_Destroy;
    });

    COND_ID_HOOK(ShouldActorInit, ACTOR_EN_RU1, enabled,[&](void* actorRef, bool*) {
        Actor* actor = (Actor*)actorRef;

        AbstractActorController::InstallCustomInit(actor, RutoController::EnRu1_Init);
    });

    COND_ID_HOOK(ShouldActorInit, ACTOR_EN_FLOORMAS, enabled,[&](void* actorRef, bool*) {
        Actor* actor = (Actor*)actorRef;

        AbstractActorController::InstallCustomInit(actor, FloormasterController::EnFloormas_Init);
    });

    COND_HOOK(OnFlagSet, enabled,[&](s16 flagType, s16 flag) {
        if (flagType == FLAG_INF_TABLE || flagType == FLAG_EVENT_CHECK_INF) {
            printf("SENDING FLAG: %i:%i\n", flagType, int(flag));
            ByteStream packet = newPacket(CLIENT_PACKET_SCENE_FLAG);
            packet << PackedUInt1((u8)(flagType));
            packet << PackedUInt2(flag);
            packet << PackedUInt1(1u);
            WritePacket(packet);
        }
    });

    COND_HOOK(OnSceneFlagSet, enabled,[&](s16 sceneNum, s16 flagType, s16 flag) {

        if (gPlayState == NULL || sceneNum != gPlayState->sceneNum) {
            return;
        }
        if (flagType != FLAG_SCENE_SWITCH && flagType != FLAG_SCENE_CLEAR && flagType != FLAG_INF_TABLE) {
            return;
        }
        if (sceneNum == SCENE_WATER_TEMPLE && flagType == FLAG_SCENE_SWITCH &&
            (flag == 0x1C || flag == 0x1D || flag == 0x1E))
            return;
        if (sceneNum == SCENE_FOREST_TEMPLE && flagType == FLAG_SCENE_SWITCH && flag == 0x1B)
            return;
        if (sceneNum == SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR && flagType == FLAG_SCENE_SWITCH && flag == 0x36)
            return;

        if (m_lockedDoorFlags.contains(flag))
            return;

        ByteStream packet = newPacket(CLIENT_PACKET_SCENE_FLAG);
        packet << PackedUInt1((u8)(flagType));
        packet << PackedUInt2(flag);
        packet << PackedUInt1(1u);
        WritePacket(packet);
    });

    COND_HOOK(OnSceneFlagUnset, enabled,[&](s16 sceneNum, s16 flagType, s16 flag) {
        if (gPlayState == NULL || sceneNum != gPlayState->sceneNum)
            return;
        if (flagType != FLAG_SCENE_SWITCH && flagType != FLAG_SCENE_CLEAR)
            return;

        ByteStream packet = newPacket(CLIENT_PACKET_SCENE_FLAG);
        packet << PackedUInt1((u8)(flagType));
        packet << PackedUInt2(flag);
        packet << PackedUInt1(0u);
        WritePacket(packet);
    });

    {
        static HOOK_ID hookId = 0;
        GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnVanillaBehavior>(hookId);

        if (enabled) {
            GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnVanillaBehavior>(
                VB_ADVANCE_DAYTIME, [this](GIVanillaBehavior, bool* should, va_list) {
                    *should = false;
                });
        }
    }

    COND_HOOK(OnTransitionRoom, enabled,[&]() {
        gPlayState->actorCtx.flags.tempSwch |= m_currentRoomTempMask;

        printf("ON TRANSITION SCENE: Current Room: %i\nLast Room: %i\nCurrent Scene: %i\nLast Scene: %i\n",
               gPlayState->roomCtx.curRoom.num, m_lastRoom, gPlayState->sceneNum, m_lastScene);

        RequestRoomSceneChange(true);
    })

    COND_HOOK(OnSceneInit, enabled,[&](int) {
        printf("ON TRANSITION SCENE: Current Room: %i\nLast Room: %i\nCurrent Scene: %i\nLast Scene: %i\n",
               gPlayState->roomCtx.curRoom.num, m_lastRoom, gPlayState->sceneNum, m_lastScene);
        RequestRoomSceneChange(false);
    });

    COND_ID_HOOK(ShouldActorInit, ACTOR_EN_DOOR, enabled,[&](void* refActor, bool*) {
        Actor* door = static_cast<Actor*>(refActor);
        s32 doorType = (door->params >> 7) & 7;
        if (doorType == DOOR_LOCKED) {
            m_lockedDoorFlags.insert(door->params & 0x3F);
        }
    });

    COND_HOOK(OnSceneInit, enabled,[&](int16_t sceneNum) { m_lockedDoorFlags.clear(); });

    COND_HOOK(OnPlayerSfx, enabled,[&](u16 sfxId) {
        if (gPlayState == nullptr) {
            return;
        }
        TransmitActorSound(m_myNetworkID, sfxId);
    });

    COND_HOOK(OnOcarinaNote, enabled,[&](u8 note, f32 modulator, s8 bend) {
        ByteStream p = newPacket(CLIENT_PACKET_OCARINA_SFX);
        p << PackedUInt2(m_myNetworkID);
        p << PackedUInt1(note);
        p << PackedFloat4(modulator);
        p << PackedInt1(bend);
        WritePacket(p);
    });

    COND_HOOK(OnMinimapDrawCompassIcons, enabled,[&]() {

        struct CompassIcon {
            Vec3f pos;
            Vec3s rot;
            f32 scale;
            Color_RGB8 color;
        };
        std::vector<CompassIcon> compassIcons;

        bool isInDungeon = gPlayState->sceneNum == SCENE_DEKU_TREE || gPlayState->sceneNum == SCENE_DODONGOS_CAVERN ||
                           gPlayState->sceneNum == SCENE_JABU_JABU || gPlayState->sceneNum == SCENE_FOREST_TEMPLE ||
                           gPlayState->sceneNum == SCENE_FIRE_TEMPLE || gPlayState->sceneNum == SCENE_WATER_TEMPLE ||
                           gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE || gPlayState->sceneNum == SCENE_SHADOW_TEMPLE ||
                           gPlayState->sceneNum == SCENE_BOTTOM_OF_THE_WELL || gPlayState->sceneNum == SCENE_ICE_CAVERN;

        s8 displayedRoomNum =
            gPlayState->roomCtx.prevRoom.num >= 0 ? gPlayState->roomCtx.prevRoom.num : gPlayState->roomCtx.curRoom.num;

        for (auto& [netId, controller] : m_networkedActors) {
            if (controller == nullptr || !controller->IsPlayer())
                continue;

            PlayerPuppetController* playerController = static_cast<PlayerPuppetController*>(controller);
            if (isInDungeon && playerController->RealRoomIndex() != displayedRoomNum)
                continue;

            Actor* actor = playerController->GetActor();
            compassIcons.push_back(CompassIcon{ actor->world.pos, actor->shape.rot, 0.3f,
                                                playerController->GetTunicColour() });
        }

        Player* player = GET_PLAYER(gPlayState);
        compassIcons.push_back(CompassIcon{ player->actor.world.pos, player->actor.shape.rot, 0.4f,
                                            CVarGetColor24(CVAR_COSMETIC("HUD.Minimap.Color"), { 100, 255, 100 }) });

        s16 leftMinimapMargin = CVarGetInteger(CVAR_COSMETIC("HUD.Margin.L"), 0);
        s16 rightMinimapMargin = CVarGetInteger(CVAR_COSMETIC("HUD.Margin.R"), 0);
        s16 bottomMinimapMargin = CVarGetInteger(CVAR_COSMETIC("HUD.Margin.B"), 0);

        s16 xMarginsMinimap;
        s16 yMarginsMinimap;
        if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.UseMargins"), 0) != 0) {
            if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosType"), 0) == ORIGINAL_LOCATION) {
                xMarginsMinimap = rightMinimapMargin;
            }
            yMarginsMinimap = bottomMinimapMargin;
        } else {
            xMarginsMinimap = 0;
            yMarginsMinimap = 0;
        }

        s16 mapWidth = isInDungeon ? R_DGN_MINIMAP_X : R_OW_MINIMAP_X;
        s16 mapStartPosX = isInDungeon ? 96 : gMapData->owMinimapWidth[R_MAP_INDEX];

        {
            GraphicsContext* __gfxCtx = gPlayState->state.gfxCtx;
            Gfx* dispRefs[4];
            (void)__gfxCtx;
            Graph_OpenDisps(dispRefs, gPlayState->state.gfxCtx, __FILE__, __LINE__);
            Gfx_SetupDL_42Overlay(gPlayState->state.gfxCtx);

            for (auto& compassIcon : compassIcons) {
                gSPMatrix(OVERLAY_DISP++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gDPSetCombineLERP(OVERLAY_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                                  PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
                gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, 255);
                gDPSetCombineMode(OVERLAY_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);

                s16 mirrorOffset =
                    ((mapWidth / 2) - ((R_COMPASS_OFFSET_X / 10) - (mapStartPosX - SCREEN_WIDTH / 2))) * 2 * 10;

                s16 tempX = (s16)compassIcon.pos.x;
                s16 tempZ = (s16)compassIcon.pos.z;
                tempX /= R_COMPASS_SCALE_X * (CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) ? -1 : 1);
                tempZ /= R_COMPASS_SCALE_Y;

                s16 tempXOffset =
                    R_COMPASS_OFFSET_X + (CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) ? mirrorOffset : 0);

                if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosType"), 0) != ORIGINAL_LOCATION) {
                    if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosType"), 0) == ANCHOR_LEFT) {
                        if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.UseMargins"), 0) != 0) {
                            xMarginsMinimap = leftMinimapMargin;
                        }
                        Matrix_Translate(
                            OTRGetDimensionFromLeftEdge((tempXOffset + (xMarginsMinimap * 10) + tempX +
                                                         (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosX"), 0) * 10)) /
                                                        10.0f),
                            (R_COMPASS_OFFSET_Y + ((yMarginsMinimap * 10) * -1) - tempZ +
                             ((CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosY"), 0) * 10) * -1)) /
                                10.0f,
                            0.0f, MTXMODE_NEW);
                    } else if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosType"), 0) == ANCHOR_RIGHT) {
                        if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.UseMargins"), 0) != 0) {
                            xMarginsMinimap = rightMinimapMargin;
                        }
                        Matrix_Translate(
                            OTRGetDimensionFromRightEdge((tempXOffset + (xMarginsMinimap * 10) + tempX +
                                                          (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosX"), 0) * 10)) /
                                                         10.0f),
                            (R_COMPASS_OFFSET_Y + ((yMarginsMinimap * 10) * -1) - tempZ +
                             ((CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosY"), 0) * 10) * -1)) /
                                10.0f,
                            0.0f, MTXMODE_NEW);
                    } else if (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosType"), 0) == ANCHOR_NONE) {
                        Matrix_Translate(
                            (tempXOffset + tempX + (CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosX"), 0) * 10) / 10.0f),
                            (R_COMPASS_OFFSET_Y + ((yMarginsMinimap * 10) * -1) - tempZ +
                             ((CVarGetInteger(CVAR_COSMETIC("HUD.Minimap.PosY"), 0) * 10) * -1)) /
                                10.0f,
                            0.0f, MTXMODE_NEW);
                    }
                } else {
                    Matrix_Translate(
                        OTRGetDimensionFromRightEdge((tempXOffset + (xMarginsMinimap * 10) + tempX) / 10.0f),
                        (R_COMPASS_OFFSET_Y + ((yMarginsMinimap * 10) * -1) - tempZ) / 10.0f, 0.0f, MTXMODE_NEW);
                }

                Matrix_Scale(compassIcon.scale, compassIcon.scale, compassIcon.scale, MTXMODE_APPLY);
                Matrix_RotateX(-1.6f, MTXMODE_APPLY);
                s16 rotation = ((0x7FFF - compassIcon.rot.y) / 0x400) *
                               (CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) ? -1 : 1);
                Matrix_RotateY(rotation / 10.0f, MTXMODE_APPLY);
                gSPMatrix(OVERLAY_DISP++, MATRIX_NEWMTX(gPlayState->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

                gDPSetPrimColor(OVERLAY_DISP++, 0, 0xFF, compassIcon.color.r, compassIcon.color.g, compassIcon.color.b,
                                255);
                gSPDisplayList(OVERLAY_DISP++, (Gfx*)gCompassArrowDL);
            }

            Graph_CloseDisps(dispRefs, gPlayState->state.gfxCtx, __FILE__, __LINE__);
        }
    });
    RegisterActorHooks(enabled);
}

void ZeldaOnlineClient::OnActorKill(Actor* actor) {

    if (actor->id == ACTOR_EN_RU1) {
        printf("BREAK\n");
    }
    if (actor->zoController == nullptr || gZeldaOnlineEngineCleanup) {
        return;
    }
    auto* controller = static_cast<AbstractActorController*>(actor->zoController);
    if (!controller->IsLeader()) {
        return;
    }

    SendActorDied(controller->NetworkID());
}

void ZeldaOnlineClient::ReportBandwidth() {
    time_t now = time(nullptr);
    if (now == m_lastReportTime)
        return;
    m_lastReportTime = now;

    uint64_t sent = m_bytesSent;
    uint64_t received = m_bytesReceived;
    m_bytesSent = 0;
    m_bytesReceived = 0;

    printf("[ZeldaOnline] bandwidth: up %.2f KB/s (%llu B)  down %.2f KB/s (%llu B)\n", sent / 1024.0, static_cast<unsigned long long>(sent), received / 1024.0, static_cast<unsigned long long>(received));
}

void ZeldaOnlineClient::OnGameFrameUpdate() {

     ReportBandwidth();
    if (m_hasWorldTime && gPlayState != NULL) {
        u16 rate = m_serverTimeRate / 20;
        u16 next = (u16)(gSaveContext.dayTime + rate);
        if (gSaveContext.dayTime > (u16)(0xFFFF - rate)) {
            gSaveContext.dayTime = 0xFFFF;
        } else if (IsNightTime(gSaveContext.dayTime) != IsNightTime(next)) {
        } else {
            gSaveContext.dayTime = next;
        }
        gSaveContext.skyboxTime = gSaveContext.dayTime;

        if (m_sceneLoadedAtNight != IsNightTime(m_serverDayTime) && IsDayNightReloadScene(gPlayState->sceneNum)) {
            m_reloadPending = true;
        }

        if (m_reloadPending && IsDayNightReloadScene(gPlayState->sceneNum) && CanReloadSceneNow(gPlayState)) {
            gSaveContext.skyboxTime = gSaveContext.dayTime = m_serverDayTime;
            m_boundaryCueOnLoad = IsNightTime(m_serverDayTime) ? 2 : 1;
            m_reloadPending = false;
            ReloadSceneInPlace(gPlayState);
        }
    }

    u8 paused = (gPlayState != NULL && (gPlayState->pauseCtx.state != 0 || gPlayState->msgCtx.msgMode != MSGMODE_NONE)) ? 1 : 0;

    if (paused != m_wasPaused) {

        WritePacket(newPacket(CLIENT_PACKET_SET_PAUSE_STATE) << PackedUInt1(paused));
        m_wasPaused = paused;
    }

    if (paused) {
        ByteStream release;
        int count = 0;

        for (auto& entry : m_networkedActors) {
            AbstractActorController* c = entry.second;
            if (c->IsLeader() && c->CanRelinquishLeadership()) {
                release << PackedUInt2((unsigned int)(c->NetworkID()));
                count++;
                c->SetLeader(false);
            }
        }

        if (count > 0) {
            ByteStream p = newPacket(CLIENT_PACKET_RELINQUISH_ACTOR_LEADER);
            p << PackedUInt2((unsigned int)(count));
            p << release;
            WritePacket(p);
        }
    }

    UpdateAppearance();

    if (0) {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            for (auto& entry : m_networkedActors) {
                AbstractActorController* c = entry.second;
                if (!c->IsPlayer())
                    continue;

                auto play = gPlayState;

                auto otherPlayerController = static_cast<PlayerPuppetController*>(c);

                Player* player = GET_PLAYER(play);
                if (player == NULL)
                    return;

                if ((player->stateFlags1 & PLAYER_STATE1_ON_HORSE) && player->rideActor != NULL) {
                    gSaveContext.horseData.scene = play->sceneNum;
                    gSaveContext.horseData.pos.x = (int16_t)player->rideActor->world.pos.x;
                    gSaveContext.horseData.pos.y = (int16_t)player->rideActor->world.pos.y;
                    gSaveContext.horseData.pos.z = (int16_t)player->rideActor->world.pos.z;
                    gSaveContext.horseData.angle = player->rideActor->world.rot.y;
                }

                gSaveContext.respawnFlag = 1;
                play->nextEntranceIndex = gSaveContext.entranceIndex;
                gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = play->nextEntranceIndex;
                gSaveContext.respawn[RESPAWN_MODE_DOWN].roomIndex = otherPlayerController->RealRoomIndex();
                gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = c->GetActor()->world.pos;
                gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = c->GetActor()->shape.rot.y;
                if (play->roomCtx.curRoom.behaviorType2 < 4) {
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = 0x0DFF;
                } else {
                    Camera* camera = GET_ACTIVE_CAM(play);
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = 0x0D00 | camera->camDataIdx;
                }
                play->transitionTrigger = TRANS_TRIGGER_START;
                play->transitionType = TRANS_TYPE_INSTANT;
                gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK_FAST;

                static int hookId = 0;
                hookId = REGISTER_VB_SHOULD(VB_INFLICT_VOID_DAMAGE, {
                    *should = false;
                    GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnVanillaBehavior>(hookId);
                });

                break;
            }
        }
    }

    m_currentFrame++;
}

void ZeldaOnlineClient::InitPuppetPlayer(Actor* actor) {
    RelinkPuppetBehindPlayer(actor);

    actor->id = ACTOR_EN_OE2;
    actor->init = PlayerPuppet_Init;
    actor->update = PlayerPuppet_Update;
    actor->draw = PlayerPuppet_Draw;
    actor->destroy = PlayerPuppet_Destroy;
}

Actor* ZeldaOnlineClient::RequestSpawnActor(s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ,
                                            s16 params) {

    if (gPlayState == nullptr)
        return nullptr;

    if (!ActorControllerFactory::Instance().IsNetworked(actorId, params) || !isConnected ||
        gPlayState->roomCtx.curRoom.num < 0) {
        return Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, actorId, posX, posY, posZ, rotX, rotY, rotZ, posX,
                                 posY, posZ, rotX, rotY, rotZ, params, 0);
    }

    SPDLOG_DEBUG("[ZeldaOnline] networked request spawn request: actor {:#06x}", actorId);

    auto sceneKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());

    ByteStream packet = newPacket(CLIENT_PACKET_REQUEST_ACTOR_SPAWN);
    packet << PackedUInt4(sceneKey);
    packet << PackedInt1((gPlayState->roomCtx.curRoom.num));
    packet << PackedUInt2((u16)(actorId));
    packet << PackedInt2(params);
    packet << PackedFloat4(posX) << PackedFloat4(posY) << PackedFloat4(posZ);
    packet << PackedInt2(rotX) << PackedInt2(rotY) << PackedInt2(rotZ);

    auto* ctx = AbstractActorController::CurrentLeaderContext();
    packet << PackedUInt2(ctx != nullptr ? (unsigned int)(ctx->NetworkID()) : 0u);
    WritePacket(packet);
    return nullptr;
}

Actor* ZeldaOnlineClient::RequestSpawnActorAsChild(AbstractActorController* parent, s16 actorId, f32 posX, f32 posY,
                                                   f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params) {
    SPDLOG_DEBUG("[ZeldaOnline] networked request spawn as child request: actor {:#06x}", actorId);

    auto sceneKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());

    ByteStream packet = newPacket(CLIENT_PACKET_REQUEST_ACTOR_SPAWN_AS_CHILD);
    packet << PackedUInt4(sceneKey);
    packet << PackedInt1(gPlayState->roomCtx.curRoom.num);
    packet << PackedUInt2((u16)(parent->NetworkID()));
    packet << PackedUInt2((u16)(actorId));
    packet << PackedInt2(params);
    packet << PackedFloat4(posX) << PackedFloat4(posY) << PackedFloat4(posZ);
    packet << PackedInt2(rotX) << PackedInt2(rotY) << PackedInt2(rotZ);

    auto* ctx = AbstractActorController::CurrentLeaderContext();
    packet << PackedUInt2(ctx != nullptr ? (unsigned int)(ctx->NetworkID()) : 0u);
    WritePacket(packet);
    return nullptr;
}

static bool IsClusterDedupActor(s16 actorId) {
    return actorId == ACTOR_EN_ISHI || actorId == ACTOR_EN_KUSA;
}

Actor* ZeldaOnlineClient::SpawnActor(s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ,
                                     s16 params) {
    if (gPlayState == nullptr)
        return nullptr;

    if (!isConnected || gPlayState->roomCtx.curRoom.num < 0 ||
        !ActorControllerFactory::Instance().IsNetworked(actorId, params)) {

        auto currentExecutingController = AbstractActorController::CurrentLeaderContext();
        if (currentExecutingController != nullptr &&
            currentExecutingController->CanSpawnActorOverNetwork(actorId, params)) {
            SendRoomTrigger("spawn", ByteStream() << PackedUInt2(actorId) << PackedFloat4(posX) << PackedFloat4(posY)
                                                  << PackedFloat4(posZ) << PackedInt2(rotX) << PackedInt2(rotY)
                                                  << PackedInt2(rotZ) << PackedInt2(params));
        }

        return Actor_SpawnDirect(&gPlayState->actorCtx, gPlayState, actorId, posX, posY, posZ, rotX, rotY, rotZ, posX,
                                 posY, posZ, rotX, rotY, rotZ, params, 0);
    }

    if (IsClusterDedupActor(actorId)) {
        if (Actor* existing = FindExistingActor(actorId, params, ACTORCAT_PROP, posX, posZ)) {
            DbgPrintf("FOUND EXISTING ACTOR: %i\n", actorId);
            return nullptr;
        }
    }

    auto currentExecutingController = AbstractActorController::CurrentLeaderContext();

    if (currentExecutingController != nullptr && currentExecutingController->GetActor()->init != nullptr &&
        !currentExecutingController->IsCreator()) {
        return nullptr;
    }

    auto sceneKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());
    Actor* actor = Actor_SpawnDirect(
        &gPlayState->actorCtx, gPlayState, actorId, posX, posY, posZ, rotX, rotY, rotZ, posX, posY, posZ, rotX, rotY,
        rotZ, params, 1);

    actor->zoLocalId = m_nextLocalID++;

    AbstractActorController* controller = actorId != ACTOR_EN_BOM ? ActorControllerFactory::Instance().Create(actorId, actor, 0, (int)(sceneKey), gPlayState->roomCtx.curRoom.num, true) : nullptr;

    if (controller) {
        controller->SetCreator(true);
    }
    DbgPrintf("[ZeldaOnline] networked spawn: actor {:#06x}", actorId);

    ByteStream packet = newPacket(CLIENT_PACKET_ACTOR_SPAWN);
    packet << PackedUInt4(sceneKey);
    packet << PackedInt1(gPlayState->roomCtx.curRoom.num);
    packet << PackedUInt2((u16)(actorId));
    packet << PackedUInt4((u32)(actor->zoLocalId));
    packet << PackedInt2(params);
    packet << PackedFloat4(posX) << PackedFloat4(posY) << PackedFloat4(posZ);
    packet << PackedInt2(rotX) << PackedInt2(rotY) << PackedInt2(rotZ);

    WritePacket(packet);

    return actor;
}

Actor* ZeldaOnlineClient::SpawnActorAsChild(Actor* parent, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX,
                                            s16 rotY, s16 rotZ, s16 params) {
    if (gPlayState == nullptr)
        return nullptr;

    bool shouldNotNetworkSpawn = !ActorControllerFactory::Instance().IsNetworked(actorId, params) || !isConnected ||
                                 gPlayState->roomCtx.curRoom.num < 0;

    if (!shouldNotNetworkSpawn) {
        auto currentExecutingController = AbstractActorController::CurrentLeaderContext();

        const bool insideParentInit = parent->init != nullptr;

        bool maySpawnDirectly = false;

        if (insideParentInit) {
            maySpawnDirectly = currentExecutingController != nullptr && currentExecutingController->IsCreator();
        } else if (currentExecutingController != nullptr &&
                   currentExecutingController->CanSpawnActorOverNetwork(actorId, params)) {
            maySpawnDirectly = true;
        }

        if (!maySpawnDirectly && (insideParentInit || m_blockedParentSpawners.contains(parent->id))) {
            if (parent->zoController != nullptr) {
                return RequestSpawnActorAsChild(static_cast<AbstractActorController*>(parent->zoController), actorId,
                                                posX, posY, posZ, rotX, rotY, rotZ, params);
            }
            return RequestSpawnActor(actorId, posX, posY, posZ, rotX, rotY, rotZ, params);
        }
    }

    Actor* actor = Actor_SpawnAsChildDirect(&gPlayState->actorCtx, parent, gPlayState, actorId, posX, posY, posZ, rotX, rotY, rotZ, posX, posY, posZ, rotX, rotY, rotZ, params, shouldNotNetworkSpawn ? 0 : 1);

    if (shouldNotNetworkSpawn) {
        auto currentExecutingController = AbstractActorController::CurrentLeaderContext();
        if (currentExecutingController != nullptr &&
            currentExecutingController->CanSpawnActorOverNetwork(actorId, params)) {
            SendRoomTrigger("spawnC", ByteStream() << PackedUInt2(actorId) << PackedFloat4(posX) << PackedFloat4(posY)
                                                   << PackedFloat4(posZ) << PackedInt2(rotX) << PackedInt2(rotY)
                                                   << PackedInt2(rotZ) << PackedInt2(params)
                                                   << PackedUInt2(currentExecutingController->NetworkID()));
        }

        return actor;
    }

    auto sceneKey = MakeSceneKey(gPlayState->sceneNum, LINK_IS_ADULT ? 1 : 0, GetSceneVariant());

    AbstractActorController* controller = actorId != ACTOR_EN_BOM ? ActorControllerFactory::Instance().Create(actorId, actor, 0, (int)(sceneKey), gPlayState->roomCtx.curRoom.num, true) : nullptr;

    if (controller) {
        controller->SetCreator(true);
    }

    int packetType = CLIENT_PACKET_ACTOR_SPAWN_AS_CHILD;
    actor->zoLocalId = m_nextLocalID++;

    int parentID = 0;

    if (parent->id == ACTOR_PLAYER && parent->zoController == nullptr) {
        parentID = m_myNetworkID;
    } else {

        auto parentController = static_cast<AbstractActorController*>(parent->zoController);
        if (parentController == nullptr) {
            packetType = CLIENT_PACKET_ACTOR_SPAWN;
        } else
            parentID = parentController->NetworkID();
    }

    SPDLOG_DEBUG("[ZeldaOnline] networked spawn as child request: actor {:#06x}", actorId);

    ByteStream packet = newPacket(packetType);
    packet << PackedUInt4(sceneKey);
    packet << PackedInt1(gPlayState->roomCtx.curRoom.num);

    if (parentID != 0)
        packet << PackedUInt2((u16)(parentID));

    packet << PackedUInt2((u16)(actorId));
    packet << PackedUInt4((u32)(actor->zoLocalId));
    packet << PackedInt2(params);
    packet << PackedFloat4(posX) << PackedFloat4(posY) << PackedFloat4(posZ);
    packet << PackedInt2(rotX) << PackedInt2(rotY) << PackedInt2(rotZ);

    WritePacket(packet);

    return actor;
}

bool ZeldaOnlineClient::RequestRoomSceneChange(bool roomTransition) {

    int roomIndex = gPlayState->roomCtx.curRoom.num;

    if (!roomTransition) {
        UpdateAppearance();
    }

    m_lastRoom = gPlayState->roomCtx.curRoom.num;
    m_lastScene = gPlayState->sceneNum;

    ByteStream packet = newPacket(CLIENT_PACKET_SET_ROOM_SCENE);
    packet << PackedUInt2((u16)(gPlayState->sceneNum));
    packet << PackedUInt1(LINK_IS_ADULT ? 1u : 0u);
    packet << PackedUInt2((u16)(GetSceneVariant()));
    packet << PackedInt1((s8)(roomIndex));
    packet << PackedUInt1(roomTransition ? 1u : 0u);
    return WritePacket(packet);
}

Actor* ZeldaOnlineClient::FindExistingActor(s16 actorId, s16 params, int category, f32 homeX, f32 homeZ) {
    ActorListEntry* list = &gPlayState->actorCtx.actorLists[category];
    Actor* actor = list->head;
    while (actor != nullptr) {
        if (actor->update && actor->id == actorId && actor->params == params &&
            fabsf(actor->home.pos.x - homeX) < 1.0f && fabsf(actor->home.pos.z - homeZ) < 1.0f) {
            return actor;
        }
        actor = actor->next;
    }
    return nullptr;
}

void ZeldaOnlineClient::SendActorDied(int actorID) {
    if (actorID == 0)
    {
        DbgPrintf("Break\n");
    }
    ByteStream packet = newPacket(CLIENT_PACKET_ACTOR_DIED);
    packet << PackedUInt2((u16)(actorID));
    WritePacket(packet);
}

void ZeldaOnlineClient::TransmitActorSound(int networkID, u16 sfxId) {
    if (!isConnected || gPlayState == nullptr) {
        return;
    }

    ByteStream packet = newPacket(CLIENT_PACKET_ACTOR_SOUND);
    packet << PackedUInt2((u16)(networkID));
    packet << PackedUInt2((u16)(sfxId));
    WritePacket(packet);
}

void ZeldaOnlineClient::SendDeclineActorLeader(int networkID) {
    ByteStream p = newPacket(CLIENT_PACKET_DECLINE_ACTOR_LEADER);
    p << PackedUInt2((unsigned int)(networkID));
    WritePacket(p);
}

void ZeldaOnlineClient::DetachAndKill(AbstractActorController* controller) {
    if (controller == nullptr) {
        return;
    }
    auto runningLocal = controller->IsRunningLocally();
    Actor* actor = controller->Detach();
    RemoveNetworkedActor(controller->NetworkID(), controller);
    delete controller;

    if (!runningLocal && actor != nullptr && actor->update != nullptr) {
        gZeldaOnlineEngineCleanup = true;
        Actor_Kill(actor);
        gZeldaOnlineEngineCleanup = false;
    }
}
}

extern "C" Actor* ZeldaOnlineClient_SpawnActor(s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ,
                                               s16 params) {

    auto instance = ZeldaOnline::ZeldaOnlineClient::Instance;

    if (gMapLoading)
        return instance->RequestSpawnActor(actorId, posX, posY, posZ, rotX, rotY, rotZ, params);

    return instance->SpawnActor(actorId, posX, posY, posZ, rotX, rotY, rotZ, params);
}

extern "C" Actor* ZeldaOnlineClient_SpawnActorAsChild(Actor* parent, s16 actorId, f32 posX, f32 posY, f32 posZ,
                                                      s16 rotX, s16 rotY, s16 rotZ, s16 params) {

    auto instance = ZeldaOnline::ZeldaOnlineClient::Instance;
    return instance->SpawnActorAsChild(parent, actorId, posX, posY, posZ, rotX, rotY, rotZ, params);
}

extern "C" int ZeldaOnlineClient_RequestRoomSceneChange(int freshLoad) {

    auto instance = ZeldaOnline::ZeldaOnlineClient::Instance;
    return 0;
}
