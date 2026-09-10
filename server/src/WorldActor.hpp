#ifndef WORLDACTORH
#define WORLDACTORH

#include <chrono>
#include <unordered_map>

#include "AbstractActor.hpp"

#include <ctime>
#include "BytePacking.hpp"
#include "ByteStream.hpp"
#include "PacketTypes.hpp"
#include "Packet.hpp"

namespace ZeldaOnline
{
	class Room;
	class Scene;
	class Player;

	class WorldActor : public AbstractActor
	{
	private:
		static constexpr std::chrono::seconds DECLINE_LIFETIME{ 30 };

		int m_params = 0;
		int m_parentID = 0;
		unsigned int m_localID = 0U;

		Scene* m_scene = nullptr;
		Room* m_room = nullptr;

		Player* m_owner = nullptr;

		time_t m_lastOwnerChange = 0;

		float m_homeX = 0.0f, m_homeY = 0.0f, m_homeZ = 0.0f;
		short m_homerotX = 0, m_homerotY = 0, m_homerotZ = 0;

		bool    m_locked = false;
		Player* m_lockHolder = nullptr;
		bool    m_awaitingLeader = false;
		bool m_sentCreatedFlag = false;
		std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> m_declinedAssignment;

	public:
		WorldActor(int networkID, Scene* scene, Room* room, int actorID, int params,
			float x, float y, float z,
			short rotX, short rotY, short rotZ, float homeX, float homeY, float homeZ,
			short homeRotX, short homeRotY, short homeRotZ, int parentID) :
			AbstractActor(networkID, actorID), m_params(params), m_scene(scene), m_room(room), m_parentID(parentID)
		{
			SetPosition(x, y, z);
			SetRotation(rotX, rotY, rotZ);
			m_homeX = homeX;
			m_homeY = homeY;
			m_homeZ = homeZ;
			m_homerotX = homeRotX;
			m_homerotY = homeRotY;
			m_homerotZ = homeRotZ;
		}

		virtual ~WorldActor() {}
		NetActorType Type() const override {
			return NetActorType::WORLD_ACTOR;
		}

		void SetLocalID(unsigned int id) {
			m_localID = id;		//Local ID for the player who spawned this
		}

		unsigned int LocalID() const {
			return m_localID;
		}

		int ParentID() const {
			return m_parentID;
		}

		void ClearParent() {
			m_parentID = 0;
		}

		int Params() const {
			return m_params;
		}

		float HomeX() const { return m_homeX; }
		float HomeY() const { return m_homeY; }
		float HomeZ() const { return m_homeZ; }
		short HomeRotX() const { return m_homerotX; }
		short HomeRotY() const { return m_homerotY; }
		short HomeRotZ() const { return m_homerotZ; }

		Scene* OwningScene() const {
			return m_scene;
		}

		Room* OwningRoom() const {
			return m_room;
		}
		void SetOwningRoom(Room* room) {
			m_room = room;
		}

		void Broadcast(const ByteStream& payload, Player* except = nullptr) const;

		Player* Owner() const {
			return m_owner;
		}

		bool SetOwner(Player* owner) {
			bool wasLocked = m_locked;
			m_owner = owner;
			m_lastOwnerChange = time(nullptr);
			m_locked = false;
			m_lockHolder = nullptr;
			if (owner != nullptr)
				m_awaitingLeader = false;
			return wasLocked;
		}

		void ReassignOwner(Player* newOwner, Player* except = nullptr);

		time_t LastOwnerChange() const {
			return m_lastOwnerChange;
		}

		bool    IsLocked()   const { return m_locked; }
		Player* LockHolder() const { return m_lockHolder; }
		bool    AwaitingLeader() const { return m_awaitingLeader; }
		void    SetAwaitingLeader(bool v) { m_awaitingLeader = v; }
		void    SetLocked(bool locked, Player* holder) {
			m_locked = locked;
			m_lockHolder = locked ? holder : nullptr;
		}

		bool ClaimCreatorFlag() {
			if (m_sentCreatedFlag) {
				return false;
			}
			m_sentCreatedFlag = true;
			return true;
		}

		bool CreatorFlagClaimed() const {
			return m_sentCreatedFlag;
		}

		void RecordDeclinedAssignment(uint64_t guid) {
			const auto now = std::chrono::steady_clock::now();

			if (m_declinedAssignment.size() > 100)
			{
				for (auto it = m_declinedAssignment.begin(); it != m_declinedAssignment.end(); )
				{
					if ((now - it->second) >= DECLINE_LIFETIME)
						it = m_declinedAssignment.erase(it);
					else
						++it;
				}
			}

			m_declinedAssignment[guid] = now;
		}

		bool HasDeclinedAssignment(uint64_t guid) const {

			auto it = m_declinedAssignment.find(guid);
			if (it == m_declinedAssignment.end())
				return false;
			return (std::chrono::steady_clock::now() - it->second) < DECLINE_LIFETIME;
		}

		void ClearDeclinedAssignment(uint64_t guid) {
			m_declinedAssignment.erase(guid);
		}
	};
}

#endif
