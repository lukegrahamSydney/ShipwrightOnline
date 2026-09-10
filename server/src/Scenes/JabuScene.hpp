#ifndef JABUSCENEH
#define JABUSCENEH

#include "../Scene.hpp"
#include "../ActorID.hpp"

#include "RutoWorldActor.hpp"

namespace ZeldaOnline
{
    class JabuScene : public Scene
    {
    private:
        RutoWorldActor* m_ruto = nullptr;
        bool m_rutoAtBoss = false;

    public:
        using Scene::Scene;

        RutoWorldActor* GetRuto() const { return m_ruto; }
        void SetRuto(RutoWorldActor* ruto) { m_ruto = ruto; }

        bool CanSyncINFFlag(int flag, bool* persistant) const override
        {
            if (flag >= 0x140 && flag <= 0x147)
            {
                *persistant = true;
                return true;
            }
            return false;
        }

        bool CanOverrideSpawnAuthorize(int actorID, int params) override {
            return actorID == ActorID::ACTOR_EN_RU1;
        }

        bool RutoAtBoss() const {
            return m_rutoAtBoss;
        }

    protected:
        WorldActor* MakeWorldActor(int networkID, Room* room, int actorID, int params,
            float x, float y, float z,
            short rotX, short rotY, short rotZ, float homeX, float homeY, float homeZ,
            short homeRotX, short homeRotY, short homeRotZ, int parentID) override
        {
            if (actorID == ActorID::ACTOR_EN_RU1)
                return new RutoWorldActor(this, networkID, this, room, actorID, params,
                    x, y, z, rotX, rotY, rotZ, homeX, homeY, homeZ, homeRotX, homeRotY, homeRotZ, parentID);

            return Scene::MakeWorldActor(networkID, room, actorID, params, x, y, z, rotX, rotY, rotZ, homeX,
                homeY, homeZ, homeRotX, homeRotY, homeRotZ, parentID);
        }

        void InitializeSceneFlags() override {
            for (int i = 0x140; i <= 0x147; ++i)
                SetINFFlag(i, false);
            m_rutoAtBoss = false;
        }

        Room* MakeRoom(int roomIndex) override;
    };

}
#endif
