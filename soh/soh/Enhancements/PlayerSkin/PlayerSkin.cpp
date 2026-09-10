#include <z64player.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <unordered_map>
#include "PlayerSkin.h"

extern "C" {
extern FlexSkeletonHeader* gPlayerSkelHeaders[2];
extern void* sEyeTextures[2][PLAYER_SKIN_EYE_COUNT];
extern void* sMouthTextures[2][PLAYER_SKIN_MOUTH_COUNT];

extern Gfx* gPlayerLeftHandBgsDLs[];
extern Gfx* gPlayerLeftHandOpenDLs[];
extern Gfx* gPlayerLeftHandClosedDLs[];
extern Gfx* gPlayerLeftHandBoomerangDLs[];

// Aliases for the file-static tables in z_player.c
extern Gfx** gPlayerSkinLeftHandSwordDLs;
extern Gfx** gPlayerSkinLeftHandSwordDLs2;
extern Gfx** gPlayerSkinLeftHandHammerDLs;
extern Gfx** gPlayerSkinLeftHandBottleDLs;
extern Gfx** gPlayerSkinRightHandOpenDLs;
extern Gfx** gPlayerSkinRightHandClosedDLs;
extern Gfx** gPlayerSkinRightHandShieldDLs;
extern Gfx** gPlayerSkinRightHandBowSlingshotDLs;
extern Gfx** gPlayerSkinRightHandBowSlingshotDLs2;
extern Gfx** gPlayerSkinRightHandOcarinaDLs;
extern Gfx** gPlayerSkinRightHandOotDLs;
extern Gfx** gPlayerSkinRightHandHookshotDLs;
extern Gfx** gPlayerSkinSwordAndSheathDLs;
extern Gfx** gPlayerSkinSheathDLs;
extern Gfx** gPlayerSkinSheathWithSwordDLs;
extern Gfx** gPlayerSkinSheathWithoutSwordDLs;
extern Gfx** gPlayerSkinWaistDLs;
extern Gfx** gPlayerSkinGauntletPlate1DLs;
extern Gfx** gPlayerSkinGauntletPlate2DLs;
extern Gfx** gPlayerSkinGauntletPlate3DLs;
extern Gfx** gPlayerSkinIronBootDLs;
extern Gfx** gPlayerSkinHoverBootDLs;
extern Gfx** gPlayerSkinBowStringDLs;
extern Gfx** gPlayerSkinHookshotReticleDLs;

extern Gfx** gPlayerSkinFirstPersonLeftForearmDLs;
extern Gfx** gPlayerSkinFirstPersonLeftHandDLs;
extern Gfx** gPlayerSkinFirstPersonRightShoulderDLs;
extern Gfx** gPlayerSkinFirstPersonForearmDLs;
extern Gfx** gPlayerSkinFirstPersonRightHandHoldingWeaponDLs;

s32 ResourceMgr_FileExists(const char* filePath);
int ResourceMgr_OTRSigCheck(const char* imgData);
}

