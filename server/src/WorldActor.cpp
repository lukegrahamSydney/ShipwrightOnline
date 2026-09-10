#include "WorldActor.hpp"
#include "Room.hpp"
#include "Scene.hpp"
#include "Player.hpp"

namespace ZeldaOnline {

	void WorldActor::Broadcast(const ByteStream& payload, Player* except) const
	{
		if (m_room)
			m_room->SendToAll(payload, except);
		else if (m_scene)
			m_scene->SendToAll(payload, except);
	}

	void WorldActor::ReassignOwner(Player* newOwner, Player* except) {
		SetOwner(newOwner);

		ByteStream leader = newPacket(SERVER_PACKET_SET_ACTOR_LEADER);
		leader << PackedUInt2((unsigned int)(NetworkID()));
		leader << PackedUInt2(newOwner ? (unsigned int)(newOwner->NetworkID()) : 0u);
		Broadcast(leader, except);
	}
}
