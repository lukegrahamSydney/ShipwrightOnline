#ifndef IDMANAGERH
#define IDMANAGERH

#include <queue>
#include <unordered_set>

namespace ZeldaOnline
{
	class IDManager
	{
	private:
		int m_nextID;
		int m_max;
		std::queue<int> m_unused;
		std::unordered_set<int> m_active;

	public:
		IDManager(int startID, int max) :
			m_nextID(startID), m_max(max) {
		}

		int newID()
		{
			int id;
			if (!m_unused.empty())
			{
				id = m_unused.front();
				m_unused.pop();
			}
			else
			{
				if (m_nextID > m_max)
					return -1;
				id = m_nextID++;
			}
			m_active.insert(id);
			return id;
		}

		void releaseID(int id)
		{
			if (m_active.erase(id) == 0)
				return;
			m_unused.push(id);
		}

		bool isActive(int id) const {
			return m_active.count(id) != 0;
		}

		int activeCount() const {
			return (int)(m_active.size());
		}

		void reset(int startID, int max)
		{
			std::queue<int>().swap(m_unused);
			m_active.clear();
			m_nextID = startID;
			m_max = max;
		}
	};
}

#endif
