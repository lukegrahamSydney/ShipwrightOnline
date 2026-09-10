#ifndef JABUROOMH
#define JABUROOMH

#include "../Room.hpp"
#include "../ActorID.hpp"
#include "JabuScene.hpp"
#include "RutoWorldActor.hpp"

namespace ZeldaOnline
{
    class JabuRoom : public Room
    {
    private:
        JabuScene* m_jabuScene;

    public:

        JabuRoom(JabuScene* scene, ActorRegistry* registry, uint64_t sceneKey, int roomIndex)
            : Room(scene, registry, sceneKey, roomIndex), m_jabuScene(scene)
        {
            printf("JABU ROOM CREATED\n");
        }

        WorldActor* SpawnActor(int actorID, int params, float x, float y, float z, short rotX, short rotY,
            short rotZ, float homeX, float homeY, float homeZ,
            short homeRotX, short homeRotY, short homeRotZ, int parentID, Player* except = nullptr, Player* owner = nullptr) override
        {
            if (actorID != ActorID::ACTOR_EN_RU1)
                return Room::SpawnActor(actorID, params, x, y, z, rotX, rotY, rotZ, homeX, homeY, homeZ, homeRotX, homeRotY, homeRotZ, parentID, except, owner);

            if (m_jabuScene->GetRuto() == nullptr)
            {
                if (m_jabuScene->RutoAtBoss())
                    return nullptr;

                auto ruto = Room::SpawnActor(actorID, params, x, y, z, rotX, rotY, rotZ, homeX, homeY, homeZ, homeRotX, homeRotY, homeRotZ, parentID, except, owner);
                m_jabuScene->SetRuto(static_cast<RutoWorldActor*>(ruto));
                return ruto;
            }

            return nullptr;
        }
    };
}

#endif
