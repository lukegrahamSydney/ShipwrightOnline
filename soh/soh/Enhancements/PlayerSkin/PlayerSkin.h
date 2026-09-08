#ifndef PLAYERSKIN_H
#define PLAYERSKIN_H

#include <libultraship/libultra.h>
#include "z64player.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLAYER_SKIN_NAME_MAX 100
#define PLAYER_SKIN_EYE_COUNT 8
#define PLAYER_SKIN_MOUTH_COUNT 4

typedef struct PlayerSkin {
    char name[PLAYER_SKIN_NAME_MAX];

    FlexSkeletonHeader* skel[2];
    void* eyeTex[2][PLAYER_SKIN_EYE_COUNT];
    void* mouthTex[2][PLAYER_SKIN_MOUTH_COUNT];

    Gfx* leftHandBgs[8];
    Gfx* leftHandOpen[4];
    Gfx* leftHandClosed[4];
    Gfx* leftHandSword[4];
    Gfx* leftHandSword2[4];
    Gfx* leftHandHammer[4];
    Gfx* leftHandBoomerang[4];
    Gfx* leftHandBottle[4];
    Gfx* rightHandOpen[4];
    Gfx* rightHandClosed[4];
    Gfx* rightHandShield[PLAYER_SHIELD_MAX * 4];
    Gfx* rightHandBowSlingshot[4];
    Gfx* rightHandBowSlingshot2[4];
    Gfx* rightHandOcarina[4];
    Gfx* rightHandOot[4];
    Gfx* rightHandHookshot[4];
    Gfx* swordAndSheath[4];
    Gfx* sheath[4];
    Gfx* sheathWithSword[(PLAYER_SHIELD_MAX + 2) * 4];
    Gfx* sheathWithoutSword[(PLAYER_SHIELD_MAX + 2) * 4];
    Gfx* waist[4];

    Gfx* gauntletPlate1[2];
    Gfx* gauntletPlate2[2];
    Gfx* gauntletPlate3[2];
    Gfx* ironBoot[2];
    Gfx* hoverBoot[2];
    Gfx* bowString[2];
    Gfx* hookshotReticle[1];

    Gfx* fpLeftForearm[2];
    Gfx* fpLeftHand[2];
    Gfx* fpRightShoulder[2];
    Gfx* fpForearm[2];
    Gfx* fpRightHandHoldingWeapon[2];

    // Built to point into the arrays above, in PLAYER_MODELTYPE order.
    Gfx** dlistGroups[PLAYER_MODELTYPE_MAX];
} PlayerSkin;

// Returns the vanilla table when name is empty, "link" or the skin is missing.
PlayerSkin* PlayerSkin_Get(const char* name);

PlayerSkin* PlayerSkin_GetVanilla(void);

struct Player;
// player->skin or the vanilla table when unset
PlayerSkin* Player_GetSkin(struct Player* player);
PlayerSkin* PlayerSkin_Reload(const char* name);

#ifdef __cplusplus
}
#endif

#endif