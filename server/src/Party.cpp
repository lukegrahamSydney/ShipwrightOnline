#include "Party.hpp"
#include "Player.hpp"
#include "Scene.hpp"

namespace ZeldaOnline
{
	void Party::SendToAll(const ByteStream& payload, Player* except)
	{
		for (auto& other : m_members)
		{
			if (other == except)
				continue;
			other->SendPacket(payload);
		}
	}

	void Party::SendToScene(Scene* scene, const ByteStream& payload, Player* except)
	{
		for (auto& other : m_members)
		{
			if (other == except || other->CurrentScene() != scene)
				continue;
			other->SendPacket(payload);
		}
	}
}

