#include "AbstractActorController.hpp"
#include "ZeldaOnlineClient.hpp"

#include "PacketTypes.hpp"
#include "Packet.hpp"
#include <soh/ActorDB.h>

namespace ZeldaOnline {
    void AbstractActorController::SendUpdate() {
        ByteStream propUpdates;
        WriteProperties(propUpdates);

        if (propUpdates.Length() > 0) {
            ZeldaOnlineClient::Instance->WritePacket(
                newPacket(CLIENT_PACKET_ACTOR_PROPERTIES)
                << PackedUInt2((unsigned int)(m_networkID)) << propUpdates);
        }

    }

    void AbstractActorController::ClaimLeadership(u8 reason) {
        if (m_isLeader || (reason == CLAIM_REASON_PROXIMITY && m_claimCooldown > 0))
            return;

        if (m_locked) {
            return;
        }

        m_claimCooldown = 40;

            SetLeader(true);
        auto* client = ZeldaOnlineClient::Instance;
        if (client == nullptr || !client->isConnected)
            return;
        ByteStream packet = newPacket(CLIENT_PACKET_CLAIM_ACTOR);
        packet << PackedUInt2((u16)(m_networkID));
        packet << PackedUInt1(reason);
        client->WritePacket(packet);
    }

    void AbstractActorController::SendTriggerToPuppets(const std::string& name, const ByteStream& data) {
        auto* client = ZeldaOnlineClient::Instance;
        if (client == nullptr || !client->isConnected || !m_isLeader) {
            return;
        }

        ByteStream packet = newPacket(CLIENT_PACKET_ACTOR_TRIGGER_TO_PUPPETS);
        packet << PackedUInt2((u16)(m_networkID));
        packet << PackedUInt1((unsigned int)(name.length()));
        packet << name;
        packet.Write(data);
        client->WritePacket(packet);
    }

    void AbstractActorController::SendTriggerToLeader(const std::string& name, const ByteStream& data) {
        auto* client = ZeldaOnlineClient::Instance;
        if (client == nullptr || !client->isConnected || m_isLeader) {
            return;
        }

        ByteStream packet = newPacket(CLIENT_PACKET_ACTOR_TRIGGER_TO_LEADER);
        packet << PackedUInt2((u16)(m_networkID));
        packet << PackedUInt1((unsigned int)(name.length()));
        packet << name;
        packet.Write(data);
        client->WritePacket(packet);
    }

    void AbstractActorController::UpdateLeadershipLock() {
        bool wantLock = m_isLeader && ShouldLockActor();

        if (wantLock != m_lockSent) {
            m_lockSent = wantLock;
            SendLeadershipLock(wantLock);
        }
    }

    void AbstractActorController::SendLeadershipLock(bool locked) {
        auto* client = ZeldaOnlineClient::Instance;
        if (client == nullptr || !client->isConnected)
            return;
        ByteStream packet = newPacket(CLIENT_PACKET_SET_ACTOR_LOCKED);
        packet << PackedUInt2((u16)(m_networkID));
        packet << PackedUInt1(locked ? 1u : 0u);
        client->WritePacket(packet);
    }

    bool AbstractActorController::IsLocalPlayerClosest() const {
        auto* client = ZeldaOnlineClient::Instance;
        if (client == nullptr) {
            return true;
        }
        f32 localDistSq = SQ(m_actor->xzDistToPlayer);
        return client->IsLocalPlayerClosestTo(m_actor->world.pos, localDistSq);
    }

    AbstractActorController::AbstractActorController(Actor* actor, int networkID, int sceneKey, int roomIndex,
                                                            bool isLeader)
        : m_actor(actor), m_networkID(networkID), m_sceneKey(sceneKey), m_roomIndex(roomIndex), m_isLeader(isLeader),
          m_originalInit(actor->init), m_originalUpdate(actor->update), m_originalDestroy(actor->destroy) {
        m_actor->zoController = this;

        m_spawnPosRot = actor->world;

        if (actor->id == ACTOR_BOSS_VA) {
            printf("BREAK\n");
        }

        if (m_originalInit)
            m_actor->init = DispatchInit;
        m_actor->update = AbstractActorController::DispatchUpdate;
        m_actor->destroy = AbstractActorController::DispatchDestroy;

    }

    void AbstractActorController::DispatchDestroy(Actor* actor, PlayState* play) {
        auto* controller = static_cast<AbstractActorController*>(actor->zoController);
        if (controller == nullptr) {
            if (actor->destroy != nullptr && actor->destroy != DispatchDestroy) {
                actor->destroy(actor, play);
            }
            return;
        }

        ActorFunc original = controller->m_originalDestroy;

        actor->zoController = nullptr;
        controller->m_actor = nullptr;

        ZeldaOnlineClient::Instance->RemoveNetworkedActor(controller->NetworkID(), controller);
        delete controller;

        if (original != NULL) {
            original(actor, play);
        }
    }

    void AbstractActorController::ActorInit(PlayState* play)
    {
        if (m_networkID == 0)
            return;

        const bool movedFromHome = m_spawnPosRot.pos.x != m_actor->home.pos.x ||
                                   m_spawnPosRot.pos.y != m_actor->home.pos.y ||
                                   m_spawnPosRot.pos.z != m_actor->home.pos.z;

        if (m_originalInit) {
            s_currentLeaderContext = this;
            m_originalInit(m_actor, play);
            s_currentLeaderContext = nullptr;
            m_originalInit = nullptr;
        }
        OnActorInit();

        if (movedFromHome) {
            m_actor->world = m_spawnPosRot;

        }
        Math_Vec3f_Copy(&m_actor->prevPos, &m_actor->world.pos);

        if (m_pendingProperties.Length() > 0)
        {
            ReadProperties(m_pendingProperties);
            m_pendingProperties.Clear();
        }
    }

    void AbstractActorController::Update(PlayState* play) {

        if (m_runningLocally) {
            m_originalUpdate(m_actor, play);
            return;
        }
        UpdateLeadershipLock();

        if (m_isLeader) {

            if (m_actor->init == nullptr)
            {
                s_actorSoundContext = s_currentLeaderContext = this;
                UpdateLeader(play);
                s_actorSoundContext = s_currentLeaderContext = nullptr;
                SendUpdate();
            }
        } else {
            if (m_actor->init == nullptr) {
                s_actorSoundContext = this;
                UpdatePuppet(play);
                s_actorSoundContext = nullptr;
            }
        }
        if (m_claimCooldown > 0)
            m_claimCooldown--;
    }
}
