#ifndef BOMBCHUALLEYH
#define BOMBCHUALLEYH

#include "../Scene.hpp"
#include "../Room.hpp"

namespace ZeldaOnline
{

    class BombchuAlleyRoom : public Room {
    public:
        using Room::Room;

        bool CanSyncTempFlag(int flag) const override {
            return false;
        }
    };

    class BombchuAlleyScene : public Scene {
    public:
        using Scene::Scene;

        bool CanSyncSceneFlag(int flag) const override {
            return false;
        }

        bool CanSyncTempFlag(int flag) const override {
            return false;
        }
    protected:
        Room* MakeRoom(int roomIndex) override {
            return new BombchuAlleyRoom(this, m_registry, m_sceneKey, roomIndex);
        }
    };

}

#endif
