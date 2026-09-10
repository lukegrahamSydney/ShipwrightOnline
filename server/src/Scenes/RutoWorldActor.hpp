#ifndef RUTOWORLDACTORH
#define RUTOWORLDACTORH

#include "../WorldActor.hpp"

namespace ZeldaOnline
{
    class JabuScene;
    class RutoWorldActor : public WorldActor
    {
    private:
        JabuScene* m_jabuScene;

        static constexpr int kVanillaParamsMask = 0x00FF;
        static constexpr int kHasMetRutoBit = 0x0100;

    public:
        RutoWorldActor(JabuScene* jabuScene, int networkID, Scene* scene, Room* room, int actorID, int params,
            float x, float y, float z,
            short rotX, short rotY, short rotZ, float homeX, float homeY, float homeZ,
            short homeRotX, short homeRotY, short homeRotZ, int parentID)
            : WorldActor(networkID, scene, room, actorID, params, x, y, z, rotX, rotY, rotZ, homeX,
                homeY, homeZ, homeRotX, homeRotY, homeRotZ, parentID),
            m_jabuScene(jabuScene)
        {
            printf("RUTO CREATED\n");
        }

        ~RutoWorldActor() override;

        int VanillaParams() const {
            return Params() & kVanillaParamsMask;
        }
        bool HasMetRutoAlready() const {
            return (Params() & kHasMetRutoBit) != 0;
        }
    };
}

#endif
