#ifndef PLAYERH
#define PLAYERH

#include <memory>
#include <unordered_set>

#include "AbstractActor.hpp"
#include "ClientConnection.hpp"
#include "ByteStream.hpp"
#include "Party.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"

namespace ZeldaOnline
{
	class Scene;
	class Room;

	class Player : public AbstractActor
	{
	public:
		explicit Player(int id) : AbstractActor(id, 0) {}

		Player(int id, std::unique_ptr<ClientConnection> connection)
			: AbstractActor(id, 0), m_connection(std::move(connection))
		{
		}

		NetActorType Type() const override {
			return NetActorType::PLAYER;
		}

		Player(const Player&) = delete;
		Player& operator=(const Player&) = delete;

		Scene* CurrentScene() const {
			return m_currentScene;
		}
		void SetCurrentScene(Scene* scene) {
			m_currentScene = scene;
		}

		Room* CurrentRoom() const {
			return m_currentRoom;
		}
		void SetCurrentRoom(Room* room) {
			m_currentRoom = room;
		}

		const ByteStream& Appearance() const {
			return m_appearance;
		}

		void SetAppearance(const ByteStream& appearance) {
			m_appearance = appearance;
			m_age = m_appearance.Read<PackedUInt1>().value();
			auto skin = m_appearance.ReadString(m_appearance.Read<PackedUInt1>().value());
			SetNickName(m_appearance.ReadString(m_appearance.Read<PackedUInt1>().value()));
			m_appearance.RewindRead();
		}

		std::unordered_set<Room*>& CreatedRooms() {
			return m_createdRooms;
		}
		void AddCreatedRoom(Room* room) {
			m_createdRooms.insert(room);
		}
		void ForgetCreatedRoom(Room* room) {
			m_createdRooms.erase(room);
		}

		ClientConnection* GetConnection() const {
			return m_connection.get();
		}

		uint8_t Age() const {
			return m_age;
		}

		bool IsBot() const {
			return m_connection == nullptr;
		}

		void SendPacket(const ByteStream& payload) {
			if (m_connection && m_connection->Connected())
				m_connection->SendPacket(payload);
		}

		void MarkForRemoval() {
			m_pendingRemoval = true;
		}
		bool PendingRemoval() const {
			return m_pendingRemoval;
		}

		bool IsPaused() const {
			return m_paused;
		}

		uint32_t PartyHash() const {
			return m_partyScenes ? PartyGetID() : 0U;
		}

		void SetPaused(bool paused) {
			m_paused = paused;
		}

		void SetNickName(const std::string& name) {
			m_nickName = name;
		}

		const std::string NickName() const {
			return m_nickName;
		}

		uint32_t PartyGetID() const
		{
			return m_party != nullptr ? m_party->ID() : 0;
		}

		const std::shared_ptr<Party>& GetParty() const
		{
			return m_party;
		}

		void SetParty(std::shared_ptr<Party> party)
		{
			m_party = std::move(party);
		}

		void SetPartyScenes(bool enabled) {
			m_partyScenes = enabled;
		}

		bool MarkInitPlayerList() {
			bool wasInit = m_initedPlayerList;
			m_initedPlayerList = true;
			return !wasInit;
		}

		bool InitedPlayerList() const {
			return m_initedPlayerList;
		}

		void SetEntranceID(int entranceID) {
			m_entranceID = entranceID;
		}

		int EntranceID() const {
			return m_entranceID;
		}

		void TeleportTo(int entranceID, int sceneNum, int roomNum, float x, float y, float z) {
			if (m_connection)
			{
				m_connection->SendPacket(newPacket(SERVER_PACKET_TELEPORT_PLAYER) << PackedInt4(entranceID) << PackedUInt2(sceneNum)
					<< PackedUInt1(roomNum) << PackedFloat4(x) << PackedFloat4(y) << PackedFloat4(z));
			}
		}

	private:
		unsigned int m_partyHash = 0;
		bool m_paused = false;
		bool m_partyScenes = false;
		bool m_initedPlayerList = false;
		int m_entranceID = -1;
		uint8_t m_age = 1;

		std::unique_ptr<ClientConnection> m_connection;
		std::shared_ptr<Party> m_party;
		ByteStream m_appearance;
		Scene* m_currentScene = nullptr;
		Room* m_currentRoom = nullptr;
		bool m_pendingRemoval = false;
		std::string m_nickName = "Player";
		std::unordered_set<Room*> m_createdRooms;
	};
}

#endif