namespace {

struct ManagedPlayerSkin;
std::unordered_map<std::string, ManagedPlayerSkin>& Skins();

void* Reroot(const std::string& skin, void* vanilla) {
    if (vanilla == nullptr || !ResourceMgr_OTRSigCheck((const char*)vanilla)) {
        return vanilla;
    }

    const std::string skinned = "__OTR__" + skin + "/" + ((const char*)vanilla + 7);
    if (!ResourceMgr_FileExists(skinned.c_str())) {
        return vanilla;
    }

    const size_t size = skinned.size() + 1;
    char* owned = (char*)malloc(size);
    if (owned == nullptr) {
        return vanilla;
    }

    memcpy(owned, skinned.c_str(), size);
    return (void*)owned;
}

struct DLTable {
    Gfx** vanilla;
    Gfx** dest;
    int count;
};

int BuildTableList(PlayerSkin& out, DLTable* tables) {
    const DLTable src[] = {
        { gPlayerLeftHandBgsDLs, out.leftHandBgs, 8 },
        { gPlayerLeftHandOpenDLs, out.leftHandOpen, 4 },
        { gPlayerLeftHandClosedDLs, out.leftHandClosed, 4 },
        { gPlayerSkinLeftHandSwordDLs, out.leftHandSword, 4 },
        { gPlayerSkinLeftHandSwordDLs2, out.leftHandSword2, 4 },
        { gPlayerSkinLeftHandHammerDLs, out.leftHandHammer, 4 },
        { gPlayerLeftHandBoomerangDLs, out.leftHandBoomerang, 4 },
        { gPlayerSkinLeftHandBottleDLs, out.leftHandBottle, 4 },
        { gPlayerSkinRightHandOpenDLs, out.rightHandOpen, 4 },
        { gPlayerSkinRightHandClosedDLs, out.rightHandClosed, 4 },
        { gPlayerSkinRightHandShieldDLs, out.rightHandShield, PLAYER_SHIELD_MAX * 4 },
        { gPlayerSkinRightHandBowSlingshotDLs, out.rightHandBowSlingshot, 4 },
        { gPlayerSkinRightHandBowSlingshotDLs2, out.rightHandBowSlingshot2, 4 },
        { gPlayerSkinRightHandOcarinaDLs, out.rightHandOcarina, 4 },
        { gPlayerSkinRightHandOotDLs, out.rightHandOot, 4 },
        { gPlayerSkinRightHandHookshotDLs, out.rightHandHookshot, 4 },
        { gPlayerSkinSwordAndSheathDLs, out.swordAndSheath, 4 },
        { gPlayerSkinSheathDLs, out.sheath, 4 },
        { gPlayerSkinSheathWithSwordDLs, out.sheathWithSword, (PLAYER_SHIELD_MAX + 2) * 4 },
        { gPlayerSkinSheathWithoutSwordDLs, out.sheathWithoutSword, (PLAYER_SHIELD_MAX + 2) * 4 },
        { gPlayerSkinWaistDLs, out.waist, 4 },
        { gPlayerSkinGauntletPlate1DLs, out.gauntletPlate1, 2 },
        { gPlayerSkinGauntletPlate2DLs, out.gauntletPlate2, 2 },
        { gPlayerSkinGauntletPlate3DLs, out.gauntletPlate3, 2 },
        { gPlayerSkinIronBootDLs, out.ironBoot, 2 },
        { gPlayerSkinHoverBootDLs, out.hoverBoot, 2 },
        { gPlayerSkinBowStringDLs, out.bowString, 2 },
        { gPlayerSkinHookshotReticleDLs, out.hookshotReticle, 1 },
        { gPlayerSkinFirstPersonLeftForearmDLs, out.fpLeftForearm, 2 },
        { gPlayerSkinFirstPersonLeftHandDLs, out.fpLeftHand, 2 },
        { gPlayerSkinFirstPersonRightShoulderDLs, out.fpRightShoulder, 2 },
        { gPlayerSkinFirstPersonForearmDLs, out.fpForearm, 2 },
        { gPlayerSkinFirstPersonRightHandHoldingWeaponDLs, out.fpRightHandHoldingWeapon, 2 },
    };

    const int count = (int)(sizeof(src) / sizeof(src[0]));
    for (int i = 0; i < count; i++) {
        tables[i] = src[i];
    }
    return count;
}

static constexpr int MAX_DL_TABLES = 40;

void FillTables(PlayerSkin& out, const std::string* skin) {
    DLTable tables[MAX_DL_TABLES];
    const int n = BuildTableList(out, tables);

    for (int t = 0; t < n; t++) {
        for (int i = 0; i < tables[t].count; i++) {
            Gfx* entry = (tables[t].vanilla != nullptr) ? tables[t].vanilla[i] : nullptr;
            tables[t].dest[i] = (skin != nullptr) ? (Gfx*)Reroot(*skin, entry) : entry;
        }
    }
}

void BuildGroups(PlayerSkin& out) {
    out.dlistGroups[PLAYER_MODELTYPE_LH_OPEN] = out.leftHandOpen;
    out.dlistGroups[PLAYER_MODELTYPE_LH_CLOSED] = out.leftHandClosed;
    out.dlistGroups[PLAYER_MODELTYPE_LH_SWORD] = out.leftHandSword;
    out.dlistGroups[PLAYER_MODELTYPE_LH_SWORD_2] = out.leftHandSword2;
    out.dlistGroups[PLAYER_MODELTYPE_LH_BGS] = out.leftHandBgs;
    out.dlistGroups[PLAYER_MODELTYPE_LH_HAMMER] = out.leftHandHammer;
    out.dlistGroups[PLAYER_MODELTYPE_LH_BOOMERANG] = out.leftHandBoomerang;
    out.dlistGroups[PLAYER_MODELTYPE_LH_BOTTLE] = out.leftHandBottle;
    out.dlistGroups[PLAYER_MODELTYPE_RH_OPEN] = out.rightHandOpen;
    out.dlistGroups[PLAYER_MODELTYPE_RH_CLOSED] = out.rightHandClosed;
    out.dlistGroups[PLAYER_MODELTYPE_RH_SHIELD] = out.rightHandShield;
    out.dlistGroups[PLAYER_MODELTYPE_RH_BOW_SLINGSHOT] = out.rightHandBowSlingshot;
    out.dlistGroups[PLAYER_MODELTYPE_RH_BOW_SLINGSHOT_2] = out.rightHandBowSlingshot2;
    out.dlistGroups[PLAYER_MODELTYPE_RH_OCARINA] = out.rightHandOcarina;
    out.dlistGroups[PLAYER_MODELTYPE_RH_OOT] = out.rightHandOot;
    out.dlistGroups[PLAYER_MODELTYPE_RH_HOOKSHOT] = out.rightHandHookshot;
    out.dlistGroups[PLAYER_MODELTYPE_SHEATH_16] = out.swordAndSheath;
    out.dlistGroups[PLAYER_MODELTYPE_SHEATH_17] = out.sheath;
    out.dlistGroups[PLAYER_MODELTYPE_SHEATH_18] = out.sheathWithSword;
    out.dlistGroups[PLAYER_MODELTYPE_SHEATH_19] = out.sheathWithoutSword;
    out.dlistGroups[PLAYER_MODELTYPE_WAIST] = out.waist;
}

void Fill(PlayerSkin& out, const std::string* skin) {
    memset(&out, 0, sizeof(out));

    for (int age = 0; age < 2; age++) {
        out.skel[age] = gPlayerSkelHeaders[age];
        if (skin != nullptr) {
            out.skel[age] = (FlexSkeletonHeader*)Reroot(*skin, (void*)gPlayerSkelHeaders[age]);
        }

        for (int i = 0; i < PLAYER_SKIN_EYE_COUNT; i++) {
            out.eyeTex[age][i] = sEyeTextures[age][i];
            if (skin != nullptr) {
                out.eyeTex[age][i] = Reroot(*skin, sEyeTextures[age][i]);
            }
        }
        for (int i = 0; i < PLAYER_SKIN_MOUTH_COUNT; i++) {
            out.mouthTex[age][i] = sMouthTextures[age][i];
            if (skin != nullptr) {
                out.mouthTex[age][i] = Reroot(*skin, sMouthTextures[age][i]);
            }
        }
    }

    FillTables(out, skin);
    BuildGroups(out);
}

struct ManagedPlayerSkin : PlayerSkin {
    ManagedPlayerSkin() {
        Fill(*this, nullptr);
    }

