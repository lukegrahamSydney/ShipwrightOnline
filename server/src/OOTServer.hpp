#ifndef OOTSERVERH
#define OOTSERVERH

#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>


#include "sox.hpp"
#include "ByteStream.hpp"
#include "ClientConnection.hpp"
#include "Player.hpp"
#include "Scene.hpp"
#include "ActorRegistry.hpp"

#include "SceneID.hpp"
#include "Scenes/JabuScene.hpp"
#include "Scenes/KakarikoScene.hpp"
#include "Scenes/BombchuAlley.hpp"
#include "Scenes/WaterTempleScene.hpp"
#include "Scenes/MiscScenes.hpp"

namespace ZeldaOnline
{
	class OOTServer
	{
	public:
		explicit OOTServer(int listenPort = 21050);
		virtual ~OOTServer();

		int run();

		void Stop() {
			m_running = false;
		}

		int Port() const {
			return m_port;
		}

		static bool IsNightTime(unsigned int dayTime)
		{
			return dayTime > 0xC000 || dayTime < 0x4555;
		}

		bool IsNightTime() const
		{
			return IsNightTime(m_worldTime);
		}

		int WorldTime() const {
			return m_worldTime;
		}

		Player* AddPlayer();

		void RemovePlayer(Player* player);

		Player* GetPlayer(int id) const;

		AbstractActor* GetActor(int id) {
			return m_actors.Get(id);
		}

		const std::unordered_map<int, std::unique_ptr<Player>>& Players() const {
			return m_players;
		}

		int PlayerCount() const {
			return (int)(m_players.size());
		}

		Scene* GetOrCreateScene(int sceneNum, int isFuture, int otherVariant, unsigned int partyHash = 0);

		Scene* GetScene(uint64_t sceneKey);

		const std::unordered_map<uint64_t, std::unique_ptr<Scene>>& Scenes() const {
			return m_scenes;
		}

		virtual std::unique_ptr<Scene> CreateScene(int sceneNum, uint64_t key, int isFuture, int otherVariant)
		{
			std::unique_ptr<Scene> scene;

			switch (sceneNum)
			{

			case Scenes::SCENE_BOMBCHU_BOWLING_ALLEY:
				scene = std::make_unique<BombchuAlleyScene>(this, &m_actors, key, sceneNum, isFuture, otherVariant);
				break;

			case Scenes::SCENE_JABU_JABU:
				scene = std::make_unique<JabuScene>(this, &m_actors, key, sceneNum, isFuture, otherVariant);
				break;

			case Scenes::SCENE_KAKARIKO_VILLAGE:
				scene = std::make_unique<KakarikoScene>(this, &m_actors, key, sceneNum, isFuture, otherVariant);
				break;

			case Scenes::SCENE_WATER_TEMPLE:
				scene = std::make_unique<WaterTempleScene>(this, &m_actors, key, sceneNum, isFuture, otherVariant);
				break;

			case Scenes::SCENE_SPIRIT_TEMPLE_BOSS:
				scene = std::make_unique<SpiritTempleBossScene>(this, &m_actors, key, sceneNum, isFuture, otherVariant);
				break;

			default:
				scene = std::make_unique<Scene>(this, &m_actors, key, sceneNum, isFuture, otherVariant);
				break;
			}

			scene->InitializeSceneFlags();

			return scene;
		}

		void SetPlayerParty(Player* player, std::shared_ptr<Party> party);

	protected:
		virtual void onClientConnected(ClientConnection* connection);

		virtual bool onHandshake(ClientConnection* connection, ByteStream& packet, std::string* error);

		virtual void onPlayerAuthenticated(Player* player);

		virtual void onIncomingData(Player* player, ByteStream& data);

		virtual void onClientDisconnected(ClientConnection* connection);

		virtual void onPlayerDisconnected(Player* player);

		void SendToAll(const ByteStream& payload, Player* except = nullptr);
		void WritePlayerEntry(ByteStream& packet, Player* player);

		void SendPlayerListTo(Player* target);

		void BroadcastPlayerJoined(Player* player);

		void BroadcastPlayerLeft(Player* player);

		void BroadcastPlayerListEntryUpdate(Player* player);
		void ClearPartyScenes(uint32_t partyID);
		void ClearPartyScene(Scene* scene);

	private:
		void RemovePlayerByGUID(uint64_t guid);

		void acceptPendingConnections();

		void pollConnections();
		void LoadConfig();

		bool serviceNewConnection(ClientConnection* connection);

		void servicePlayer(Player* player);

		void reapConnections();
		bool CanSpawnActor(Player* player, int spawnerNetID, int actorID, int parentID, int params);
		int m_port;
		SoxHandle m_listenSocket;
		unsigned int m_daySpeed = 320 / 2;
		unsigned int m_nightSpeed = 640 / 2;

		unsigned int m_worldTime = 0x8000;
		std::chrono::steady_clock::time_point m_lastTimeTick = std::chrono::steady_clock::now();

		void tickWorldTime();

		bool m_running;

		std::vector<std::unique_ptr<ClientConnection>> m_newConnections;

		std::unordered_map<int, std::unique_ptr<Player>> m_players;

		std::unordered_map<uint64_t, std::unique_ptr<Scene>> m_scenes;

		ActorRegistry m_actors;
		std::vector<std::pair<std::string, std::string>> m_skins;
	};
}

#endif
