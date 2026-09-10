#ifndef MISCSCENESH
#define MISCSCENESH

#include "../Scene.hpp"
#include "../Flags.hpp"

namespace ZeldaOnline
{
    class SpiritTempleBossScene : public Scene {
    public:
        using Scene::Scene;

        void InitializeSceneFlags() override {
            SetEventINFFlag(EVENTCHKINF_FINISHED_NABOORU_BATTLE, false);
            SetEventINFFlag(EVENTCHKINF_NABOORU_ORDERED_TO_FIGHT_BY_TWINROVA, false);
        }
    };
};
#endif
