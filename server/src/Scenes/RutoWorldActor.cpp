#include "RutoWorldActor.hpp"
#include "JabuScene.hpp"

namespace ZeldaOnline
{

    RutoWorldActor::~RutoWorldActor()
    {
        if (m_jabuScene != nullptr)
            m_jabuScene->SetRuto(nullptr);
    }
}
