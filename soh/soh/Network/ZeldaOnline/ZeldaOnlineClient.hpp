#ifndef ZELDAONLINEH
#define ZELDAONLINEH

#define ACTOR_FLAG_ZO_USER1 (1 << 31)
#define ACTOR_FLAG_ZO_USER2 (1 << 30)
#define ACTOR_FLAG_ZO_USER3 (1 << 29)

#define ZO_HORSE_PUPPET 0x4000

#include "global.h"
#ifdef __cplusplus
#include <unordered_set>
#include "ZNetworking.hpp"
#include "ByteStream.hpp"
#include "BytePacking.hpp"
#include "PacketTypes.hpp"
#include "AbstractActorController.hpp"
#include <ctime>

namespace ZeldaOnline {

class ZeldaOnlineClient : public ZNetworking {
  private:
    static const unsigned int FRAME_HEADER_SIZE = 2;
    static const unsigned int MAX_FRAME_PAYLOAD = 0xFFFF;

    enum RequestRoomSceneChange  {
        RequestRoomSceneChangeNone,
        RequestRoomSceneChangeRoom,
        RequestRoomSceneChangeScene
    } ;
    std::string m_host;
    int m_port = 21050;

    ByteStream recvBuffer;

    std::unordered_map<int, AbstractActorController*> m_networkedActors;
    ByteStream outgoingBuffer;
    int m_myNetworkID = 0;

    bool m_hasWorldTime = false;
    u16 m_serverTimeRate = 200;
    u16 m_serverDayTime = 0;
    u8 m_boundaryCueOnLoad = 0;
    bool m_sceneLoadedAtNight = false;
    bool m_reloadPending = false;
    u8 m_wasPaused = 0;

    int m_lastScene = -1;
    int m_lastRoom = -1;

    u32 m_nextLocalID = 1;
    std::unordered_set<int> m_lockedDoorFlags;

    ByteStream m_lastAppearanceBlob;

    bool m_spawningPuppetPlayer = false;
    u8 m_lastSentLinkAge = 0xFF;

    bool m_applyingRemoteSpawn = false;
    int m_partyID = 0;
    uint32_t m_currentRoomTempMask = 0;

    bool m_sendFullPlayerProps = true;
    ByteStream m_lastPlayerProps;
    u32 m_currentFrame = 0;
    bool m_hooksEnabled = false;
    std::unordered_set<int> m_blockedParentSpawners;
    bool m_didDisconnect = false;
    uint64_t m_bytesSent = 0;
    uint64_t m_bytesReceived = 0;
    time_t m_lastReportTime = 0;
    RequestRoomSceneChange m_roomSceneChange = RequestRoomSceneChange::RequestRoomSceneChangeNone;

    void ReportBandwidth();
    bool NextPacket(ByteStream& out);
    void SendPacket_PlayerUpdate();
    void UpdateAppearance();
    void RegisterNetworkingHook(bool enabled);
    void RegisterHooks(bool enabled);
    void OnActorKill(Actor* actor);
    void OnGameFrameUpdate();
    void InitPuppetPlayer(Actor* actor);

    void GetTunicColours(Player* player, Color_RGB8* out);

  public:
    static ZeldaOnlineClient* Instance;

    ZeldaOnlineClient();

    void Enable();
    void Disable() {
        ZNetworking::Disable();
    }

    static bool IsNightTime(u16 dayTime) {
        return dayTime > 0xC000 || dayTime < 0x4555;
    }

    static bool IsDayNightReloadScene(int sceneNum) {
        switch (sceneNum) {
            case 0x1B:
            case 0x1C:
            case 0x20:
            case 0x21:
            case 0x22:
            case 0x23:
            case 0x52:
            case 0x63:
                return true;
            default:
                return false;
        }
    }

    void OnIncomingData(const char* payload, int length) override;

    void ProcessOutgoingPackets() override;

    void OnConnected() override;
    void OnDisconnected() override;
    void OnConnectionClosedBeforeConnect() override;

    virtual void OnIncomingPacket(ByteStream& packet);

    void RemoveNetworkedActor(int networkID, AbstractActorController* expected) {
        for (auto it = m_networkedActors.begin(); it != m_networkedActors.end();) {
            if (it->second == expected)
                it = m_networkedActors.erase(it);
            else
                ++it;
        }
    }

    AbstractActorController* GetNetworkController(int networkID) {
        auto it = m_networkedActors.find(networkID);
        return it == m_networkedActors.end() ? nullptr : it->second;
    }

    bool WritePacket(const ByteStream& data);
    void SendSceneTrigger(const std::string& name, const ByteStream& payload);
    void SendRoomTrigger(const std::string& name, const ByteStream& payload);
    void SendHitTrigger(ActorID actor, f32 posX, f32 posY, f32 posZ);
    Actor* RequestSpawnActor(s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);
    Actor* RequestSpawnActorAsChild(AbstractActorController* parent, s16 actorId, f32 posX, f32 posY, f32 posZ,
                                    s16 rotX, s16 rotY, s16 rotZ, s16 params);

    Actor* SpawnActor(s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);
    Actor* SpawnActorAsChild(Actor* parent, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ,
                             s16 params);
    bool RequestRoomSceneChange(bool roomTransition = false);
    Actor* FindExistingActor(s16 actorId, s16 params, int category, f32 homeX, f32 homeZ);

    bool IsLocalPlayerClosestTo(const Vec3f& pos, f32 localDistSq) {
        for (auto& entry : m_networkedActors) {
            AbstractActorController* c = entry.second;
            if (!c->IsPlayer())
                continue;
            Vec3f d = { c->GetActor()->world.pos.x - pos.x, 0.0f, c->GetActor()->world.pos.z - pos.z };
            if (SQ(d.x) + SQ(d.z) < localDistSq)
                return false;
        }
        return true;
    }

    void RegisterActorHooks(bool enabled);
    void OnSceneTrigger(AbstractActorController* sendingPlayer, const std::string& name, ByteStream& data);
    void SendActorDied(int actorID);

    void TransmitActorSound(int networkID, u16 sfxId);

    u32 CurrentFrame() const {
        return m_currentFrame;
    }

    void SendDeclineActorLeader(int networkID);

    void DetachAndKill(AbstractActorController* controller);
};
}

#endif

#ifdef __cplusplus
extern "C" {
#endif

Actor* ZeldaOnlineClient_SpawnActor(s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ,
                                    s16 params);

Actor* ZeldaOnlineClient_SpawnActorAsChild(Actor* parent, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY,
                                           s16 rotZ, s16 params);

int ZeldaOnlineClient_RequestRoomSceneChange(int freshLoad);
extern u8 gZeldaOnlineEngineCleanup;

#ifdef __cplusplus
}
#endif

#endif
