#include "JabuScene.hpp"
#include "JabuRoom.hpp"

namespace ZeldaOnline
{
    Room* JabuScene::MakeRoom(int roomIndex)
    { 
        return new JabuRoom(this, m_registry, m_sceneKey, roomIndex);
    }

}
