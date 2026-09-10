#include "KakarikoScene.hpp"
#include "KakarikoRoom.hpp"

namespace ZeldaOnline
{
    Room* KakarikoScene::MakeRoom(int roomIndex)
    {
        return new KakarikoRoom(this, m_registry, m_sceneKey, roomIndex);
    }
}
