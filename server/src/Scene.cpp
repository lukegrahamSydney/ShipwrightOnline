#include "Scene.hpp"
#include "OOTServer.hpp"

void ZeldaOnline::Scene::OnFirstPlayerEnters() {
	if (!IsDungeon())
	{
		ResetScene();
		return;
	}

	//If dungeons do not reset, only reset the actors and clear rooms
	//if (!m_server->DungeonsReset())
	//	ResetActors();

	//If dungeons reset, make sure it's been the required amount of minutes
	else if (PartyID() == 0 && (std::chrono::steady_clock::now() - m_lastPlayerLeftTime) > std::chrono::minutes(m_server->DungeonsResetTimer()))
	{
		ResetScene();
	}
	else ResetActors();
}

void ZeldaOnline::Scene::AddPlayer(Player* player, bool isRefresh)
{
	Scene* old = player->CurrentScene();
	if (old == this)
		return;
	if (old)
		old->RemovePlayer(player);

	if (!isRefresh && m_players.size() == 0)
		OnFirstPlayerEnters();

	for (auto& entry : m_players)
	{
		entry.second->SendPacket(MakePlayerSpawnPacket(player));
		player->SendPacket(MakePlayerSpawnPacket(entry.second));
	}

	std::printf("[trace] scene %llu: add p%d (introducing %zu existing players)\n",
		m_sceneKey, player->NetworkID(), m_players.size());
	m_players[player->NetworkID()] = player;
	player->SetCurrentScene(this);




	if (m_eventINFFlags.size() > 0)
		player->SendPacket(newPacket(SERVER_PACKET_INF_FLAGS_LIST) << PackedUInt4(ClientSceneKey()) << BuildEventINFFlagsPacket());

	if (m_INFFlags.size() > 0)
		player->SendPacket(newPacket(SERVER_PACKET_INF_FLAGS_LIST) << PackedUInt4(ClientSceneKey()) << BuildINFFlagsPacket());

	ByteStream sceneFlagsPacket = newPacket(SERVER_PACKET_SCENE_FLAGS);
	sceneFlagsPacket << PackedUInt4(ClientSceneKey());
	sceneFlagsPacket << PackedUInt4(m_sceneFlags);
	sceneFlagsPacket << PackedUInt4(m_clearFlags);
	sceneFlagsPacket << PackedUInt4(m_tempFlags);
	sceneFlagsPacket << PackedUInt4(m_lockedDoorsMask);
	sceneFlagsPacket << PackedUInt8(m_sessionID);
	auto& party = player->GetParty();
	if (party) {
		sceneFlagsPacket << PackedUInt1(party->DungeonKeys(m_mapIndex));
	} else sceneFlagsPacket << PackedUInt1(0);

	player->SendPacket(sceneFlagsPacket);
	SendSceneScopedActors(player);
	AdoptOwnerlessActors(player);
}
