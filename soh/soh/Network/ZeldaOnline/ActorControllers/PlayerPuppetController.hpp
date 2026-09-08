#ifndef PLAYERPUPPETCONTROLLERH
#define PLAYERPUPPETCONTROLLERH

#include "../AbstractActorController.hpp"
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
        PPROP_HORSE_STATE_FLAGS,
        PPROP_HORSE_SPEEDXZ,
        PPROP_HELD_ACTOR,
        PPROP_HITTING,
        PPROP_BOOMERANG,
        PPROP_HOOKSHOT,
        PPROP_TUNIC_COLOUR,
        PPROP_ON_PLATFORM,
        PPROP_NICKNAME,
        PPROP_PAUSED,
        PPROP_TUNIC,
        PPROP_BOOTS,
        PPROP_SHIELD,
        PPROP_BUTTONITEM0,
        PPROP_SOUND_FREQUENCY,
        PPROP_FISHING_ROD,
        PPROP_MASK,
        PPROP_SPEEDXZ, 
    };

    PlayerPuppetController(Actor* actor, int networkID, int sceneKey, const ByteStream& appearanceBlob)
        : AbstractActorController(actor, networkID, sceneKey, -1, false) {
        ApplyAppearance(appearanceBlob);
    }

    ~PlayerPuppetController() override {
        KillHorse();
        KillBoomerang();
        KillHookshot();
    }

    void OnActorInit() override {
        Player* player = reinterpret_cast<Player*>(m_actor);

        m_itemAction = player->itemAction;
        m_heldItemAction = player->heldItemAction;
        m_buttonItem0 = player->heldItemId;
        m_modelGroup = player->modelGroup;
        m_currentTunic = player->currentTunic;
        m_currentBoots = player->currentBoots;
        m_currentShield = player->currentShield;
        m_stateFlags1 = player->stateFlags1;
        m_stateFlags2 = player->stateFlags2;
        m_movementFlags = player->skelAnime.movementFlags;
        m_prevTransl = player->skelAnime.prevTransl;
        m_upperLimbRot = player->upperLimbRot;
        m_heldGetItemId = 0;
        m_pos = player->actor.world.pos;

        for (int i = 0; i < 24; i++) {
            m_jointTable[i] = player->skelAnime.jointTable[i];
        }
    }

    static void PuppetInit(Actor* actor, PlayState* play);
    static void PuppetUpdate(Actor* actor, PlayState* play);
    static void PuppetDraw(Actor* actor, PlayState* play);
    static void PuppetDestroy(Actor* actor, PlayState* play);

    f32 m_ocarinaModulator = 0.0f;
    s8 m_ocarinaBend = 0;
    u8 m_ocarinaNote = 0xFF;

    bool IsPlayer() const override {
        return true;
    }

    int RealRoomIndex() const {
        return m_realRoomIndex;
    }

    void ApplyAppearance(const ByteStream& blob);

    Actor* SatelliteHorse() const {
        return m_horse;
    }

    static void BuildLocalPlayerProperties(Player* player, const std::string& nickName, ByteStream& out);
    void ClearSatelliteHorse() {
        m_horse = nullptr;
    }

    void DrawSwordQuadDebug(PlayState* play);

    const Color_RGB8& GetTunicColour() const {
        return m_tunicColour;
    }

    int LinkAge() const {
        return m_linkAge;
    }

    const std::string& SkinReference() const {
        return m_skinRef;
    }

    const std::string& SkinName() const {
        return m_skinName;
    }
    float SoundFrequencyMultiplier() const {
        return m_soundFreqMultiplier;
    }

    bool HasFishingLure() const {
        return m_fishing.m_rodCastState >= 0;
    }

    Vec3f_& FishingLurePos() {
        return m_fishing.m_lurePos;
    }

  protected:
    void UpdatePuppet(PlayState* play) override;

    void KillHorse();
    void KillHookshot();

    void KillBoomerang() {
        if (m_boomerang != nullptr) {
            Actor_Kill(m_boomerang);
            m_boomerang = nullptr;
        }
    }

    bool ApplyCustomProperty(unsigned int index, ByteStream& data, unsigned int propLen) override;
    void OnPropertiesApplied(u64 changed) override;

    void DrawFishingRod(PlayState* play);
    void DrawFishingLureAndLine(PlayState* play);

  private:
    Vec3f m_pos{};
    u8 m_linkAge = 0;

    s8 m_currentTunic = 0;
    s8 m_currentBoots = 0;
    s8 m_currentShield = 0;
    u8 m_buttonItem0 = 0;
    s32 m_modelGroup = 0;

    Vec3s m_jointTable[24]{};
    Vec3s m_upperLimbRot{};
    u8 m_movementFlags = 0;
    Vec3s m_prevTransl{};

    u32 m_stateFlags1 = 0;
    u32 m_stateFlags2 = 0;
    u8 m_csAction = 0;
    s8 m_itemAction = 0;
    s8 m_heldItemAction = 0;
    s32 m_invincibilityTimer = 0;
    s16 m_heldGetItemId = 0;
    s32 m_leftHandType = 0;
    f32 m_unk_85C = 0.0f;
    s32 m_actionVar1 = 0;
    s16 m_unk860 = 0;
    u8 m_modelLeftHandType = 0;
    u8 m_modelRightHandType = 0;
    u8 m_modelSheathType = 0;

    Vec3s m_unk_3BC{};
    u8 m_riding = 0;
    u8 m_horseAnimIndex = 0;
    s16 m_horseYaw = 0;
    u8 m_horseType = 0;
    Vec3f m_horsePos{};
    Vec3s m_horseRot{};
    u8 m_horseMounted = 0;
    u8 m_horseAnimFrame = 0;
    u32 m_heldActorId = 0;
    u8 m_currentMask = 0;
    BunnyEarKinematics m_bunnyEarKinematics{};
    u8 m_horseStateFlags = 0;
    float m_horseSpeedXZ = 0.0f;
    u8 m_onPlatform = 0;

    Actor* m_horse = nullptr;
    Actor* m_boomerang = nullptr;
    Actor* m_hookshot = nullptr;

    u8 m_pendingHorsePresent = 0;

    std::string m_nickName = "Player";
    bool m_paused = false;
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
    std::string m_skinName = "";
    std::string m_skinRef = "";
    float m_soundFreqMultiplier = 1.0f;

    struct {
        s16 m_rodCastState = -1;
        f32 m_rodBendRotY = 0.0f; // sRodBendRotY
        f32 m_rodBendX = 0.0f;    // D_80B7A6AC
        f32 m_rodCastBend = 0.0f; // D_80B7A6B8
        f32 m_rodHookBend = 0.0f; // D_80B7A6BC
        f32 m_rodHitBend = 0.0f;  // D_80B7A6C0
        f32 m_rodStickX = 0.0f;   // player->unk_858
        f32 m_rodStickY = 0.0f;   // player->unk_85C
        Vec3f m_rodTipPos{};

        Vec3f m_lurePos{};
        Vec3f m_lureRot{};
        Vec3f m_lureHookRefPos[2]{};
        f32 m_lure1Rotate = 0.0f;
        f32 m_lurePosZOffset = 0.0f;
        f32 m_lineScale = 0.0f;
        u8 m_lureEquipped = 0;
    } m_fishing;
    void UpdateSwordHitbox(PlayState* play);
};
} // namespace ZeldaOnline
#endif