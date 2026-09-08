#ifndef JYAGOROIWACONTROLLERH
#define JYAGOROIWACONTROLLERH

#include "../AbstractActorController.hpp"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Jya_Goroiwa/z_bg_jya_goroiwa.h"

void BgJyaGoroiwa_Wait(BgJyaGoroiwa*, PlayState* play);
void BgJyaGoroiwa_Move(BgJyaGoroiwa*, PlayState* play);
void BgJyaGoroiwa_UpdateRotation(BgJyaGoroiwa*);
void BgJyaGoroiwa_UpdateCollider(BgJyaGoroiwa*);
}

namespace ZeldaOnline {

class JyaGoroiwaController : public AbstractActorController {
  public:
    using AbstractActorController::AbstractActorController;

    BgJyaGoroiwa* Typed() const {
        return reinterpret_cast<BgJyaGoroiwa*>(m_actor);
    }

  protected:
    static constexpr u8 ID_UNKNOWN = 0xFF;

    using GoroiwaActionFunc = void (*)(BgJyaGoroiwa*, PlayState*);
    static const GoroiwaActionFunc* ActionTable(size_t* count) {
        static const GoroiwaActionFunc sTable[] = {
            BgJyaGoroiwa_Move,
            BgJyaGoroiwa_Wait,
        };
        *count = sizeof(sTable) / sizeof(sTable[0]);
        return sTable;
    }

    u8 CurrentActionIndex() const {
        size_t count;
        const GoroiwaActionFunc* table = ActionTable(&count);
        for (size_t i = 0; i < count; i++)
            if (table[i] == Typed()->actionFunc)
                return (u8)(i);
        return ID_UNKNOWN;
    }

    u8 CurrentColliderRoles() const {
        BgJyaGoroiwa* rock = Typed();
        u8 roles = COLL_OC;
        if (rock->collider.base.atFlags & AT_ON)
            roles |= COLL_AT;
        return roles;
    }

    enum {
        PROP_ACTION = PROP_CUSTOM_START,
        PROP_STATE,
        PROP_COLL_ROLES,
    };

    void BuildCustomProperties(ByteStream& out) override {
        BgJyaGoroiwa* rock = Typed();

        PackProperty(PROP_ACTION, PackedUInt1(CurrentActionIndex()), out);
        PackProperty(PROP_STATE,
                     ByteStream() << PackedFloat4(rock->speedFactor) << PackedFloat4(rock->yOffsetSpeed)
                                  << PackedInt2(rock->hasHit) << PackedInt2(rock->waitTimer),
                     out);
        PackProperty(PROP_COLL_ROLES, PackedUInt1(CurrentColliderRoles()), out);

        BuildStandardExtendedProperty(PROP_VELOCITY, out);
        BuildStandardExtendedProperty(PROP_WORLD_ROT, out);
        BuildStandardExtendedProperty(PROP_SHAPE_YOFFSET, out);
        BuildStandardExtendedProperty(PROP_FLAGS, out);
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override {
        BgJyaGoroiwa* rock = Typed();

        switch (index) {
            case PROP_ACTION: {
                u8 id = (u8)(data.Read<PackedUInt1>().value());
                size_t count;
                const GoroiwaActionFunc* table = ActionTable(&count);
                if (id < count)
                    rock->actionFunc = table[id];
                break;
            }
            case PROP_STATE:
                rock->speedFactor = data.Read<PackedFloat4>().value();
                rock->yOffsetSpeed = data.Read<PackedFloat4>().value();
                rock->hasHit = (s16)(data.Read<PackedInt2>().value());
                rock->waitTimer = (s16)(data.Read<PackedInt2>().value());
                break;
            case PROP_COLL_ROLES:
                m_roles = (u8)(data.Read<PackedUInt1>().value());
                break;
            default:
                return false;
        }
        return true;
    }

    void UpdatePuppet(PlayState* play) override {
        BgJyaGoroiwa* rock = Typed();
        Player* player = GET_PLAYER(play);

        if (player->stateFlags1 &
            (PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE))
            return;


        if (rock->collider.base.atFlags & AT_HIT) {
            ClaimLeadership(CLAIM_REASON_NOW);
            UpdateLeader(play);
            return;
        }

        BgJyaGoroiwa_UpdateRotation(rock);
        BgJyaGoroiwa_UpdateCollider(rock);

        if (m_roles != 0)
            RegisterColliderBase(play, &rock->collider.base, m_roles);

        if (rock->actionFunc == BgJyaGoroiwa_Move)
            Audio_PlayActorSound2(&rock->actor, NA_SE_EV_BIGBALL_ROLL - SFX_FLAG);
    }

  private:
    u8 m_roles = 0;
};

} // namespace ZeldaOnline

#endif