    explicit ManagedPlayerSkin(const std::string& skin) {
        Fill(*this, &skin);
    }

    ManagedPlayerSkin(const ManagedPlayerSkin& other) : PlayerSkin(other) {
        BuildGroups(*this);
    }

    ManagedPlayerSkin& operator=(const ManagedPlayerSkin& other) {
        PlayerSkin::operator=(other);
        BuildGroups(*this);
        return *this;
    }
};

static_assert(std::is_standard_layout<ManagedPlayerSkin>::value, "ManagedPlayerSkin must stay standard_layout");

std::unordered_map<std::string, ManagedPlayerSkin>& Skins() {
    static std::unordered_map<std::string, ManagedPlayerSkin> sSkins;
    return sSkins;
}

PlayerSkin& Vanilla() {
    static ManagedPlayerSkin sVanilla;
    return sVanilla;
}

} // namespace

extern "C" PlayerSkin* PlayerSkin_GetVanilla(void) {
    return &Vanilla();
}

extern "C" PlayerSkin* Player_GetSkin(Player* player) {
    if (player == nullptr || player->skin == nullptr) {
        return &Vanilla();
    }
    return player->skin;
}

extern "C" PlayerSkin* PlayerSkin_Get(const char* name) {
    if (name == nullptr || name[0] == 0) {
        return &Vanilla();
    }

    const std::string key = name;

    auto& skins = Skins();

    auto it = skins.find(key);
    if (it != skins.end()) {
        return &it->second;
    }

    ManagedPlayerSkin fresh(key);
    snprintf(fresh.name, sizeof(fresh.name), "%s", name);

    return &skins.emplace(key, fresh).first->second;
}

extern "C" PlayerSkin* PlayerSkin_Reload(const char* name) {

    if (name == nullptr || name[0] == 0) {
        return &Vanilla();
    }

    auto& skins = Skins();

    auto it = skins.find(name);
    if (it == skins.end()) {
        return PlayerSkin_Get(name);
    }

    it->second = ManagedPlayerSkin(std::string(name));
    snprintf(it->second.name, sizeof(it->second.name), "%s", name);

    return &it->second;
}