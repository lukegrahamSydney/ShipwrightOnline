#ifndef BOMBCHUCONTROLLERH
#define BOMBCHUCONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_En_Bom_Chu/z_en_bom_chu.h"

void EnBomChu_WaitForRelease(EnBomChu* chu, PlayState* play);
void EnBomChu_Move(EnBomChu* chu, PlayState* play);
void EnBomChu_WaitForKill(EnBomChu* chu, PlayState* play);
}

namespace ZeldaOnline {

class BombchuController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    EnBomChu* Typed() const {
        return reinterpret_cast<EnBomChu*>(m_actor);
    }



    void OnBecomeLeader() override {
        if (IsCreator()) {
            return; // ours from the start: normal
        }

        if (m_actor != nullptr && m_actor->update != nullptr) {
            Actor_Kill(m_actor);
        }
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using ChuActionFunc = void (*)(EnBomChu*, PlayState*);
    static const ChuActionFunc* ActionTable(size_t* count) {
        static const ChuActionFunc sTable[] = {
            EnBomChu_WaitForRelease,
            EnBomChu_Move,
            EnBomChu_WaitForKill,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const ChuActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_TIMER,
        PROP_AXIS,
    };

    void BuildCustomProperties(ByteStream& out) override {
        EnBomChu* chu = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_TIMER, PackedInt2(chu->timer), out);


        PackProperty(PROP_AXIS,
                     ByteStream() << PackedFloat4(chu->axisForwards.x) << PackedFloat4(chu->axisForwards.y)
                                  << PackedFloat4(chu->axisForwards.z) << PackedFloat4(chu->axisUp.x)
                                  << PackedFloat4(chu->axisUp.y) << PackedFloat4(chu->axisUp.z)
                                  << PackedFloat4(chu->axisLeft.x) << PackedFloat4(chu->axisLeft.y)
                                  << PackedFloat4(chu->axisLeft.z),
                     out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_GRAVITY, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        EnBomChu* chu = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const ChuActionFunc* table = ActionTable(&count);
                if (id < count)
                    chu->actionFunc = table[id];
                break;
            }
            case PROP_TIMER:
                chu->timer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_AXIS:
                chu->axisForwards.x = data.Read<PackedFloat4>().value();
                chu->axisForwards.y = data.Read<PackedFloat4>().value();
                chu->axisForwards.z = data.Read<PackedFloat4>().value();
                chu->axisUp.x = data.Read<PackedFloat4>().value();
                chu->axisUp.y = data.Read<PackedFloat4>().value();
                chu->axisUp.z = data.Read<PackedFloat4>().value();
                chu->axisLeft.x = data.Read<PackedFloat4>().value();
                chu->axisLeft.y = data.Read<PackedFloat4>().value();
                chu->axisLeft.z = data.Read<PackedFloat4>().value();
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        EnBomChu* chu = Typed();

        Collider_UpdateSpheres(0, &chu->collider);
        RegisterColliderBase(play, &chu->collider.base, COLL_AC | COLL_OC);
    }
};

} // namespace ZeldaOnline

#endif