#include "KakarikoRoom.hpp"
#include "../OOTServer.hpp"

void ZeldaOnline::KakarikoRoom::ClearActors()
{
    if(!m_scene->Server()->IsNightTime())
        m_kakarikoScene->StorePennedCuccos(m_worldActors);
    Room::ClearActors();
}
