#ifndef ROOMH
#define ROOMH

#include <cstdio>
#include <unordered_map>
#include <cmath>

#include "Player.hpp"
#include "WorldActor.hpp"
#include "ActorListHelper.hpp"
#include "ActorRegistry.hpp"
#include "ByteStream.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"

namespace ZeldaOnline
{
	ByteStream MakeActorSpawnPacket(uint32_t clientSceneKey, WorldActor& actor);

	class Room
	{
	public:
		struct StaticActor {
			int actorID;
			int params;
			float x, y, z;
			int rotX, rotY, rotZ;
		};


	protected:
		class Scene* m_scene = nullptr;
		ActorRegistry* m_registry = nullptr;
		uint64_t m_sceneKey = 0;
		int m_roomIndex = 0;

		std::unordered_map<int, Player*> m_players;

		Player* m_leader = nullptr;

		Player* m_creator = nullptr;
		unsigned int m_tempFlags = 0;


		std::vector<WorldActor*> m_worldActors;
		std::vector<StaticActor> m_staticActors;

		ByteStream makeSpawnPacket(WorldActor& actor) const
		{
			return MakeActorSpawnPacket(ClientSceneKey(), actor);
		}

	public:
		Room(class Scene* scene, ActorRegistry* registry, uint64_t sceneKey, int roomIndex) :
			m_scene(scene), m_registry(registry), m_sceneKey(sceneKey), m_roomIndex(roomIndex)
		{
		}

		const std::vector<StaticActor>& StaticActors() const {
			return m_staticActors;
		}

		void AddStaticActor(const StaticActor& entry) {
			m_staticActors.push_back(entry);
		}

		void SetTempFlag(int flag, bool on)
		{
			if (flag < 0x20 || flag >= 0x40)
				return;

			unsigned int bit = 1u << (flag - 0x20);
			if (on)
				m_tempFlags |= bit;
			else
				m_tempFlags &= ~bit;
		}

		int RoomIndex() const {
			return m_roomIndex;
		}

		uint64_t SceneKey() const {
			return m_sceneKey;
		}

		ByteStream MakeSpawnPacket(WorldActor& actor) const {
			return makeSpawnPacket(actor);
		}

		uint32_t ClientSceneKey() const {
			return (uint32_t)m_sceneKey;
		}

		Player* Leader() const {
			return m_leader;
		}

		Player* Creator() const {
			return m_creator;
		}

		void SetCreator(Player* creator) {
			if (m_creator == creator)
				return;
			if (m_creator != nullptr)
				m_creator->ForgetCreatedRoom(this);
			m_creator = creator;
			if (m_creator != nullptr)
				m_creator->AddCreatedRoom(this);
		}
		void ClearCreator() {
			SetCreator(nullptr);
		}

		void SetCreatorRaw(Player* creator) {
			m_creator = creator;
		}

		void SetLeader(Player* leader) {
			m_leader = leader;
		}
		void ClearLeader() {
			m_leader = nullptr;
		}


		const std::unordered_map<int, Player*>& Players() const {
			return m_players;
		}
		int PlayerCount() const {
			return (int)(m_players.size());
		}

		virtual bool CanSyncTempFlag(int flag) const {
			return true;
		}
		static void NotifySceneRoomEmptied(class Scene* scene, int roomIndex);

		void AddPlayer(Player* player)
		{
			Room* old = player->CurrentRoom();
			if (old == this)
				return;
			if (old)
			{
				Scene* oldScene = old->m_scene;
				int oldIndex = old->RoomIndex();
				old->RemovePlayer(player);

				if (oldScene)
					NotifySceneRoomEmptied(oldScene, oldIndex);
			}

			bool wasEmptyOfPlayers = m_players.empty();

			if (m_creator == nullptr && m_worldActors.empty())
			{
				SetCreator(player);
				std::printf("[trace] room %d: p%d is CREATOR\n", m_roomIndex, player->NetworkID());
			}

			m_players[player->NetworkID()] = player;
			std::printf("[trace] room %d: add p%d (leader now p%d), snapshot %zu players\n",
				m_roomIndex, player->NetworkID(),
				m_leader ? m_leader->NetworkID() : player->NetworkID(), m_players.size());
			player->SetCurrentRoom(this);

			if (wasEmptyOfPlayers || m_leader == nullptr)
			{
				Player* previousLeader = m_leader;
				m_leader = player;

				for (WorldActor* actor : m_worldActors)
				{
					Player* owner = actor->Owner();
					if (owner == nullptr || owner == previousLeader)
						actor->ReassignOwner(m_leader, player);
				}
			}

			{
				ByteStream packet = newPacket(SERVER_PACKET_ROOM_TEMP_FLAGS);
				packet << PackedUInt4((unsigned int)(ClientSceneKey()));
				packet << PackedInt1(m_roomIndex);
				packet << PackedUInt4(m_tempFlags);
				player->SendPacket(packet);
			}

			for (auto& actor : m_staticActors)
			{
				ByteStream packet = newPacket(SERVER_PACKET_ACTOR_STATIC_SPAWN);
				packet << PackedUInt4((unsigned int)(ClientSceneKey()));
				packet << PackedInt1(m_roomIndex);
				packet << PackedUInt2(actor.actorID);
				packet << PackedFloat4(actor.x);
				packet << PackedFloat4(actor.y);
				packet << PackedFloat4(actor.z);
				packet << PackedInt2(actor.rotX);
				packet << PackedInt2(actor.rotY);
				packet << PackedInt2(actor.rotZ);
				packet << PackedInt2(actor.params);
				player->SendPacket(packet);
			}

			for (WorldActor* actor : m_worldActors)
			{
				printf("Sending spawn packet for %i with params %i\n", actor->ActorID(), actor->Params());
				player->SendPacket(makeSpawnPacket(*actor));
			}

			std::printf("[trace] room %d: sent %zu actors to p%d\n",
				m_roomIndex, m_worldActors.size(), player->NetworkID());

			OnPlayerSetupActorsDone(player);

			player->SendPacket(newPacket(SERVER_PACKET_ROOM_LOADED) << PackedUInt4(ClientSceneKey()) << PackedInt1(RoomIndex()));
		}

