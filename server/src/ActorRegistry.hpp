#ifndef ACTORREGISTRYH
#define ACTORREGISTRYH

#include <unordered_map>

#include "AbstractActor.hpp"
#include "IDManager.hpp"

namespace ZeldaOnline
{
	class ActorRegistry
	{
	private:
		IDManager m_ids{ 1, 0xFFFF };

		std::unordered_map<int, AbstractActor*> m_actors;

	public:
		int NewID() {
			return m_ids.newID();
		}

		void Add(AbstractActor* actor) {
			m_actors[actor->NetworkID()] = actor;
		}

		void Remove(AbstractActor* actor) {
			m_actors.erase(actor->NetworkID());
			m_ids.releaseID(actor->NetworkID());
		}

		AbstractActor* Get(int networkID) {
			auto it = m_actors.find(networkID);
			return it != m_actors.end() ? it->second : nullptr;
		}

		int ActiveCount() const {
			return (int)(m_actors.size());
		}
	};
}

#endif
