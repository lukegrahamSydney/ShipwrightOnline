#ifndef ACTORORDERH
#define ACTORORDERH

#include <algorithm>
#include <memory>
#include <vector>

#include "WorldActor.hpp"

namespace ZeldaOnline
{
	//
	inline void InsertActorOrdered(std::vector<WorldActor*>& actors, WorldActor* actor)
	{
		const int parentID = actor->ParentID();

		if (parentID != 0)
		{
			auto parent = std::find_if(actors.begin(), actors.end(),
				[parentID](WorldActor* other) { return other->NetworkID() == parentID; });

			if (parent != actors.end())
			{
				auto pos = parent + 1;
				while (pos != actors.end() && (*pos)->ParentID() == parentID)
					++pos;

				actors.insert(pos, actor);
				return;
			}
		}

		actors.push_back(actor);
	}

	inline void EraseActorOrdered(std::vector<WorldActor*>& actors, int networkID)
	{
		actors.erase(std::remove_if(actors.begin(), actors.end(),
			[networkID](WorldActor* other) { return other->NetworkID() == networkID; }),
			actors.end());
	}

	inline WorldActor* FindActorOrdered(const std::vector<WorldActor*>& actors, int networkID)
	{
		auto it = std::find_if(actors.begin(), actors.end(),
			[networkID](WorldActor* other) { return other->NetworkID() == networkID; });

		return it != actors.end() ? *it : nullptr;
	}

	inline void InsertActorOrdered(std::vector<std::unique_ptr<WorldActor>>& actors,
		std::unique_ptr<WorldActor> actor)
	{
		const int parentID = actor->ParentID();

		if (parentID != 0)
		{
			auto parent = std::find_if(actors.begin(), actors.end(),
				[parentID](const std::unique_ptr<WorldActor>& other) { return other->NetworkID() == parentID; });

			if (parent != actors.end())
			{
				auto pos = parent + 1;
				while (pos != actors.end() && (*pos)->ParentID() == parentID)
					++pos;

				actors.insert(pos, std::move(actor));
				return;
			}
		}

		actors.push_back(std::move(actor));
	}

	inline void EraseActorOrdered(std::vector<std::unique_ptr<WorldActor>>& actors, int networkID)
	{
		actors.erase(std::remove_if(actors.begin(), actors.end(),
			[networkID](const std::unique_ptr<WorldActor>& other) {
				return other->NetworkID() == networkID;
			}),
			actors.end());
	}

	inline WorldActor* FindActorOrdered(const std::vector<std::unique_ptr<WorldActor>>& actors, int networkID)
	{
		auto it = std::find_if(actors.begin(), actors.end(),
			[networkID](const std::unique_ptr<WorldActor>& other) { return other->NetworkID() == networkID; });

		return it != actors.end() ? it->get() : nullptr;
	}
}

#endif