		void RemovePlayer(Player* player)
		{
			std::printf("[trace] room %d: remove p%d (players %zu -> %zu)\n",
				m_roomIndex, player->NetworkID(), m_players.size(), m_players.size() - 1);
			m_players.erase(player->NetworkID());
			if (player->CurrentRoom() == this)
				player->SetCurrentRoom(nullptr);

			if (m_leader == player)
				m_leader = nullptr;

			if (m_creator == player) {
				ClearCreator();
				std::printf("[trace] room %d: creator p%d left -- spawn requests now closed\n",
					m_roomIndex, player->NetworkID());
			}

			if (m_leader == nullptr && !m_players.empty())
				m_leader = m_players.begin()->second;

			for (WorldActor* actor : m_worldActors)
			{
				if (actor->Owner() == player)
				{
					actor->ReassignOwner(m_leader);
				}
			}

			if (m_players.empty() && m_leader == nullptr)
			{
				m_tempFlags = 0;
			}
		}

		WorldActor* FindActor(int actorID, int params, float homeX, float homeZ) const
		{
			for (WorldActor* a : m_worldActors)
			{
				if (a->ActorID() == actorID && a->Params() == params &&
					std::fabs(a->HomeX() - homeX) < 1.0f &&
					std::fabs(a->HomeZ() - homeZ) < 1.0f)
				{
					return a;
				}
			}
			return nullptr;
		}

		void SendToAll(const ByteStream& payload, Player* except = nullptr)
		{
			for (auto& entry : m_players)
			{
				if (entry.second == except)
					continue;
				entry.second->SendPacket(payload);
			}
		}

		virtual WorldActor* SpawnActor(int actorID, int params,
			float x, float y, float z,
			short rotX, short rotY, short rotZ, float homeX, float homeY, float homeZ,
			short homeRotX, short homeRotY, short homeRotZ, int parentID,
			Player* except = nullptr, Player* owner = nullptr);

		void AdoptActor(WorldActor* actor) {
			InsertActorOrdered(m_worldActors, actor);
			actor->SetOwningRoom(this);
			actor->SetRoomIndex(m_roomIndex);
		}

		void ReleaseActor(int networkID) {
			EraseActorOrdered(m_worldActors, networkID);
		}

		WorldActor* GetWorldActor(int networkID)
		{
			return FindActorOrdered(m_worldActors, networkID);
		}

		const std::vector<WorldActor*>& WorldActors() const {
			return m_worldActors;
		}

		void OnPlayerSetupActorsDone(Player* player)
		{
			AdoptOwnerlessActors(player);
		}

		void OnPlayerUnpaused(Player* player)
		{
			AdoptOwnerlessActors(player);
		}

		void AdoptOwnerlessActors(Player* player)
		{
			if (m_players.find(player->NetworkID()) == m_players.end())
				return;

			if (player->IsPaused())
				return;

			std::printf("[trace] AdoptOwnerlessActors(%i)\n", player->NetworkID());

			if (m_leader == nullptr)
				m_leader = player;

			bool adoptedAny = false;
			for (WorldActor* actor : m_worldActors)
			{
				if (!actor->AwaitingLeader() && actor->Owner() != nullptr)
					continue;

				if (actor->HasDeclinedAssignment(player->Guid()))
					continue;

				actor->ReassignOwner(player);
				adoptedAny = true;
			}

			if (adoptedAny)
				std::printf("[trace] room %d: setup-done adopt by p%d\n", m_roomIndex, player->NetworkID());
		}
		virtual void ClearActors();

		void ClearActorLock(WorldActor& actor) {
			if (actor.IsLocked()) {
				actor.SetLocked(false, nullptr);
				ByteStream p = newPacket(SERVER_PACKET_SET_ACTOR_LOCKED);
				p << PackedUInt2((unsigned int)(actor.NetworkID()));
				p << PackedUInt1(0u);
				SendToAll(p);
			}
		}

		Player* AnyOtherEligiblePlayer(Player* except, const WorldActor* actor) const {
			for (auto& e : m_players)
			{
				Player* candidate = e.second;
				if (candidate == except || candidate->IsPaused())
					continue;
				if (actor->HasDeclinedAssignment(candidate->Guid()))
					continue;
				return candidate;
			}
			return nullptr;
		}

	};
}

#endif