#ifndef KAKARIKOROOMH
#define KAKARIKOROOMH

#include "../Room.hpp"
#include "../ActorID.hpp"
#include "KakarikoScene.hpp"

namespace ZeldaOnline
{
    class KakarikoRoom : public Room
    {
    private:
        KakarikoScene* m_kakarikoScene;

    public:
        KakarikoRoom(KakarikoScene* scene, ActorRegistry* registry, uint64_t sceneKey, int roomIndex)
            : Room(scene, registry, sceneKey, roomIndex), m_kakarikoScene(scene)
        {
        }

        WorldActor* SpawnActor(int actorID, int params, float x, float y, float z, short rotX, short rotY,
            short rotZ, float homeX, float homeY, float homeZ,
            short homeRotX, short homeRotY, short homeRotZ, int parentID, Player* except = nullptr, Player* owner = nullptr) override
        {
            if (actorID == ActorID::ACTOR_EN_NIW &&
                m_kakarikoScene->IsCuccoPenned(KakarikoScene::CuccoIndexForSpawn(homeX, homeZ)))
            {
                x = KakarikoScene::PEN_RESTORE_X;
                y = KakarikoScene::PEN_RESTORE_Y;
                z = KakarikoScene::PEN_RESTORE_Z;
                params = 0;
            }

            return Room::SpawnActor(actorID, params, x, y, z, rotX, rotY, rotZ, homeX, homeY, homeZ, homeRotX, homeRotY, homeRotZ, parentID, except, owner);
        }

        void ClearActors() override;
    };
}

#endif
