#ifndef KAKARIKOSCENEH
#define KAKARIKOSCENEH

#include "../Scene.hpp"
#include "../ActorID.hpp"

namespace ZeldaOnline
{
    class KakarikoScene : public Scene
    {
    public:
        using Scene::Scene;


        static constexpr float PEN_CENTRE_X = 330.0f;
        static constexpr float PEN_CENTRE_Z = 1610.0f;
        static constexpr float PEN_HALF_X = 90.0f;
        static constexpr float PEN_HALF_Z = 190.0f;

        static constexpr float PEN_RESTORE_X = 300.0f;
        static constexpr float PEN_RESTORE_Y = 100.0f;
        static constexpr float PEN_RESTORE_Z = 1530.0f;

        static bool IsInPen(float x, float z)
        {
            return fabsf(x - PEN_CENTRE_X) < PEN_HALF_X && fabsf(z - PEN_CENTRE_Z) < PEN_HALF_Z;
        }

        static int CuccoIndexForSpawn(float x, float z)
        {
            struct PenPos { float x, z; };
            static const PenPos sKakarikoPosList[] = {
                { -1697.0f, 870.0f }, { 57.0f, -673.0f },  { 796.0f, 1639.0f }, { 1417.0f, 169.0f },
                { -60.0f, -46.0f },   { -247.0f, 854.0f }, { 1079.0f, -47.0f },
            };

            for (int i = 0; i < (int)(sizeof(sKakarikoPosList) / sizeof(sKakarikoPosList[0])); i++)
            {
                if (fabsf(x - sKakarikoPosList[i].x) < 40.0f && fabsf(z - sKakarikoPosList[i].z) < 40.0f)
                    return i;
            }
            return -1;
        }

        static uint16_t CuccoBit(int index)
        {
            return (uint16_t)(0x0200 << index);
        }

        bool IsCuccoPenned(int index) const
        {
            return index >= 0 && (m_pennedCuccos & CuccoBit(index)) != 0;
        }

        void StorePennedCuccos(const std::vector<WorldActor*>& actors)
        {
            m_pennedCuccos = 0;
            for (WorldActor* actor : actors)
            {
                if (actor->ActorID() != ActorID::ACTOR_EN_NIW)
                    continue;

                int index = CuccoIndexForSpawn(actor->HomeX(), actor->HomeZ());
                if (index < 0)
                    continue;

                if (IsInPen(actor->X(), actor->Z()))
                {
                    m_pennedCuccos |= CuccoBit(index);
                }
            }

            if (m_pennedCuccos == 0xFE00) {
                m_pennedCuccos = 0;
            }
        }

        uint16_t m_pennedCuccos = 0;
    protected:
        Room* MakeRoom(int roomIndex) override;

    private:

    };
}

#endif
