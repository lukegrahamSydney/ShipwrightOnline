#include "Room.hpp"
#include "Scene.hpp"

namespace ZeldaOnline
{
	ByteStream MakeActorSpawnPacket(uint32_t clientSceneKey, WorldActor& actor)
	{
		ByteStream packet;
		if (actor.ParentID() == 0)
			packet = newPacket(SERVER_PACKET_ACTOR_SPAWN);
		else packet = newPacket(SERVER_PACKET_ACTOR_SPAWN_AS_CHILD) << PackedUInt2(actor.ParentID());

		packet << PackedUInt4((unsigned int)(clientSceneKey));
		packet << PackedInt1(actor.RoomIndex());
		packet << PackedUInt2((unsigned int)(actor.NetworkID()));
		packet << PackedUInt2((unsigned int)(actor.ActorID()));
		packet << PackedInt2(actor.Params());
		packet << PackedFloat4(actor.X()) << PackedFloat4(actor.Y()) << PackedFloat4(actor.Z());
		packet << PackedInt2(actor.RotX()) << PackedInt2(actor.RotY()) << PackedInt2(actor.RotZ());
		packet << PackedFloat4(actor.HomeX()) << PackedFloat4(actor.HomeY()) << PackedFloat4(actor.HomeZ());
		packet << PackedInt2(actor.HomeRotX()) << PackedInt2(actor.HomeRotY()) << PackedInt2(actor.HomeRotZ());
		packet << PackedUInt2(actor.Owner() ? actor.Owner()->NetworkID() : 0);
		packet << PackedUInt1(actor.IsLocked() ? 1u : 0u);
		packet << PackedUInt1(actor.ClaimCreatorFlag() ? 1u : 0u);

		if (actor.HasProperties())
			packet.Write(actor.FullProperties());

		return packet;
	}

	void Room::NotifySceneRoomEmptied(Scene* scene, int roomIndex)
	{
		scene->DestroyRoomIfEmpty(roomIndex);
	}

	WorldActor* Room::SpawnActor(int actorID, int params,
		float x, float y, float z,
		short rotX, short rotY, short rotZ,
		float homeX, float homeY, float homeZ,
		short homeRotX, short homeRotY, short homeRotZ, int parentID,
		Player* except, Player* owner)
	{
		if (!m_scene)
			return nullptr;

		int id = m_registry->NewID();
		if (id < 0)
			return nullptr;

		printf("SPAWNING ACTOR: %i with params %i\n", actorID, params);

		WorldActor* actor = m_scene->CreateActor(id, this, actorID, params,
			x, y, z, rotX, rotY, rotZ, homeX, homeY, homeZ, homeRotX, homeRotY, homeRotZ, parentID);
		AdoptActor(actor);
		actor->SetOwner(owner ? owner : m_leader);
		m_registry->Add(actor);

		if (owner != nullptr)
		{
			actor->ClaimCreatorFlag();
		}

		if (actor->CreatorFlagClaimed())
		{
			SendToAll(makeSpawnPacket(*actor), except);
		}
		else
		{
			for (auto& entry : m_players)
			{
				Player* player = entry.second;
				if (player == except)
					continue;
				player->SendPacket(makeSpawnPacket(*actor));
			}
		}

		return actor;
	}

	void Room::ClearActors()
	{
		ClearCreator();
		if (!m_scene)
		{
			m_worldActors.clear();
			return;
		}

		std::vector<int> ids;
		ids.reserve(m_worldActors.size());
		for (WorldActor* actor : m_worldActors)
			ids.push_back(actor->NetworkID());
		m_worldActors.clear();
		for (int id : ids)
		{
			if (WorldActor* actor = m_scene->GetActor(id))
				m_registry->Remove(actor);
			m_scene->EraseActor(id);
		}
		m_staticActors.clear();
	}

}