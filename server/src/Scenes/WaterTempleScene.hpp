#ifndef WATERTEMPLESCENEH
#define WATERTEMPLESCENEH

#include "../Scene.hpp"

namespace ZeldaOnline
{


    class WaterTempleScene : public Scene {
    public:
        using Scene::Scene;

    protected:
        void InitializeSceneFlags() override {
            //30 means water is at top. This scene flag is initialized here on a brand new game save
            m_sceneFlags = 1u << 30;

        }
    };

}

#endif
