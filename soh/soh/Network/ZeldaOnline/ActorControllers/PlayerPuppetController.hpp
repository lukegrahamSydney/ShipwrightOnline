#ifndef PLAYERPUPPETCONTROLLERH
#define PLAYERPUPPETCONTROLLERH

#include "../AbstractActorController.hpp"
#include "../PlayerPuppet.hpp"
#include "../PacketTypes.hpp"

#include <cstring>

namespace ZeldaOnline {

class PlayerPuppetController : public AbstractActorController {
  public:

    enum PlayerProperties {
        PPROP_MOVEMENT_FLAGS = PROP_CUSTOM_START,
        PPROP_REAL_ROOM_INDEX,
        PPROP_PREV_TRANSL,
        PPROP_UPPER_LIMB_ROT,
        PPROP_UNK_3BC,
        PPROP_POSE,
        PPROP_STATE_FLAGS1,
        PPROP_STATE_FLAGS2,
        PPROP_ITEM_ACTION,
        PPROP_HELD_ITEM_ACTION,
        PPROP_MODEL_GROUP,
        PPROP_LEFT_HAND_TYPE,
        PPROP_RIGHT_HAND_TYPE,
        PPROP_SHEATH_TYPE,
        PPROP_INVINCIBILITY,
        PPROP_HELD_GET_ITEM,
        PPROP_CS_ACTION,
        PPROP_HELD_ITEM_SCALE,
        PPROP_STICK_FLAME_TIMER,
        PPROP_HORSE_PRESENT,
        PPROP_HORSE_MOUNTED,
        PPROP_HORSE_ANIM,
        PPROP_HORSE_FRAME,
        PPROP_HORSE_POS_X,
        PPROP_HORSE_POS_Y,
        PPROP_HORSE_POS_Z,
        PPROP_HORSE_ROT_X,
        PPROP_HORSE_ROT_Y,
        PPROP_HORSE_ROT_Z,
        PPROP_HELD_ACTOR,
        PPROP_HITTING,
        PPROP_BOOMERANG,
        PPROP_TUNIC_COLOUR,
        PPROP_ON_PLATFORM,
    };

    PlayerPuppetController(Actor* actor, int networkID, int sceneKey, const ByteStream& appearanceBlob)
        : AbstractActorController(actor, networkID, sceneKey, -1, false) {
        Player* player = reinterpret_cast<Player*>(actor);

        std::memset(&m_state, 0, sizeof(m_state));
        m_state.itemAction = player->itemAction;
        m_state.heldItemAction = player->heldItemAction;
        m_state.buttonItem0 = player->heldItemId;
        m_state.modelGroup = player->modelGroup;
        m_state.currentTunic = player->currentTunic;
        m_state.currentBoots = player->currentBoots;
        m_state.currentShield = player->currentShield;
        m_state.stateFlags1 = player->stateFlags1;
        m_state.stateFlags2 = player->stateFlags2;
        m_state.movementFlags = player->skelAnime.movementFlags;
        m_state.prevTransl = player->skelAnime.prevTransl;
        m_state.upperLimbRot = player->upperLimbRot;
        m_state.heldGetItemId = 0;
        m_state.linkAge = 1;
        m_state.pos = player->actor.world.pos;

        for (int i = 0; i < 24; i++) {
            m_state.jointTable[i] = player->skelAnime.jointTable[i];
        }

        ApplyAppearance(appearanceBlob);

    }

    ~PlayerPuppetController() override {
        KillHorse();
        KillBoomerang();
    }

    PlayerPuppetState* PuppetState() {
        return &m_state;
    }

    bool IsPlayer() const override {
        return true;
    }

    int RealRoomIndex() const {
        return m_realRoomIndex;
    }

    void ApplyAppearance(const ByteStream& blob) {
        ByteStream data = blob;
        if (data.BytesLeft() < 6) {
            return;
        }
        m_state.linkAge = (u8)(data.Read<PackedUInt1>().value());
        m_state.currentTunic = (u8)(data.Read<PackedUInt1>().value());
        m_state.currentBoots = (u8)(data.Read<PackedUInt1>().value());
        m_state.currentShield = (u8)(data.Read<PackedUInt1>().value());
        m_state.buttonItem0 = (u8)(data.Read<PackedUInt1>().value());
        unsigned int nameLen = data.Read<PackedUInt1>().value();
        if (nameLen > 16 || data.BytesLeft() < nameLen) {
            nameLen = 0;
        }
        if (nameLen > 0) {
            std::string name = data.ReadString(nameLen);
            std::memcpy(m_state.name, name.c_str(), nameLen);
        }
        m_state.name[nameLen] = '\0';
    }

    Actor* SatelliteHorse() const {
        return m_horse;
    }

    static void BuildLocalPlayerProperties(Player* player, Actor* horseActor, ByteStream& out);
    void ClearSatelliteHorse() {
        m_horse = nullptr;
    }

    void DrawSwordQuadDebug(PlayState* play);

    const Color_RGB8& GetTunicColour() const {
        return m_tunicColour;
    }
  protected:
    void UpdatePuppet(PlayState* play) override;

    void KillHorse();

    void KillBoomerang() {
        if (m_boomerang != nullptr) {
            Actor_Kill(m_boomerang);
            m_boomerang = nullptr;
        }
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override;
    void OnPropertiesApplied(u64 changed) override;

  private:
    PlayerPuppetState m_state;
    Actor* m_horse = nullptr;
    Actor* m_boomerang = nullptr;

    u8 m_pendingHorsePresent = 0;

    ColliderQuad m_swordQuad{};
    bool m_swordQuadInit = false;
    bool m_hitting = false;
    Vec3f m_hittingTip{}, m_hittingBase{};
    u32 m_hittingDmgFlags = 0;
    u8 m_hittingDamage = 0;
    Vec3f m_meleePrevTip{}, m_meleePrevBase{};
    bool m_hasValidHitQuad = false;
    int m_realRoomIndex = 0;
    Color_RGB8 m_tunicColour{ 30, 105, 27 };

    void UpdateSwordHitbox(PlayState* play);

};
}
#endif
